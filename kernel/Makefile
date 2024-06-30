include common.mk

# arguments #
CFLAGS=-c -ffreestanding -Iinclude
ASMFLAGS=-f$(ASMARCH)
LDFLAGS=-T kernel/linker.ld -ffreestanding -nostdlib -lgcc

# object files #
OBJFILES=build/kernel/multiboota.o build/kernel/multiboot.o build/kernel/main.o build/kernel/gdt.o build/kernel/string.o build/kernel/port.o build/kernel/tty.o build/kernel/idt.o build/kernel/idta.o build/kernel/ps2.o build/kernel/paging.o build/kernel/heap.o

all: build/e.clair

build/e.clair: $(OBJFILES) kernel/linker.ld
	$(DST_FILENAME)
	$(LD) $(LDFLAGS) -o build/e.clair $(OBJFILES)

# objs #
build/kernel/multiboota.o: kernel/multiboota.asm
	$(SRC_FILENAME)
	$(ASM) $(ASMFLAGS) -o build/kernel/multiboota.o kernel/multiboota.asm

build/kernel/multiboot.o: kernel/multiboot.c include/e.clair/multiboot.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/multiboot.o kernel/multiboot.c

build/kernel/main.o: kernel/main.c
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/main.o kernel/main.c

build/kernel/gdt.o: kernel/mm/gdt.c include/e.clair/mm/gdt.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/gdt.o kernel/mm/gdt.c

build/kernel/string.o: kernel/util/string.c include/e.clair/string.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/string.o kernel/util/string.c

build/kernel/port.o: kernel/io/port.c include/e.clair/io/port.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/port.o kernel/io/port.c

build/kernel/tty.o: kernel/tty.c include/e.clair/tty.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/tty.o kernel/tty.c

build/kernel/idt.o: kernel/idt.c include/e.clair/idt.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/idt.o kernel/idt.c

build/kernel/idta.o: kernel/idta.asm include/e.clair/idt.h
	$(SRC_FILENAME)
	$(ASM) $(ASMFLAGS) -o build/kernel/idta.o kernel/idta.asm

build/kernel/ps2.o: kernel/driver/ps2.c include/e.clair/driver/ps2.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/ps2.o kernel/driver/ps2.c

build/kernel/paging.o: kernel/mm/paging.c include/e.clair/mm/paging.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/paging.o kernel/mm/paging.c

build/kernel/heap.o: kernel/heap.c include/e.clair/heap.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/heap.o kernel/heap.c
