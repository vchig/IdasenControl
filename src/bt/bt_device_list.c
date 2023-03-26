#include "bt_device_list.h"

#include "bt_device.h"

#define BT_DEVICE_INTERFACE_NAME "org.bluez.Device1"

struct _BtDeviceList {
  GObject    parent;
  BtManager *manager;
  GSList    *devices;
};

// Предварительное объявление функции инициализации интерфейса.
static void bt_device_list_model_iface_init(GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(BtDeviceList,
                              bt_device_list,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL, bt_device_list_model_iface_init))

static void bt_device_list_init(BtDeviceList *self) {
  (void)self;
}

static void bt_device_list_dispose(GObject *object) {
  BtDeviceList *self = BT_DEVICE_LIST(object);
  g_slist_free_full(self->devices, g_object_unref);
  g_clear_pointer(&self->manager, g_object_unref);
  G_OBJECT_CLASS(bt_device_list_parent_class)->dispose(object);
}

static void bt_device_list_class_init(BtDeviceListClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = bt_device_list_dispose;
}

///////////////////////////////////////
//                                   //
// Реализация интерфейса GListModel. //
//                                   //
///////////////////////////////////////

/// @brief Вспомогательный метод для посылки сигналов об изменении количества устройств.
/// @param self
/// @param position Позиция в которой список изменился.
/// @param removed Количество удаляемых элементов.
/// @param added Количество добавляемых элементов.
static void bt_device_list_items_changed(BtDeviceList *self, guint position, guint removed, guint added) {
  g_list_model_items_changed(G_LIST_MODEL(self), position, removed, added);
  if(removed != added) {
  }
}

/// @brief Метод интерфейса GListModel, возвращающий тип данных в списке.
/// @param self указатель на реализацию.
/// @return Тип элементов списка.
static GType bt_device_list_model_get_item_type(GListModel *list) {
  (void)list;
  return BT_TYPE_DEVICE;
}

/// @brief Метод интерфейса GListModel, возвращающий количество элементов в списке.
/// @param self указатель на реализацию.
/// @return количество элементов
static guint bt_device_list_model_get_n_items(GListModel *list) {
  g_return_val_if_fail(BT_IS_DEVICE_LIST(list), 0u);
  return g_slist_length(BT_DEVICE_LIST(list)->devices);
}

/// @brief Метод интерфейса GListModel, возвращающий указатель на элемент по заданной позиции.
/// @param self указатель на реализацию.
/// @param position позиция элемента в списке.
/// @return указатель на элемент в списке.
static gpointer bt_device_list_model_get_item(GListModel *list, guint position) {
  g_return_val_if_fail(BT_IS_DEVICE_LIST(list), NULL);
  BtDeviceList *self = BT_DEVICE_LIST(list);
  if(position < g_slist_length(self->devices)) {
    return g_object_ref(g_slist_nth_data(self->devices, position));
  }
  return NULL;
}

static void bt_device_list_model_iface_init(GListModelInterface *iface) {
  iface->get_item_type = bt_device_list_model_get_item_type;
  iface->get_n_items   = bt_device_list_model_get_n_items;
  iface->get_item      = bt_device_list_model_get_item;
}

static void bt_device_list_device_added(GObject *sender, gchar const *device_id, gpointer udata) {
  (void)sender;
  g_return_if_fail(BT_IS_DEVICE_LIST(udata));
  BtDeviceList *self   = BT_DEVICE_LIST(udata);
  BtDevice     *device = bt_manager_get_device(self->manager, device_id);
  if(device) {
    self->devices = g_slist_append(self->devices, g_object_ref(device));
    bt_device_list_items_changed(self, g_slist_length(self->devices) - 1u, 0u, 1u);
  }
}

static gint bt_device_compare_func(gconstpointer value, gconstpointer udata) {
  g_return_val_if_fail(BT_IS_DEVICE((gpointer)value), -1);
  return g_strcmp0(bt_device_get_id(BT_DEVICE((gpointer)value)), udata);
}

static void bt_device_list_device_removed(GObject *sender, gchar const *device_id, gpointer udata) {
  (void)sender;
  g_return_if_fail(BT_IS_DEVICE_LIST(udata));
  BtDeviceList *self     = BT_DEVICE_LIST(udata);
  GSList       *item     = g_slist_find_custom(self->devices, device_id, bt_device_compare_func);
  gint          position = g_slist_position(self->devices, item);
  if(NULL != item && -1 != position) {
    self->devices = g_slist_remove_link(self->devices, item);
    g_object_unref(item->data);
    bt_device_list_items_changed(self, position, 1u, 0u);
  }
}

BtDeviceList *bt_device_list_new(BtManager *manager) {
  BtDeviceList *self = BT_DEVICE_LIST(g_object_new(BT_TYPE_DEVICE_LIST, NULL));
  self->manager      = g_object_ref(manager);
  g_signal_connect(self->manager, "device-added", G_CALLBACK(bt_device_list_device_added), self);
  g_signal_connect(self->manager, "device-removed", G_CALLBACK(bt_device_list_device_removed), self);
  GSList *item = bt_manager_get_devices(self->manager);
  for(; item != NULL; item = item->next) {
    bt_device_list_device_added(G_OBJECT(self->manager), bt_device_get_id(BT_DEVICE(item->data)), self);
  }
  return self;
}
