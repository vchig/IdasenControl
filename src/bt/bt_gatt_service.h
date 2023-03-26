#pragma once
#include "bt_gatt_characteristic.h"

G_BEGIN_DECLS

#define BT_TYPE_GATT_SERVICE (bt_gatt_service_get_type())

G_DECLARE_FINAL_TYPE(BtGattService, bt_gatt_service, BT, GATT_SERVICE, GObject)

BtGattService *bt_gatt_service_new(BtProxy *proxy, gchar const *id);

void bt_gatt_service_append_gatt_characteristic(BtGattService *self, BtGattCharacteristic *gatt_char);

void bt_gatt_service_remove_gatt_characteristic(BtGattService *self, BtGattCharacteristic *gatt_char);

gchar const *bt_gatt_service_get_uuid(BtGattService *self);

gchar const *bt_gatt_service_get_id(BtGattService *self);

BtGattCharacteristic *bt_gatt_service_find_char_by_uuid(BtGattService *self, gchar const *uuid);

G_END_DECLS
