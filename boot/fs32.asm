;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef FS32_ASM
%define FS32_ASM

%define FS32_FUNCTION_PRINTS 1
%define FS32_FUNCTION_READ_BLOCK 2

; call into bios from protected mode ;
; eax = function ;
; ebx = argument ;
fs32_call_bios:
	pushad

	mov dword[fs32_function], eax
	mov dword[fs32_argument], ebx
	
	mov eax, esp
	mov dword[fs32_stack], esp
	
	mov eax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	jmp 0x18:.enter16
.enter16:
	[bits 16]
	
	mov eax, cr0
	and eax, 0xfffffffc
	mov cr0, eax
	
	jmp 0x0000:.exitpm
.exitpm:
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	sti
	
	; functions ;
	mov eax, dword[fs32_function]
	mov ebx, dword[fs32_argument]
	
	cmp eax, FS32_FUNCTION_PRINTS
	je .prints
	
	cmp eax, FS32_FUNCTION_READ_BLOCK
	je .read_block
	
	jmp .done
; print string ;
.prints:
	mov esi, ebx
	call print
	jmp .done
; read ecfs block ;
.read_block:
	mov eax, ebx
	call ecfs_read_block
	mov al, '.'
	call printc
	jmp .done
; finished ;
.done:
	cli
	
	mov ax, 0
	mov ds, ax
	lgdt [pm16_gdt_descriptor]
	
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	jmp 0x08:.enter_pm
.enter_pm:
	[bits 32]
	
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	mov eax, dword[fs32_stack]
	mov esp, eax
	
	popad
	ret

; data ;
fs32_stack dd 0
fs32_function dd 0
fs32_argument dd 0

%endif ; FS32_ASM ;
