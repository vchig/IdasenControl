#include "bt_gatt_characteristic.h"

#define START_NOTIFY_METHOD_NAME "StartNotify"
#define STOP_NOTIFY_METHOD_NAME "StopNotify"
#define WRITE_VALUE_METHOD_NAME "WriteValue"
#define READ_VALUE_METHOD_NAME "ReadValue"

#define VALUE_PROP_NAME "Value"
#define UUID_PROP_NAME "UUID"
#define SERVICE_PROP_NAME "Service"

struct _BtGattCharacteristic {
  GObject   parent;
  BtProxy  *proxy;
  GVariant *value;
  gchar    *uuid;
  gchar    *service_id;
};

enum {
  PROP_0,
  PROP_VALUE,
  PROP_UUID,
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

G_DEFINE_FINAL_TYPE(BtGattCharacteristic, bt_gatt_characteristic, G_TYPE_OBJECT)

static GVariant *make_gatt_characteristic_write_value_params(guchar const *buffer, size_t buffer_size) {
  GVariant       *value = g_variant_new_from_data(G_VARIANT_TYPE("ay"), buffer, buffer_size, TRUE, NULL, NULL);
  GVariantBuilder builder_options;
  g_variant_builder_init(&builder_options, G_VARIANT_TYPE("a{sv}"));
  return g_variant_new("(@aya{sv})", value, &builder_options);
}

static void bt_gatt_characteristic_init(BtGattCharacteristic *self) {
  (void)self;
}

static void bt_gatt_characteristic_dispose(GObject *object) {
  BtGattCharacteristic *self = BT_GATT_CHARACTERISTIC(object);
  g_clear_pointer(&self->proxy, g_object_unref);
  g_free(self->uuid);
  g_free(self->service_id);
  G_OBJECT_CLASS(bt_gatt_characteristic_parent_class)->dispose(object);
}

static void bt_gatt_characteristic_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(object));
  BtGattCharacteristic *self = BT_GATT_CHARACTERISTIC(object);
  switch(property_id) {
    case PROP_VALUE: g_value_set_variant(value, self->value); break;
    case PROP_UUID: g_value_set_string(value, self->uuid); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void bt_gatt_characteristic_class_init(BtGattCharacteristicClass *klass) {
  G_OBJECT_CLASS(klass)->dispose      = bt_gatt_characteristic_dispose;
  G_OBJECT_CLASS(klass)->get_property = bt_gatt_characteristic_get_property;

  g_properties[PROP_VALUE] =
      g_param_spec_variant("value", "", "", G_VARIANT_TYPE_BYTESTRING, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_UUID] = g_param_spec_string("uuid", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properties);
}

static void
bt_gatt_characteristic_property_changed(GObject *sender, gchar const *prop_name, GVariant *prop_value, gpointer udata) {
  (void)sender; // Is BtProxy
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(udata));
  BtGattCharacteristic *self = BT_GATT_CHARACTERISTIC(udata);
  if(g_str_equal(prop_name, VALUE_PROP_NAME)) {
    g_clear_pointer(&self->value, g_variant_unref);
    self->value = g_variant_ref(prop_value);
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_VALUE]);
  }
}

BtGattCharacteristic *bt_gatt_characteristic_new(BtProxy *proxy) {
  g_return_val_if_fail(BT_IS_PROXY(proxy), NULL);
  BtGattCharacteristic *self = g_object_new(BT_TYPE_GATT_CHARACTERISTIC, NULL);
  self->proxy                = proxy;
  self->value                = bt_proxy_get_property(self->proxy, VALUE_PROP_NAME);
  self->uuid                 = bt_proxy_get_string_property(self->proxy, UUID_PROP_NAME);
  self->service_id           = bt_proxy_get_string_property(self->proxy, SERVICE_PROP_NAME);
  g_signal_connect(self->proxy, "property-changed", G_CALLBACK(bt_gatt_characteristic_property_changed), self);
  return self;
}

void bt_gatt_characteristic_start_notify(BtGattCharacteristic *self, GError **error) {
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self));
  g_return_if_fail(error == NULL || *error == NULL);
  bt_proxy_call(self->proxy, START_NOTIFY_METHOD_NAME, NULL, error);
}

void bt_gatt_characteristic_stop_notify(BtGattCharacteristic *self, GError **error) {
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self));
  g_return_if_fail(error == NULL || *error == NULL);
  bt_proxy_call(self->proxy, STOP_NOTIFY_METHOD_NAME, NULL, error);
}

GVariant *bt_gatt_characteristic_get_value(BtGattCharacteristic *self) {
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC(self), NULL);
  return self->value;
}

gchar const *bt_gatt_characteristic_get_uuid(BtGattCharacteristic *self) {
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC(self), NULL);
  return self->uuid;
}

gchar const *bt_gatt_characteristic_get_service_id(BtGattCharacteristic *self) {
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC(self), NULL);
  return self->service_id;
}

void bt_gatt_characteristic_write_value(BtGattCharacteristic *self,
                                        guchar const         *buffer,
                                        size_t                buffer_size,
                                        GError              **error) {
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self));
  GVariant *arguments = make_gatt_characteristic_write_value_params(buffer, buffer_size);
  bt_proxy_call(self->proxy, WRITE_VALUE_METHOD_NAME, arguments, error);
}

GVariant *bt_gatt_characteristic_read_value(BtGattCharacteristic *self, GError **error) {
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC(self), NULL);
  GVariantBuilder builder_options;
  g_variant_builder_init(&builder_options, G_VARIANT_TYPE("a{sv}"));
  GVariant *args = g_variant_new("(a{sv})", &builder_options);
  return bt_proxy_call(self->proxy, READ_VALUE_METHOD_NAME, args, error);
}
