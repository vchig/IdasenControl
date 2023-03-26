#include "idasen_desk.h"

#include "bt/bt_manager.h"

#include <glib/gi18n.h>
#include <math.h>

#define GATT_SERVICE_HEIGHT_UUID "99fa0020-338a-1024-8a49-009c0215f78a"
#define GATT_CHARAC_HEIGHT_UUID "99fa0021-338a-1024-8a49-009c0215f78a"

#define GATT_SERVICE_COMMAND_UUID "99fa0001-338a-1024-8a49-009c0215f78a"
#define GATT_CHARAC_COMMAND_UUID "99fa0002-338a-1024-8a49-009c0215f78a"

#define GATT_SERVICE_INPUT_UUID "99fa0030-338a-1024-8a49-009c0215f78a"
#define GATT_CHARAC_INPUT_UUID "99fa0031-338a-1024-8a49-009c0215f78a"

static const guchar  g_up_command[2]     = { 0x47, 0x00 };
static const guchar  g_down_command[2]   = { 0x46, 0x00 };
static const guchar  g_stop_command[2]   = { 0xFF, 0x00 };
static const guchar  g_wakeup_command[2] = { 0xFE, 0x00 };
static const gdouble g_height_base  = 62.0;  // Базовая (минимальная) высота стола в см.
static const gdouble g_height_range = 65.0;  // Диапазон высот стола в см.
static const gdouble g_height_epsilon = 0.1; // Погрешность измерения высоты.

static const guint g_move_interval = 200; // Интервал отправки позиции

struct _IdasenDesk {
  GObject parent;

  BtManager            *manager;
  BtDevice             *device;
  BtGattCharacteristic *gatt_height;
  BtGattCharacteristic *gatt_input;
  BtGattCharacteristic *gatt_command;
  gchar                *address;
  gdouble               actual_height;
  gdouble               actual_velocity;
  gdouble               needed_height;
};

enum {
  PROP_0,
  PROP_DEVICE_ADDRESS,  // Адресс блютуз устройства.
  PROP_TITLE,           // Название.
  PROP_CONNECTED,       // Связь установлена.
  PROP_ACTUAL_HEIGHT,   // Высота стола.
  PROP_ACTUAL_VELOCITY, // Скорость стола.
  PROP_IS_READY,        // Флаг готовности к управлению
  N_PROPERTIES,
};

static GParamSpec *g_properties[N_PROPERTIES] = { NULL };

G_DEFINE_FINAL_TYPE(IdasenDesk, idasen_desk, G_TYPE_OBJECT)

static gboolean idasen_desk_mote_to(gpointer udata) {
  g_return_val_if_fail(IDASEN_IS_DESK(udata), FALSE);
  IdasenDesk *self = IDASEN_DESK(udata);
  g_return_val_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_input), FALSE);
  union {
    guint16 value;
    guchar  bytes[2];
  } raw;
  raw.value     = (self->needed_height - g_height_base) * 100.0;
  GError *error = NULL;
  bt_gatt_characteristic_write_value(self->gatt_input, raw.bytes, sizeof(raw.bytes), &error);
  if(NULL != error) {
    g_warning("Failed move to command: %s\n", error->message);
  }
  // Если не достигли позиции, то продолжаем движение.
  if(fabs(self->actual_height - self->needed_height) > g_height_epsilon && fabs(self->actual_velocity) > 0.0) {
    return (NULL == error) ? TRUE : FALSE;
  }
  return FALSE;
}

static void idasen_desk_init(IdasenDesk *self) {
  (void)self;
}

static void idasen_desk_dispose(GObject *object) {
  IdasenDesk *self = IDASEN_DESK(object);
  g_clear_pointer(&self->manager, g_object_unref);
  g_clear_pointer(&self->device, g_object_unref);
  g_clear_pointer(&self->gatt_height, g_object_unref);
  g_clear_pointer(&self->gatt_input, g_object_unref);
  g_clear_pointer(&self->gatt_command, g_object_unref);
  g_free(self->address);
  G_OBJECT_CLASS(idasen_desk_parent_class)->dispose(object);
}

static void idasen_desk_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_DESK(object));
  IdasenDesk *self = IDASEN_DESK(object);
  switch(property_id) {
    case PROP_DEVICE_ADDRESS: idasen_desk_set_device_address(self, g_value_get_string(value)); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_desk_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_DESK(object));
  IdasenDesk *self = IDASEN_DESK(object);
  switch(property_id) {
    case PROP_DEVICE_ADDRESS: g_value_set_string(value, idasen_desk_get_device_address(self)); break;
    case PROP_TITLE: g_value_set_string(value, idasen_desk_get_title(self)); break;
    case PROP_CONNECTED: g_value_set_boolean(value, idasen_desk_get_is_connected(self)); break;
    case PROP_ACTUAL_HEIGHT: g_value_set_double(value, idasen_desk_get_actual_height(self)); break;
    case PROP_ACTUAL_VELOCITY: g_value_set_double(value, idasen_desk_get_actual_velocity(self)); break;
    case PROP_IS_READY: g_value_set_boolean(value, idasen_desk_is_ready(self)); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_desk_class_init(IdasenDeskClass *klass) {
  G_OBJECT_CLASS(klass)->set_property = idasen_desk_set_property;
  G_OBJECT_CLASS(klass)->get_property = idasen_desk_get_property;
  G_OBJECT_CLASS(klass)->dispose      = idasen_desk_dispose;

  g_properties[PROP_DEVICE_ADDRESS] =
      g_param_spec_string("device-address", "", "", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_TITLE] = g_param_spec_string("title", "", "", "", G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_CONNECTED] =
      g_param_spec_boolean("connected", "", "", FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_ACTUAL_HEIGHT] = g_param_spec_double("actual-height",
                                                         "",
                                                         "",
                                                         g_height_base,
                                                         g_height_base + g_height_range,
                                                         g_height_base,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_ACTUAL_VELOCITY] =
      g_param_spec_double("actual-velocity", "", "", -6.208, 6.208, 0.0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_properties[PROP_IS_READY] =
      g_param_spec_boolean("is-ready", "", "", FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properties);
}

static void idasen_desk_set_actual_height(IdasenDesk *self, gdouble height) {
  g_return_if_fail(height >= g_height_base);
  g_return_if_fail(height <= (g_height_base + g_height_range));
  if(self->actual_height != height) {
    self->actual_height = height;
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_ACTUAL_HEIGHT]);
  }
}

static void idasen_desk_set_actual_velocity(IdasenDesk *self, gdouble velocity) {
  if(self->actual_velocity != velocity) {
    self->actual_velocity = velocity;
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_ACTUAL_VELOCITY]);
  }
}

static void idasen_desk_height_process(IdasenDesk *self, GVariant *value) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  GVariantIter iter;
  g_variant_iter_init(&iter, value);
  union {
    gint16 raw[2];
    guchar bytes[4];
  } raw              = { 0 };
  unsigned int count = 0;
  while(g_variant_iter_loop(&iter, "y", &raw.bytes[count++])) {
    if(count > sizeof(raw)) {
      g_warning("Read desk height and velocity out of range.\n");
      count = sizeof(raw) - 1;
    }
  }
  idasen_desk_set_actual_height(self, (double)raw.raw[0] / 100.0 + g_height_base);
  idasen_desk_set_actual_velocity(self, (double)raw.raw[1] / 1000.0);
}

static void idasen_desk_height_changed(GObject *sender, GParamSpec *pspec, gpointer udata) {
  g_return_if_fail(G_IS_PARAM_SPEC_VARIANT(pspec));
  GValue value = G_VALUE_INIT;
  g_object_get_property(sender, "value", &value);
  g_return_if_fail(G_VALUE_HOLDS_VARIANT(&value));
  idasen_desk_height_process(IDASEN_DESK(udata), g_value_get_variant(&value));
}

static void idasen_desk_setup_height_charac(IdasenDesk *self, BtGattCharacteristic *charac) {
  g_clear_pointer(&self->gatt_height, g_object_unref);
  self->gatt_height = BT_GATT_CHARACTERISTIC(g_object_ref(charac));
  g_signal_connect(self->gatt_height, "notify::value", G_CALLBACK(idasen_desk_height_changed), self);
  GError *error = NULL;
  bt_gatt_characteristic_start_notify(self->gatt_height, &error);
  if(error) {
    g_warning("Failed start notify: %s\n", error->message);
  }
  error            = NULL;
  GVariant *result = bt_gatt_characteristic_read_value(self->gatt_height, &error);
  if(error) {
    g_warning("Failed read value: %s\n", error->message);
  } else {
    GVariant *value = NULL;
    g_variant_get(result, "(@ay)", &value);
    idasen_desk_height_process(self, value);
    g_variant_unref(value);
    g_variant_unref(result);
  }
  g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_IS_READY]);
}

static void idasen_desk_setup_command_gatt(IdasenDesk *self, BtGattCharacteristic *command) {
  g_clear_pointer(&self->gatt_command, g_object_unref);
  self->gatt_command = BT_GATT_CHARACTERISTIC(g_object_ref(command));
  g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_IS_READY]);
}

static void idasen_desk_setup_input_gatt(IdasenDesk *self, BtGattCharacteristic *input) {
  g_clear_pointer(&self->gatt_input, g_object_unref);
  self->gatt_input = BT_GATT_CHARACTERISTIC(g_object_ref(input));
  g_object_notify_by_pspec(G_OBJECT(self), g_properties[PROP_IS_READY]);
}

static void idasen_desk_char_added(GObject *sender, BtGattCharacteristic *charac, gpointer udata) {
  (void)sender;
  g_return_if_fail(IDASEN_IS_DESK(udata));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(charac));
  gchar const *uuid = bt_gatt_characteristic_get_uuid(charac);
  if(g_str_equal(uuid, GATT_CHARAC_HEIGHT_UUID)) {
    idasen_desk_setup_height_charac(IDASEN_DESK(udata), charac);
  } else if(g_str_equal(uuid, GATT_CHARAC_COMMAND_UUID)) {
    idasen_desk_setup_command_gatt(IDASEN_DESK(udata), charac);
  } else if(g_str_equal(uuid, GATT_CHARAC_INPUT_UUID)) {
    idasen_desk_setup_input_gatt(IDASEN_DESK(udata), charac);
  }
}

static void idasen_desk_check_characteristic(IdasenDesk *self, BtGattService *service) {
  BtGattCharacteristic *characteristic = bt_gatt_service_find_char_by_uuid(service, GATT_CHARAC_HEIGHT_UUID);
  if(characteristic) {
    idasen_desk_setup_height_charac(self, characteristic);
  }
  characteristic = bt_gatt_service_find_char_by_uuid(service, GATT_CHARAC_COMMAND_UUID);
  if(characteristic) {
    idasen_desk_setup_command_gatt(self, characteristic);
  }
  characteristic = bt_gatt_service_find_char_by_uuid(service, GATT_CHARAC_INPUT_UUID);
  if(characteristic) {
    idasen_desk_setup_input_gatt(self, characteristic);
  }
}

static void idasen_desk_service_added(GObject *sender, BtGattService *service, gpointer udata) {
  (void)sender;
  g_return_if_fail(IDASEN_IS_DESK(udata));
  g_return_if_fail(BT_IS_GATT_SERVICE(service));
  IdasenDesk *self = IDASEN_DESK(udata);

  g_signal_connect(service, "char-added", G_CALLBACK(idasen_desk_char_added), self);
  idasen_desk_check_characteristic(self, service);
}

static void idasen_desk_change_device(IdasenDesk *self, BtDevice *device) {
  g_clear_pointer(&self->device, g_object_unref);
  g_clear_pointer(&self->gatt_height, g_object_unref);
  g_clear_pointer(&self->gatt_input, g_object_unref);
  g_clear_pointer(&self->gatt_command, g_object_unref);
  self->actual_height   = g_height_base;
  self->actual_velocity = 0.0;
  if(device) {
    self->device = BT_DEVICE(g_object_ref(device));
    g_signal_connect(self->device, "service-append", G_CALLBACK(idasen_desk_service_added), self);
    gchar const *service_uuids[] = { GATT_SERVICE_HEIGHT_UUID,
                                     GATT_SERVICE_COMMAND_UUID,
                                     GATT_SERVICE_INPUT_UUID,
                                     NULL };
    for(int i = 0; service_uuids[i]; ++i) {
      BtGattService *service = bt_device_find_service_by_uuid(self->device, service_uuids[i]);
      if(service) {
        idasen_desk_check_characteristic(self, service);
      }
    }
  }
  for(int prop = 1; prop < N_PROPERTIES; ++prop) {
    g_object_notify_by_pspec(G_OBJECT(self), g_properties[prop]);
  }
}

static void idasen_desk_device_added(GObject *sender, gchar const *device_id, gpointer udata) {
  (void)sender;
  g_return_if_fail(IDASEN_IS_DESK(udata));
  IdasenDesk *self   = IDASEN_DESK(udata);
  BtDevice   *device = bt_manager_get_device(self->manager, device_id);
  if(self->address && g_str_equal(self->address, bt_device_get_address(device))) {
    idasen_desk_change_device(self, device);
  }
}

static void idasen_desk_device_removed(GObject *sender, gchar const *device_id, gpointer udata) {
  (void)sender;
  g_return_if_fail(IDASEN_IS_DESK(udata));
  IdasenDesk *self = IDASEN_DESK(udata);
  if(self->device) {
    if(g_str_equal(bt_device_get_id(self->device), device_id)) {
      idasen_desk_change_device(self, NULL);
    }
  }
}

IdasenDesk *idasen_desk_new(BtManager *manager) {
  IdasenDesk *self = IDASEN_DESK(g_object_new(IDASEN_TYPE_DESK, NULL));
  self->manager    = BT_MANAGER(g_object_ref(manager));
  g_signal_connect(self->manager, "device-added", G_CALLBACK(idasen_desk_device_added), self);
  g_signal_connect(self->manager, "device-removed", G_CALLBACK(idasen_desk_device_removed), self);
  return self;
}

void idasen_desk_set_device_address(IdasenDesk *self, gchar const *address) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(address);
  g_free(self->address);
  self->address = g_strdup(address);
  idasen_desk_change_device(self, bt_manager_get_device_by_address(self->manager, self->address));
}

gchar const *idasen_desk_get_device_address(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), NULL);
  return self->address;
}

void idasen_desk_connect(IdasenDesk *self) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_DEVICE(self->device));
  bt_device_pair(self->device);
  bt_device_connect(self->device);
}

gchar const *idasen_desk_get_title(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), NULL);
  return self->device ? bt_device_get_alias(self->device) : _("Unknown");
}

gboolean idasen_desk_get_is_connected(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), FALSE);
  return self->device ? bt_device_is_connected(self->device) : FALSE;
}

gdouble idasen_desk_get_actual_height(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), g_height_base);
  return self->actual_height;
}

gdouble idasen_desk_get_actual_velocity(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), 0.0);
  return self->actual_velocity;
}

void idasen_desk_set_height(IdasenDesk *self, gdouble height) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_input));
  g_return_if_fail(height >= g_height_base);
  g_return_if_fail(height <= (g_height_base + g_height_range));
  self->needed_height = height;
  idasen_desk_mote_to(self);
  g_timeout_add(g_move_interval, idasen_desk_mote_to, self);
}

void idasen_desk_up(IdasenDesk *self) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_command));
  GError *error = NULL;
  bt_gatt_characteristic_write_value(self->gatt_command, g_up_command, sizeof(g_up_command), &error);
  if(NULL != error) {
    g_warning("Failed send up command: %s\n", error->message);
  }
}

void idasen_desk_down(IdasenDesk *self) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_command));
  GError *error = NULL;
  bt_gatt_characteristic_write_value(self->gatt_command, g_down_command, sizeof(g_down_command), &error);
  if(NULL != error) {
    g_warning("Failed send down command: %s\n", error->message);
  }
}

void idasen_desk_stop(IdasenDesk *self) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_command));
  GError *error = NULL;
  bt_gatt_characteristic_write_value(self->gatt_command, g_stop_command, sizeof(g_stop_command), &error);
  if(NULL != error) {
    g_warning("Failed send stop command: %s\n", error->message);
  }
}

void idasen_desk_wakeup(IdasenDesk *self) {
  g_return_if_fail(IDASEN_IS_DESK(self));
  g_return_if_fail(BT_IS_GATT_CHARACTERISTIC(self->gatt_command));
  GError *error = NULL;
  bt_gatt_characteristic_write_value(self->gatt_command, g_wakeup_command, sizeof(g_wakeup_command), &error);
  if(NULL != error) {
    g_warning("Failed send wakeup command: %s\n", error->message);
  }
}

gboolean idasen_desk_is_ready(IdasenDesk *self) {
  g_return_val_if_fail(IDASEN_IS_DESK(self), FALSE);
  return (self->gatt_command && self->gatt_height && self->gatt_input && idasen_desk_get_is_connected(self)) ? TRUE
                                                                                                             : FALSE;
}
