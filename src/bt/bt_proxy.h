#pragma once
#include <gio/gio.h>

G_BEGIN_DECLS

#define BT_TYPE_PROXY (bt_proxy_get_type())

G_DECLARE_INTERFACE(BtProxy, bt_proxy, BT, PROXY, GObject)

struct _BtProxyInterface {
  GTypeInterface parent_iface;

  GVariant *(*call)(BtProxy *self, gchar const *method_name, GVariant *arguments, GError **error);
  GVariant *(*get_property)(BtProxy *self, gchar const *name);

  gpointer padding[8];
};

GVariant *bt_proxy_call(BtProxy *self, gchar const *method_name, GVariant *arguments, GError **error);

GVariant *bt_proxy_get_property(BtProxy *self, gchar const *property_name);

gboolean bt_proxy_get_boolean_property(BtProxy *self, gchar const *property_name);

gchar *bt_proxy_get_string_property(BtProxy *self, gchar const *property_name);

void bt_proxy_property_changed(BtProxy *self, gchar const *property_name, GVariant *value);

G_END_DECLS
