protos = files('bootstrap.idl', 'posix.idl')
prog_python = import('python').find_installation('python3')

gen = generator(prog_python, 
output: ['@BASENAME@.c', '@BASENAME@.h', '@BASENAME@_srv.h'], 
arguments: [join_paths(meson.current_source_dir(), '../subprojects/hermes/main.py'), '-d', '@BUILD_DIR@', '@INPUT@'])

gen_src = gen.process(protos)

generated_protos = static_library('protos', gen_src, dependencies: deps)

protos_dep = declare_dependency(include_directories: generated_protos.private_dir_include(),
  link_with: generated_protos, dependencies: deps, c_args: c_args, link_args: link_args)

deps += protos_dep