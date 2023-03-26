# IDÅSEN Control

Приложение для управления столом IDÅSEN через блютуз. Приложение построено на GTK4/libadwaita. Работа с bluetooth осуществляется через bluez (DBus).

# Сборка
```sh
meson setup .build
meson compile -C .build
```

# Сборка flatpak
```sh
flatpak-builder --force-clean .build-flatpak resources/data/io.github.vchig.IdasenControl.yml
```

# Установка через flatpak
```sh
flatpak-builder --install --user --force-clean .build-flatpak resources/data/io.github.vchig.IdasenControl.yml
```
