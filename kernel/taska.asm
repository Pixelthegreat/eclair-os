	[global task_switch]
	[global task_handle_signal]
	[global task_handle_signal_size]
	[global task_test]
	
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

; switch to task ;
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

; userspace signal handler ;
%define TASK_STACK_ADDR_SIGHANDLER 0x8000
%define TASK_STACK_ADDR_SIGEIP 0x8004

task_handle_signal:
	call dword[TASK_STACK_ADDR_SIGHANDLER]
	jmp dword[TASK_STACK_ADDR_SIGEIP]
.end:

; test entry point for task ;
%define INT_MAX 2147483647
%define BUF_SIZE 32

task_test:
	mov eax, 2 ; open ;
	mov ebx, fname ; path ;
	mov ecx, 0x3 ; flags: FS_READ | FS_WRITE ;
	mov edx, 0 ; mask ;
	int 0x80
	
	cmp eax, INT_MAX
	jg .end
	
	mov dword[ttyfd], eax
	
	; write message ;
	mov eax, 4 ; write ;
	mov ebx, dword[ttyfd]
	mov ecx, msg
	mov edx, msg_len
	int 0x80
	
	cmp eax, INT_MAX
	jg .end
	
	; read input ;
	mov eax, 3 ; read ;
	mov ebx, dword[ttyfd]
	mov ecx, buf
	mov edx, BUF_SIZE
	int 0x80
	
	cmp eax, INT_MAX
	jg .end
	
	mov edx, eax
	dec edx
	mov eax, 4 ; write ;
	int 0x80
	
	cmp eax, INT_MAX
	jg .end
	
	; close tty file ;
	mov eax, 6 ; close ;
	mov ebx, dword[ttyfd]
	int 0x80
	
	cmp eax, INT_MAX
	jg .end
.end:
	mov eax, 1 ; exit ;
	int 0x80
	
	hlt

section .data
	task_handle_signal_size dd task_handle_signal.end-task_handle_signal
	fname db "/dev/tty0", 0
	msg db "Hello world from user mode!", 10
	msg_len equ $-msg

section .bss
	ttyfd resd 1
	buf resb BUF_SIZE
