def gen_lib(name, cfiles=None):
    if cfiles is None:
        cfiles = ((f'{name}.c', f'ec/{name}.h'),)
    return {
            'name': name,
            'out': f'build/lib/lib{name}.a',
            'c-files': cfiles,
        }

def module():
    return {
        'name': 'lib',
        'flags': ('BUILD_AR',),

        'srcdir': 'lib',
        'objdir': 'build/lib-obj',
        'depsdir': 'include',

        'cflags': '-ffreestanding -Iinclude',

        'targets': (
                gen_lib('image'),
                gen_lib('sound'),
            )
        }
