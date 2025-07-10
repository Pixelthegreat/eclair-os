def gentool(name, cflags=None, ldflags=None):
    cflags = cflags if cflags is not None else ''
    ldflags = ldflags if ldflags is not None else ''

    return {
            'name': name,
            'out': f'build/{name}',
            'srcdir': 'tools',
            'depsdir': 'tools',
            'objdir': 'build/tools',

            'cflags': cflags,
            'ldflags': ldflags,

            'c-files': (
                    (f'{name}.c'),
                )
            }

def module():
    return {
        'name': 'tools',
        'flags': ('HOST_CC',),

        'targets': (
                gentool('bootimage'),
                gentool('mkecfs'),
                gentool('mntecfs',
                        '`pkg-config --cflags fuse` -g',
                        '`pkg-config --libs fuse` -lm -g',
                        ),
            )
        }
