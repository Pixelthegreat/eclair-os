;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef DISK_ASM
%define DISK_ASM

%define DISK_SECTOR_SIZE 512

; get useful disk info ;
disk_init:
	pusha
	
	mov ah, 0x08
	mov dl, byte[bootdev]
	mov di, 0
	int 0x13
	
	jc disk_error
	
	; unpack chs information ;
	mov byte[disk_heads], dh
	inc byte[disk_heads]
	push cx
	and cx, 0x3f
	mov byte[disk_sectors], cl
	pop cx
	shr cx, 6
	mov word[disk_cylinders], cx
	inc word[disk_cylinders]
	
	mov eax, 0
	mov al, byte[disk_heads]
	mov dword[disk_heads], eax
	mov al, byte[disk_sectors]
	mov dword[disk_sectors], eax
	
	; check if lba addressing is support ;
	mov ah, 0x41
	mov dl, byte[bootdev]
	mov bx, 0x55aa
	int 0x13
	
	jc .end
	and cx, 0x1
	cmp cx, 0
	je .end
	
	mov byte[disk_dap_supported], 1
.end:
	popa
	ret

; convert lba to chs address ;
; eax = lba ;
disk_lba_to_chs:
	push ebx
	push eax
	
	mov edx, 0
	mov ebx, dword[disk_sectors]
	div ebx
	
	mov cl, dl ; sector number ;
	inc cl
	
	mov edx, 0
	mov ebx, dword[disk_heads]
	div ebx
	
	mov ch, al ; cylinder ;
	mov dh, dl ; head ;
	mov dl, byte[bootdev]
	
	pop eax
	pop ebx
	ret

; read sector from disk ;
; eax = sector number ;
; bx = load address ;
disk_read_sector:
	pushad
	
	cmp byte[disk_dap_supported], 0
	jne .lba
.chs:
	call disk_lba_to_chs
	mov ah, 0x02
	mov al, 1
	push bx
	mov bx, 0
	mov es, bx
	pop bx
	int 0x13
	
	jc disk_error
	cmp ah, 0
	jne disk_error_code
	jmp .end
.lba:
	push ds
	
	mov word[disk_dap.segment], es
	mov word[disk_dap.offset], bx
	mov ecx, 0
	mov cx, ax
	mov dword[disk_dap.lba_start_low], ecx
	
	mov ah, 0x42
	mov dl, byte[bootdev]
	
	mov si, disk_dap
	push bx
	mov bx, 0
	mov ds, bx
	pop bx
	
	int 0x13
	pop ds
	
	jc disk_error
	cmp ah, 0
	jne disk_error_code
.end:
	popad
	ret

; print disk related error ;
disk_error:
	mov si, disk_error_msg
	call print_error

; print error with code ;
disk_error_code:
	push ax
	mov si, disk_error_msg
	call print
	pop ax
	
	mov al, ah
	mov ah, 0
	call print_hex
	
	mov si, disk_error_code_msg
	call print
	mov si, newline
	call print_error

; data ;
disk_error_msg db "Disk error!", 10, 0
disk_error_code_msg db "Disk error: ", 0
newline db 10, 0

disk_heads dd 0
disk_cylinders dw 0
disk_sectors dd 0
disk_dap_supported db 0

disk_dap:
.size db 0x10
.reserved db 0
.nsectors dw 1
.offset dw 0
.segment dw 0
.lba_start_low dd 0
.lba_start_high dd 0

%endif ; DISK_ASM ;
