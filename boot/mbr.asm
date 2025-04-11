%ifndef MBR_ASM
%define MBR_ASM

%define MBR_START 440
%define MBR_ENTRY_START 6
%define MBR_ENTRY_SIZE 16

%define MBR_TYPE_LINUX 0x83
%define MBR_TYPE_ECLAIR 0xec
%define MBR_BOOTABLE 0x80

; main table ;
struc mbr
	.signature resd 1
	.reserved resw 1
	.entries resq 8
	.boot resw 1
endstruc

; entry ;
struc mbr_entry
	.attributes resb 1
	.start_chs resb 3
	.type resb 1
	.last_chs resb 3
	.start_lba resd 1
	.sector_count resd 1
endstruc

; search mbr for bootable partition ;
; ax (return) = partition number ;
mbr_search:
	push si
	push bx
	
	mov si, 0x7C00
	add si, MBR_START+MBR_ENTRY_START
	
	mov ax, 0
.loop:
	mov bx, word[si+mbr_entry.attributes]
	and bx, MBR_BOOTABLE
	cmp bx, 0
	jne .done
	
	inc ax
	add si, MBR_ENTRY_SIZE
	cmp ax, 4
	je .error
	jmp .loop
.error:
	mov si, mbr_error_msg
	call print_error
.done:
	pop bx
	pop si
	ret

; load filesystem ;
mbr_load_fs:
	pusha
	call mbr_search
	
	mov si, 0x7C00
	add si, MBR_START+MBR_ENTRY_START
	
	mov bx, 16
	mul bx
	add si, ax
	
	mov al, byte[si+mbr_entry.type]
	cmp al, MBR_TYPE_LINUX
	je .linux
	cmp al, MBR_TYPE_ECLAIR
	je .eclair
.error:
	mov si, mbr_fs_error_msg
	call print_error
.linux:
	push si
	mov si, mbr_linux_msg
	call print
	pop si
	
	mov byte[mbr_part_fs], MBR_TYPE_LINUX
	
	mov eax, dword[si+mbr_entry.start_lba]
	call ext2_init
	jmp .done
.eclair:
	push si
	mov si, mbr_eclair_msg
	call print
	pop si
	
	mov byte[mbr_part_fs], MBR_TYPE_ECLAIR
	
	mov eax, dword[si+mbr_entry.start_lba]
	call ecfs_init
.done:
	popa
	ret

; data ;
mbr_part_fs db 0

mbr_error_msg db "Failed to locate bootable partition!", 10, 0
mbr_fs_error_msg db "Unrecognized file system on partition!", 10, 0
mbr_linux_msg db "File system is Ext2/3/4...", 10, 0
mbr_eclair_msg db "File system is EcFS...", 10, 0

%endif ; MBR_ASM ;
