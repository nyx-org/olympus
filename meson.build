project('olympus', 'c')

ichor = subproject('ichor')
stdc = subproject('stdc-shim')

deps = [ichor.get_variable('dep'), stdc.get_variable('dep')]

subdir('src')