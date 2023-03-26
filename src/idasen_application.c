#include "idasen_application.h"

#include "app_config.h"
#include "bt/dbus/bt_dbus_manager.h"
#include "idasen_desk.h"
#include "idasen_main_window.h"
#include "idasen_position.h"
#include "idasen_preference.h"

#include <glib/gi18n.h>

static guint const _1s = 1000u;

struct _IdasenApplication {
  AdwApplication parent;
  GSettings     *settings;
  BtManager     *manager;
  IdasenDesk    *desk;
  guint          reconnect_timer;
};

static gboolean height_convert(GBinding *binding, const GValue *from_value, GValue *to_value, gpointer udata) {
  (void)binding;
  (void)udata;
  g_return_val_if_fail(G_VALUE_HOLDS_DOUBLE(from_value) && G_VALUE_HOLDS_STRING(to_value), FALSE);
  gdouble height = g_value_get_double(from_value);
  char   *text   = g_strdup_printf(_("%.1f cm"), height);
  g_value_set_string(to_value, text);
  g_free(text);
  return TRUE;
}

static void action_show_about(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  gchar const *app_name  = _("IDÅSEN table control");
  gchar const *comments  = _("Program to control idasen desk.");
  gchar const *icon_name = "io.github.vchig.IdasenControl";
  GtkLicense   license   = GTK_LICENSE_MIT_X11;
  gchar const *autors[]  = { "Vadim Chigrinov", NULL };
  gchar const *version   = "0.0.1";
  gtk_show_about_dialog(gtk_application_get_active_window(GTK_APPLICATION(udata)),
                        "program-name",
                        app_name,
                        "comments",
                        comments,
                        "logo-icon-name",
                        icon_name,
                        "license-type",
                        license,
                        "authors",
                        autors,
                        "version",
                        version,
                        NULL);
}

static void action_preference(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  IdasenPreference *pref = idasen_preference_new();
  gtk_window_set_transient_for(GTK_WINDOW(pref), gtk_application_get_active_window(GTK_APPLICATION(udata)));
  gtk_window_present(GTK_WINDOW(pref));
}

static void action_up(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  idasen_desk_up(IDASEN_APPLICATION(udata)->desk);
}

static void action_down(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  idasen_desk_down(IDASEN_APPLICATION(udata)->desk);
}

static void action_stand(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self = IDASEN_APPLICATION(udata);
  idasen_desk_wakeup(self->desk);
  idasen_desk_stop(self->desk);
  idasen_desk_set_height(self->desk, g_settings_get_double(self->settings, "desk-stand-position"));
}

static void action_sit(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self = IDASEN_APPLICATION(udata);
  idasen_desk_wakeup(self->desk);
  idasen_desk_stop(self->desk);
  idasen_desk_set_height(self->desk, g_settings_get_double(self->settings, "desk-sit-position"));
}

static void idasen_set_position(GObject *object, gdouble position, gpointer udata) {
  (void)object;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self = IDASEN_APPLICATION(udata);
  idasen_desk_wakeup(self->desk);
  idasen_desk_stop(self->desk);
  idasen_desk_set_height(self->desk, position);
}

static void action_set_position(GSimpleAction *action, GVariant *parameter, gpointer udata) {
  (void)action;
  (void)parameter;
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self            = IDASEN_APPLICATION(udata);
  GtkWindow         *position_window = idasen_position_new(idasen_desk_get_actual_height(self->desk));
  g_object_set(position_window, "title", _("Move to"), "button-label", _("Move"), NULL);
  g_signal_connect(position_window, "position-selected", G_CALLBACK(idasen_set_position), udata);
  gtk_window_set_transient_for(GTK_WINDOW(position_window), gtk_application_get_active_window(GTK_APPLICATION(self)));
  gtk_window_present(position_window);
}

static GActionEntry g_action_entries[] = {
  { "about", action_show_about, NULL, NULL, NULL },
  { "set-position", action_set_position, NULL, NULL, NULL },
  { "preference", action_preference, NULL, NULL, NULL },
  { "up", action_up, NULL, NULL, NULL },
  { "down", action_down, NULL, NULL, NULL },
  { "stand", action_stand, NULL, NULL, NULL },
  { "sit", action_sit, NULL, NULL, NULL },
};

G_DEFINE_FINAL_TYPE(IdasenApplication, idasen_application, ADW_TYPE_APPLICATION)

static gboolean idasen_application_reconnect_timeout(gpointer udata) {
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self = IDASEN_APPLICATION(udata);
  g_debug("Try to connect '%s' desk.\n", idasen_desk_get_title(self->desk));
  if(!idasen_desk_get_is_connected(self->desk)) {
    idasen_desk_connect(self->desk);
  }
  gboolean is_connected = idasen_desk_get_is_connected(self->desk);
  g_debug("Connection %s\n", (is_connected ? "success" : "failed"));
  if(is_connected) {
    self->reconnect_timer = 0u;
  }
  return !is_connected;
}

static void settings_changed(GSettings *settings, gchar *key, gpointer udata) {
  g_return_if_fail(IDASEN_IS_APPLICATION(udata));
  IdasenApplication *self = IDASEN_APPLICATION(udata);
  if(g_str_equal(key, "background-mode")) {
    g_settings_get_boolean(settings, "background-mode") ? g_application_hold(self) : g_application_release(self);
  } else if(g_str_equal(key, "reconnect")) {
    if(self->reconnect_timer) {
      g_source_remove(self->reconnect_timer);
      self->reconnect_timer = 0u;
    }
    if(g_settings_get_boolean(settings, "reconnect")) {
      self->reconnect_timer = g_timeout_add(g_settings_get_uint(settings, "reconnect-timeout") * _1s,
                                            idasen_application_reconnect_timeout,
                                            self);
    }
  } else if(g_str_equal(key, "reconnect-timeout")) {
    if(self->reconnect_timer) {
      g_source_remove(self->reconnect_timer);
      self->reconnect_timer = g_timeout_add(g_settings_get_uint(settings, "reconnect-timeout") * _1s,
                                            idasen_application_reconnect_timeout,
                                            self);
    }
  }
}

static void idasen_application_init(IdasenApplication *self) {
  self->settings = g_settings_new(APPLICATION_ID);
  self->manager  = bt_dbus_manager_get_default();
  self->desk     = idasen_desk_new(self->manager);
}

static void idasen_application_startup(GApplication *app) {
  IdasenApplication *self = IDASEN_APPLICATION(app);
  g_settings_bind(self->settings, "desk-mac-address", self->desk, "device-address", G_SETTINGS_BIND_DEFAULT);
  if(g_settings_get_boolean(self->settings, "background-mode")) {
    g_application_hold(self);
  }
  if(g_settings_get_boolean(self->settings, "reconnect")) {
    self->reconnect_timer = g_timeout_add(g_settings_get_uint(self->settings, "reconnect-timeout") * _1s,
                                          idasen_application_reconnect_timeout,
                                          self);
  }
  // Подписываемся на изменения настроек
  g_signal_connect(self->settings, "changed", G_CALLBACK(settings_changed), self);
  // Устанавливаем Action'ы уровня приложения.
  g_action_map_add_action_entries(G_ACTION_MAP(app), g_action_entries, G_N_ELEMENTS(g_action_entries), app);
  G_APPLICATION_CLASS(idasen_application_parent_class)->startup(app);
}

static void idasen_application_activate(GApplication *app) {
  IDASEN_IS_APPLICATION(app);
  IdasenApplication *self = IDASEN_APPLICATION(app);

  GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
  if(NULL == window) {
    window = idasen_main_window_new(GTK_APPLICATION(app));
    g_object_bind_property(self->desk, "title", window, "desk-name", G_BINDING_SYNC_CREATE);
    g_object_bind_property_full(self->desk,
                                "actual-height",
                                window,
                                "desk-height",
                                G_BINDING_SYNC_CREATE,
                                height_convert,
                                NULL,
                                NULL,
                                NULL);
    g_object_bind_property(self->desk, "is-ready", window, "enable-control", G_BINDING_SYNC_CREATE);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, APPLICATION_PATH "/css/styles.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
  }
  gtk_window_present(GTK_WINDOW(window));
}

static void idasen_application_dispose(GObject *object) {
  IdasenApplication *self = IDASEN_APPLICATION(object);
  g_clear_pointer(&self->settings, g_object_unref);
  g_clear_pointer(&self->manager, g_object_unref);
  g_clear_pointer(&self->desk, g_object_unref);
  G_OBJECT_CLASS(idasen_application_parent_class)->dispose(object);
}

static void idasen_application_class_init(IdasenApplicationClass *klass) {
  G_APPLICATION_CLASS(klass)->startup  = idasen_application_startup;
  G_APPLICATION_CLASS(klass)->activate = idasen_application_activate;
  G_OBJECT_CLASS(klass)->dispose       = idasen_application_dispose;
}

IdasenApplication *idasen_application_new() {
  return g_object_new(IDASEN_TYPE_APPLICATION,
                      "application-id",
                      APPLICATION_ID,
                      "flags",
                      G_APPLICATION_FLAGS_NONE,
                      NULL);
}
