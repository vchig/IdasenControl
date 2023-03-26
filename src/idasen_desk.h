#pragma once
#include "bt/bt_manager.h"

G_BEGIN_DECLS

#define IDASEN_TYPE_DESK (idasen_desk_get_type())

G_DECLARE_FINAL_TYPE(IdasenDesk, idasen_desk, IDASEN, DESK, GObject)

/// @brief
/// @param
/// @return
IdasenDesk *idasen_desk_new(BtManager *manager);

/// @brief Устанавливает блютуз адрес устройства.
/// @param self Указатель на объект.
/// @param address Новый адрес устройства.
void idasen_desk_set_device_address(IdasenDesk *self, gchar const *address);

/// @brief Возвращает блютуз адрес устройства.
/// @param self Указатель на объект.
/// @return адрес устройства.
gchar const *idasen_desk_get_device_address(IdasenDesk *self);

/// @brief
/// @param self
void idasen_desk_connect(IdasenDesk *self);

/// @brief
/// @param self
/// @return
gchar const *idasen_desk_get_title(IdasenDesk *self);

/// @brief Возвращает статус подключения к блютуз устройство.
/// @param self Указатель на объект.
/// @return TRUE устройство подключено.
gboolean idasen_desk_get_is_connected(IdasenDesk *self);

/// @brief Возвращает текущую высоту стола.
/// @param self Указатель на объект.
/// @return Текущая высота стола в сантиметрах.
gdouble idasen_desk_get_actual_height(IdasenDesk *self);

/// @brief Возвращает текущую скорость стола.
/// @param self Указатель на объект.
/// @return Текущая скорость стола в см/с.
gdouble idasen_desk_get_actual_velocity(IdasenDesk *self);

/// @brief Устанавливает необходимую высоту.
/// Перед началом движения необходимо послать последовательность команд wakeup и stop.
/// Подсмотрено в https://github.com/rhyst/idasen-controller
/// @param self Указатель на объект.
/// @param height Высота в сантиметрах.
void idasen_desk_set_height(IdasenDesk *self, gdouble height);

/// @brief Отправляет команду на подъём стола.
/// @param self Указатель на объект.
void idasen_desk_up(IdasenDesk *self);

/// @brief Отправляет команду на опускание стола.
/// @param self Указатель на объект.
void idasen_desk_down(IdasenDesk *self);

/// @brief Отправляет команду Stop?, зачем нужна эта команда - хз.
/// @param self Указатель на объект.
void idasen_desk_stop(IdasenDesk *self);

/// @brief Отправляет команду WakeUp?, зачем нужна эта команда - хз.
/// @param self Указатель на объект.
void idasen_desk_wakeup(IdasenDesk *self);

/// @brief Возвращает признак готовности стола.
/// @param self Указатель на объект.
/// @return Признак готовности стола.
gboolean idasen_desk_is_ready(IdasenDesk *self);

G_END_DECLS
