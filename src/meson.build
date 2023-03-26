subdir('bt')

app_include = include_directories('.')

app_sources = files (
  'desk_position_row.c',
  'idasen_application.c',
  'idasen_desk.c',
  'idasen_main_window.c',
  'idasen_position.c',
  'idasen_preference.c',
  'main.c'
)

app_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
]

config_h = configure_file(
  output: 'app_config.h',
  configuration: config_data
)
