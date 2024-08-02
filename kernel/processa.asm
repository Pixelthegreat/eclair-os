	[global process_switch]
	
	[extern process_active]
	
	struc process
		.esp: resd 1
		.esp0: resd 1
		.pagedir: resd 1
		.next: resd 1
		.state: resd 1
	endstruc

section .text
process_switch:
	push eax
	push esi
	push edi
	push ebp
	
	; store old process state ;
	mov edi, [process_active]
	mov [edi+process.esp], esp
	
	; load next process state ;
	mov esi, [esp+(4+1)*4]
	mov [process_active], esi
	
	mov esp, [esi+process.esp]
	mov eax, [esi+process.pagedir]
	; todo: task state segment ;
	
	mov ecx, cr3
	cmp eax, ecx
	
	je .done
	mov cr3, eax

.done:
	pop ebp
	pop edi
	pop esi
	pop ebx
	
	ret
