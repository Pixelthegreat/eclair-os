include common.mk

.PHONY: kernel boot bootdisk setup clean run

QEMUARGS=-drive if=ide,id=ata0.0,index=0,file=bootdisk.img,format=raw

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
	@rm -v build/kernel/* build/e.clair

run:
	qemu-system-i386 $(QEMUARGS)

run_uart_stdout:
	qemu-system-i386 $(QEMUARGS) -serial /dev/stdout

run_debug:
	qemu-system-i386 $(QEMUARGS) -d int
