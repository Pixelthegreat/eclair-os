	[global process_switch]

	[extern process_active]

	struc process
		.esp: resd 1
		.esp0: resd 1
		.pagedir: resd 1
		.prev: resd 1
		.next: resd 1
	endstruc

section .text

; switch process ;
process_switch:
	push ebx
	push esi
	push edi
	push ebp
	
	; save esp ;
	mov edi, [process_active]
	mov [edi+process.esp], esp
	
	; load next process state ;
	mov esi, [esp+(4+1)*4]
	mov [process_active], esi
	
	; load values from process ;
	mov esp, [esi+process.esp]
	mov eax, [esi+process.pagedir]
	mov ecx, cr3
	
	; update virtual address space ;
	cmp eax, ecx
	je .done
	mov cr3, eax

.done:
	pop ebp
	pop edi
	pop esi
	pop ebx
	ret
