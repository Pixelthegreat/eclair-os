include config.mk

SRC_FILENAME=@echo $(notdir $<)
DST_FILENAME=@echo $(notdir $@)
BUILD_TARGETNAME=@echo == $@ ==

CC=$(TARGET)-gcc
LD=$(TARGET)-gcc
ASM=nasm
AR=$(TARGET)-ar

ASMARCH=elf32
LDARCH=elf_i386
