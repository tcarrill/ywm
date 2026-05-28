project(
  'ywm',
  'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'buildtype=debugoptimized',
  ],
)

wlroots          = dependency('wlroots',          version: '>= 0.17.0')
wayland_server   = dependency('wayland-server',   version: '>= 1.21')
xkbcommon        = dependency('xkbcommon')
cairo            = dependency('cairo')
drm              = dependency('libdrm')
wayland_scanner  = find_program('wayland-scanner')
wayland_protocols = dependency('wayland-protocols')
wp_dir           = wayland_protocols.get_variable('pkgdatadir')

# Generate xdg-shell server header (needed by wlroots xdg_shell header)
xdg_shell_h = custom_target(
  'xdg-shell-protocol.h',
  output  : 'xdg-shell-protocol.h',
  input   : wp_dir / 'stable/xdg-shell/xdg-shell.xml',
  command : [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  build_by_default : true,
)

add_project_arguments('-DWLR_USE_UNSTABLE', '-D_POSIX_C_SOURCE=200809L', language: 'c')

ywm_inc = include_directories('include')

executable(
  'ywm',
  'src/main.c',
  'src/config.c',
  'src/menu.c',
  'src/server.c',
  'src/view.c',
  xdg_shell_h,
  include_directories : ywm_inc,
  dependencies : [wlroots, wayland_server, xkbcommon, cairo, drm],
  install : true,
)
