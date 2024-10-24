module = {
        'name': 'kernel',
        'targets': (
                {
                    'name': 'kernel',
                    'out': 'build/e.clair',
                    'srcdir': 'kernel',
                    'objdir': 'build/kernel',
                    'depsdir': 'include/e.clair',

                    # flags #
                    'cflags': '-ffreestanding -Iinclude',
                    'asmflags': '-f$(ASMARCH)',
                    'ldflags': '-T kernel/linker.ld -ffreestanding -nostdlib -lgcc',

                    # files #
                    'c-files': (

                            # device drivers #
                            ('driver/ata.c', 'driver/ata.h'),
                            ('driver/device.c', 'driver/device.h'),
                            ('driver/fb.c', 'driver/fb.h'),
                            ('driver/pit.c', 'driver/pit.h'),
                            ('driver/ps2.c', 'driver/ps2.h'),
                            ('driver/rtc.c', 'driver/rtc.h'),
                            ('driver/vgacon.c', 'driver/vgacon.h'),

                            # partition and file systems #
                            ('fs/ext2.c', 'fs/ext2.h'),
                            ('fs/mbr.c', 'fs/mbr.h'),

                            # architecture io #
                            ('io/port.c', 'io/port.h'),

                            # memory management #
                            ('mm/gdt.c', 'mm/gdt.h'),
                            ('mm/heap.c', 'mm/heap.h'),
                            ('mm/paging.c', 'mm/paging.h'),

                            # utilities #
                            ('util/string.c', 'string.h'),

                            # virtual file system #
                            ('vfs/fs.c', 'vfs/fs.h'),

                            # general #
                            ('idt.c', 'idt.h'),
                            ('main.c'),
                            ('multiboot.c', 'multiboot.h'),
                            ('panic.c', 'panic.h'),
                            ('tty.c', 'tty.h'),
                        ),
                    'asm-files': (
                            ('idta.asm', 'idt.h'),
                            ('multiboota.asm', 'multiboot.h'),
                        )
                },
            )
        }
