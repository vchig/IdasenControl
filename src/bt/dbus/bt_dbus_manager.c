#include "bt_dbus_manager.h"

#include "../bt_adapter.h"
#include "../bt_device.h"
#include "bt_dbus_proxy.h"

#define BLUEZ_BUS_NAME "org.bluez"
#define BLUEZ_IFACE_ADAPTER1 "org.bluez.Adapter1"
#define BLUEZ_IFACE_DEVICE1 "org.bluez.Device1"
#define BLUEZ_IFACE_GATT_SERVICE1 "org.bluez.GattService1"
#define BLUEZ_IFACE_GATT_CHARACTERISTIC1 "org.bluez.GattCharacteristic1"
#define BLUEZ_IFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define BLUEZ_PATH "/"
#define BLUEZ_GET_MANAGED_OBJECTS "GetManagedObjects"
#define BLUEZ_INTERFACE_ADDED "InterfacesAdded"
#define BLUEZ_INTERFACE_REMOVED "InterfacesRemoved"

struct _BtDbusManager {
  GObject          parent;
  GDBusConnection *connection;
  GHashTable      *adapters;
  GSList          *devices;
  guint            watcher_id;
  guint            iface_added;
  guint            iface_removed;
};

/// @brief Наследуемые свойства.
enum {
  PROP_0,
  PROP_N_ADAPTERS,
  PROP_N_DEVICE,
};

#define BT_TYPE_DBUS_MANAGER (bt_dbus_manager_get_type())

G_DECLARE_FINAL_TYPE(BtDbusManager, bt_dbus_manager, BT, DBUS_MANAGER, GObject)

static void bt_dbus_manager_iface_init(BtManagerInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(BtDbusManager,
                              bt_dbus_manager,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(BT_TYPE_MANAGER, bt_dbus_manager_iface_init))

static GDBusProxy *
create_dbus_proxy_sync(GDBusConnection *connection, gchar const *object_path, gchar const *iface_name, GError **error) {
  return g_dbus_proxy_new_sync(connection,
                               G_DBUS_PROXY_FLAGS_NONE,
                               NULL,
                               BLUEZ_BUS_NAME,
                               object_path,
                               iface_name,
                               NULL,
                               error);
}

static void bt_dbus_manager_add_adapter(BtDbusManager *self, GDBusProxy *proxy, gchar const *adapter_id) {
  BtAdapter *adapter = bt_adapter_new(bt_dbus_proxy_new(g_object_ref(proxy)));
  g_hash_table_insert(self->adapters, (gpointer)adapter_id, adapter);
  bt_manager_adapter_added(BT_MANAGER(self), adapter_id);
  g_object_notify(G_OBJECT(self), "n-adapters");
}

static void bt_dbus_manager_add_device(BtDbusManager *self, GDBusProxy *proxy, gchar const *device_id) {
  BtDevice *device = bt_device_new(bt_dbus_proxy_new(g_object_ref(proxy)), device_id);
  self->devices    = g_slist_append(self->devices, device);
  bt_manager_device_added(BT_MANAGER(self), device_id);
  g_object_notify(G_OBJECT(self), "n-devices");
}

static gint find_bt_device_by_gatt_uuid(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_DEVICE((gpointer)value), -1);
  BtDevice    *self  = BT_DEVICE((gpointer)value);
  GVariant    *uuids = bt_device_get_uuids(self);
  GVariantIter iter;
  g_variant_iter_init(&iter, uuids);
  gchar *uuid;
  while(g_variant_iter_loop(&iter, "s", &uuid)) {
    if(g_str_equal(uuid, udata)) {
      return 0;
    }
  }
  return -1;
}

static gint find_bt_device_by_gatt_id(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_DEVICE((gpointer)value), -1);
  return bt_device_find_service_by_id(BT_DEVICE((gpointer)value), udata) ? 0 : -1;
}

static void bt_dbus_manager_interface_added(BtDbusManager *self, gchar const *object_path, gchar const *iface_name) {
  g_return_if_fail(BT_IS_DBUS_MANAGER(self));
  GError     *error = NULL;
  GDBusProxy *proxy = create_dbus_proxy_sync(self->connection, object_path, iface_name, &error);
  if(error) {
    g_warning("Error create GDbusProxy for '%s' - '%s'. Error: %s\n", object_path, iface_name, error->message);
    return;
  }
  if(g_str_equal(iface_name, BLUEZ_IFACE_ADAPTER1)) {
    bt_dbus_manager_add_adapter(self, proxy, object_path);
  } else if(g_str_equal(iface_name, BLUEZ_IFACE_DEVICE1)) {
    bt_dbus_manager_add_device(self, proxy, object_path);
  } else if(g_str_equal(iface_name, BLUEZ_IFACE_GATT_SERVICE1)) {
    BtGattService *service = bt_gatt_service_new(bt_dbus_proxy_new(g_object_ref(proxy)), object_path);
    GSList *item = g_slist_find_custom(self->devices, bt_gatt_service_get_uuid(service), find_bt_device_by_gatt_uuid);
    if(item) {
      bt_device_append_gatt_service(BT_DEVICE(item->data), BT_GATT_SERVICE(g_object_ref(service)));
    }
    g_object_unref(service);
  } else if(g_str_equal(iface_name, BLUEZ_IFACE_GATT_CHARACTERISTIC1)) {
    BtGattCharacteristic *gatt_char  = bt_gatt_characteristic_new(bt_dbus_proxy_new(g_object_ref(proxy)));
    gchar const          *service_id = bt_gatt_characteristic_get_service_id(gatt_char);
    GSList               *item       = g_slist_find_custom(self->devices, service_id, find_bt_device_by_gatt_id);
    if(item) {
      BtGattService *service = bt_device_find_service_by_id(BT_DEVICE(item->data), service_id);
      bt_gatt_service_append_gatt_characteristic(service, BT_GATT_CHARACTERISTIC(g_object_ref(gatt_char)));
    }
    g_object_unref(gatt_char);
  }
  g_object_unref(proxy);
}

static void bt_dbus_manager_remove_adapter(BtDbusManager *self, gchar const *adapter_id) {
  gpointer found = g_hash_table_lookup(self->adapters, adapter_id);
  g_clear_pointer(&found, g_object_unref);
  bt_manager_adapter_removed(BT_MANAGER(self), adapter_id);
  g_object_notify(G_OBJECT(self), "n-adapters");
}

static void bt_dbus_manager_remove_device(BtDbusManager *self, gchar const *device_id) {
  GSList *item     = g_slist_find_custom(self->devices, device_id, bt_device_compare_by_id);
  gint    position = g_slist_position(self->devices, item);
  if(NULL != item && -1 != position) {
    self->devices = g_slist_remove_link(self->devices, item);
    g_object_unref(item->data);
    bt_manager_device_removed(BT_MANAGER(self), device_id);
    g_object_notify(G_OBJECT(self), "n-devices");
  }
}

static void bt_dbus_manager_interface_removed(BtDbusManager *self, gchar const *object_path, gchar const *iface_name) {
  g_return_if_fail(BT_IS_DBUS_MANAGER(self));
  if(g_str_equal(iface_name, BLUEZ_IFACE_ADAPTER1)) {
    bt_dbus_manager_remove_adapter(self, object_path);
  } else if(g_str_equal(iface_name, BLUEZ_IFACE_DEVICE1)) {
    bt_dbus_manager_remove_device(self, object_path);
  }
}

static void bluez_interfaces_added(GDBusConnection *connection,
                                   const gchar     *sender_name,
                                   const gchar     *object_path,
                                   const gchar     *interface_name,
                                   const gchar     *signal_name,
                                   GVariant        *parameters,
                                   gpointer         udata) {
  (void)connection;
  (void)sender_name;
  (void)object_path;
  (void)interface_name;
  (void)signal_name;
  (void)parameters;
  GVariantIter *iter  = NULL;
  gchar const  *path  = NULL;
  GVariant     *iface = NULL;
  g_variant_get(parameters, "(oa{sa{sv}})", &path, &iter);
  gchar const *name = NULL;
  while(g_variant_iter_loop(iter, "{&s@a{sv}}", &name, &iface)) {
    bt_dbus_manager_interface_added(BT_DBUS_MANAGER(udata), path, name);
  }
  g_variant_iter_free(iter);
}

static void bluez_interfaces_removed(GDBusConnection *connection,
                                     const gchar     *sender_name,
                                     const gchar     *object_path,
                                     const gchar     *interface_name,
                                     const gchar     *signal_name,
                                     GVariant        *parameters,
                                     gpointer         udata) {
  (void)connection;
  (void)sender_name;
  (void)object_path;
  (void)interface_name;
  (void)signal_name;
  (void)parameters;
  GVariantIter *iter = NULL;
  gchar const  *path = NULL;
  g_variant_get(parameters, "(oas)", &path, &iter);
  gchar const *name = NULL;
  while(g_variant_iter_loop(iter, "s", &name)) {
    bt_dbus_manager_interface_removed(BT_DBUS_MANAGER(udata), path, name);
  }
  g_variant_iter_free(iter);
}

static void bluez_get_managed_objects(GObject *source_object, GAsyncResult *res, gpointer udata) {
  GError   *error  = NULL;
  GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);
  if(NULL == error) {
    GVariantIter *object_iter           = NULL;
    gchar const  *object_path           = NULL;
    GVariant     *ifaces_and_properties = NULL;
    g_variant_get(result, "(a{oa{sa{sv}}})", &object_iter);
    while(g_variant_iter_loop(object_iter, "{&o@a{sa{sv}}}", &object_path, &ifaces_and_properties)) {
      GVariantIter *iface_iter = NULL;
      gchar const  *iface_name = NULL;
      GVariant     *properties = NULL;
      g_variant_get(ifaces_and_properties, "a{sa{sv}}", &iface_iter);
      while(g_variant_iter_loop(iface_iter, "{&s@a{sv}}", &iface_name, &properties)) {
        bt_dbus_manager_interface_added(BT_DBUS_MANAGER(udata), object_path, iface_name);
      }
      g_variant_iter_free(iface_iter);
    }
    g_variant_iter_free(object_iter);
    g_variant_unref(result);
  } else {
    g_printerr("Unable to get managed objects from org.bluez: %s\n", error->message);
  }
}

static void
bt_dbus_manager_name_appeared(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer udata) {
  (void)name;
  (void)name_owner;
  g_return_if_fail(BT_IS_DBUS_MANAGER(udata));
  BtDbusManager *self = BT_DBUS_MANAGER(udata);
  self->connection    = connection;

  self->iface_added = g_dbus_connection_signal_subscribe(connection,
                                                         BLUEZ_BUS_NAME,
                                                         BLUEZ_IFACE_OBJECT_MANAGER,
                                                         BLUEZ_INTERFACE_ADDED,
                                                         BLUEZ_PATH,
                                                         NULL,
                                                         G_DBUS_SIGNAL_FLAGS_NONE,
                                                         bluez_interfaces_added,
                                                         udata,
                                                         NULL);

  self->iface_removed = g_dbus_connection_signal_subscribe(connection,
                                                           BLUEZ_BUS_NAME,
                                                           BLUEZ_IFACE_OBJECT_MANAGER,
                                                           BLUEZ_INTERFACE_REMOVED,
                                                           BLUEZ_PATH,
                                                           NULL,
                                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                                           bluez_interfaces_removed,
                                                           udata,
                                                           NULL);

  g_dbus_connection_call(connection,
                         BLUEZ_BUS_NAME,
                         BLUEZ_PATH,
                         BLUEZ_IFACE_OBJECT_MANAGER,
                         BLUEZ_GET_MANAGED_OBJECTS,
                         NULL,
                         G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
                         G_DBUS_CALL_FLAGS_NONE,
                         -1,
                         NULL,
                         bluez_get_managed_objects,
                         udata);
}

static void bt_dbus_manager_init(BtDbusManager *self) {
  self->adapters   = g_hash_table_new(g_str_hash, g_str_equal);
  self->watcher_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                                      BLUEZ_BUS_NAME,
                                      G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                      bt_dbus_manager_name_appeared,
                                      NULL,
                                      self,
                                      NULL);
}

static GObject *
bt_dbus_manager_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_properties) {
  static GObject *instance = NULL;
  if(NULL == instance) {
    instance =
        G_OBJECT_CLASS(bt_dbus_manager_parent_class)->constructor(type, n_construct_properties, construct_properties);
    return instance;
  }
  return g_object_ref(instance);
}

static void bt_dbus_manager_finalize(GObject *object) {
  BtDbusManager *self = BT_DBUS_MANAGER(object);
  if(self->connection) {
    g_dbus_connection_signal_unsubscribe(self->connection, self->iface_added);
    g_dbus_connection_signal_unsubscribe(self->connection, self->iface_removed);
  }
  g_bus_unwatch_name(self->watcher_id);
  g_slist_free_full(self->devices, g_object_unref);
  G_OBJECT_CLASS(bt_dbus_manager_parent_class)->finalize(object);
}

static void bt_dbus_manager_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(BT_IS_DBUS_MANAGER(object));
  BtDbusManager *self = BT_DBUS_MANAGER(object);
  switch(property_id) {
    case PROP_N_ADAPTERS: g_value_set_uint(value, g_hash_table_size(self->adapters)); break;
    case PROP_N_DEVICE: g_value_set_uint(value, g_slist_length(self->devices)); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void bt_dbus_manager_class_init(BtDbusManagerClass *klass) {
  G_OBJECT_CLASS(klass)->constructor  = bt_dbus_manager_constructor;
  G_OBJECT_CLASS(klass)->finalize     = bt_dbus_manager_finalize;
  G_OBJECT_CLASS(klass)->get_property = bt_dbus_manager_get_property;

  g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_N_ADAPTERS, "n-adapters");
  g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_N_DEVICE, "n-devices");
}

static BtAdapter *bt_dbus_manager_get_adapter(BtManager *iface, gchar const *id) {
  g_return_val_if_fail(BT_IS_DBUS_MANAGER(iface), NULL);
  BtDbusManager *self = BT_DBUS_MANAGER(iface);
  return BT_ADAPTER(g_hash_table_lookup(self->adapters, id));
}

static BtDevice *bt_dbus_manager_get_device(BtManager *iface, gchar const *id) {
  g_return_val_if_fail(BT_IS_DBUS_MANAGER(iface), NULL);
  BtDbusManager *self = BT_DBUS_MANAGER(iface);
  GSList        *item = g_slist_find_custom(self->devices, id, bt_device_compare_by_id);
  return item ? BT_DEVICE(item->data) : NULL;
}

static GSList *bt_dbus_manager_get_devices(BtManager *iface) {
  g_return_val_if_fail(BT_IS_DBUS_MANAGER(iface), NULL);
  BtDbusManager *self = BT_DBUS_MANAGER(iface);
  return self->devices;
}

static void bt_dbus_manager_iface_init(BtManagerInterface *iface) {
  iface->get_adapter = bt_dbus_manager_get_adapter;
  iface->get_device  = bt_dbus_manager_get_device;
  iface->get_devices = bt_dbus_manager_get_devices;
}

BtManager *bt_dbus_manager_get_default(void) {
  return BT_MANAGER(g_object_new(BT_TYPE_DBUS_MANAGER, NULL));
}
