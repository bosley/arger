 project('arger', 'cpp',
        version : '0.1.0',
        default_options : ['cpp_std=c++20'])

project_description = 'A header-only C++ library for parsing command line arguments.'

project_headers = [
  'include/arger.hpp'
]

public_headers = include_directories('include')

build_args = []

project_target = shared_library(
  meson.project_name(),
  [],
  install : true,
  cpp_args : build_args,
  gnu_symbol_visibility : 'hidden',
  include_directories : public_headers,
)


# Make this useable as a dependency subproject
project_dep = declare_dependency(
  include_directories: public_headers
)
set_variable(meson.project_name() + '_dep', project_dep)

# Make this library usable from the system's
# package manager.
install_headers(project_headers, subdir : meson.project_name())

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : meson.project_name(),
  filebase : meson.project_name(),
  description : project_description,
  subdirs : meson.project_name(),
)

# ==========
# Unit Tests
# ==========

if not meson.is_subproject()
  add_languages('cpp')
  test('missing_required',
    executable(
      'require_test',
      files(['tests/required.cpp']),
      dependencies : [project_dep],
      install : false
    )
  )
  test('get_arg',
    executable(
      'get_arg',
      files(['tests/get_arg.cpp']),
      dependencies : [project_dep],
      install : false
    ),
    args : ['--bool', '1'],
  )
  test('help',
    executable(
      'help',
      files(['tests/help.cpp']),
      dependencies : [project_dep],
      install : false
    ),
    args : ['--help'],
  )
endif

