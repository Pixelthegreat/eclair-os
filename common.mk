include config.mk

SRC_FILENAME=@echo $(notdir $<)
DST_FILENAME=@echo $(notdir $@)

CC=$(TARGET)-gcc
LD=$(TARGET)-ld
ASM=nasm
AR=$(TARGET)-ar

ASMARCH=elf32
LDARCH=elf_i386
