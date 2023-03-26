#pragma once
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDASEN_TYPE_MAIN_WINDOW (idasen_main_window_get_type())

G_DECLARE_FINAL_TYPE(IdasenMainWindow, idasen_main_window, IDASEN, MAIN_WINDOW, GtkApplicationWindow)

GtkWindow *idasen_main_window_new(GtkApplication *app);

gchar const *idasen_main_window_get_desk_name(IdasenMainWindow *self);

void idasen_main_window_set_desk_name(IdasenMainWindow *self, gchar const *name);

gchar const *idasen_main_window_get_desk_height(IdasenMainWindow *self);

void idasen_main_window_set_desk_height(IdasenMainWindow *self, gchar const *value);

G_END_DECLS
