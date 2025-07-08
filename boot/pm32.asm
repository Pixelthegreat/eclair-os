;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef PM32_ASM
%define PM32_ASM

%define PM32_PAGE_SIZE 0x1000

%define PM32_BOOT_PTR 0x500
%define PM32_BOOT_MAGIC 0xc73a3912

; boot pointer structure ;
struc pm32_boot_pointer
	.magic: resd 1
	.pointer: resd 1
endstruc

; main structure ;
%define PM32_INDEX(i) i*4

%define PM32_BOOT_STRUCT_CMDLINE 0
%define PM32_BOOT_STRUCT_MEMMAP 1
%define PM32_BOOT_STRUCT_FRAMEBUF 2
%define PM32_BOOT_STRUCT_INITRD 3

%define PM32_BOOT_STRUCT_COUNT 4

struc pm32_boot
	.checksum resd 1
	.size resd 1
	.offsets resd PM32_BOOT_STRUCT_COUNT
endstruc

; memory map entry ;
%define PM32_MEMMAP_NULL 0
%define PM32_MEMMAP_USABLE 1
%define PM32_MEMMAP_UNUSABLE 2

struc pm32_memmap_entry
	.base_low resd 1
	.base_high resd 1
	.length_low resd 1
	.length_high resd 1
	.type resd 1
endstruc

struc pm32_boot_memmap_entry
	.type resd 1
	.start resd 1
	.end resd 1
endstruc

; framebuffer info ;
%define PM32_FRAMEBUF_RGB 1

struc pm32_boot_framebuf
	.type resd 1
	.addr resd 1
	.pitch resd 1
	.width resd 1
	.height resd 1
	.depth resd 1
	.rmask_sz resb 1
	.rmask_pos resb 1
	.gmask_sz resb 1
	.gmask_pos resb 1
	.bmask_sz resb 1
	.bmask_pos resb 1
endstruc

; initrd info ;
struc pm32_boot_initrd
	.addr resd 1
	.size resd 1
endstruc

; align value ;
; eax = value to align ;
; ebx = size to align to ;
; eax (return) = aligned value ;
pm32_align:
	push ebx
	
	dec ebx
	add eax, ebx
	not ebx
	and eax, ebx
	
	pop ebx
	ret

; copy string ;
; esi = source ;
; edi = destination ;
; eax (return) = length/bytes copied ;
pm32_strcpy:
	push esi
	push edi
	
	mov eax, 0
.loop:
	push eax
	mov al, byte[esi]
	mov byte[edi], al
	
	cmp al, 0
	pop eax
	je .done
	
	inc esi
	inc edi
	inc eax
	jmp .loop
.done:
	pop edi
	pop esi
	ret

; load cmdline data ;
; esi = main structure ;
pm32_load_boot_cmdline:
	pusha
	
	mov eax, dword[esi+pm32_boot.size]
	mov dword[esi+pm32_boot.offsets+PM32_BOOT_STRUCT_CMDLINE*4], eax
	
	mov edi, esi
	add edi, eax
	
	push esi
	mov esi, 0
	mov si, word[menu_cmdline]
	
	call pm32_strcpy
	inc eax
	
	pop esi
	add dword[esi+pm32_boot.size], eax
	
	popa
	ret

; load memory map data ;
; esi = main structure ;
pm32_load_boot_memmap:
	pusha
	
	mov eax, dword[esi+pm32_boot.size]
	mov dword[esi+pm32_boot.offsets+PM32_BOOT_STRUCT_MEMMAP*4], eax
	mov edx, esi
	add esi, eax

	mov edi, 0
	mov di, word[memory_map_area]
	mov cx, word[memory_map_count]
.loop:
	mov eax, dword[edi+pm32_memmap_entry.base_low]
	mov dword[esi+pm32_boot_memmap_entry.start], eax
	mov dword[esi+pm32_boot_memmap_entry.end], eax
	
	mov eax, dword[edi+pm32_memmap_entry.length_low]
	add dword[esi+pm32_boot_memmap_entry.end], eax
	
	cmp eax, dword[pm32_region_length]
	jl .type
	
	mov dword[pm32_region_length], eax
	mov eax, dword[edi+pm32_memmap_entry.base_low]
	mov dword[pm32_initrd_area], eax
.type:
	mov dword[esi+pm32_boot_memmap_entry.type], PM32_MEMMAP_USABLE
	cmp dword[di+pm32_memmap_entry.type], 1
	je .next
	
	mov dword[esi+pm32_boot_memmap_entry.type], PM32_MEMMAP_UNUSABLE
.next:
	push esi
	mov esi, edx
	add dword[esi+pm32_boot.size], pm32_boot_memmap_entry_size
	pop esi
	
	add esi, pm32_boot_memmap_entry_size
.iter:
	dec cx
	jz .done
	
	add di, 24
	jmp .loop
.done:
	mov dword[esi+pm32_boot_memmap_entry.type], PM32_MEMMAP_NULL
	mov dword[esi+pm32_boot_memmap_entry.start], 0
	mov dword[esi+pm32_boot_memmap_entry.end], 0
	
	mov esi, edx
	add dword[esi+pm32_boot.size], pm32_boot_memmap_entry_size
	
	mov eax, dword[pm32_initrd_area]
	mov ebx, dword[elf32_kernel_end]
	cmp eax, ebx
	jge .end
	
	mov dword[pm32_initrd_area], ebx
.end:
	add dword[pm32_initrd_area], 0x2000 ; so as to not override boot structs ;
	mov eax, dword[pm32_initrd_area]
	mov dword[pm32_initrd_base], eax
	
	popa
	ret

; load framebuffer data ;
; esi = main structure ;
pm32_load_boot_framebuf:
	pusha
	
	cmp dword[vbe_selected_mode.addr], 0
	je .end
	
	mov eax, dword[esi+pm32_boot.size]
	mov dword[esi+pm32_boot.offsets+PM32_BOOT_STRUCT_FRAMEBUF*4], eax
	
	mov edi, esi
	add edi, eax
	
	mov dword[edi+pm32_boot_framebuf.type], PM32_FRAMEBUF_RGB
	
	mov eax, dword[vbe_selected_mode.addr]
	mov dword[edi+pm32_boot_framebuf.addr], eax
	
	mov eax, 0
	mov ax, word[vbe_selected_mode.width]
	mov dword[edi+pm32_boot_framebuf.width], eax
	
	mov ax, word[vbe_selected_mode.height]
	mov dword[edi+pm32_boot_framebuf.height], eax
	
	mov ax, word[vbe_selected_mode.pitch]
	mov dword[edi+pm32_boot_framebuf.pitch], eax
	
	mov ax, word[vbe_selected_mode.depth]
	mov dword[edi+pm32_boot_framebuf.depth], eax
	
	mov al, byte[vbe_selected_mode.red_mask_sz]
	mov byte[edi+pm32_boot_framebuf.rmask_sz], al
	
	mov al, byte[vbe_selected_mode.green_mask_sz]
	mov byte[edi+pm32_boot_framebuf.gmask_sz], al
	
	mov al, byte[vbe_selected_mode.blue_mask_sz]
	mov byte[edi+pm32_boot_framebuf.bmask_sz], al
	
	mov al, byte[vbe_selected_mode.red_field_pos]
	mov byte[edi+pm32_boot_framebuf.rmask_pos], al
	
	mov al, byte[vbe_selected_mode.green_field_pos]
	mov byte[edi+pm32_boot_framebuf.gmask_pos], al
	
	mov al, byte[vbe_selected_mode.blue_field_pos]
	mov byte[edi+pm32_boot_framebuf.bmask_pos], al
	
	add dword[esi+pm32_boot.size], pm32_boot_framebuf_size
.end:
	popa
	ret

; load initial ramdisk ;
; esi = location of boot structure ;
pm32_load_boot_initrd:
	pusha
	mov edx, esi
	
	cmp byte[config_initrd], 0
	je .end
	
	mov eax, FS32_FUNCTION_PRINTS
	mov ebx, pm32_initrd_msg
	call fs32_call_bios
	
	mov edi, 0
	mov di, word[config_initrd_segment]
	shl edi, 4
	add edi, ecfs_file_s.blk
	
	mov cx, ecfs_file_s.blk
.loop:
	mov ebx, dword[edi]
	cmp ebx, 0
	je .done
	
	mov eax, FS32_FUNCTION_READ_BLOCK
	call fs32_call_bios
	
	mov eax, 0
	mov ax, word[ecfs_block_size]
	mov esi, 0
	mov si, word[ecfs_block_area]
	push edi
	mov edi, dword[pm32_initrd_area]
	call elf32_memcpy
	pop edi
	add dword[pm32_initrd_area], eax
	
	add edi, 4
	add cx, 4
	sub ax, 4
	cmp cx, ax
	jl .loop
	
	add edi, 4
	mov cx, 0
	jmp .loop
.done:
	; load structure ;
	mov esi, edx
	
	mov eax, dword[esi+pm32_boot.size]
	mov dword[esi+pm32_boot.offsets+PM32_BOOT_STRUCT_INITRD*4], eax
	
	mov edi, esi
	add edi, eax
	
	mov eax, dword[pm32_initrd_base]
	mov dword[edi+pm32_boot_initrd.addr], eax
	
	mov ebx, eax
	mov eax, dword[pm32_initrd_area]
	sub eax, ebx
	mov dword[edi+pm32_boot_initrd.size], eax
	
	add dword[esi+pm32_boot.size], pm32_boot_initrd_size
.end:
	popa
	ret

; calculate checksum value ;
; esi = location of boot structure ;
pm32_load_boot_checksum:
	pusha
	mov edi, esi
	
	mov eax, 4
	mov ebx, dword[edi+pm32_boot.size]
	mov dword[edi+pm32_boot.checksum], 0
	
	add esi, eax
.loop:
	cmp eax, ebx
	je .done
	
	push eax
	mov eax, 0
	mov al, byte[esi]
	add dword[edi+pm32_boot.checksum], eax
	pop eax
	
	inc eax
	inc esi
	jmp .loop
.done:
	popa
	ret

; load boot protocol structures ;
pm32_load_boot:
	pusha
	
	mov eax, dword[elf32_kernel_end]
	mov ebx, PM32_PAGE_SIZE
	call pm32_align
	
	mov dword[pm32_boot_struct], eax
	
	mov esi, PM32_BOOT_PTR
	mov dword[esi+pm32_boot_pointer.magic], PM32_BOOT_MAGIC
	mov dword[esi+pm32_boot_pointer.pointer], eax
	
	mov esi, eax
	
	mov dword[esi+pm32_boot.size], pm32_boot_size
	
	call pm32_load_boot_cmdline
	call pm32_load_boot_memmap
	call pm32_load_boot_framebuf
	call pm32_load_boot_initrd
	call pm32_load_boot_checksum
	
	popa
	ret

; main protected mode ;
pm32_main:
	call elf32_load
	call pm32_load_boot
	
	jmp elf32_jump
.end:
	hlt
	jmp .end

; data ;
pm32_initrd_msg db "Loading ramdisk...", 0

pm32_boot_struct dd 0
pm32_initrd_base dd 0
pm32_initrd_area dd 0
pm32_region_length dd 0

%endif ; PM32_ASM ;
