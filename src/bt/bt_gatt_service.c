#include "bt_gatt_service.h"

#define UUID_PROP_NAME "UUID"

struct _BtGattService {
  GObject  parent;
  BtProxy *proxy;
  GSList  *gatt_chars;
  gchar   *id;
  gchar   *uuid;
};

enum {
  PROP_0,
  PROP_UUID,
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

enum {
  SIG_CHARAC_ADD,
  SIG_CHARAC_REM,
  N_SIGNALS,
};

static guint g_signals[N_SIGNALS] = { 0u };

G_DEFINE_FINAL_TYPE(BtGattService, bt_gatt_service, G_TYPE_OBJECT)

static void bt_gatt_service_init(BtGattService *self) {
  (void)self;
}

static void bt_gatt_service_dispose(GObject *object) {
  BtGattService *self = BT_GATT_SERVICE(object);
  g_slist_free_full(self->gatt_chars, g_object_unref);
  g_free(self->id);
  g_free(self->uuid);
  G_OBJECT_CLASS(bt_gatt_service_parent_class)->dispose(object);
}

static void bt_gatt_service_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  switch(property_id) {
    case PROP_UUID: g_value_set_string(value, bt_gatt_service_get_uuid(BT_GATT_SERVICE(object))); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void bt_gatt_service_class_init(BtGattServiceClass *klass) {
  G_OBJECT_CLASS(klass)->dispose      = bt_gatt_service_dispose;
  G_OBJECT_CLASS(klass)->get_property = bt_gatt_service_get_property;

  g_properties[PROP_UUID] = g_param_spec_string("uuid", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properties);

  g_signals[SIG_CHARAC_ADD] = g_signal_new("char-added",
                                           BT_TYPE_GATT_SERVICE,
                                           G_SIGNAL_RUN_LAST,
                                           0,
                                           NULL,
                                           NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           BT_TYPE_GATT_CHARACTERISTIC);

  g_signals[SIG_CHARAC_REM] = g_signal_new("char-removed",
                                           BT_TYPE_GATT_SERVICE,
                                           G_SIGNAL_RUN_LAST,
                                           0,
                                           NULL,
                                           NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           BT_TYPE_GATT_CHARACTERISTIC);
}

static void
bt_gatt_service_property_changed(GObject *sender, gchar const *prop_name, GVariant *prop_value, gpointer udata) {
  (void)sender; // Is BtProxy
  g_return_if_fail(BT_IS_GATT_SERVICE(udata));
  BtGattService *self = BT_GATT_SERVICE(udata);
  if(g_str_equal(prop_name, UUID_PROP_NAME)) {
    g_free(self->uuid);
    self->uuid = g_strdup(g_variant_get_string(prop_value, NULL));
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_UUID]);
  }
}

BtGattService *bt_gatt_service_new(BtProxy *proxy, gchar const *id) {
  BtGattService *self = g_object_new(BT_TYPE_GATT_SERVICE, NULL);
  self->id            = g_strdup(id);
  self->proxy         = proxy;
  self->uuid          = bt_proxy_get_string_property(self->proxy, UUID_PROP_NAME);
  g_signal_connect(self->proxy, "property-changed", G_CALLBACK(bt_gatt_service_property_changed), self);
  return self;
}

void bt_gatt_service_append_gatt_characteristic(BtGattService *self, BtGattCharacteristic *gatt_char) {
  g_return_if_fail(BT_IS_GATT_SERVICE(self));
  self->gatt_chars = g_slist_append(self->gatt_chars, gatt_char);
  g_signal_emit(self, g_signals[SIG_CHARAC_ADD], 0, gatt_char);
}

void bt_gatt_service_remove_gatt_characteristic(BtGattService *self, BtGattCharacteristic *gatt_char) {
  g_return_if_fail(BT_IS_GATT_SERVICE(self));
  self->gatt_chars = g_slist_remove(self->gatt_chars, gatt_char);
  g_signal_emit(self, g_signals[SIG_CHARAC_REM], 0, gatt_char);
}

gchar const *bt_gatt_service_get_uuid(BtGattService *self) {
  g_return_val_if_fail(BT_IS_GATT_SERVICE(self), NULL);
  return self->uuid;
}

gchar const *bt_gatt_service_get_id(BtGattService *self) {
  g_return_val_if_fail(BT_IS_GATT_SERVICE(self), NULL);
  return self->id;
}

static gint find_characteristic_by_uuid(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC((gpointer)value), -1);
  return g_strcmp0(bt_gatt_characteristic_get_uuid(BT_GATT_CHARACTERISTIC((gpointer)value)), udata);
}

BtGattCharacteristic *bt_gatt_service_find_char_by_uuid(BtGattService *self, gchar const *uuid) {
  g_return_val_if_fail(BT_IS_GATT_SERVICE(self), NULL);
  GSList *item = g_slist_find_custom(self->gatt_chars, uuid, find_characteristic_by_uuid);
  return item ? BT_GATT_CHARACTERISTIC(item->data) : NULL;
}
