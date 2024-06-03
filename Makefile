.PHONY: kernel setup clean run

# primary targets #
all: kernel

kernel:
	@echo == kernel \(e.clair\) ==
	@make -s -f kernel/Makefile all

# setup routines #
setup:
	@mkdir -pv build build/kernel

clean:
	@rm -v build/kernel/* build/e.clair

run:
	qemu-system-x86_64 -kernel build/e.clair
