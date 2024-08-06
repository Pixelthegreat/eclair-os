	[global process_switch]
	[global process_signal]
	
	[extern process_active]
	[extern process_lock_ts]
	[extern process_unlock_ts]
	[extern process_handle_signal]

%define SIGNAL_COUNT 2

	struc process
		.esp: resd 1
		.esp0: resd 1
		.pagedir: resd 1
		.next: resd 1
		.state: resd 1
		.wakeup: resq 1
		.nticks: resd 1
		.waitres: resd 1
		.signal: resd 1
		.ret: resd 1
	endstruc

%define STATE_READY 1
%define STATE_RUNNING 2
%define STATE_SLEEPING 3

section .bss
ret_addr: resd 1

section .text
process_switch:
	push eax
	push esi
	push edi
	push ebp
	
	; store old process state ;
	mov edi, [process_active]
	mov [edi+process.esp], esp
	
	cmp dword[edi+process.state], STATE_RUNNING
	jne .cont
	
	mov dword[edi+process.state], STATE_READY
.cont:
	
	; load next process state ;
	mov esi, [esp+(4+1)*4]
	mov [process_active], esi
	
	mov esp, [esi+process.esp]
	mov eax, [esi+process.pagedir]
	; todo: task state segment ;
	mov dword[esi+process.state], STATE_RUNNING
	
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

; handle signal ;
process_signal:
	push esi
	push eax
	
	; call signal handler ;
	call process_handle_signal
	
	; load correct return address ;
	call process_lock_ts
	mov esi, [process_active]
	mov dword[esi+process.signal], 0
	
	mov eax, [esi+process.ret]
	mov [ret_addr], eax
	
	pop eax
	pop esi
	
	call process_unlock_ts
	jmp [ret_addr]
