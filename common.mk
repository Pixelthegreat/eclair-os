include config.mk

SRC_FILENAME=@echo $(notdir $<)
DST_FILENAME=@echo $(notdir $@)
BUILD_TARGETNAME=@echo == $@ ==

HOST_CC=cc
HOST_LD=cc

CC=$(TARGET)-gcc
LD=$(TARGET)-gcc
ASM=nasm
AR=$(TARGET)-ar
STRIP=$(TARGET)-strip

ASMARCH=elf32
LDARCH=elf_i386
