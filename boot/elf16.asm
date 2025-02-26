%ifndef ELF16_ASM
%define ELF16_ASM

; elf type helpers ;
%define ELF_HALF word
%define ELF_RESH resw 1

%define ELF_OFF dword
%define ELF_RESO resd 1

%define ELF_ADDR dword
%define ELF_RESA resd 1

%define ELF_WORD dword
%define ELF_RESW resd 1

%define ELF_SWORD dword
%define ELF_RESSW resd 1

; elf header ;
%define ELF_NIDENT 16

struc elf_header
	.ident resb ELF_NIDENT
	.type ELF_RESH
	.machine ELF_RESH
	.version ELF_RESW
	.entry ELF_RESA
	.phoff ELF_RESO
	.shoff ELF_RESO
	.flags ELF_RESW
	.ehsize ELF_RESH
	.phentsize ELF_RESH
	.phnum ELF_RESH
	.shentsize ELF_RESH
	.shnum ELF_RESH
	.shstrndx ELF_RESH
endstruc

%define ELF_IDENT_MAG 0
%define ELF_IDENT_CLASS 4
%define ELF_IDENT_DATA 5

%define ELF_CLASS_32 1
%define ELF_DATA_2LSB 1

%define ELF_TYPE_NONE 0
%define ELF_TYPE_REL 1
%define ELF_TYPE_EXEC 2

%define ELF_MACHINE_386 3
%define ELF_VERSION_CURRENT 1

; section header ;
struc elf_section_header
	.name ELF_RESW
	.type ELF_RESW
	.flags ELF_RESW
	.addr ELF_RESA
	.offset ELF_RESO
	.size ELF_RESW
	.link ELF_RESW
	.info ELF_RESW
	.addralign ELF_RESW
	.entsize ELF_RESW
endstruc

%define ELF_SH_TYPE_NULL 0
%define ELF_SH_TYPE_PROGBITS 1
%define ELF_SH_TYPE_SYMTAB 2
%define ELF_SH_TYPE_STRTAB 3
%define ELF_SH_TYPE_RELA 4
%define ELF_SH_TYPE_NOBITS 8
%define ELF_SH_TYPE_REL 9

%define ELF_SH_FLAG_WRITE 0x01
%define ELF_SH_FLAG_ALLOC 0x02

; program header ;
struc elf_program_header
	.type ELF_RESW
	.offset ELF_RESO
	.vaddr ELF_RESA
	.paddr ELF_RESA
	.filesz ELF_RESW
	.memsz ELF_RESW
	.flags ELF_RESW
	.align ELF_RESW
endstruc

%define ELF_PH_TYPE_LOAD 1

; validate kernel file ;
elf16_validate_kernel:
	pusha
	push es
	
	mov bx, CONFIG_KERNEL_BASE
	mov es, bx
	mov si, 0x0000
	
	mov ax, 4
	mov di, elf_magic_buf
	call memcpy
	
	mov si, di
	mov di, elf_magic
	
	call memcmp
	
	cmp ax, 0
	jne .error
	
	mov si, 0x0000
	
	cmp byte[es:si+ELF_IDENT_CLASS], ELF_CLASS_32
	jne .error
	
	cmp byte[es:si+ELF_IDENT_DATA], ELF_DATA_2LSB
	jne .error
	
	cmp ELF_HALF[es:si+elf_header.type], ELF_TYPE_EXEC
	jne .error
	
	cmp ELF_HALF[es:si+elf_header.machine], ELF_MACHINE_386
	jne .error
	
	cmp ELF_WORD[es:si+elf_header.version], ELF_VERSION_CURRENT
	jne .error
	
	pop es
	popa
	ret
.error:
	mov si, elf16_error_msg
	call print_error

; data ;
elf_magic db 0x7f, "ELF"
elf_magic_buf db 0, 0, 0, 0

elf16_error_msg db "Failed to recognize ELF file!", 10, 0

%endif ; ELF16_ASM ;
