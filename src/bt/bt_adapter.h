#pragma once
#include "bt_proxy.h"

G_BEGIN_DECLS

#define BT_TYPE_ADAPTER (bt_adapter_get_type())

G_DECLARE_FINAL_TYPE(BtAdapter, bt_adapter, BT, ADAPTER, GObject)

BtAdapter *bt_adapter_new(BtProxy *proxy);

void bt_adapter_start_discovery(BtAdapter *self);

void bt_adapter_stop_discovery(BtAdapter *self);

gchar const *bt_adapter_get_address(BtAdapter *self);

G_END_DECLS
