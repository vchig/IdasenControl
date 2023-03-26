#include "app_config.h"
#include "idasen_application.h"

#include <glib/gi18n.h>
#include <locale.h>

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  textdomain(GETTEXT_PACKAGE);

  g_autoptr(IdasenApplication) app = idasen_application_new();
  if(FALSE == g_application_register(G_APPLICATION(app), NULL, NULL)) {
    g_printerr("Failed register application");
  }
  return g_application_run(G_APPLICATION(app), argc, argv);
}
