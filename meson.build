project('olympus', 'c')


ichor = subproject('ichor')
deps = [ichor.get_variable('dep')]

c_args = ['-ffreestanding', '-nostdlib']
link_args = ['-nostdlib']

subdir('protos')

if get_option('protos_only') == false
    subdir('src')
endif
