import enum

USAGE_STRING="""Kernel Arguments:
    kernel-drivers=<drivers>  Specify a (comma separated) list of optional drivers to include in build:
                                all (default), bga, uhci, ext2"""

class Driver(enum.Enum):
    BGA = 0x1
    UHCI = 0x2
    EXT2 = 0x4
    
    ALL = BGA | UHCI | EXT2

    def get_directory(self):
        return {
                'bga': 'driver',
                'uhci': 'driver',
                'ext2': 'fs',
                }[self.name.lower()]

# get optional driver info #
def get_opt_drivers():
    drivers = pybuild.get_arg('kernel-drivers')
    if type(drivers) != str:
        drivers = 'all'
    drivers = drivers.split(',')

    bits = 0
    for driver in drivers:
        if driver:
            bits |= getattr(Driver, driver.upper()).value

    # add files #
    cfiles = []
    cflags = []

    for flag in Driver:
        if flag != Driver.ALL and bits & flag.value:
            name = flag.name.lower()
            directory = flag.get_directory()

            cfiles.append((f'{directory}/{name}.c', f'{directory}/{name}.h'))
            cflags.append(f'-DDRIVER_{name.upper()}')

    return cfiles, cflags

# get main module info #
def module():
    driver_cfiles, driver_cflags = get_opt_drivers()

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
                    'cflags': f'-ffreestanding -Iinclude {" ".join(driver_cflags)}',
                    'asmflags': '-f$(ASMARCH)',
                    'ldflags': '-T kernel/linker.ld -ffreestanding -nostdlib -lgcc',

                    # files #
                    'c-files': (

                            # device drivers #
                            ('driver/ata.c', 'driver/ata.h'),
                            ('driver/device.c', 'driver/device.h'),
                            ('driver/fb.c', 'driver/fb.h'),
                            ('driver/fbcon.c', 'driver/fbcon.h'),
                            ('driver/fbfont.c', 'driver/fbfont.h'),
                            ('driver/pci.c', 'driver/pci.h'),
                            ('driver/pit.c', 'driver/pit.h'),
                            ('driver/ps2.c', 'driver/ps2.h'),
                            ('driver/rtc.c', 'driver/rtc.h'),
                            ('driver/uart.c', 'driver/uart.h'),
                            ('driver/vgacon.c', 'driver/vgacon.h'),

                            # partition and file systems #
                            ('fs/ecfs.c', 'fs/ecfs.h'),
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
                            ('init.c', 'init.h'),
                            ('main.c'),
                            ('multiboot.c', 'multiboot.h'),
                            ('panic.c', 'panic.h'),
                            ('syscall.c', 'syscall.h'),
                            ('task.c', 'task.h'),
                            ('tty.c', 'tty.h'),

                            # optional drivers #
                            *driver_cfiles,
                        ),
                    'asm-files': (
                            ('idta.asm', 'idt.h'),
                            ('multiboota.asm', 'multiboot.h'),
                            ('taska.asm', 'task.h'),
                        ),
                },
            )
        }

def print_help():
    print(USAGE_STRING)
