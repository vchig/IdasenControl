project('idasen-control', 'c',
        version: '0.0.1',
        license: 'MIT',
        meson_version: '>=0.59.0',
        default_options: [
          'warning_level=3'
        ]
)

app_id = 'io.github.vchig.IdasenControl'
app_name = meson.project_name()
prefix = get_option('prefix')
builddir = meson.current_build_dir()
localedir = join_paths(prefix, get_option('localedir'))

config_data = configuration_data()
config_data.set_quoted('APPLICATION_ID', app_id)
config_data.set_quoted('APPLICATION_NAME', app_name)
config_data.set_quoted('APPLICATION_PATH', '/io/github/vchig/IdasenControl')
config_data.set_quoted('GETTEXT_PACKAGE', app_name)
config_data.set_quoted('LOCALEDIR', localedir)

gnome = import('gnome')

subdir('po')
subdir('resources')
subdir('src')

executable(meson.project_name(),
  [ bt_sources, app_sources, app_resources, bt_dbus_impl ],
  c_args: [],
  link_args: [],
  dependencies: app_deps,
  include_directories : app_include,
  install: true,
)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
