	[global task_switch]
	
	[extern task_active]
	[extern task_nlockpost]
	[extern task_postponed]
	
	[extern page_dir_wrap]
	
	[extern gdt_tss]
	
	struc task
		.esp0: resd 1
		.esp: resd 1
		.cr3: resd 1
		.dir: resd 1
		.prev: resd 1
		.next: resd 1
		.state: resd 1
	endstruc
	
	struc tss
		.prev: resd 1
		.esp0: resd 1
		.ss0: resd 1
	endstruc

section .text
task_switch:
	cmp dword[task_nlockpost], 0
	je .cont
	mov dword[task_postponed], 1
	ret
.cont:
	push ebx
	push esi
	push edi
	push ebp
	
	mov edi, [task_active]
	mov [edi+task.esp], esp
	
	mov esi,[esp+(4+1)*4]
	mov [task_active], esi
	
	mov eax, [esi+task.esp0]
	mov [gdt_tss+tss.esp0], eax
	
	mov esp, [esi+task.esp]
	
	mov eax, [esi+task.cr3]
	mov ecx, cr3
	cmp eax, ecx
	je .done
	
	mov cr3, eax
	mov eax, [esi+task.dir]
	mov dword[page_dir_wrap], eax
.done:
	pop ebp
	pop edi
	pop esi
	pop ebx
	
	ret
