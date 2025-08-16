import os

def gen_bin(name, cfiles=None, ldflags=None, subdir=None):
    if cfiles is None:
        cfiles = (f'{name}.c',)
    if ldflags is None:
        ldflags = ''
    target = {
            'name': name,
            'out': f'build/bin/{name}',
            'c-files': cfiles,
            'ldflags': ldflags + ' build/libc.a -lgcc',
        }
    if subdir is not None:
        target['srcdir'] = f'bin/{name}'
        target['objdir'] = f'build/bin-obj/{subdir}'
        target['depsdir'] = 'include'
    return target

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
                gen_bin('gfx-mouse'),
                gen_bin('gfx', ldflags='-limage'),
                gen_bin('hello'),
                gen_bin('hexd'),
                gen_bin('init'),
                gen_bin('ls'),
                gen_bin('playsnd', ldflags='-lsound'),
                gen_bin('sh'),
                gen_bin('sleep'),
                gen_bin('su'),
                gen_bin('sysinfo'),

                # window manager #
                gen_bin('wm-test', ldflags='-lwm'),
                gen_bin('wm', ldflags='-Ofast', cfiles=(
                    ('input.c', 'wm/input.h'),
                    ('main.c'),
                    ('screen.c', 'wm/screen.h'),
                    ('server.c', 'wm/server.h', 'ec/wm.h'),
                    ('window.c', 'wm/window.h'),
                ), subdir='wm')
            )
        }
