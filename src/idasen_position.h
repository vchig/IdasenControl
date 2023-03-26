#pragma once
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDASEN_TYPE_POSITION (idasen_position_get_type())

G_DECLARE_FINAL_TYPE(IdasenPosition, idasen_position, IDASEN, POSITION, GtkWindow)

GtkWindow *idasen_position_new(double init_position);

G_END_DECLS
