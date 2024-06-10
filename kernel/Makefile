include common.mk

# arguments #
CFLAGS=-c -m32 -ffreestanding -Iinclude
ASMFLAGS=-f$(ASMARCH)
LDFLAGS=-T kernel/linker.ld -m$(LDARCH)

# object files #
OBJFILES=build/kernel/multiboot.o build/kernel/main.o build/kernel/gdt.o build/kernel/string.o build/kernel/port.o build/kernel/tty.o

all: build/e.clair

build/e.clair: $(OBJFILES) kernel/linker.ld
	$(DST_FILENAME)
	$(LD) $(LDFLAGS) -o build/e.clair $(OBJFILES)

# objs #
build/kernel/multiboot.o: kernel/multiboot.asm
	$(SRC_FILENAME)
	$(ASM) $(ASMFLAGS) -o build/kernel/multiboot.o kernel/multiboot.asm

build/kernel/loader.o: kernel/loader.asm
	$(SRC_FILENAME)
	$(ASM) $(ASMFLAGS) -o build/kernel/loader.o kernel/loader.asm

build/kernel/main.o: kernel/main.c
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/main.o kernel/main.c

build/kernel/gdt.o: kernel/mm/gdt.c include/e.clair/mm/gdt.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/gdt.o kernel/mm/gdt.c

build/kernel/string.o: kernel/util/string.c include/string.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/string.o kernel/util/string.c

build/kernel/port.o: kernel/io/port.c include/e.clair/io/port.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/port.o kernel/io/port.c

build/kernel/tty.o: kernel/tty.c include/e.clair/tty.h
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/tty.o kernel/tty.c
