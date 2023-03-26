#include "bt_device.h"

#define CONNECTED_PROPERTY_NAME "Connected"
#define PAIRED_PROPERTY_NAME "Paired"
#define ALIAS_PROPERTY_NAME "Alias"
#define ADDRESS_PROPERTY_NAME "Address"
#define UUIDS_PROPERTY_NAME "UUIDs"
#define PAIR_METHOD_NAME "Pair"
#define CONNECT_METHOD_NAME "Connect"
#define DISCONNECT_METHOD_NAME "Disconnect"

struct _BtDevice {
  GObject   parent;
  BtProxy  *proxy;
  GSList   *gatt_services;
  gchar    *id;
  gchar    *alias;
  gchar    *address;
  GVariant *uuids;
  gboolean  is_paired;
  gboolean  is_connected;
};

enum {
  PROP_0,
  PROP_ALIAS,        // Псевдоним
  PROP_ADDRESS,      // Mac адрес устройства
  PROP_IS_PAIRED,    // Флаг сопряжения
  PROP_IS_CONNECTED, // Флаг подключения
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

enum {
  SIG_SERVICE_ADD,
  SIG_SERVICE_REM,
  N_SIGNALS,
};

static guint g_signals[N_SIGNALS] = { 0u };

G_DEFINE_FINAL_TYPE(BtDevice, bt_device, G_TYPE_OBJECT)

static void bt_device_init(BtDevice *self) {
  (void)self;
}

static void bt_device_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(BT_IS_DEVICE(object));
  BtDevice *self = BT_DEVICE(object);
  switch(property_id) {
    case PROP_ALIAS: g_value_set_string(value, bt_device_get_alias(self)); break;
    case PROP_ADDRESS: g_value_set_string(value, bt_device_get_address(self)); break;
    case PROP_IS_PAIRED: g_value_set_boolean(value, bt_device_is_paired(self)); break;
    case PROP_IS_CONNECTED: g_value_set_boolean(value, bt_device_is_connected(self)); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void bt_device_dispose(GObject *object) {
  BtDevice *self = BT_DEVICE(object);
  g_clear_pointer(&self->proxy, g_object_unref);
  g_free(self->id);
  g_free(self->alias);
  g_free(self->address);
  g_slist_free_full(self->gatt_services, g_object_unref);
  g_clear_pointer(&self->uuids, g_variant_unref);
  G_OBJECT_CLASS(bt_device_parent_class)->dispose(object);
}

static void bt_device_class_init(BtDeviceClass *klass) {
  G_OBJECT_CLASS(klass)->get_property = bt_device_get_property;
  G_OBJECT_CLASS(klass)->dispose      = bt_device_dispose;

  g_properties[PROP_ALIAS]   = g_param_spec_string("alias", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_ADDRESS] = g_param_spec_string("address", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_IS_PAIRED] =
      g_param_spec_boolean("is-paired", "", "", FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_IS_CONNECTED] =
      g_param_spec_boolean("is-connected", "", "", FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properties);

  g_signals[SIG_SERVICE_ADD] = g_signal_new("service-append",
                                            BT_TYPE_DEVICE,
                                            G_SIGNAL_RUN_LAST,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            G_TYPE_NONE,
                                            1,
                                            BT_TYPE_GATT_SERVICE);
  g_signals[SIG_SERVICE_REM] = g_signal_new("service-removed",
                                            BT_TYPE_DEVICE,
                                            G_SIGNAL_RUN_LAST,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            G_TYPE_NONE,
                                            1,
                                            BT_TYPE_GATT_SERVICE);
}

static void
bt_device_proxy_property_changed(GObject *sender, gchar const *prop_name, GVariant *prop_value, gpointer udata) {
  (void)sender; // Is BtProxy
  g_return_if_fail(BT_IS_DEVICE(udata));
  BtDevice *self = BT_DEVICE(udata);
  if(g_str_equal(prop_name, ALIAS_PROPERTY_NAME)) {
    g_free(self->alias);
    self->alias = g_strdup(g_variant_get_string(prop_value, NULL));
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_ALIAS]);
  } else if(g_str_equal(prop_name, ADDRESS_PROPERTY_NAME)) {
    self->address = g_strdup(g_variant_get_string(prop_value, NULL));
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_ADDRESS]);
  } else if(g_str_equal(prop_name, PAIRED_PROPERTY_NAME)) {
    self->is_paired = g_variant_get_boolean(prop_value);
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_IS_PAIRED]);
  } else if(g_str_equal(prop_name, CONNECTED_PROPERTY_NAME)) {
    self->is_connected = g_variant_get_boolean(prop_value);
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_IS_CONNECTED]);
  }
}

BtDevice *bt_device_new(BtProxy *proxy, gchar const *id) {
  g_return_val_if_fail(BT_IS_PROXY(proxy), NULL);
  BtDevice *self     = BT_DEVICE(g_object_new(BT_TYPE_DEVICE, NULL));
  self->id           = g_strdup(id);
  self->proxy        = proxy;
  self->alias        = bt_proxy_get_string_property(self->proxy, ALIAS_PROPERTY_NAME);
  self->address      = bt_proxy_get_string_property(self->proxy, ADDRESS_PROPERTY_NAME);
  self->uuids        = bt_proxy_get_property(self->proxy, UUIDS_PROPERTY_NAME);
  self->is_paired    = bt_proxy_get_boolean_property(self->proxy, PAIRED_PROPERTY_NAME);
  self->is_connected = bt_proxy_get_boolean_property(self->proxy, CONNECTED_PROPERTY_NAME);
  g_signal_connect(self->proxy, "property-changed", G_CALLBACK(bt_device_proxy_property_changed), self);
  return self;
}

gchar const *bt_device_get_id(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  return self->id;
}

gchar const *bt_device_get_alias(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  return self->alias;
}

gchar const *bt_device_get_address(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  return self->address;
}

gboolean bt_device_is_paired(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), FALSE);
  return self->is_paired;
}

void bt_device_pair(BtDevice *self) {
  g_return_if_fail(BT_IS_DEVICE(self));
  g_return_if_fail(BT_IS_PROXY(self->proxy));
  if(FALSE == bt_device_is_paired(self)) {
    GError *error = NULL;
    bt_proxy_call(self->proxy, PAIR_METHOD_NAME, NULL, &error);
    if(NULL != error) {
      g_warning("Failed pairing with device '%s'. Error: %s\n", bt_device_get_alias(self), error->message);
    }
  }
}

gboolean bt_device_is_connected(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), FALSE);
  return self->is_connected;
}

void bt_device_connect(BtDevice *self) {
  g_return_if_fail(BT_IS_DEVICE(self));
  g_return_if_fail(BT_IS_PROXY(self->proxy));
  if(FALSE == bt_device_is_connected(self)) {
    GError *error = NULL;
    bt_proxy_call(self->proxy, CONNECT_METHOD_NAME, NULL, &error);
    if(NULL != error) {
      g_warning("Failed connecting to device '%s'. Error: %s\n", bt_device_get_alias(self), error->message);
    }
  }
}

void bt_device_disconnect(BtDevice *self) {
  g_return_if_fail(BT_IS_DEVICE(self));
  g_return_if_fail(BT_IS_PROXY(self->proxy));
  if(bt_device_is_connected(self)) {
    GError *error = NULL;
    bt_proxy_call(self->proxy, DISCONNECT_METHOD_NAME, NULL, &error);
    if(NULL != error) {
      g_warning("Failed disconnect from device '%s'. Error: %s\n", bt_device_get_alias(self), error->message);
    }
  }
}

GVariant *bt_device_get_uuids(BtDevice *self) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  return self->uuids;
}

void bt_device_append_gatt_service(BtDevice *self, BtGattService *gatt_service) {
  g_return_if_fail(BT_IS_DEVICE(self));
  self->gatt_services = g_slist_append(self->gatt_services, gatt_service);
  g_signal_emit(self, g_signals[SIG_SERVICE_ADD], 0, gatt_service);
}

void bt_device_remove_gatt_service(BtDevice *self, BtGattService *gatt_service) {
  g_return_if_fail(BT_IS_DEVICE(self));
  self->gatt_services = g_slist_remove(self->gatt_services, gatt_service);
  g_signal_emit(self, g_signals[SIG_SERVICE_REM], 0, gatt_service);
}

static int find_service_by_uuid(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_GATT_SERVICE((gpointer)value), -1);
  return g_strcmp0(bt_gatt_service_get_uuid(BT_GATT_SERVICE((gpointer)value)), udata);
}

BtGattService *bt_device_find_service_by_uuid(BtDevice *self, gchar const *service_uuid) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  GSList *service = g_slist_find_custom(self->gatt_services, service_uuid, find_service_by_uuid);
  return service ? BT_GATT_SERVICE(service->data) : NULL;
}

static int find_service_by_id(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_GATT_SERVICE((gpointer)value), -1);
  return g_strcmp0(bt_gatt_service_get_id(BT_GATT_SERVICE((gpointer)value)), udata);
}

BtGattService *bt_device_find_service_by_id(BtDevice *self, gchar const *service_id) {
  g_return_val_if_fail(BT_IS_DEVICE(self), NULL);
  GSList *service = g_slist_find_custom(self->gatt_services, service_id, find_service_by_id);
  return service ? BT_GATT_SERVICE(service->data) : NULL;
}

gint bt_device_compare_by_id(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_DEVICE((gpointer)value), -1);
  return g_strcmp0(bt_device_get_id(BT_DEVICE((gpointer)value)), udata);
}
