#include "desk_position_row.h"

struct _DeskPositionRow {
  AdwActionRow   parent;
  GtkAdjustment *adjustment;
  GtkWidget     *spin_button;
  gdouble        position;
};

enum {
  PROP_0,
  PROP_POSITION,
  N_PROPS,
};

static GParamSpec *g_props[N_PROPS] = { 0u };

G_DEFINE_FINAL_TYPE(DeskPositionRow, desk_position_row, ADW_TYPE_ACTION_ROW)

static void desk_position_row_init(DeskPositionRow *self) {
  self->adjustment  = gtk_adjustment_new(62.0, 62.0, 127.0, 0.1, 1.0, 0.0);
  self->spin_button = gtk_spin_button_new(self->adjustment, 0.0, 1);
  gtk_widget_set_valign(self->spin_button, GTK_ALIGN_CENTER);
  g_object_bind_property(self, "position", self->adjustment, "value", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  adw_action_row_add_suffix(ADW_ACTION_ROW(self), self->spin_button);
}

static void desk_position_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  g_return_if_fail(DESK_IS_POSITION_ROW(object));
  DeskPositionRow *self = DESK_POSITION_ROW(object);
  switch(property_id) {
    case PROP_POSITION:
      self->position = g_value_get_double(value);
      g_object_notify_by_pspec(G_OBJECT(self), g_props[PROP_POSITION]);
      break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void desk_position_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(DESK_IS_POSITION_ROW(object));
  DeskPositionRow *self = DESK_POSITION_ROW(object);
  switch(property_id) {
    case PROP_POSITION: g_value_set_double(value, self->position); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void desk_position_dispose(GObject *obj) {
  G_OBJECT_CLASS(desk_position_row_parent_class)->dispose(obj);
}

static void desk_position_row_class_init(DeskPositionRowClass *klass) {
  G_OBJECT_CLASS(klass)->set_property = desk_position_set_property;
  G_OBJECT_CLASS(klass)->get_property = desk_position_get_property;
  G_OBJECT_CLASS(klass)->dispose      = desk_position_dispose;

  g_props[PROP_POSITION] =
      g_param_spec_double("position", "", "", 0.0, 200.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPS, g_props);
}

DeskPositionRow *desk_position_row_new(void) {
  return g_object_new(DESK_TYPE_POSITION_ROW, NULL);
}
