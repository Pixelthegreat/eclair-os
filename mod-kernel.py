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
                            ('multiboot.c', 'multiboot.h'),
                            ('main.c'),
                            ('mm/gdt.c', 'mm/gdt.h'),
                            ('util/string.c', 'string.h'),
                            ('io/port.c', 'io/port.h'),
                            ('tty.c', 'tty.h'),
                            ('idt.c', 'idt.h'),
                            ('driver/ps2.c', 'driver/ps2.h'),
                            ('mm/paging.c', 'mm/paging.h'),
                            ('mm/heap.c', 'mm/heap.h'),
                            ('driver/device.c', 'driver/device.h'),
                            ('driver/ata.c', 'driver/ata.h'),
                            ('vfs/fs.c', 'vfs/fs.h'),
                            ('fs/mbr.c', 'fs/mbr.h'),
                            ('fs/ext2.c', 'fs/ext2.h'),
                            ('driver/pit.c', 'driver/pit.h'),
                            ('driver/vgacon.c', 'driver/vgacon.h'),
                            ('process.c', 'process.h'),
                            ('driver/rtc.c', 'driver/rtc.h'),
                            ('panic.c', 'panic.h'),
                            ('driver/fb.c', 'driver/fb.h'),
                        ),
                    'asm-files': (
                            ('multiboota.asm', 'multiboot.h'),
                            ('idta.asm', 'idt.h'),
                            ('processa.asm', 'process.h'),
                        )
                },
            )
        }
