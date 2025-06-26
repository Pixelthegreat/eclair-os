def module():
    return {
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
                            ('stage0.asm'),
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
                            ('main.asm',
                             'config.asm',
                             'disk.asm',
                             'ecfs.asm',
                             'elf16.asm',
                             'elf32.asm',
                             'ext2.asm',
                             'fs32.asm',
                             'mbr.asm',
                             'memory.asm',
                             'menu.asm',
                             'pm16.asm',
                             'pm32.asm',
                             'print.asm',
                             'vbe.asm'),
                        )
                },
            )
        }
