import os

def gen_bin(name, cfiles=None, links=None, ldflags=None, subdir=None):
    if cfiles is None:
        cfiles = (f'{name}.c',)
    if links is None:
        links = ()
    if ldflags is None:
        ldflags = ''

    liblinks = tuple(f'build/lib/lib{link}.a' for link in links)

    target = {
            'name': name,
            'out': f'build/bin/{name}',
            'c-files': cfiles,
            'ldflags': ' '.join(liblinks) + f' {ldflags} build/libc.a -lgcc',
            'depends': liblinks + ('build/libc.a',),
        }
    if subdir is not None:
        target['srcdir'] = f'bin/{name}'
        target['objdir'] = f'build/bin-obj/{subdir}'
        target['depsdir'] = 'include'
    return target

def gen_app(name, cfiles=None, links=None, ldflags=None, subdir=None):
    links = links if links is not None else ()
    ldflags = ldflags if ldflags is not None else ''

    return gen_bin(name, cfiles=cfiles, links=links + ('crepe', 'image', 'wm'), ldflags=ldflags, subdir=subdir)

def module():
    return {
        'name': 'bin',
        'srcdir': 'bin',
        'objdir': 'build/bin-obj',
        'depsdir': 'bin',

        'cflags': '-ffreestanding -Iinclude',
        'ldflags': '-T bin/linker.ld -ffreestanding -nostdlib -Lbuild/lib',

        'extra-ld': ('@$(STRIP) -g $@',),

        'targets': (
                gen_bin('cat'),
                gen_bin('clear'),
                gen_bin('cp'),
                gen_bin('gfx-mouse'),
                gen_bin('gfx', links=('image',)),
                gen_bin('hello'),
                gen_bin('hexd'),
                gen_bin('init'),
                gen_bin('ls'),
                gen_bin('playsnd', links=('sound',)),
                gen_bin('sh'),
                gen_bin('sleep'),
                gen_bin('stat'),
                gen_bin('su'),
                gen_bin('sysinfo'),
                gen_bin('touch'),

                # window manager #
                gen_bin('wm-test', links=('crepe', 'image', 'wm')),
                gen_bin('wm', ldflags='-Ofast', cfiles=(
                    ('input.c', 'wm/input.h'),
                    ('main.c'),
                    ('screen.c', 'wm/screen.h'),
                    ('server.c', 'wm/server.h', 'ec/wm.h'),
                    ('window.c', 'wm/window.h'),
                ), subdir='wm'),

                # graphical apps #
                gen_app('about'),
                gen_app('desktop'),
                gen_app('login'),
            )
        }
