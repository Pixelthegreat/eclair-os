%ifndef MENU_ASM
%define MENU_ASM

%define VGA_WIDTH 80
%define VGA_HEIGHT 25

%define MENU_HEIGHT 21

; clear ;
menu_clear:
	pusha
	
	mov ax, 0
	mov bx, 0
	mov cx, 0
	mov dx, 0
	
	; set cursor ;
	mov ah, 0x02
	mov dh, 0
	mov dl, 0
	int 0x10
	
	; clear ;
	mov ah, 0x09
	mov al, ' '
	mov bl, 0x7
	mov cx, VGA_WIDTH*VGA_HEIGHT
	int 0x10
	
	popa
	ret

; disable cursor ;
menu_disable_cursor:
	pusha
	
	mov ah, 0x03
	mov bh, 0
	int 0x10
	mov word[menu_cursor_shape], cx
	
	mov ah, 0x01
	mov ch, 0x26
	mov cl, 0x07
	int 0x10
	
	popa
	ret

; enable cursor ;
menu_enable_cursor:
	pusha
	
	mov ah, 0x01
	mov cx, word[menu_cursor_shape]
	int 0x10
	
	popa
	ret

; print integer ;
; ax = integer ;
%define MENU_PRINT_INT_BUFSZ 16

menu_print_int:
	pusha
	
	mov bx, 0
	sub sp, MENU_PRINT_INT_BUFSZ
	
	cmp ax, 0
	je .zero
.loop1:
	cmp bx, MENU_PRINT_INT_BUFSZ
	je .loop2
	
	mov dx, 0
	mov cx, 10
	div cx
	
	add dl, '0'
	mov si, sp
	add si, bx
	mov byte[si], dl
	
	inc bx
	cmp ax, 0
	jne .loop1
.loop2:
	cmp bx, 0
	je .done
	dec bx
	
	mov ah, 0x0e
	mov si, sp
	add si, bx
	mov al, byte[si]
	
	push bx
	mov bh, 0
	mov bl, 0x7
	int 0x10
	pop bx
	
	jmp .loop2
.zero:
	mov ah, 0x0e
	mov al, '0'
	mov bh, 0
	mov bl, 0x7
	int 0x10
.done:
	add sp, MENU_PRINT_INT_BUFSZ
	popa
	ret

; main menu item ;
; ax = index ;
menu_main_item:
	pusha
	
	mov si, word[config_entry_area]
	mov bx, config_entry_size
	mul bx
	add si, ax
	
	push si
	add si, config_entry.name
	call print
	pop si
	
	popa
	ret

; display list ;
; ax = number of items ;
; bx = first item to draw ;
; si = iterator subroutine ;
; ax (return) = number of items drawn ;
menu_print_list:
	push bx
	push cx
	call menu_clear
	call menu_disable_cursor
	
	push ax
	mov al, 10
	call printc
	pop ax
	
	mov cx, bx
	add cx, MENU_HEIGHT
	
	push cx
	mov cx, ax
	mov ax, bx
	mov bx, cx
	pop cx
.loop:
	cmp ax, cx
	je .done
	cmp ax, bx
	je .done
	
	push ax
	mov al, ' '
	call printc
	call printc
	call printc
	call printc
	pop ax
	
	push ax
	inc ax
	call menu_print_int
	mov al, '.'
	call printc
	mov al, ' '
	call printc
	pop ax
	
	cmp si, 0
	je .next
	call si
.next:
	push ax
	mov al, 10
	call printc
	pop ax
	
	inc ax
	jmp .loop
.done:
	pop cx
	pop bx
	ret

; display list and wait for selection ;
; ax = number of items ;
; si = iterator subroutine ;
; ax (return) = selection index ;
menu_list:
	pusha
	
	mov bx, 0
	mov cx, 0
.redraw:
	push ax
	push bx
	cmp bx, MENU_HEIGHT-1
	jl .redraw_nobx
	
	sub bx, MENU_HEIGHT-1
	jmp .redraw_print
.redraw_nobx:
	mov bx, 0
.redraw_print:
	call menu_print_list
	pop bx
	pop ax
.recursor:
	push ax
	push cx
	push bx
	
	; clamp cx and bx to menu height ;
	cmp cx, MENU_HEIGHT-1
	jl .recursor_nosetcx
	
	mov cx, MENU_HEIGHT-1
.recursor_nosetcx:
	cmp bx, MENU_HEIGHT-1
	jl .recursor_nosetbx
	
	mov bx, MENU_HEIGHT-1
.recursor_nosetbx:
	push bx
	
	mov ah, 0x02
	mov bh, 0
	mov dl, 2
	mov dh, cl
	inc dh
	mov cx, 1
	int 0x10
	
	mov ah, 0x09
	mov al, ' '
	mov bl, 0x7
	int 0x10
	
	mov ah, 0x02
	pop bx
	mov dh, bl
	inc dh
	push bx
	mov bh, 0
	mov bl, 0x7
	int 0x10
	
	mov ah, 0x09
	mov al, 0x04
	int 0x10
	
	pop bx
	pop bx
	pop cx
	pop ax
.loop:
	push ax
	mov ah, 0x00
	int 0x16
	
	; up ;
	cmp ah, 0x48
	je .up
	cmp al, 'w'
	je .up
	
	; down ;
	cmp ah, 0x50
	je .down
	cmp al, 's'
	je .down
	
	; enter ;
	cmp al, 10
	je .end
	cmp al, 13
	je .end
	
	pop ax
	jmp .loop
.up:
	pop ax
	mov cx, bx
	dec bx
	
	push ax
	mov dx, ax
	mov ax, bx
	mov bx, dx
	mov dx, 0
	div bx
	mov bx, dx
	pop ax
	
	cmp bx, MENU_HEIGHT-1
	jge .redraw
	
	jmp .recursor
.down:
	pop ax
	mov cx, bx
	inc bx
	
	push ax
	mov dx, ax
	mov ax, bx
	mov bx, dx
	mov dx, 0
	div bx
	mov bx, dx
	pop ax
	
	cmp cx, MENU_HEIGHT-1
	jge .redraw
	
	jmp .recursor
.end:
	pop ax
.done:
	mov word[menu_option], bx
	
	popa
	mov ax, word[menu_option]
	ret

; show menu ;
menu_display:
	pusha
	
	mov si, word[config_data_area]
	mov ax, word[si+config_data.count]
	mov si, menu_main_item
	call menu_list
	
	popa
	ret

; load menu option ;
menu_load:
	pusha
	call menu_enable_cursor
	
	mov ah, 0x02
	mov bh, 0
	mov dh, MENU_HEIGHT+2
	mov dl, 2
	int 0x10
	
	mov si, menu_boot_msg1
	call print
	
	mov si, word[config_entry_area]
	mov ax, word[menu_option]
	mov bx, config_entry_size
	mul bx
	add si, ax
	
	push si
	add si, config_entry.name
	call print
	
	mov si, menu_boot_msg2
	call print
	pop si
	
	mov ax, word[menu_option]
	call config_load_kernel
	
	popa
	ret

; data ;
menu_option dw 0
menu_cursor_shape dw 0

menu_boot_msg1 db "Booting ", 0
menu_boot_msg2 db "...", 0

%endif ; MENU_ASM ;
