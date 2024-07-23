%define ADDR_START 0xC0000000
	
	global multiboot_data_magic
	global multiboot_data_info
	
	extern kernel_main
	extern _kernel_start
	extern _kernel_end

section .multiboot.data
header_start:
	; magic number ;
header_magic dd 0xe85250d6
	; architecture ;
header_arch dd 0
	; length ;
header_length dd header_end - header_start
	; checksum ;
header_checksum dd 0x100000000 - (0xe85250d6 + (header_end - header_start))
	; end tag ;
header_end_tag:
	dw 0
	dw 0
	dd 8
header_end:

; initial stack ;
section .bootstrap_stack nobits
stack_bottom:
	resb 32768 ; 32 KiB ;
stack_top:

; page tables ;
section .bss nobits
	align 4096
boot_page_dir:
	resb 4096
boot_page_table1:
	resb 4096
multiboot_data_magic:
	resb 4
multiboot_data_info:
	resb 4

; kernel entry ;
section .multiboot.text
	global _start
_start:
	; multiboot structure info ;
	mov [multiboot_data_magic-ADDR_START], eax
	mov [multiboot_data_info-ADDR_START], ebx
	
	mov edi, boot_page_table1 - ADDR_START
	mov esi, 0
	mov ecx, 1023

.f1:
	; only map kernel ;
	cmp esi, _kernel_start
	jl .f2
	cmp esi, (_kernel_end - ADDR_START)
	jge .f3
	
	; map addresses as rw for now ;
	mov edx, esi
	or edx, 0x003
	mov [edi], edx

.f2:
	; continue ;
	add esi, 4096
	add edi, 4
	loop .f1

.f3:
	; map vga video memory ;
	mov dword[boot_page_table1 - ADDR_START + 1023 * 4], 0x000B8003

	; map in directory ;
	mov dword[boot_page_dir - ADDR_START], boot_page_table1 - ADDR_START + 0x003
	mov dword[boot_page_dir - ADDR_START + 768 * 4], boot_page_table1 - ADDR_START + 0x003
	
	; update page directory pointer ;
	mov ecx, boot_page_dir - ADDR_START
	mov cr3, ecx
	
	; enable paging ;
	mov ecx, cr0
	or ecx, 0x80010000
	mov cr0, ecx
	
	; jump to higher half ;
	lea ecx, .f4
	jmp ecx

section .text
.f4:
	; unmap identity paging ;
	mov dword[boot_page_dir], 0
	
	; force tlb flush ;
	mov ecx, cr3
	mov cr3, ecx
	
	; set up stack ;
	mov esp, stack_top
	
	; enter kernel ;
	call kernel_main
	
	;cli
	;hlt
	jmp $
