;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
; For more information on EcFS, see include/ec/ecfs.h
;
%ifndef ECFS_ASM
%define ECFS_ASM

%define ECFS_SIG0 0x2f
%define ECFS_SIG1 0xec
%define ECFS_SIG2 0x45
%define ECFS_SIG3 0x43

; head block ;
%define ECFS_HBLK_DATASZ 1024
%define ECFS_HBLK_START 2
%define ECFS_HBLK_SIZE 2

struc ecfs_hblk
	.signature resb 4
	.osid resb 12
	.blk_tlrm resd 1
	.nblk_tlrm resd 1
	.blk_brb resd 1
	.nblk_brb resd 1
	.blk_root resd 1
	.blksz resd 1
	.nblk resd 1
	.nblk_alloc resd 1
	.nblk_rsvd resd 1
	.revision resd 1
endstruc

; file entry ;
%define ECFS_FLAG_REG 0x1
%define ECFS_FLAG_DIR 0x2
%define ECFS_FLAG_SLINK 0x4

struc ecfs_file_s
	.name resb 128
	.flags resd 1
	.uid resd 1
	.gid resd 1
	.mask resd 1
	.atime_lo resd 1
	.atime_hi resd 1
	.mtime_lo resd 1
	.mtime_hi resd 1
	.ctime_lo resd 1
	.ctime_hi resd 1
	.fsz resd 1
	.nblk resd 1
	.osimpl resb 8
	.blk:
endstruc

; initialize file system ;
; eax = lba of fs base ;
ecfs_init:
	pusha
	mov dword[ecfs_start_lba], eax
	
	mov ax, ECFS_HBLK_DATASZ
	call allocate
	mov word[ecfs_hblk_area], si
	
	mov eax, dword[ecfs_start_lba]
	add eax, ECFS_HBLK_START
	mov bx, 0
.loop:
	push bx
	mov bx, si
	call disk_read_sector
	pop bx
	
	inc eax
	inc bx
	add si, DISK_SECTOR_SIZE
	cmp bx, ECFS_HBLK_SIZE
	jne .loop
.cont:
	mov si, word[ecfs_hblk_area]
	
	push si
	add si, ecfs_hblk.signature
	cmp byte[si], ECFS_SIG0
	jne .error
	inc si
	cmp byte[si], ECFS_SIG1
	jne .error
	inc si
	cmp byte[si], ECFS_SIG2
	jne .error
	inc si
	cmp byte[si], ECFS_SIG3
	jne .error
	pop si
	
	mov eax, dword[si+ecfs_hblk.blksz]
	mov word[ecfs_block_size], ax
	
	call allocate
	mov word[ecfs_block_area], si
	call allocate
	mov word[ecfs_file_area], si
	
	popa
	ret
.error:
	mov si, ecfs_sig_error_msg
	call print_error

; read a block ;
; eax = block id ;
ecfs_read_block:
	pushad
	
	mov ebx, 0
	mov bx, word[ecfs_block_size]
	shr ebx, 9
	
	mul ebx
	add eax, dword[ecfs_start_lba]
	
	push si
	mov si, word[ecfs_block_area]
.loop:
	cmp ebx, 0
	je .done
	dec ebx
	
	push ebx
	mov ebx, 0
	mov es, bx
	mov bx, si
	call disk_read_sector
	pop ebx
	
	add si, DISK_SECTOR_SIZE
	inc eax
	jmp .loop
.done:
	pop si
	popad
	ret

; read block from a file ;
; eax = block index ;
; eax (return) = 0 if successful, 1 if block was zero
ecfs_read_file_block:
	pushad
	
	mov si, word[ecfs_file_area]
	
	mov ebx, 0
	mov bx, ecfs_file_s.blk
	shr bx, 2
	add eax, ebx
	
	mov bx, word[ecfs_block_size]
	shr bx, 2
	cmp eax, ebx
	jge .error
	
	shl eax, 2
	add si, ax
	mov eax, dword[si]
	cmp eax, 0
	je .one
	
	call ecfs_read_block
	
	popad
	mov eax, 0
	ret
.one:
	popad
	mov eax, 1
	ret
.error:
	mov si, ecfs_size_error_msg
	call print_error

; search directory contents ;
; si = file path ;
ecfs_search_directory:
	pushad
	
	push si
	mov si, word[ecfs_file_area]
	
	mov eax, 0
	mov ebx, dword[si+ecfs_file_s.nblk]
	mov ecx, 0
	pop si
.loop:
	cmp eax, ebx
	jge .error
	
	push eax
	call ecfs_read_file_block
	cmp eax, 0
	pop eax
	jne .next
	
	mov di, word[ecfs_block_area]
	add di, ecfs_file_s.name
	
	push eax
	mov ax, 128
	call strncmp
	cmp ax, 0
	pop eax
	je .end
.next:
	inc eax
	jmp .loop
.end:
	push si
	push di
	mov ax, word[ecfs_block_size]
	mov si, word[ecfs_block_area]
	mov di, word[ecfs_file_area]
	call memcpy
	pop di
	pop si
	
	popad
	ret
.error:
	mov si, ecfs_file_error_msg
	call print_error

; load a file into memory ;
; si = root directory name ;
; di = file path ;
; bx = subroutine to handle block ;
ecfs_load_file:
	pushad
	
	push si
	mov si, word[ecfs_hblk_area]
	mov eax, dword[si+ecfs_hblk.blk_root]
	
	call ecfs_read_block
	
	push di
	mov ax, word[ecfs_block_size]
	mov si, word[ecfs_block_area]
	mov di, word[ecfs_file_area]
	call memcpy
	pop di
	pop si
	
	call ecfs_search_directory
	
	mov si, di
	call ecfs_search_directory
	mov si, word[ecfs_file_area]
	mov di, bx
	
	mov eax, 0
	mov ebx, dword[si+ecfs_file_s.nblk]
.loop:
	cmp eax, ebx
	jge .end
	
	push eax
	call ecfs_read_file_block
	pop eax
	
	call di
	
	inc eax
	jmp .loop
.end:
	popad
	ret

; load file metadata into contiguous memory ;
; si = root directory name ;
; di = file path ;
; bx = initial segment ;
ecfs_load_metadata:
	pushad
	push gs
	
	push si
	mov si, word[ecfs_hblk_area]
	mov eax, dword[si+ecfs_hblk.blk_root]
	
	call ecfs_read_block
	
	push di
	mov ax, word[ecfs_block_size]
	mov si, word[ecfs_block_area]
	mov di, word[ecfs_file_area]
	call memcpy
	pop di
	pop si
	
	call ecfs_search_directory
	
	mov si, di
	call ecfs_search_directory
	
	mov si, word[ecfs_file_area]
	
	mov gs, bx
	mov bx, 0
	mov di, bx
	call memcpy
	add bx, word[ecfs_block_size]
.loop:
	mov di, si
	add di, word[ecfs_block_size]
	sub di, 4
	
	mov eax, dword[di]
	cmp eax, 0
	je .end
	
	call ecfs_read_block
	mov eax, 0
	
	mov ax, word[ecfs_block_size]
	mov si, word[ecfs_block_area]
	mov di, bx
	call memcpy
	
	add bx, word[ecfs_block_size]
	cmp bx, 0
	jne .loop
	mov ax, gs
	add ax, 0x1000
	mov gs, ax
	
	jmp .loop
.end:
	pop gs
	popad
	ret

; data ;
ecfs_start_lba dd 0
ecfs_hblk_area dw 0
ecfs_block_area dw 0
ecfs_file_area dw 0
ecfs_block_size dw 0
ecfs_sig_error_msg db "Failed to recognize EcFS signature!", 10, 0
ecfs_file_error_msg db "Failed to locate file!", 10, 0
ecfs_size_error_msg db "File to big to load!", 10, 0

%endif ; ECFS_ASM ;
