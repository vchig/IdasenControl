#pragma once
#include "bt_device.h"
#include "bt_manager.h"

G_BEGIN_DECLS

#define BT_TYPE_DEVICE_LIST (bt_device_list_get_type())

G_DECLARE_FINAL_TYPE(BtDeviceList, bt_device_list, BT, DEVICE_LIST, GObject)

/// @brief
/// @param manager
/// @return
BtDeviceList *bt_device_list_new(BtManager *manager);

G_END_DECLS
