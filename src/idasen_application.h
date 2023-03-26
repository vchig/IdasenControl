#pragma once
#include <adwaita.h>

G_BEGIN_DECLS

#define IDASEN_TYPE_APPLICATION (idasen_application_get_type())

G_DECLARE_FINAL_TYPE(IdasenApplication, idasen_application, IDASEN, APPLICATION, AdwApplication)

IdasenApplication *idasen_application_new();

G_END_DECLS
