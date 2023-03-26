schema_src = 'io.github.vchig.IdasenControl.gschema.xml'
compiled = gnome.compile_schemas(build_by_default: true,
                                 depend_files: schema_src)

install_data(schema_src,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

scalable_iconsdir = join_paths('icons', 'hicolor', 'scalable', 'apps')
install_data(
  'io.github.vchig.IdasenControl.svg',
  install_dir: join_paths(get_option('datadir'), scalable_iconsdir)
)

# Desktop файл
install_data('io.github.vchig.IdasenControl.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'),
)
