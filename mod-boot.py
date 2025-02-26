module = {
        'name': 'boot',
        'targets': (
                {
                    'name': 'stage0',
                    'flags': ('NO_LINK',),
                    'out': 'build/boot/stage0.bin',
                    'srcdir': 'boot',
                    'depsdir': 'boot',

                    'asmflags': '-fbin -Iboot',
                    'asm-files': (
                            ('stage0.asm',),
                        )
                },
                {
                    'name': 'stage1',
                    'flags': ('NO_LINK',),
                    'out': 'build/boot/stage1.bin',
                    'srcdir': 'boot',
                    'depsdir': 'boot',
                    'asmflags': '-fbin -Iboot',
                    'asm-files': (
                            ('main.asm',),
                        )
                },
                {
                    'name': 'bootimage',
                    'flags': ('HOST_CC',),
                    'out': 'build/bootimage',
                    'srcdir': 'boot',
                    'objdir': 'build/boot',
                    'depsdir': 'boot',

                    'cflags': '',
                    'ldflags': '',
                    'c-files': (
                            ('bootimage.c'),
                        )
                },
            )
        }
