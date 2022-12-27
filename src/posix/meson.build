sources = files('posix.c')

subdir('fs')

sources += base_files

inc_dirs = include_directories('include')

executable('posix.elf', sources, include_directories: inc_dirs,dependencies: deps)