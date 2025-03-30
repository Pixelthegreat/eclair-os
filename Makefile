include common.mk

.PHONY: kernel boot bin bootdisk setup clean install_boot run run_uart_stdout run_debug help

QEMUARGS_ALL=-drive if=ide,id=ata0.0,file=bootdisk.img,format=raw $(QEMUARGS)
PYBUILDARGS_ALL=$(PYBUILDARGS)

# primary targets #
all: kernel boot bin bootdisk

# os kernel #
kernel:
	$(BUILD_TARGETNAME)
	@make -s -f kernel.mk all

# bootloader #
boot:
	$(BUILD_TARGETNAME)
	@make -s -f boot.mk all

# application binaries #
bin:
	$(BUILD_TARGETNAME)
	@make -s -f bin.mk all

# bootdisk setup #
bootdisk: kernel bin
	$(BUILD_TARGETNAME)
	@./bootdisk.sh

# setup routines #
setup:
	@mkdir -pv build/kernel build/boot build/bin-obj build/bin
	@./pybuild $(PYBUILDARGS_ALL)

clean:
	@rm -v build/kernel/* build/boot/* build/e.clair

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
	"    setup           - Prepare for build\n"\
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
	"    PYBUILDARGS     - Pass arguments to ./pybuild (setup)\n"\
	"    QEMUARGS        - Pass arguments to QEMU (run, run_debug, run_uart_stdout)"
