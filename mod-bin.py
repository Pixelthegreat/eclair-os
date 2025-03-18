module = {
        'name': 'bin',
        'targets': (
                {
                    'name': 'init',
                    'out': 'build/bin/init',
                    'srcdir': 'bin',
                    'objdir': 'build/bin-obj',
                    'depsdir': 'bin',

                    'cflags': '-ffreestanding -Iinclude',
                    'ldflags': '-T bin/linker.ld -ffreestanding -nostdlib -lgcc',

                    # files #
                    'c-files': (
                            ('init.c'),
                        )
                },
            )
        }
