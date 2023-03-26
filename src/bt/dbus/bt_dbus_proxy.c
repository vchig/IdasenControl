#include "bt_dbus_proxy.h"

#define BT_TYPE_DBUS_PROXY (bt_dbus_proxy_get_type())

struct _BtDbusProxy {
  GObject     parent;
  GDBusProxy *proxy;
};

static void bt_dbus_proxy_iface_init(BtProxyInterface *iface);

G_DECLARE_FINAL_TYPE(BtDbusProxy, bt_dbus_proxy, BT, DBUS_PROXY, GObject)

G_DEFINE_FINAL_TYPE_WITH_CODE(BtDbusProxy,
                              bt_dbus_proxy,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(BT_TYPE_PROXY, bt_dbus_proxy_iface_init))

static void bt_dbus_proxy_properties_changed(GDBusProxy *device_proxy,
                                             GVariant   *changed_properties,
                                             char      **invalidated_properties,
                                             gpointer    udata) {
  (void)device_proxy;
  (void)invalidated_properties;
  g_return_if_fail(BT_IS_PROXY(udata));
  BtProxy *iface = BT_PROXY(udata);
  if(g_variant_n_children(changed_properties) > 0) {
    GVariantIter *iter;
    const gchar  *key;
    GVariant     *value;
    g_variant_get(changed_properties, "a{sv}", &iter);
    while(g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
      bt_proxy_property_changed(iface, key, value);
    }
    g_variant_iter_free(iter);
  }
}

static void bt_dbus_proxy_init(BtDbusProxy *self) {
  (void)self;
}

static void bt_dbus_proxy_dispose(GObject *object) {
  BtDbusProxy *self = BT_DBUS_PROXY(object);
  g_clear_pointer(&self->proxy, g_object_unref);
  G_OBJECT_CLASS(bt_dbus_proxy_parent_class)->dispose(object);
}

static void bt_dbus_proxy_class_init(BtDbusProxyClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = bt_dbus_proxy_dispose;
}

static GVariant *bt_dbus_proxy_call(BtProxy *iface, gchar const *method_name, GVariant *arguments, GError **error) {
  g_return_val_if_fail(BT_IS_DBUS_PROXY(iface), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  BtDbusProxy *self = BT_DBUS_PROXY(iface);
  g_return_val_if_fail(G_IS_DBUS_PROXY(self->proxy), NULL);
  return g_dbus_proxy_call_sync(self->proxy, method_name, arguments, G_DBUS_CALL_FLAGS_NONE, 2000u, NULL, error);
}

static GVariant *bt_dbus_proxy_get_property(BtProxy *iface, gchar const *property_name) {
  g_return_val_if_fail(BT_IS_DBUS_PROXY(iface), NULL);
  BtDbusProxy *self = BT_DBUS_PROXY(iface);
  g_return_val_if_fail(G_IS_DBUS_PROXY(self->proxy), NULL);
  return g_dbus_proxy_get_cached_property(self->proxy, property_name);
}

static void bt_dbus_proxy_iface_init(BtProxyInterface *iface) {
  iface->call         = bt_dbus_proxy_call;
  iface->get_property = bt_dbus_proxy_get_property;
}

BtProxy *bt_dbus_proxy_new(GDBusProxy *proxy) {
  BtDbusProxy *self = g_object_new(BT_TYPE_DBUS_PROXY, NULL);
  self->proxy       = proxy;
  g_signal_connect(self->proxy, "g-properties-changed", G_CALLBACK(bt_dbus_proxy_properties_changed), self);
  return BT_PROXY(self);
}
