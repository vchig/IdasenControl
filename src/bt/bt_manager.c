#include "bt_manager.h"

enum {
  SIG_ADAPTER_ADDED,
  SIG_ADAPTER_REMOVED,
  SIG_DEVICE_ADDED,
  SIG_DEVICE_REMOVED,
  N_SIGNALS,
};

static guint g_signals[N_SIGNALS] = { 0u };

G_DEFINE_INTERFACE(BtManager, bt_manager, G_TYPE_OBJECT)

static gint bt_manager_find_device_by_address(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_DEVICE((gpointer)value), -1);
  return g_strcmp0(bt_device_get_address(BT_DEVICE((gpointer)value)), udata);
}

static void bt_manager_default_init(BtManagerInterface *iface) {
  (void)iface;
  g_signals[SIG_ADAPTER_ADDED] =
      g_signal_new("adapter-added", BT_TYPE_MANAGER, G_SIGNAL_RUN_LAST, 0, 0, 0, 0, G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signals[SIG_ADAPTER_REMOVED] =
      g_signal_new("adapter-removed", BT_TYPE_MANAGER, G_SIGNAL_RUN_LAST, 0, 0, 0, 0, G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signals[SIG_DEVICE_ADDED] =
      g_signal_new("device-added", BT_TYPE_MANAGER, G_SIGNAL_RUN_LAST, 0, 0, 0, 0, G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signals[SIG_DEVICE_REMOVED] =
      g_signal_new("device-removed", BT_TYPE_MANAGER, G_SIGNAL_RUN_LAST, 0, 0, 0, 0, G_TYPE_NONE, 1, G_TYPE_STRING);

  g_object_interface_install_property(iface,
                                      g_param_spec_uint("n-devices", "", "", 0, G_MAXUINT32, 0, G_PARAM_READABLE));
  g_object_interface_install_property(iface,
                                      g_param_spec_uint("n-adapters", "", "", 0, G_MAXUINT32, 0, G_PARAM_READABLE));
}

BtAdapter *bt_manager_get_adapter(BtManager *self, gchar const *id) {
  g_return_val_if_fail(BT_IS_MANAGER(self), NULL);
  g_return_val_if_fail(id != NULL, NULL);
  BtManagerInterface *iface = BT_MANAGER_GET_IFACE(self);
  g_return_val_if_fail(iface->get_adapter != NULL, NULL);
  return iface->get_adapter(self, id);
}

BtDevice *bt_manager_get_device(BtManager *self, gchar const *id) {
  g_return_val_if_fail(BT_IS_MANAGER(self), NULL);
  g_return_val_if_fail(id != NULL, NULL);
  BtManagerInterface *iface = BT_MANAGER_GET_IFACE(self);
  g_return_val_if_fail(iface->get_device != NULL, NULL);
  return iface->get_device(self, id);
}

GSList *bt_manager_get_devices(BtManager *self) {
  g_return_val_if_fail(BT_IS_MANAGER(self), NULL);
  BtManagerInterface *iface = BT_MANAGER_GET_IFACE(self);
  g_return_val_if_fail(iface->get_devices != NULL, NULL);
  return iface->get_devices(self);
}

BtDevice *bt_manager_get_device_by_address(BtManager *self, gchar const *address) {
  GSList *devices = bt_manager_get_devices(self);
  GSList *item    = g_slist_find_custom(devices, address, bt_manager_find_device_by_address);
  return item ? item->data : NULL;
}

void bt_manager_adapter_added(BtManager *self, gchar const *id) {
  g_return_if_fail(BT_IS_MANAGER(self));
  g_return_if_fail(id != NULL);
  g_signal_emit(self, g_signals[SIG_ADAPTER_ADDED], 0, id);
}

void bt_manager_adapter_removed(BtManager *self, gchar const *id) {
  g_return_if_fail(BT_IS_MANAGER(self));
  g_return_if_fail(id != NULL);
  g_signal_emit(self, g_signals[SIG_ADAPTER_REMOVED], 0, id);
}

void bt_manager_device_added(BtManager *self, gchar const *id) {
  g_return_if_fail(BT_IS_MANAGER(self));
  g_return_if_fail(id != NULL);
  g_signal_emit(self, g_signals[SIG_DEVICE_ADDED], 0, id);
}

void bt_manager_device_removed(BtManager *self, gchar const *id) {
  g_return_if_fail(BT_IS_MANAGER(self));
  g_return_if_fail(id != NULL);
  g_signal_emit(self, g_signals[SIG_DEVICE_REMOVED], 0, id);
}
