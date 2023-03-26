#pragma once
#include "bt_adapter.h"
#include "bt_device.h"

G_BEGIN_DECLS
#define BT_TYPE_MANAGER (bt_manager_get_type())

G_DECLARE_INTERFACE(BtManager, bt_manager, BT, MANAGER, GObject)

struct _BtManagerInterface {
  GTypeInterface parent;

  /// @brief Запрашивает адаптер по идентификатору.
  /// @param iface Указатель на интерфейс.
  /// @param id Идентификатор адаптера.
  /// @return Указатель на объект адаптера или NULL, если адаптер отсутствует.
  BtAdapter *(*get_adapter)(BtManager *iface, gchar const *id);

  /// @brief Запрашивает устройство по идентификатору.
  /// @param iface Указатель на интерфейс.
  /// @param id Идентификатор устройства.
  /// @return Указатель на устройство или NULL, если устройство отсутствует.
  BtDevice *(*get_device)(BtManager *iface, gchar const *id);

  /// @brief Запрашивает список устройств.
  GSList *(*get_devices)(BtManager *iface);

  /// @brief Для добавления новых функций без изменения ABI.
  gpointer padding[8];
};

BtAdapter *bt_manager_get_adapter(BtManager *self, gchar const *id);

BtDevice *bt_manager_get_device(BtManager *self, gchar const *id);

GSList *bt_manager_get_devices(BtManager *self);

/// @brief Возвращает устройство с заданным адресом.
/// @param self Указатель на интерфейс.
/// @param address Адрес искомого устройства.
/// @return Указатель на устройство или NULL, если устройство с заданным адресом отсутствует.
BtDevice *bt_manager_get_device_by_address(BtManager *self, gchar const *address);

void bt_manager_adapter_added(BtManager *self, gchar const *id);

void bt_manager_adapter_removed(BtManager *self, gchar const *id);

void bt_manager_device_added(BtManager *self, gchar const *id);

void bt_manager_device_removed(BtManager *self, gchar const *id);

G_END_DECLS
