include common.mk

.PHONY: kernel boot libc bin tools
.PHONY: bootdisk setup_common setup_init_common setup setup_init
.PHONY: clean install_boot run run_uart_stdout run_debug help

QEMUARGS_ALL=-drive if=ide,id=ata0.0,file=bootdisk.img,format=raw -usb -device piix3-usb-uhci,id=uhci -vga cirrus $(QEMUARGS)
PYBUILDARGS_ALL=-arg mk-outdir=build $(PYBUILDARGS)

# primary targets #
all: kernel boot libc bin tools bootdisk

# os kernel #
kernel:
	$(BUILD_TARGETNAME)
	@make -s -f build/kernel.mk all

# bootloader #
boot:
	$(BUILD_TARGETNAME)
	@make -s -f build/boot.mk all

# c library #
libc:
	$(BUILD_TARGETNAME)
	@make -s -f build/libc.mk all

# application binaries #
bin:
	$(BUILD_TARGETNAME)
	@make -s -f build/bin.mk all

# userspace tools #
tools:
	$(BUILD_TARGETNAME)
	@make -s -f build/tools.mk all

# bootdisk setup #
bootdisk: kernel bin
	$(BUILD_TARGETNAME)
	@./bootdisk.sh

# setup routines #
setup_common:
	@mkdir -pv build/kernel build/boot build/libc build/bin-obj build/bin build/tools
	@./pybuild $(PYBUILDARGS_ALL)

setup_init_common:
	@./genbootdisk.sh

setup: setup_common

# To explain the order here:
#  - Build directories and Makefiles are generated
#  - Host tools (bootimage, mkecfs, mntecfs) are built
#  - 'bootdisk.img' is generated
#  - The bootloader is built
#  - The bootloader is installed to `bootdisk.img`
setup_init: setup_common tools setup_init_common boot install_boot
	@echo "\e[32mSetup complete\e[39m"

clean:
	@rm -v build/kernel/* build/boot/* build/bin-obj/* build/bin/* build/e.clair build/boot.bin build/bootimage build/mkecfs build/mntecfs

# re-install bootloader #
install_boot:
	@cp -v build/boot/stage0.bin build/boot.bin
	@dd if=build/boot/stage1.bin of=build/boot.bin bs=512 seek=1 conv=notrunc
	@build/bootimage

# run program #
run:
	qemu-system-i386 $(QEMUARGS_ALL)

run_uart_stdout:
	qemu-system-i386 $(QEMUARGS_ALL) -serial /dev/stdout

run_debug:
	qemu-system-i386 $(QEMUARGS_ALL) -d int

# help information #
help:
	@echo \
	"Common commands:\n"\
	"    help            - Display this message\n"\
	"    (none)/all      - Build all OS components\n"\
	"    setup_init      - Prepare for build\n"\
	"    setup           - Update build system\n"\
	"    clean           - Remove built objects\n"\
	"    run             - Run the OS in QEMU\n"\
	"Other commands:\n"\
	"    kernel          - Build kernel\n"\
	"    boot            - Build bootloader\n"\
	"    bin             - Build userspace\n"\
	"    bootdisk        - Copy files to the bootdisk\n"\
	"    install_boot    - Install the bootloader to the bootdisk\n"\
	"    run_debug       - Run the OS in QEMU with debugging\n"\
	"    run_uart_stdout - Run the OS in QEMU with the UART terminal promoted to /dev/stdout\n"\
	"Common arguments:\n"\
	"    PYBUILDARGS     - Pass arguments to ./pybuild (setup_init, setup)\n"\
	"    QEMUARGS        - Pass arguments to QEMU (run, run_debug, run_uart_stdout)"
