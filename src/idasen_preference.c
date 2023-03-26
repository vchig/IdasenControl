#include "idasen_preference.h"

#include "app_config.h"
#include "bt/bt_device_list.h"
#include "bt/dbus/bt_dbus_manager.h"
#include "desk_position_row.h"

#include <glib/gi18n.h>

struct _IdasenPreference {
  AdwPreferencesWindow parent;

  BtDeviceList    *bt_device_list;
  GSettings       *settings;
  GtkListBox      *devices_list_box;
  DeskPositionRow *stand_position_row;
  DeskPositionRow *sit_position_row;
  AdwActionRow    *background_mode;
  AdwActionRow    *repeat_connection;
  AdwActionRow    *repeat_timeout;
  GtkAdjustment   *repeat_adjustment;
};

G_DEFINE_FINAL_TYPE(IdasenPreference, idasen_preference, ADW_TYPE_PREFERENCES_WINDOW)

static GtkWidget *create_list_box_row(gpointer item, gpointer udata) {
  (void)udata;
  g_return_val_if_fail(BT_IS_DEVICE(item), NULL);
  BtDevice  *dev = BT_DEVICE(item);
  GtkWidget *row = adw_action_row_new();
  g_object_bind_property(dev, "alias", row, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property(dev, "address", row, "subtitle", G_BINDING_SYNC_CREATE);
  return row;
}

static gboolean flag_to_icon_name(GBinding *binding, const GValue *from, GValue *to, gpointer udata) {
  (void)binding;
  (void)udata;
  g_return_val_if_fail(G_VALUE_HOLDS_BOOLEAN(from) && G_VALUE_HOLDS_STRING(to), FALSE);
  g_value_set_string(to, (g_value_get_boolean(from) ? "bluetooth-active" : "bluetooth-disabled"));
  return TRUE;
}

static gboolean flag_to_yes_no(GBinding *binding, const GValue *from, GValue *to, gpointer udata) {
  (void)binding;
  (void)udata;
  g_return_val_if_fail(G_VALUE_HOLDS_BOOLEAN(from) && G_VALUE_HOLDS_STRING(to), FALSE);
  g_value_set_string(to, (g_value_get_boolean(from) ? _("Yes") : _("No")));
  return TRUE;
}

static void connect_switch_state_changed(GtkSwitch *self, gboolean state, gpointer udata) {
  (void)self;
  g_return_if_fail(BT_IS_DEVICE(udata));
  BtDevice *device = BT_DEVICE(udata);
  if(state) {
    if(!bt_device_is_paired(device)) {
      bt_device_pair(device);
    }
    bt_device_connect(device);
  } else {
    bt_device_disconnect(device);
  }
}

static void select_button_clicked(GtkButton *self, gpointer udata) {
  (void)self;
  g_return_if_fail(BT_IS_DEVICE(udata));
  BtDevice  *device   = BT_DEVICE(udata);
  GSettings *settings = g_settings_new(APPLICATION_ID);
  g_settings_set_string(settings, "desk-mac-address", bt_device_get_address(device));
  g_object_unref(settings);
}

static GtkWindow *create_device_info(BtDevice *device) {
  GtkWindow  *window  = NULL;
  GError     *error   = NULL;
  GtkBuilder *builder = gtk_builder_new();
  if(gtk_builder_add_from_resource(builder, APPLICATION_PATH "/ui/bluetooth_device_info.ui", &error)) {
    GtkImage *image = GTK_IMAGE(gtk_builder_get_object(builder, "bt_image"));
    g_object_bind_property_full(device,
                                "is-connected",
                                image,
                                "icon-name",
                                G_BINDING_SYNC_CREATE,
                                flag_to_icon_name,
                                NULL,
                                NULL,
                                NULL);
    GtkLabel *label_address = GTK_LABEL(gtk_builder_get_object(builder, "label_address"));
    g_object_bind_property(device, "address", label_address, "label", G_BINDING_SYNC_CREATE);
    GtkLabel *label_paired = GTK_LABEL(gtk_builder_get_object(builder, "label_paired"));
    g_object_bind_property_full(device,
                                "is-paired",
                                label_paired,
                                "label",
                                G_BINDING_SYNC_CREATE,
                                flag_to_yes_no,
                                NULL,
                                NULL,
                                NULL);
    GtkSwitch *switch_connect = GTK_SWITCH(gtk_builder_get_object(builder, "switch_connect"));
    g_object_bind_property(device, "is-connected", switch_connect, "state", G_BINDING_SYNC_CREATE);
    g_signal_connect(switch_connect, "state-set", G_CALLBACK(connect_switch_state_changed), device);
    GtkButton *select_button = GTK_BUTTON(gtk_builder_get_object(builder, "select_button"));
    g_signal_connect(select_button, "clicked", G_CALLBACK(select_button_clicked), device);
    window = GTK_WINDOW(g_object_ref(gtk_builder_get_object(builder, "root")));
    g_object_bind_property(device, "alias", window, "title", G_BINDING_SYNC_CREATE);
    g_object_unref(builder);
  } else {
    g_warning("Error: %s\n", error->message);
  }
  return window;
}

static void device_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer udata) {
  (void)udata;
  if(row) {
    gchar const *address = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));
    BtManager   *manager = bt_dbus_manager_get_default();
    BtDevice    *device  = bt_manager_get_device_by_address(manager, address);
    if(device) {
      GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(box)));
      GtkWindow *info   = create_device_info(device);
      gtk_window_set_transient_for(info, parent);
      gtk_window_present(info);
      g_object_unref(info);
    }
    g_object_unref(manager);
    gtk_list_box_unselect_row(box, row);
  }
}

static void idasen_preference_init(IdasenPreference *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
  BtManager *manager   = bt_dbus_manager_get_default();
  self->bt_device_list = bt_device_list_new(manager);
  self->settings       = g_settings_new(APPLICATION_ID);
  g_object_unref(manager);
  gtk_list_box_bind_model(self->devices_list_box, G_LIST_MODEL(self->bt_device_list), create_list_box_row, NULL, NULL);
  g_settings_bind(self->settings, "desk-stand-position", self->stand_position_row, "position", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind(self->settings, "desk-sit-position", self->sit_position_row, "position", G_SETTINGS_BIND_DEFAULT);

  GtkWidget *bmode = gtk_switch_new();
  gtk_widget_set_valign(bmode, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(self->background_mode), bmode);
  g_settings_bind(self->settings, "background-mode", bmode, "active", G_SETTINGS_BIND_DEFAULT);

  GtkWidget *repeat = gtk_switch_new();
  gtk_widget_set_valign(repeat, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(self->repeat_connection), repeat);
  g_settings_bind(self->settings, "reconnect", repeat, "active", G_SETTINGS_BIND_DEFAULT);

  GtkWidget *timeout = gtk_spin_button_new(self->repeat_adjustment, 0.0, 0);
  gtk_widget_set_valign(timeout, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(self->repeat_timeout), timeout);
  g_object_bind_property(repeat, "active", self->repeat_timeout, "sensitive", G_BINDING_SYNC_CREATE);
  g_settings_bind(self->settings, "reconnect-timeout", self->repeat_adjustment, "value", G_SETTINGS_BIND_DEFAULT);
}

static void idasen_preference_dispose(GObject *object) {
  IdasenPreference *self = IDASEN_PREFERENCE(object);
  g_clear_pointer(&self->settings, g_object_unref);
  G_OBJECT_CLASS(idasen_preference_parent_class)->dispose(object);
}

static void idasen_preference_class_init(IdasenPreferenceClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = idasen_preference_dispose;
  g_type_ensure(DESK_TYPE_POSITION_ROW);
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), APPLICATION_PATH "/ui/preference.ui");
  // Виджеты
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, devices_list_box);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, stand_position_row);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, sit_position_row);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, background_mode);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, repeat_connection);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, repeat_timeout);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), IdasenPreference, repeat_adjustment);
  // Сигналы
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass), device_row_selected);
}

IdasenPreference *idasen_preference_new(void) {
  return g_object_new(IDASEN_TYPE_PREFERENCE, NULL);
}
