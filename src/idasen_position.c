#include "idasen_position.h"

#include "app_config.h"

struct _IdasenPosition {
  GtkWindow parent;
  gchar    *button_label;
  gdouble   postion;
};

enum {
  PROP_0,
  PROP_POSITION,
  PROP_BUTTON_LABEL,
  N_PROPERTIES,
};

static GParamSpec *g_properteis[N_PROPERTIES] = { NULL };

enum {
  SIG_POSITION_SELECTED,
  N_SIGNALS,
};

static guint g_signals[N_SIGNALS] = { 0u };

G_DEFINE_FINAL_TYPE(IdasenPosition, idasen_position, GTK_TYPE_WINDOW)

static void idasen_position_add_pressed(GtkWidget *widget, gpointer udata) {
  (void)udata;
  g_return_if_fail(IDASEN_IS_POSITION(widget));
  IdasenPosition *self = IDASEN_POSITION(widget);
  g_signal_emit(widget, g_signals[SIG_POSITION_SELECTED], 0, self->postion);
  gtk_window_close(GTK_WINDOW(self));
}

static void idasen_position_init(IdasenPosition *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

static void idasen_position_dispose(GObject *object) {
  IdasenPosition *self = IDASEN_POSITION(object);
  g_free(self->button_label);
  G_OBJECT_CLASS(idasen_position_parent_class)->dispose(object);
}

static void idasen_position_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_POSITION(object));
  IdasenPosition *self = IDASEN_POSITION(object);
  switch(property_id) {
    case PROP_POSITION: self->postion = g_value_get_double(value); break;
    case PROP_BUTTON_LABEL:
      g_free(self->button_label);
      self->button_label = g_strdup(g_value_get_string(value));
      break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_position_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  g_return_if_fail(IDASEN_IS_POSITION(object));
  IdasenPosition *self = IDASEN_POSITION(object);
  switch(property_id) {
    case PROP_POSITION: g_value_set_double(value, self->postion); break;
    case PROP_BUTTON_LABEL: g_value_set_string(value, self->button_label); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
  }
}

static void idasen_position_class_init(IdasenPositionClass *klass) {
  G_OBJECT_CLASS(klass)->dispose      = idasen_position_dispose;
  G_OBJECT_CLASS(klass)->set_property = idasen_position_set_property;
  G_OBJECT_CLASS(klass)->get_property = idasen_position_get_property;

  g_properteis[PROP_POSITION] =
      g_param_spec_double("position", "", "", 62.0, 127.0, 62.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_properteis[PROP_BUTTON_LABEL] =
      g_param_spec_string("button-label", "", "", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROPERTIES, g_properteis);

  g_signals[SIG_POSITION_SELECTED] = g_signal_new("position-selected",
                                                  IDASEN_TYPE_POSITION,
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  G_TYPE_NONE,
                                                  1,
                                                  G_TYPE_DOUBLE);

  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), APPLICATION_PATH "/ui/position.ui");
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass), idasen_position_add_pressed);
}

GtkWindow *idasen_position_new(double init_position) {
  return g_object_new(IDASEN_TYPE_POSITION, "position", init_position, NULL);
}
