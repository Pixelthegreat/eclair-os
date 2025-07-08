;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef PRINT_ASM
%define PRINT_ASM

%define PRINT_ZERO 48

; print character ;
; al = character ;
printc:
	push ax
	push bx
	
	mov ah, 0x0e
	mov bh, 0
	mov bl, 0x7
	int 0x10
	
	cmp al, 10
	jne .end
	
	mov al, 13
	int 0x10
.end:
	pop bx
	pop ax
	ret

; print string ;
print:
	push ax
	push si
.loop:
	lodsb
	cmp al, 0
	je .done
	call printc
	jmp .loop
.done:
	pop si
	pop ax
	ret

; print string until newline ;
printnl:
	push ax
	push si
.loop:
	mov ah, 0x0e
	lodsb
	cmp al, 0
	je .done
	cmp al, 10
	je .done
	int 0x10
	cmp al, 10
	jne .loop
	mov al, 13
	int 0x10
	jmp .loop
.done:
	pop si
	pop ax
	ret

; print hex integer ;
print_hex:
	push ax
	push bx
	push si
	
	mov bx, 0
.loop:
	push ax
	mov si, .hex_chars
	shr ax, 12
	and ax, 0xf
	add si, ax
	lodsb
	
	mov ah, 0x0e
	int 0x10
	pop ax
	
	shl ax, 4
	
	inc bx
	cmp bx, 4
	jne .loop
	jmp .done
.zero:
	mov ah, 0x0e
	mov al, PRINT_ZERO
	int 0x10
.done:
	pop si
	pop bx
	pop ax
	ret

.hex_chars db "0123456789abcdef"

; print string and stop ;
print_error:
	call print
	cli
.halt:
	hlt
	jmp .halt

%endif ; PRINT_ASM ;
