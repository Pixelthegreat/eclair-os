.PHONY: kernel setup clean run

QEMUARGS=-drive if=ide,id=ata0.0,index=0,file=build/bootdisk.img,format=raw

# primary targets #
all: kernel bootdisk

kernel:
	@echo == kernel \(e.clair\) ==
	@make -s -f kernel/Makefile all

bootdisk: kernel
	@echo == bootdisk ==
	@./bootdisk.sh

# setup routines #
setup:
	@mkdir -pv build build/kernel

clean:
	@rm -v build/kernel/* build/e.clair

run:
	qemu-system-x86_64 $(QEMUARGS)

run_debug:
	qemu-system-x86_64 $(QEMUARGS) -d int
