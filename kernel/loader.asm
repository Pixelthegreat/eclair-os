; jump to the kernel, among other things ;
	[global loader]
	[extern kernel_main]
	
	MAGIC equ 0x1badb002
	FLAGS equ 0x3
	CHECKSUM equ -(MAGIC+FLAGS)

section .text
	[bits 32]
	
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

loader:
	call kernel_main
	cli
	jmp $
