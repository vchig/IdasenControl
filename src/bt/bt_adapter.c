#include "bt_adapter.h"

#define BT_PROP_ADDRESS "Address"

struct _BtAdapter {
  GObject  parent;
  BtProxy *proxy;
  gchar   *address;
};

enum {
  PROP_0,
  PROP_ADDRESS,
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

G_DEFINE_FINAL_TYPE(BtAdapter, bt_adapter, G_TYPE_OBJECT)

static void
bt_adapter_proxy_property_changed(GObject *sender, gchar const *prop_name, GVariant *prop_value, gpointer udata) {
  (void)sender; // Is BtProxy
  g_return_if_fail(BT_IS_ADAPTER(udata));
  BtAdapter *self = BT_ADAPTER(udata);
  if(g_str_equal(prop_name, BT_PROP_ADDRESS)) {
    g_free(self->address);
    self->address = g_strdup(g_variant_get_string(prop_value, NULL));
  }
}

static void bt_adapter_init(BtAdapter *self) {
  (void)self;
}

static void bt_adapter_dispose(GObject *object) {
  BtAdapter *self = BT_ADAPTER(object);
  g_clear_pointer(&self->proxy, g_object_unref);
  g_free(self->address);
  G_OBJECT_CLASS(bt_adapter_parent_class)->dispose(object);
}

static void bt_adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(BT_IS_ADAPTER(object));
  BtAdapter *self = BT_ADAPTER(object);
  switch(property_id) {
    case PROP_ADDRESS: g_value_set_string(value, self->address); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void bt_adapter_class_init(BtAdapterClass *klass) {
  G_OBJECT_CLASS(klass)->dispose      = bt_adapter_dispose;
  G_OBJECT_CLASS(klass)->get_property = bt_adapter_get_property;

  g_properties[PROP_ADDRESS] = g_param_spec_string("address", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
}

BtAdapter *bt_adapter_new(BtProxy *proxy) {
  BtAdapter *self = g_object_new(BT_TYPE_ADAPTER, NULL);
  self->proxy     = proxy;
  self->address   = bt_proxy_get_string_property(self->proxy, BT_PROP_ADDRESS);
  g_signal_connect(self->proxy, "property-changed", G_CALLBACK(bt_adapter_proxy_property_changed), self);
  return self;
}

void bt_adapter_start_discovery(BtAdapter *self) {
  g_return_if_fail(BT_IS_ADAPTER(self));
  g_return_if_fail(BT_IS_PROXY(self->proxy));
  GError *error = NULL;
  bt_proxy_call(self->proxy, "StartDiscovery", NULL, &error);
  if(NULL != error) {
    g_warning("Failed start discovery. Error: %s\n", error->message);
  }
}

void bt_adapter_stop_discovery(BtAdapter *self) {
  g_return_if_fail(BT_IS_ADAPTER(self));
  g_return_if_fail(BT_IS_PROXY(self->proxy));
  GError *error = NULL;
  bt_proxy_call(self->proxy, "StopDiscovery", NULL, &error);
  if(NULL != error) {
    g_warning("Failed stop discovery. Error: %s\n", error->message);
  }
}

gchar const *bt_adapter_get_address(BtAdapter *self) {
  g_return_val_if_fail(BT_IS_ADAPTER(self), NULL);
  return self->address;
}
