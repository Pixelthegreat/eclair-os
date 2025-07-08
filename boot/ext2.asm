;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef EXT2_ASM
%define EXT2_ASM

%define EXT2_SIG 0xef53
%define EXT2_SB_START 2
%define EXT2_SB_SIZE 2
%define EXT2_SB_DATASZ 1024
%define EXT2_INODE_SIZE 128
%define EXT2_BGD_SIZE 32

; superblock ;
struc ext2_superblock
	.ninodes resd 1
	.nblocks resd 1
	.nsublocks resd 1
	.nfreeblocks resd 1
	.nfreeinodes resd 1
	.sbblock resd 1
	.blocksz resd 1
	.fragsz resd 1
	.nbgblocks resd 1
	.nbgfrags resd 1
	.nbginodes resd 1
	.lastmnttime resd 1
	.lastwritetime resd 1
	.nlastfsck resw 1
	.nuntilfsck resw 1
	.signature resw 1
	resb 4
	.verminor resw 1
	resb 12
	.vermajor resw 1
	resb 10
	.inodesz resw 1
endstruc

; block group descriptor ;
struc ext2_bg_descriptor
	.bbub resd 1
	.biub resd 1
	.itab resd 1
	.nfreeblocks resw 1
	.nfreeinodes resw 1
	.ndirs resw 1
endstruc

; inode ;
struc ext2_inode_s
	.type: resw 1
	.uid: resw 1
	.szlow: resd 1
	.tmaccess: resd 1
	.tmcreation: resd 1
	.tmmodification: resd 1
	.tmdeletion: resd 1
	.gid: resw 1
	.nlinks: resw 1
	.nsectors: resd 1
	.flags: resd 1
	.osimpl: resd 1
	.dbp: resd 12
	.sibp: resd 1
	.dibp: resd 1
	.tibp: resd 1
	.ngeneration: resd 1
	.bextattrib: resd 1
	.szhigh: resd 1
	.bfragment: resd 1
	.osimpl2: resb 12
endstruc

; directory entry ;
struc ext2_dirent
	.inode: resd 1
	.entsz: resw 1
	.lenlow: resb 1
	.type: resb 1
	.name:
endstruc

; initialize file system ;
; eax = lba of fs base ;
ext2_init:
	pusha
	mov dword[ext2_start_lba], eax
	
	mov ax, EXT2_SB_DATASZ
	call allocate
	mov word[ext2_sb_area], si
	
	mov eax, dword[ext2_start_lba]
	add eax, EXT2_SB_START
	mov bx, 0
.loop:
	push bx
	mov bx, si
	call disk_read_sector
	pop bx
	
	inc eax
	inc bx
	add si, DISK_SECTOR_SIZE
	cmp bx, EXT2_SB_SIZE
	jne .loop
.cont:
	mov si, word[ext2_sb_area]
	
	mov ax, word[si+ext2_superblock.signature]
	cmp ax, EXT2_SIG
	jne .error
	
	mov eax, 1024
	mov ebx, dword[si+ext2_superblock.blocksz]
.loop2:
	cmp ebx, 0
	je .block
	
	shl eax, 1
	dec ebx
	jmp .loop2
.block:
	mov word[ext2_block_size], ax
	
	push si
	call allocate
	mov word[ext2_block_area], si
	pop si
	
	mov dx, 0
	mov bx, EXT2_BGD_SIZE
	div bx
	mov word[ext2_bgds_per_block], ax
	
	mov ax, EXT2_INODE_SIZE
	cmp byte[si+ext2_superblock.vermajor], 1
	jl .cont2
	
	mov ax, word[si+ext2_superblock.inodesz]
.cont2:
	call allocate
	mov word[ext2_inode_area], si
	mov word[ext2_inode_size], ax
	
	mov dx, 0
	mov ax, word[ext2_block_size]
	div word[ext2_inode_size]
	mov word[ext2_inodes_per_block], ax
	
	popa
	ret
.error:
	mov si, ext2_sig_error_msg
	call print_error

; read an inode ;
; eax = inode id ;
ext2_read_inode:
	pusha
	push si
	mov si, word[ext2_sb_area]
	
	; determine block group ;
	dec eax
	mov edx, 0
	div dword[si+ext2_superblock.nbginodes]
	mov ebx, eax ; block group ;
	mov ecx, edx ; inode index ;
	
	; load block group descriptor ;
	mov edx, 0
	div word[ext2_bgds_per_block]
	inc eax
	
	cmp word[ext2_block_size], 1024
	jne .cont
	inc eax
.cont:
	call ext2_read_block
	
	mov ax, EXT2_BGD_SIZE
	mul dx
	mov si, word[ext2_block_area]
	add si, ax
	
	; load inode ;
	mov edx, 0
	mov eax, ecx
	div word[ext2_inodes_per_block]
	add eax, dword[si+ext2_bg_descriptor.itab]
	call ext2_read_block
	
	mov ax, word[ext2_inode_size]
	mul dx
	mov si, word[ext2_block_area]
	add si, ax
	
	push di
	mov ax, word[ext2_inode_size]
	mov di, word[ext2_inode_area]
	call memcpy
	pop di
	
	pop si
	popa
	ret

; read a block ;
; eax = block id ;
ext2_read_block:
	pushad
	
	mov ebx, 0
	mov bx, word[ext2_block_size]
	
	shr ebx, 9
	
	mul ebx
	add eax, dword[ext2_start_lba]
	
	push si
	mov si, word[ext2_block_area]
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

; read block from contents of inode ;
; ax = block index ;
ext2_read_inode_block:
	pushad
	
	mov si, word[ext2_inode_area]
	cmp ax, 12
	jl .less
	
	sub ax, 12
	cmp ax, 256
	jge .error
	
	push eax
	mov eax, dword[si+ext2_inode_s.sibp]
	call ext2_read_block
	pop eax
	
	mov si, word[ext2_block_area]
	mov dx, 4
	mul dx
	add si, ax
	mov eax, dword[si]
	jmp .end
.less:
	add si, ext2_inode_s.dbp
	mov dx, 4
	mul dx
	add si, ax
	mov eax, dword[si]
.end:
	call ext2_read_block
	popad
	ret
.error:
	mov si, ext2_size_error_msg
	call print_error

; search directory contents ;
; si = file path ;
ext2_search_directory:
	pushad
	
	push si
	mov si, word[ext2_inode_area]
	
	mov eax, 0
	mov ebx, dword[si+ext2_inode_s.szlow]
	mov ecx, 0
	pop si
.loop:
	cmp eax, ebx
	jge .error
	
	push ecx
	push eax
	mov edx, 0
	mov cx, word[ext2_block_size]
	div cx
	pop eax
	pop ecx
	
	mov di, word[ext2_block_area]
	add di, dx
	
	cmp edx, 0
	jne .dirent
	
	push eax
	mov eax, ecx
	call ext2_read_inode_block
	pop eax
	inc ecx
.dirent:
	push ebx
	push eax
	call strlen
	mov bx, ax
	mov ah, 0
	mov al, byte[di+ext2_dirent.lenlow]
	
	cmp ax, bx
	jne .next
	
	push di
	add di, ext2_dirent.name
	call memcmp
	pop di
	
	cmp ax, 0
	je .end
.next:
	pop eax
	
	mov ebx, 0
	mov bx, word[di+ext2_dirent.entsz]
	add eax, ebx
	pop ebx
	
	jmp .loop
.end:
	pop eax
	pop ebx
	
	mov eax, dword[di+ext2_dirent.inode]
	call ext2_read_inode
	
	popad
	ret
.error:
	mov si, ext2_file_error_msg
	call print_error

; load a file into memory ;
; si = root directory name ;
; di = file path ;
; bx = subroutine to handle block ;
ext2_load_file:
	pushad
	
	mov eax, 2
	call ext2_read_inode
	call ext2_search_directory
	
	mov si, di
	call ext2_search_directory
	mov si, word[ext2_inode_area]
	mov di, bx
	
	mov eax, 0
	mov ebx, dword[si+ext2_inode_s.szlow]
	mov ecx, 0
.loop:
	cmp eax, ebx
	jge .end
	
	push eax
	mov eax, ecx
	call ext2_read_inode_block
	pop eax
	
	call di
	
	push ebx
	mov ebx, 0
	mov bx, word[ext2_block_size]
	add eax, ebx
	pop ebx
	
	inc ecx
	jmp .loop
.end:	
	popad
	ret

; data ;
ext2_start_lba dd 0
ext2_sb_area dw 0
ext2_block_area dw 0
ext2_inode_area dw 0
ext2_block_size dw 0
ext2_inode_size dw 0
ext2_bgds_per_block dw 0
ext2_inodes_per_block dw 0
ext2_sig_error_msg db "Failed to recognize Ext2 signature!", 10, 0
ext2_file_error_msg db "Failed to locate file!", 10, 0
ext2_size_error_msg db "File too big to load!", 10, 0

%endif ; EXT2_ASM ;
