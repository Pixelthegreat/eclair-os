include common.mk

.PHONY: kernel bootdisk setup clean run

QEMUARGS=-drive if=ide,id=ata0.0,index=0,file=build/bootdisk.img,format=raw

# primary targets #
all: kernel bootdisk

kernel:
	$(BUILD_TARGETNAME)
	@make -s -f kernel.mk all

# bootdisk setup #
bootdisk: kernel
	$(BUILD_TARGETNAME)
	@./bootdisk.sh

# setup routines #
setup:
	@mkdir -pv build build/kernel
	@./pybuild

clean:
	@rm -v build/kernel/* build/e.clair

run:
	qemu-system-i386 $(QEMUARGS)

run_debug:
	qemu-system-i386 $(QEMUARGS) -d int
