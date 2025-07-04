def gen_bin(name, cfiles=None):
    if cfiles is None:
        cfiles = (f'{name}.c',)
    return {
            'name': name,
            'out': f'build/bin/{name}',
            'c-files': cfiles,
        }

def module():
    return {
        'name': 'bin',
        'srcdir': 'bin',
        'objdir': 'build/bin-obj',
        'depsdir': 'bin',

        'cflags': '-ffreestanding -Iinclude',
        'ldflags': '-T bin/linker.ld -ffreestanding -nostdlib build/libc.a -lgcc',

        'extra-ld': ('@$(STRIP) -g $@',),

        'targets': (
                gen_bin('cat'),
                gen_bin('clear'),
                gen_bin('gfx'),
                gen_bin('hello'),
                gen_bin('hexd'),
                gen_bin('init'),
                gen_bin('ls'),
                gen_bin('sh'),
            )
        }
