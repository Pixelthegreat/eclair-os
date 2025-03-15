; TODO: change page flags from 0x007 to 0x003 to disable user mode access ;
%define ADDR_START 0xC0000000
%define WIDTH 640
%define HEIGHT 480
%define DEPTH 24

%define MEMMAP_TAG 0x6
%define FRAMEBUFFER_TAG 0x8
%define MEMLAYOUT_TAG 0x10

%define S3B_MAGIC 0xc73a3912
%define S3B_PTR 0x500

%define MAGIC 0xe85250d6
	
	global multiboot_data_magic
	global multiboot_data_info
	global boot_data_info
	global kernel_stack_top
	
	extern kernel_main
	extern _kernel_start
	extern _kernel_end

section .multiboot.data
header_start:
	; magic number ;
header_magic dd MAGIC
	; architecture ;
header_arch dd 0
	; length ;
header_length dd header_end - header_start
	; checksum ;
header_checksum dd -(MAGIC + (header_end - header_start))
	; framebuffer tag ;
header_framebuffer_tag:
	dw 5
	dw 0
	dd 24
	dd WIDTH
	dd HEIGHT
	dd DEPTH
	dd 0 ; padding ;
	; information request tag ;
header_inforeq_tag:
	dw 1
	dw 0
	dd 24
	dd MEMMAP_TAG ; memory map ;
	dd FRAMEBUFFER_TAG ; framebuffer ;
	dd MEMLAYOUT_TAG ; basic memory info ;
	dd 0 ; reserved ;
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
kernel_stack_top:

; page tables ;
section .bss nobits
	align 4096
boot_page_dir:
	resb 4096
boot_page_table1:
	resb 4096
boot_page_table2:
	resb 4096
multiboot_data_magic:
	resb 4
multiboot_data_info:
	resb 4
boot_data_info:
	resb 4

; kernel entry ;
section .multiboot.text
	global _start
_start:
	; multiboot structure info ;
	mov [multiboot_data_magic-ADDR_START], eax
	mov [multiboot_data_info-ADDR_START], ebx
	
	cmp ebx, 0
	jne .prep
	
	; s3boot structure info ;
	mov eax, dword[S3B_PTR]
	cmp eax, S3B_MAGIC
	jne .prep
	
	mov eax, dword[S3B_PTR+4]
	mov dword[boot_data_info-ADDR_START], eax
.prep:
	mov edi, boot_page_table1 - ADDR_START
	mov esi, 0
	mov ecx, 2047

.f1:
	; only map kernel ;
	cmp esi, _kernel_start
	jl .f2
	cmp esi, (_kernel_end - ADDR_START)
	jge .f3
	
	; map addresses as rw for now ;
	mov edx, esi
	or edx, 0x007
	mov [edi], edx

.f2:
	; continue ;
	add esi, 4096
	add edi, 4
	loop .f1

.f3:
	; map vga video memory ;
	mov dword[boot_page_table1 - ADDR_START + 1023 * 4], 0x000B8007

	; map in directory ;
	mov dword[boot_page_dir - ADDR_START], boot_page_table1 - ADDR_START + 0x007
	mov dword[boot_page_dir - ADDR_START + 4], boot_page_table2 - ADDR_START + 0x1000 + 0x007
	mov dword[boot_page_dir - ADDR_START + 768 * 4], boot_page_table1 - ADDR_START + 0x007
	mov dword[boot_page_dir - ADDR_START + 769 * 4], boot_page_table2 - ADDR_START + 0x1000 + 0x007
	
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
	
.loop:
	hlt
	jmp .loop
