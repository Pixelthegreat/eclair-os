;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
	[org 0x7C00]
	[bits 16]

	jmp short start
	
	times 16-($-$$) db 0

; s3boot block for identifying os boot device ;
boot:
	.magic dd 0xc73a3912
	.osid db "eclair-os"
	times 12-($-.osid) db ' '

; start ;
start:
	cli
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	mov ax, 0x7C00
	mov bp, ax
	mov sp, ax
	
	mov eax, 0
	jmp 0x0000:.next
.next:
	sti
	
	mov si, boot_msg
	call print

	mov byte[bootdev], dl
	call read_sector

	mov dl, byte[bootdev]
	jmp stage1_area
.halt:
	hlt
	jmp .halt

; read bootloader sectors ;
read_sector:
	mov ah, 0x02
	mov dh, 0
	mov dl, byte[bootdev]
	mov al, stage1_nsectors
	mov cl, stage1_sector
	
	push bx
	mov bx, 0
	mov es, bx
	pop bx
	mov bx, stage1_area
	int 0x13
	
	call check_error
	ret

; check for error ;
check_error:
	jc print_error
	
	cld
	mov si, stage1_area
	lodsb
	cmp al, 0
	je print_error
	
	ret

; print error string ;
print_error:
	mov si, disk_error_msg
	call print
	
	cli
.halt:
	hlt
	jmp .halt

; print string ;
print:
	mov ah, 0x0e
	lodsb
	cmp al, 0
	je .done
	int 0x10
	jmp print
.done:
	ret

; data ;
bootdev db 0

boot_msg db "Loading second stage...", 10, 13, 0
disk_error_msg db "Disk error!", 10, 13, 0

stage1_area equ 0x7E00 ; memory location ;
stage1_sector equ 2 ; start sector ;
stage1_nsectors equ 42 ; number of sectors ;

; boot signature ;
times 510-($-$$) db 0
dw 0xaa55
