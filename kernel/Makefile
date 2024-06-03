include common.mk

# arguments #
CFLAGS=-c -m32 -ffreestanding -mno-red-zone
ASMFLAGS=-f$(ASMARCH)
LDFLAGS=-T kernel/linker.ld -m$(LDARCH)

# object files #
OBJFILES=build/kernel/loader.o build/kernel/main.o

all: build/e.clair

build/e.clair: $(OBJFILES) kernel/linker.ld
	$(DST_FILENAME)
	$(LD) $(LDFLAGS) -o build/e.clair $(OBJFILES)

# objs #
build/kernel/loader.o: kernel/loader.asm
	$(SRC_FILENAME)
	$(ASM) $(ASMFLAGS) -o build/kernel/loader.o kernel/loader.asm

build/kernel/main.o: kernel/main.c
	$(SRC_FILENAME)
	$(CC) $(CFLAGS) -o build/kernel/main.o kernel/main.c
