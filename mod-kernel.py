def module():
    return {
        'name': 'kernel',
        'targets': (
                {
                    'name': 'kernel',
                    'out': 'build/e.clair',
                    'srcdir': 'kernel',
                    'objdir': 'build/kernel',
                    'depsdir': 'include/kernel',

                    # flags #
                    'cflags': '-ffreestanding -Iinclude',
                    'asmflags': '-f$(ASMARCH)',
                    'ldflags': '-T kernel/linker.ld -ffreestanding -nostdlib -lgcc',

                    # files #
                    'c-files': (

                            # device drivers #
                            ('driver/ata.c', 'driver/ata.h'),
                            ('driver/bga.c', 'driver/bga.h'),
                            ('driver/device.c', 'driver/device.h'),
                            ('driver/fb.c', 'driver/fb.h'),
                            ('driver/fbcon.c', 'driver/fbcon.h'),
                            ('driver/fbfont.c', 'driver/fbfont.h'),
                            ('driver/pci.c', 'driver/pci.h'),
                            ('driver/pit.c', 'driver/pit.h'),
                            ('driver/ps2.c', 'driver/ps2.h'),
                            ('driver/rtc.c', 'driver/rtc.h'),
                            ('driver/uart.c', 'driver/uart.h'),
                            ('driver/uhci.c', 'driver/uhci.h'),
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
                            ('vfs/devfs.c', 'vfs/devfs.h'),
                            ('vfs/fs.c', 'vfs/fs.h'),

                            # general #
                            ('boot.c', 'boot.h'),
                            ('elf.c', 'elf.h'),
                            ('idt.c', 'idt.h'),
                            ('main.c'),
                            ('multiboot.c', 'multiboot.h'),
                            ('panic.c', 'panic.h'),
                            ('syscall.c', 'syscall.h'),
                            ('task.c', 'task.h'),
                            ('tty.c', 'tty.h'),
                        ),
                    'asm-files': (
                            ('idta.asm', 'idt.h'),
                            ('multiboota.asm', 'multiboot.h'),
                            ('taska.asm', 'task.h'),
                        ),
                },
            )
        }
