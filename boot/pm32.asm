%ifndef PM32_ASM
%define PM32_ASM

; main protected mode ;
pm32_main:
	call elf32_load
	jmp elf32_jump
.end:
	hlt
	jmp .end

; data ;
pm32_msg db "Welcome to protected mode!", 0

%endif ; PM32_ASM ;
