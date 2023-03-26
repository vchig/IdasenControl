#include "idasen_main_window.h"

#include "app_config.h"
#include "bt/bt_device_list.h"
#include "bt/bt_manager.h"
#include "bt/dbus/bt_dbus_manager.h"
#include "idasen_desk.h"
#include "idasen_position.h"

struct _IdasenMainWindow {
  GtkApplicationWindow parent;
  gchar               *desk_name;
  gchar               *desk_height;
  gboolean             enable_control;
};

enum {
  PROP_0,
  PROP_DESK_NAME,
  PROP_DESK_HEIGHT,
  PROP_ENABLE_CONTROL,
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE(IdasenMainWindow, idasen_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void idasen_main_window_init(IdasenMainWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

static void idasen_main_window_dispose(GObject *object) {
  IdasenMainWindow *self = IDASEN_MAIN_WINDOW(object);
  g_free(self->desk_name);
  g_free(self->desk_height);
  G_OBJECT_CLASS(idasen_main_window_parent_class)->dispose(object);
}

static void
idasen_main_window_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_MAIN_WINDOW(object));
  IdasenMainWindow *self = IDASEN_MAIN_WINDOW(object);
  switch(property_id) {
    case PROP_DESK_NAME: idasen_main_window_set_desk_name(self, g_value_get_string(value)); break;
    case PROP_DESK_HEIGHT: idasen_main_window_set_desk_height(self, g_value_get_string(value)); break;
    case PROP_ENABLE_CONTROL: self->enable_control = g_value_get_boolean(value); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_main_window_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_MAIN_WINDOW(object));
  IdasenMainWindow *self = IDASEN_MAIN_WINDOW(object);
  switch(property_id) {
    case PROP_DESK_NAME: g_value_set_string(value, idasen_main_window_get_desk_name(self)); break;
    case PROP_DESK_HEIGHT: g_value_set_string(value, idasen_main_window_get_desk_height(self)); break;
    case PROP_ENABLE_CONTROL: g_value_set_boolean(value, self->enable_control); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_main_window_class_init(IdasenMainWindowClass *klass) {
  G_OBJECT_CLASS(klass)->dispose      = idasen_main_window_dispose;
  G_OBJECT_CLASS(klass)->set_property = idasen_main_window_set_property;
  G_OBJECT_CLASS(klass)->get_property = idasen_main_window_get_property;

  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), APPLICATION_PATH "/ui/main_window.ui");

  g_properties[PROP_DESK_NAME] =
      g_param_spec_string("desk-name", "", "", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_DESK_HEIGHT] =
      g_param_spec_string("desk-height", "", "", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_ENABLE_CONTROL] =
      g_param_spec_boolean("enable-control", "", "", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properties);
}

GtkWindow *idasen_main_window_new(GtkApplication *app) {
  return g_object_new(IDASEN_TYPE_MAIN_WINDOW, "application", app, NULL);
}

gchar const *idasen_main_window_get_desk_name(IdasenMainWindow *self) {
  g_return_val_if_fail(IDASEN_IS_MAIN_WINDOW(self), NULL);
  return self->desk_name;
}

void idasen_main_window_set_desk_name(IdasenMainWindow *self, gchar const *name) {
  g_return_if_fail(IDASEN_IS_MAIN_WINDOW(self));
  g_free(self->desk_name);
  self->desk_name = g_strdup(name);
}

gchar const *idasen_main_window_get_desk_height(IdasenMainWindow *self) {
  g_return_val_if_fail(IDASEN_IS_MAIN_WINDOW(self), NULL);
  return self->desk_height;
}

void idasen_main_window_set_desk_height(IdasenMainWindow *self, gchar const *value) {
  g_return_if_fail(IDASEN_IS_MAIN_WINDOW(self));
  g_free(self->desk_height);
  self->desk_height = g_strdup(value);
}
