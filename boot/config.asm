%ifndef CONFIG_ASM
%define CONFIG_ASM

%define CONFIG_STRING_SZ 32
%define CONFIG_KERNEL_BASE 0x1000

; config data ;
struc config_data
	.count: resw 1
	.timeout: resw 1
endstruc

; config entry data ;
struc config_entry
	.name: resb CONFIG_STRING_SZ
	.kernel: resb CONFIG_STRING_SZ
endstruc

; copy ;
; ax = index of block ;
config_copy:
.ext2:
	pushad
	
	mov ax, word[ext2_block_size]
	call allocate
	
	mov di, si
	mov si, word[ext2_block_area]
	call memcpy
	
	popad
	ret

; load config file ;
config_load:
	cmp word[mbr_part_fs], MBR_TYPE_LINUX
	je .ext2
.ext2:
	pusha
	mov si, config_msg
	call print
	
	mov bx, word[memory_breakp]
	mov word[config_area], bx
	
	mov si, boot_path
	mov di, config_path
	mov bx, config_copy.ext2
	call ext2_load_file
	
	mov si, word[ext2_inode_area]
	mov si, word[si+ext2_inode_s.szlow]
	add si, word[config_area]
	mov byte[si], 0
	
	popa
	ret

; parse config file ;
config_parse:
	pusha
	mov ax, config_data_size
	call allocate
	mov word[config_data_area], si
	mov word[si+config_data.count], 0
	
	mov di, si
	mov si, word[config_area]
.loop:
	lodsb
	cmp al, 0
	je .end
	
	; whitespace ;
	cmp al, ' '
	je .loop
	
	cmp al, 9
	je .loop
	
	cmp al, 10
	je .loop
	
	; comment ;
	cmp al, '#'
	je .comment
	
	; menu entry ;
	cmp al, '/'
	je .entry
	
	; parameter ;
	jmp .param
.comment:
	mov ah, 10
	call strchrnl
	cmp si, 0
	je .end
	
	jmp .loop
.entry:
	push si
	mov ax, config_entry_size
	call allocate
	cmp word[config_entry_area], 0
	jne .entry_next
	
	mov word[config_entry_area], si
.entry_next:
	mov di, si
	pop si
	
	push di
	add di, config_entry.name
	mov ax, CONFIG_STRING_SZ
	call strcpynl
	mov di, word[config_data_area]
	inc word[di+config_data.count]
	pop di
	
	mov ah, 10
	call strchrnl
	cmp si, 0
	je .end
	
	jmp .loop
.param: ; main menu parameter ;
	push di
	mov di, si
	dec di ; because lodsb incremented di before ;
	
	mov ah, '='
	call strchrnl
	cmp si, 0
	je .expchar_error
	
	mov ax, si
	sub ax, di
	dec ax
	
	push si
	
	mov si, config_timeout_param
	mov bx, ax
	push ax
	call strlen
	cmp ax, bx
	pop ax
	jne .param_next1
	
	call memcmp
	cmp ax, 0
	je .param_timeout
.param_next1:
	mov si, config_kernel_param
	mov bx, ax
	push ax
	call strlen
	cmp ax, bx
	pop ax
	jne .param_error
	
	call memcmp
	cmp ax, 0
	je .param_kernel
	
	jmp .param_error
.param_timeout:
	push di
	mov di, word[config_entry_area]
	cmp di, 0
	pop di
	jne .unexp_param_error
	
	pop si
	pop di
	
	mov ah, 10
	call atoich
	
	mov word[di+config_data.timeout], ax
	
	cmp si, 0
	je .end
	
	jmp .loop
.param_kernel:
	push di
	mov di, word[config_entry_area]
	cmp di, 0
	pop di
	je .unexp_param_error
	
	pop si
	pop di
	
	push di
	add di, config_entry.kernel
	mov ax, CONFIG_STRING_SZ
	call strcpynl
	pop di
	
	mov ah, 10
	call strchrnl
	cmp si, 0
	je .end
	
	jmp .loop
.char_error:
	mov si, config_char_error_msg1
	call print
	mov ah, 0x0e
	int 0x10
	mov si, config_error_msg2
	call print_error
.expchar_error:
	mov si, config_expchar_error_msg1
	call print
	push ax
	mov al, ah
	mov ah, 0x0e
	int 0x10
	pop ax
	mov si, config_expchar_error_msg2
	call print
	mov ah, 0x0e
	int 0x10
	mov si, config_error_msg2
	call print_error
.param_error:
	mov si, config_param_error_msg1
	call print
	mov si, di
	call printnl
	mov si, config_error_msg2
	call print_error
.unexp_param_error:
	mov si, config_unexp_param_error_msg1
	call print
	mov si, di
	call printnl
	mov si, config_error_msg2
	call print_error
.end:
	popa
	ret

; copy kernel blocks ;
; ax = index ;
config_copy_kernel:
.ext2:
	pusha
	
	mov si, word[ext2_block_area]
	
	push bx
	mov bx, 0
	mov es, bx
	mov bx, word[config_kernel_segment]
	mov gs, bx
	pop bx
	
	mov di, word[config_kernel_size]
	
	push ax
	mov ax, word[ext2_block_size]
	call memcpy
	pop ax
	
	mov bx, 0
	mov gs, bx
	
	add di, word[ext2_block_size]
	mov word[config_kernel_size], di
	cmp di, 0
	jne .end
	
	add word[config_kernel_segment], 0x1000
.end:
	popa
	ret

; load kernel ;
; ax = index of entry ;
config_load_kernel:
	
	cmp word[mbr_part_fs], MBR_TYPE_LINUX
	je .ext2
.ext2:
	pusha
	mov word[config_kernel_segment], CONFIG_KERNEL_BASE
	mov word[config_kernel_size], 0
	
	mov si, boot_path
	
	mov di, word[config_entry_area]
	mov bx, config_entry_size
	mul bx
	add di, ax
	add di, config_entry.kernel
	mov bx, config_copy_kernel.ext2
	call ext2_load_file
	
	call elf16_validate_kernel
	
	popa
	ret

; data ;
config_area dw 0
config_data_area dw 0
config_entry_area dw 0
config_kernel_segment dw 0
config_kernel_size dw 0

boot_path db "boot", 0
config_path db "menu.cfg", 0
config_msg db "Loading config file...", 10, 0
config_error_msg2 db "'", 10, 0
config_char_error_msg1 db "Config: Unrecognized character '", 0
config_expchar_error_msg1 db "Config: Expected '", 0
config_expchar_error_msg2 db "' but got '", 0
config_param_error_msg1 db "Config: Unrecognized parameter '", 0
config_unexp_param_error_msg1 db "Config: Unexpected parameter '", 0

; config parameters ;
config_timeout_param db "timeout", 0
config_kernel_param db "kernel", 0

%endif ; CONFIG_ASM ;
