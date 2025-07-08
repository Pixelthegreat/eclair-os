def gen_bin(name, cfiles=None, ldflags=None):
    if cfiles is None:
        cfiles = (f'{name}.c',)
    if ldflags is None:
        ldflags = ''
    return {
            'name': name,
            'out': f'build/bin/{name}',
            'c-files': cfiles,
            'ldflags': ldflags + ' build/libc.a -lgcc',
        }

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
                gen_bin('gfx', ldflags='-limage'),
                gen_bin('hello'),
                gen_bin('hexd'),
                gen_bin('init'),
                gen_bin('ls'),
                gen_bin('sh'),
            )
        }
