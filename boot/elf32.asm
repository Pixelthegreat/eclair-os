%ifndef ELF32_ASM
%define ELF32_ASM

%define ELF32_STACK_ADDR 0x80000

; 32-bit memcpy ;
; eax = count ;
; esi = source ;
; edi = destination ;
elf32_memcpy:
	pushad
.loop:
	cmp eax, 0
	je .done
	dec eax
	
	mov bl, byte[esi]
	mov byte[edi], bl
	
	inc esi
	inc edi
	jmp .loop
.done:
	popad
	ret

; 32-bit memset ;
; eax = count ;
; bl = byte ;
; esi = buffer ;
elf32_memset:
	pushad
.loop:
	cmp eax, 0
	je .done
	dec eax
	
	mov byte[esi], bl
	
	inc esi
	inc edi
	jmp .loop
.done:
	popad
	ret

; load data from program headers ;
elf32_load_program:
	pushad
	
	mov esi, dword[elf32_kernel_addr]
	mov eax, 0
	mov ebx, 0
	mov bx, ELF_HALF[esi+elf_header.phnum]
	
	mov edi, esi
	add edi, ELF_OFF[esi+elf_header.phoff]
.loop:
	cmp eax, ebx
	je .end
	
	cmp ELF_WORD[edi+elf_program_header.type], ELF_PH_TYPE_LOAD
	jne .next
	
	mov ecx, dword[elf32_kernel_addr]
	add ecx, ELF_OFF[edi+elf_program_header.offset]
	
	mov edx, ELF_ADDR[edi+elf_program_header.paddr]
	
	push eax
	mov eax, ELF_WORD[edi+elf_program_header.filesz]
	push esi
	push edi
	
	mov esi, ecx
	mov edi, edx
	call elf32_memcpy
	
	pop edi
	pop esi
	pop eax
	
	mov ecx, ELF_ADDR[edi+elf_program_header.paddr]
	add ecx, ELF_WORD[edi+elf_program_header.filesz]
	
	push eax
	mov eax, ELF_WORD[edi+elf_program_header.memsz]
	sub eax, ELF_WORD[edi+elf_program_header.filesz]
	
	push ebx
	mov bl, 0
	
	push esi
	mov esi, ecx
	call elf32_memset
	pop esi
	
	pop ebx
	pop eax
.next:
	add edi, elf_program_header_size
	inc eax
	jmp .loop
.end:
	popad
	ret

; load kernel sections into proper memory addresses ;
elf32_load:
	pushad
	
	mov eax, CONFIG_KERNEL_BASE
	shl eax, 4
	mov dword[elf32_kernel_addr], eax
	
	call elf32_load_program
	
	popad
	ret

; jump into kernel space ;
elf32_jump:
	mov esi, dword[elf32_kernel_addr]
	mov edi, ELF_ADDR[esi+elf_header.entry]
	
	mov eax, ELF32_STACK_ADDR
	mov ebp, eax
	mov esp, eax
	
	mov dword[elf32_kernel_entry], edi
	
	mov eax, 0
	mov ebx, 0
	mov ecx, 0
	mov edx, 0
	mov esi, 0
	mov edi, 0
	jmp dword[elf32_kernel_entry]

; data ;
elf32_kernel_addr dd 0
elf32_kernel_entry dd 0

%endif ; ELF32_ASM ;
