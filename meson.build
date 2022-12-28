project('olympus', 'c')


ichor = subproject('ichor')
deps = [ichor.get_variable('dep')]

subdir('protos')

if get_option('protos_only') == false
    subdir('src')
endif
