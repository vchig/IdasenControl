#pragma once
#include <adwaita.h>

G_BEGIN_DECLS

#define IDASEN_TYPE_PREFERENCE (idasen_preference_get_type())

G_DECLARE_FINAL_TYPE(IdasenPreference, idasen_preference, IDASEN, PREFERENCE, AdwPreferencesWindow)

IdasenPreference *idasen_preference_new(void);

G_END_DECLS
