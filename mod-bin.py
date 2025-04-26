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
                gen_bin('init'),
            )
        }
