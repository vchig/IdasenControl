#pragma once
#include "bt_proxy.h"

G_BEGIN_DECLS

#define BT_TYPE_GATT_CHARACTERISTIC (bt_gatt_characteristic_get_type())

G_DECLARE_FINAL_TYPE(BtGattCharacteristic, bt_gatt_characteristic, BT, GATT_CHARACTERISTIC, GObject)

BtGattCharacteristic *bt_gatt_characteristic_new(BtProxy *proxy);

void bt_gatt_characteristic_start_notify(BtGattCharacteristic *self, GError **error);

void bt_gatt_characteristic_stop_notify(BtGattCharacteristic *self, GError **error);

GVariant *bt_gatt_characteristic_get_value(BtGattCharacteristic *self);

gchar const *bt_gatt_characteristic_get_uuid(BtGattCharacteristic *self);

gchar const *bt_gatt_characteristic_get_service_id(BtGattCharacteristic *self);

void bt_gatt_characteristic_write_value(BtGattCharacteristic *self,
                                        guchar const         *buffer,
                                        size_t                buffer_size,
                                        GError              **error);

GVariant *bt_gatt_characteristic_read_value(BtGattCharacteristic *self, GError **error);

G_END_DECLS
