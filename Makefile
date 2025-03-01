include common.mk

.PHONY: kernel boot bootdisk setup clean install_boot run run_uart_stdout run_debug

QEMUARGS_ALL=-drive if=ide,id=ata0.0,file=bootdisk.img,format=raw $(QEMUARGS)

# primary targets #
all: kernel boot bootdisk

# os kernel #
kernel:
	$(BUILD_TARGETNAME)
	@make -s -f kernel.mk all

# bootloader #
boot:
	$(BUILD_TARGETNAME)
	@make -s -f boot.mk all

# bootdisk setup #
bootdisk: kernel
	$(BUILD_TARGETNAME)
	@./bootdisk.sh

# setup routines #
setup:
	@mkdir -pv build/kernel build/boot
	@./pybuild

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
