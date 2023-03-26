#pragma once
#include "bt_gatt_service.h"

G_BEGIN_DECLS

#define BT_TYPE_DEVICE (bt_device_get_type())

G_DECLARE_FINAL_TYPE(BtDevice, bt_device, BT, DEVICE, GObject)

BtDevice *bt_device_new(BtProxy *proxy, gchar const *id);

gchar const *bt_device_get_id(BtDevice *self);

gchar const *bt_device_get_alias(BtDevice *self);

gchar const *bt_device_get_address(BtDevice *self);

gboolean bt_device_is_paired(BtDevice *self);

void bt_device_pair(BtDevice *self);

gboolean bt_device_is_connected(BtDevice *self);

void bt_device_connect(BtDevice *self);

void bt_device_disconnect(BtDevice *self);

GVariant *bt_device_get_uuids(BtDevice *self);

void bt_device_append_gatt_service(BtDevice *self, BtGattService *gatt_service);

void bt_device_remove_gatt_service(BtDevice *self, BtGattService *gatt_service);

BtGattService *bt_device_find_service_by_uuid(BtDevice *self, gchar const *service_uuid);

BtGattService *bt_device_find_service_by_id(BtDevice *self, gchar const *service_id);

/// @brief
/// @param value
/// @param udata
/// @return
gint bt_device_compare_by_id(gconstpointer value, gconstpointer udata);

G_END_DECLS
