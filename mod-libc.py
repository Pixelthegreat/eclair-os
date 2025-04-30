def module():
    return {
        'name': 'libc',
        'flags': ('BUILD_AR',),

        'srcdir': 'libc',
        'objdir': 'build/libc',
        'depsdir': 'include',
        
        'cflags': '-ffreestanding -Iinclude',
        'asmflags': '-f$(ASM_ARCH)',

        'targets': (
                {
                    'name': 'libc',
                    'out': 'build/libc.a',

                    # files #
                    'c-files': (
                            ('crt0.c'),
                            ('ec.c', 'ec.h'),
                            ('errno.c', 'errno.h'),
                            ('printf.c', 'stdio.h'),
                            ('stdio.c', 'stdio.h'),
                            ('stdlib.c', 'stdlib.h'),
                            ('memory.c', 'stdlib.h'),
                            ('string.c', 'string.h'),
                        )
                },
            )
        }
