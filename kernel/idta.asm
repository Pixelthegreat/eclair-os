%define SYSINT 0x80
	; be prepared ;
	global isr0
	global isr1
	global isr2
	global isr3
	global isr4
	global isr5
	global isr6
	global isr7
	global isr8
	global isr9
	global isr10
	global isr11
	global isr12
	global isr13
	global isr14
	global isr15
	global isr16
	global isr17
	global isr18
	global isr19
	global isr20
	global isr21
	global isr22
	global isr23
	global isr24
	global isr25
	global isr26
	global isr27
	global isr28
	global isr29
	global isr30
	global isr31
	
	global irq0
	global irq1
	global irq2
	global irq3
	global irq4
	global irq5
	global irq6
	global irq7
	global irq8
	global irq9
	global irq10
	global irq11
	global irq12
	global irq13
	global irq14
	global irq15
	
	global sysint
	
	; handlers ;
	extern idt_isr_handler
	extern idt_irq_handler
	extern idt_int_handler

isr0:
	push byte 0
	push byte 0
	jmp isr_common_stub

isr1:
	push byte 0
	push byte 1
	jmp isr_common_stub

isr2:
	push byte 0
	push byte 2
	jmp isr_common_stub

isr3:
	push byte 0
	push byte 3
	jmp isr_common_stub

isr4:
	push byte 0
	push byte 4
	jmp isr_common_stub

isr5:
	push byte 0
	push byte 5
	jmp isr_common_stub

isr6:
	push byte 0
	push byte 6
	jmp isr_common_stub

isr7:
	push byte 0
	push byte 7
	jmp isr_common_stub

isr8:
	; zero ;
	push byte 8
	jmp isr_common_stub

isr9:
	push byte 0
	push byte 9
	jmp isr_common_stub

isr10:
	; invalid tss ;
	push byte 10
	jmp isr_common_stub

isr11:
	; segment not present ;
	push byte 11
	jmp isr_common_stub

isr12:
	; stack segment fault ;
	push byte 12
	jmp isr_common_stub

isr13:
	; general protection fault ;
	push byte 13
	jmp isr_common_stub

isr14:
	; page fault ;
	push byte 14
	jmp isr_common_stub

isr15:
	push byte 0
	push byte 15
	jmp isr_common_stub

isr16:
	push byte 0
	push byte 16
	jmp isr_common_stub

isr17:
	; alignment check ;
	push byte 17
	jmp isr_common_stub

isr18:
	push byte 0
	push byte 18
	jmp isr_common_stub

isr19:
	push byte 0
	push byte 19
	jmp isr_common_stub

isr20:
	push byte 0
	push byte 20
	jmp isr_common_stub

isr21:
	; control protection exception ;
	push byte 21
	jmp isr_common_stub

isr22:
	push byte 0
	push byte 22
	jmp isr_common_stub

isr23:
	push byte 0
	push byte 23
	jmp isr_common_stub

isr24:
	push byte 0
	push byte 24
	jmp isr_common_stub

isr25:
	push byte 0
	push byte 25
	jmp isr_common_stub

isr26:
	push byte 0
	push byte 26
	jmp isr_common_stub

isr27:
	push byte 0
	push byte 27
	jmp isr_common_stub

isr28:
	push byte 0
	push byte 28
	jmp isr_common_stub

isr29:
	; vmm communication exception ;
	push byte 29
	jmp isr_common_stub

isr30:
	; security exception ;
	push byte 30
	jmp isr_common_stub

isr31:
	push byte 0
	push byte 31
	jmp isr_common_stub

; irqs ;
irq0:
	push byte 0
	push byte 32
	jmp irq_common_stub

irq1:
	push byte 0
	push byte 33
	jmp irq_common_stub

irq2:
	push byte 0
	push byte 34
	jmp irq_common_stub

irq3:
	push byte 0
	push byte 35
	jmp irq_common_stub

irq4:
	push byte 0
	push byte 36
	jmp irq_common_stub

irq5:
	push byte 0
	push byte 37
	jmp irq_common_stub

irq6:
	push byte 0
	push byte 38
	jmp irq_common_stub

irq7:
	push byte 0
	push byte 39
	jmp irq_common_stub

irq8:
	push byte 0
	push byte 40
	jmp irq_common_stub

irq9:
	push byte 0
	push byte 41
	jmp irq_common_stub

irq10:
	push byte 0
	push byte 42
	jmp irq_common_stub

irq11:
	push byte 0
	push byte 43
	jmp irq_common_stub

irq12:
	push byte 0
	push byte 44
	jmp irq_common_stub

irq13:
	push byte 0
	push byte 45
	jmp irq_common_stub

irq14:
	push byte 0
	push byte 46
	jmp irq_common_stub

irq15:
	push byte 0
	push byte 47
	jmp irq_common_stub

sysint:
	push byte 0
	push SYSINT
	jmp int_common_stub

; the moment you've been waiting for ;
isr_common_stub:
	pusha ; gp regs ;
	mov ax, ds
	push eax ; data segment selector ;
	
	; kernel data segment ;
	mov ax, 0x10 ; sp_data ;
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	push esp ; idt_regs_t ;
	call idt_isr_handler
	pop eax
	
	; restore regs ;
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	popa
	
	add esp, 8 ; n_int and err_code ;
	iret

; for irqs ;
irq_common_stub:
	pusha ; gp regs ;
	mov ax, ds
	push eax ; data segment selector ;
	
	; kernel data segment ;
	mov ax, 0x10 ; sp_data ;
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	push esp ; idt_regs_t ;
	call idt_irq_handler
	pop eax
	
	; restore regs ;
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	popa
	
	add esp, 8 ; n_int and err_code ;
	iret

; for software interrupts ;
int_common_stub:
	pusha ; gp regs ;
	mov ax, ds
	push eax ; data segment selector ;
	
	; kernel data segment ;
	mov ax, 0x10 ; sp_data ;
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	push esp ; idt_regs_t ;
	call idt_int_handler
	pop eax
	
	; restore regs ;
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	popa
	
	add esp, 8 ; n_int and err_code ;
	iret
