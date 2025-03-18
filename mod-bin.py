module = {
        'name': 'bin',
        'targets': (
                {
                    'name': 'init',
                    'out': 'build/bin/init',
                    'srcdir': 'bin',
                    'objdir': 'build/bin-obj',
                    'depsdir': 'bin',

                    'asmflags': '-f$(ASMARCH)',
                    'ldflags': '-T bin/linker.ld -ffreestanding -nostdlib -lgcc',

                    # files #
                    'asm-files': (
                            ('init.asm'),
                        )
                },
            )
        }
