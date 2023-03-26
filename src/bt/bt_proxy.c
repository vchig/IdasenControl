#include "bt_proxy.h"

enum {
  SIGNAL_PROPERTY_CHANGED,
  N_SIGNALS,
};

static guint g_signals[N_SIGNALS] = { 0u };

G_DEFINE_INTERFACE(BtProxy, bt_proxy, G_TYPE_OBJECT)

static void bt_proxy_default_init(BtProxyInterface *klass) {
  (void)klass;
  g_signals[SIGNAL_PROPERTY_CHANGED] = g_signal_new("property-changed",
                                                    BT_TYPE_PROXY,
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    G_TYPE_NONE,
                                                    2,
                                                    G_TYPE_STRING,
                                                    G_TYPE_VARIANT);
}

GVariant *bt_proxy_call(BtProxy *self, gchar const *method_name, GVariant *arguments, GError **error) {
  g_return_val_if_fail(BT_IS_PROXY(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  BtProxyInterface *iface = BT_PROXY_GET_IFACE(self);
  g_return_val_if_fail(iface->call != NULL, NULL);
  return iface->call(self, method_name, arguments, error);
}

GVariant *bt_proxy_get_property(BtProxy *self, gchar const *property_name) {
  g_return_val_if_fail(BT_IS_PROXY(self), NULL);
  BtProxyInterface *iface = BT_PROXY_GET_IFACE(self);
  g_return_val_if_fail(iface->get_property != NULL, NULL);
  return iface->get_property(self, property_name);
}

gboolean bt_proxy_get_boolean_property(BtProxy *self, gchar const *property_name) {
  GVariant *property_value = bt_proxy_get_property(self, property_name);
  g_return_val_if_fail(property_value, FALSE);
  gboolean result = g_variant_get_boolean(property_value);
  g_variant_unref(property_value);
  return result;
}

gchar *bt_proxy_get_string_property(BtProxy *self, gchar const *property_name) {
  GVariant *property_value = bt_proxy_get_property(self, property_name);
  g_return_val_if_fail(property_value, FALSE);
  gchar *result = g_strdup(g_variant_get_string(property_value, NULL));
  g_variant_unref(property_value);
  return result;
}

void bt_proxy_property_changed(BtProxy *self, gchar const *property_name, GVariant *value) {
  g_return_if_fail(BT_IS_PROXY(self));
  g_signal_emit(self, g_signals[SIGNAL_PROPERTY_CHANGED], 0, property_name, value);
}
