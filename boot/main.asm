	[org 0x7E00]
	[bits 16]

	jmp main

; includes ;
%include "print.asm"
%include "disk.asm"
%include "memory.asm"
%include "mbr.asm"
%include "ext2.asm"
%include "ecfs.asm"
%include "config.asm"
%include "vbe.asm"
%include "menu.asm"
%include "elf16.asm"
%include "pm16.asm"

; main ;
main:
	mov byte[bootdev], dl
	
	mov si, stage1_msg
	call print
	
	call memory_load_map
	
	call disk_init
	call mbr_load_fs
	
	call config_load
	call config_parse
	
	call vbe_load
	
	call menu_init
	call menu_main
	call menu_load
	
	; if all is well, enter protected mode ;
	jmp pm16_enter

; data ;
bootdev db 0
stage1_msg db "Loaded second stage!", 10, 0

; 32 bit code ;
	[bits 32]

%include "elf32.asm"
%include "pm32.asm"

stage1_end equ $
