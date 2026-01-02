project(
  'ltf',
  'c',
  version: '2.0.0-alpha',
  default_options: [
    'buildtype=release',
    'c_std=gnu2x',
    'warning_level=3',
    'default_library=static',
  ],
)

project_root = meson.project_source_root()
macro_prefix_map_flag = '-fmacro-prefix-map=' + project_root + '=.'
add_project_arguments(macro_prefix_map_flag, language: 'c')

is_macos = host_machine.system() == 'darwin'
is_macos_arm64 = is_macos and host_machine.cpu_family() == 'aarch64'
is_macos_x86_64 = is_macos and host_machine.cpu_family() == 'x86-64'
is_linux = host_machine.system() == 'linux'

fs = import('fs')

ver_full = meson.project_version()
ver_core = ver_full.split('-')[0]
ver_parts = ver_core.split('.')
ver_major = ver_parts[0].to_int()
ver_minor = ver_parts[1].to_int()
ver_patch = ver_parts[2].to_int()

cfg = configuration_data()
cfg.set('LTF_VERSION_MAJOR', ver_major)
cfg.set('LTF_VERSION_MINOR', ver_minor)
cfg.set('LTF_VERSION_PATCH', ver_patch)
cfg.set_quoted('LTF_VERSION', ver_full)

version_h = configure_file(
  input: 'include/version.h.in',
  output: 'version.h',
  configuration: cfg,
)

git_header = custom_target('git-version-header',
  output : 'git_version.h',
  command : [files('scripts/gen_git_header.sh')],
  capture : true,
  build_always_stale : true,
)

opt_deps = []

thread_dep = dependency('threads', required: true, static: true)
json_dep = dependency(
  'json-c',
  method: 'pkg-config',
  required: true,
  static: true,
)
libcurl_dep = dependency(
  'libcurl',
  method: 'pkg-config',
  required: true,
  static: false,
)

picotui_dep = dependency(
 'unibilium',
  method: 'pkg-config',
  required: true,
  static: true,
)
#----------------------------------------------------------------------

#--- Lua ---------------------------------------------------------------
# Lua’s pkg‑config file is annoyingly named differently on each distro.
lua_pc_names = [
  'lua5.4', # Debian/Ubuntu, Arch
  'lua-5.4', # Fedora/RHEL
]

lua_dep = dependency(
  lua_pc_names,
  method: 'pkg-config',
  required: true,
  static: true,
)
#----------------------------------------------------------------------

#--- libserialport ----------------------------------------------------
libsp_dep = dependency(
  'libserialport',
  method: 'pkg-config',
  required: true,
  static: true,
)
# libserialport requires apple frameworks when used statically on macOS
if is_macos
  opt_deps += [dependency('CoreFoundation'), dependency('IOKit')]
endif
#----------------------------------------------------------------------

#--- libssh2 ----------------------------------------------------
libssh_dep = dependency(
  'libssh2',
  method : 'pkg-config',
  required : true,
  static : true
)
#----------------------------------------------------------------------

main_source = 'src/main.c'
sources = [
  version_h,
  git_header,
  'src/ltf_hooks.c',
  'src/ltf_init.c',
  'src/ltf_logs.c',
  'src/ltf_log_level.c',
  'src/ltf_target.c',
  'src/ltf_test.c',
  'src/ltf_test_scenarios.c',
  'src/ltf_tui.c',
  'src/ltf_vars.c',
  'src/ltf_secrets.c',
  'src/ltf_state.c',
  'src/headless.c',
  'src/cmd_parser.c',
  'src/ltf_eval.c',
  'src/internal_logging.c',
  'src/keyword_status.c',
  'src/project_parser.c',
  'src/picotui.c',
  'src/test_case.c',
  'src/test_logs.c',
  'src/util/da.c',
  'src/util/files.c',
  'src/util/lua.c',
  'src/util/lua_hooks.c',
  'src/util/os.c',
  'src/util/string.c',
  'src/util/time.c',
  'src/util/line_cache.c',
  'src/modules/hooks/ltf-hooks.c',
  'src/modules/http/ltf-http.c',
  'src/modules/json/ltf-json.c',
  'src/modules/proc/ltf-proc.c',
  'src/modules/serial/ltf-serial.c',
  'src/modules/ltf/ltf.c',
  'src/modules/ssh/ltf-ssh-lib.c',
  'src/modules/ssh/ltf-ssh-channel.c',
  'src/modules/ssh/ltf-ssh-session.c',
  'src/modules/ssh/ltf-ssh-sftp.c',
  'src/modules/util/util.c',
]

include_dir = include_directories('include')

ltf_dir_path = get_option('ltf_dir_path')
if ltf_dir_path == ''
  # default: ~/.ltf
  ltf_dir_path = fs.expanduser('~/.ltf')
endif
add_project_arguments('-DLTF_DIR_PATH="@0@"'.format(ltf_dir_path), language: 'c')

ltf_lib = static_library(
  'ltf_core',
  sources,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
    libssh_dep,
  ],
  install: false,
)

ltf_dep = declare_dependency(
  link_with: ltf_lib,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
    libssh_dep,
  ],
)

executable(
  'ltf',
  main_source,
  link_with: ltf_lib,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
    libssh_dep,
  ],
  install: true,
)

install_subdir('lib', install_dir: ltf_dir_path)
