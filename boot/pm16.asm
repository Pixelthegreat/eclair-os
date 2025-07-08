;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef PM16_ASM
%define PM16_ASM

; check if a20 line is enabled ;
pm16_check_a20:
	pushf
	push ds
	push es
	push di
	push si
	
	cli
	xor ax, ax
	mov es, ax
	
	not ax
	mov ds, ax
	
	mov di, 0x500
	mov si, 0x510
	
	mov al, byte[es:di]
	push ax
	
	mov al, byte[ds:si]
	push ax
	
	mov byte[es:di], 0x00
	mov byte[ds:si], 0xff
	
	cmp byte[es:di], 0xff
	
	pop ax
	mov byte[ds:si], al
	
	pop ax
	mov byte[es:di], al
	
	mov ax, 0
	je .end
	
	mov ax, 1
.end:
	pop si
	pop di
	pop es
	pop ds
	popf
	
	ret

; enable a20 line ;
; code is taken from wiki.osdev.org ;
pm16_enable_a20:
	pusha
	
	call pm16_check_a20
	cmp ax, 1
	je .end
.bios:
	mov ax, 0x2403
	int 0x15
	jb .kbd
	cmp ah, 0
	jnz .kbd
	
	mov ax, 0x2402
	int 0x15
	jb .kbd
	cmp ah, 0
	jnz .kbd
	
	cmp al, 1
	jz .end
	
	mov ax, 0x2401
	int 0x15
	jb .kbd
	cmp ah, 0
	jnz .kbd
	
	call pm16_check_a20
	cmp ax, 1
	jne .kbd
	
	jmp .end
.kbd:
	cli
	call .wait_io1
	mov al, 0xad
	out 0x64, al
	
	call .wait_io1
	mov al, 0xd0
	out 0x64, al
	
	call .wait_io2
	in al, 0x60
	push eax
	
	call .wait_io1
	mov al, 0xd1
	out 0x64, al
	
	call .wait_io1
	pop eax
	or al, 2
	out 0x60, al
	
	call .wait_io1
	mov al, 0xae
	out 0x64, al
	
	call .wait_io1
	sti
	
	call pm16_check_a20
	cmp ax, 1
	jne .fast
	
	jmp .end
.wait_io1:
	in al, 0x64
	test al, 2
	jnz .wait_io1
	ret
.wait_io2:
	in al, 0x64
	test al, 1
	jz .wait_io2
	ret
.fast:
	in al, 0x92
	test al, 2
	jnz .end
	or al, 2
	and al, 0xfe
	out 0x92, al
	
	call pm16_check_a20
	cmp ax, 1
	jne .error
.end:
	popa
	ret
.error:
	mov si, pm16_error_msg
	call print_error

; enter protected mode ;
; steps: ;
; - check if a20 line enabled ;
; - enable a20 line if not ;
; - disable interrupts including nmi ;
; - load global descriptor table ;
; - jump to 32 bit code ;
; a20 checking code is taken from wiki.osdev.org ;
pm16_enter:
	call pm16_enable_a20
	
	; disable interrupts ;
	cli
	
	mov ax, 0
	in al, 0x70
	or ax, 0x80
	out 0x70, al
	
	; load gdt and enable protected mode ;
	lgdt [pm16_gdt_descriptor]
	
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	; jump to 32 bit code ;
	mov eax, pm16_gdt_dataseg
	mov ebx, 0
	mov ecx, 0
	mov edx, 0
	
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov eax, 0x7C00
	mov ebp, eax
	mov esp, eax
	
	jmp pm16_gdt_codeseg:pm32_main

; data ;
pm16_error_msg db "Failed to enter protected mode!", 0

; global descriptor table ;
pm16_gdt:
pm16_gdt_nulldesc:
	dd 0
	dd 0
pm16_gdt_codedesc:
	dw 0xffff ; limit lo ;
	dw 0x0000 ; base lo ;
	db 0x00 ; base lo ;
	db 0b10011010 ; access flags ;
	db 0b11001111 ; flags/limit hi ;
	db 0x00 ; base hi ;
pm16_gdt_datadesc:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0b10010010
	db 0b11001111
	db 0x00
pm16_gdt_int_codedesc:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0b10011110
	db 0b00000000
	db 0x00
pm16_gdt_int_datadesc:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0b10010010
	db 0b00000000
	db 0x00
pm16_gdt_end:
pm16_gdt_descriptor:
	.size dw pm16_gdt_end - pm16_gdt - 1
	.gdt dd pm16_gdt

pm16_gdt_codeseg equ pm16_gdt_codedesc - pm16_gdt_nulldesc
pm16_gdt_dataseg equ pm16_gdt_datadesc - pm16_gdt_nulldesc

%endif ; PM16_ASM ;
