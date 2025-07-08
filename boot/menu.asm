;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef MENU_ASM
%define MENU_ASM

%define VGA_WIDTH 80
%define VGA_HEIGHT 25

%define MENU_HEIGHT 23

%define MENU_VIDEO_DEFAULT 0

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

; display list ;
; ax = number of items ;
; bx = first item to draw ;
; si = iterator subroutine ;
; ax (return) = number of items drawn ;
menu_print_list:
	push bx
	push cx
	call menu_clear
	
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
	
	cmp bx, 0
	je .up_wrap
	
	dec bx
	jmp .up_next
.up_wrap:
	mov bx, ax
	dec bx
.up_next:
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

; initialize menu ;
menu_init:
	pusha
	
	call menu_disable_cursor
	
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

; show menu ;
menu_display:
	pusha
.loop:
	mov si, word[config_data_area]
	mov ax, word[si+config_data.count]
	mov si, menu_main_item
	call menu_list
	
	mov si, word[config_entry_area]
	mov ax, word[menu_option]
	mov bx, config_entry_size
	mul bx
	add si, ax
	
	cmp byte[si+config_entry.type], 1
	jne .done
	
	call menu_mode_display
	mov ax, word[menu_option]
	mov word[menu_video_mode], ax
	jmp .loop
.done:
	popa
	ret

; main ;
menu_main:
	pusha
	
	mov si, word[config_data_area]
	mov ax, word[si+config_data.timeout]
	cmp ax, 0
	je .menu
	
	call menu_clear
	push ax
	mov al, 10
	call printc
	mov al, ' '
	call printc
	call printc
	pop ax
	
	mov si, menu_wait_msg
	call print
	
	mov bx, 10
	mul bx
	
	; tenth of a second ;
	mov cx, 0x1
	mov dx, 0x86a0
.loop:
	cmp ax, 0
	je .end
	dec ax
	
	push ax
	mov ah, 0x86
	int 0x15
	
	mov ah, 0x01
	int 0x16
	pop ax
	
	jnz .menu
.next:
	push ax
	push dx
	mov dx, 0
	mov bx, 10
	div bx
	mov bx, dx
	pop dx
	pop ax
	
	cmp bx, 0
	jne .loop
	
	push ax
	mov al, '.'
	call printc
	pop ax
	
	jmp .loop
.menu:
	mov ah, 0x00
	int 0x16
	
	call menu_display
.end:
	popa
	ret

; print resolution info ;
; si = resolution info ;
menu_print_resolution:
	pusha
	
	mov ax, word[si+vbe_res.width]
	call menu_print_int
	mov al, 'x'
	call printc
	mov ax, word[si+vbe_res.height]
	call menu_print_int
	
	call vbe_get_res_highest_bpp
	cmp bx, 0xffff
	je .end
	
	cmp ax, 8
	je .color8
	
	cmp ax, 16
	je .color16
	
	jmp .color24
.color8:
	mov si, menu_color8_msg
	call print
	
	jmp .end
.color16:
	mov si, menu_color16_msg
	call print
	
	jmp .end
.color24:
	mov si, menu_color24_msg
	call print
	
	jmp .end
.end:
	popa
	ret

; mode menu item ;
; ax = index ;
menu_mode_item:
	pusha
	
	cmp ax, 1
	jge .item
	
	mov di, word[vbe_res_area]
	mov ax, word[vbe_pref_res]
	mov bx, vbe_res_size
	mul bx
	add di, ax
	
	mov si, menu_default_msg1
	call print
	mov si, di
	call menu_print_resolution
	mov si, menu_default_msg2
	call print
	
	jmp .end
.item:
	sub ax, 1
	
	mov si, word[vbe_res_area]
	mov bx, vbe_res_size
	mul bx
	add si, ax
	
	call menu_print_resolution
.end:
	popa
	ret

; show video mode menu ;
menu_mode_display:
	pusha
	
	mov ax, word[vbe_res_count]
	add ax, 1
	mov si, menu_mode_item
	call menu_list
	
	popa
	ret

; load menu option ;
menu_load:
	pusha
	call menu_clear
	call menu_enable_cursor
	
	mov al, 10
	call printc
	mov al, ' '
	call printc
	call printc
	
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
	
	push si
	add si, config_entry.cmdline
	mov word[menu_cmdline], si
	pop si
	
	mov ax, word[menu_option]
	call config_load_kernel
	call config_load_initrd
	
	mov ax, word[menu_video_mode]
	cmp ax, 0
	je .next1
	
	dec ax
	jmp .next2
.next1:
	mov ax, word[vbe_pref_res]
.next2:
	call vbe_set_mode
	
	popa
	ret

; data ;
menu_option dw 0
menu_cursor_shape dw 0
menu_cmdline dw 0
menu_video_mode dw MENU_VIDEO_DEFAULT

menu_wait_msg db "Press any key to open menu.", 0
menu_default_msg1 db "Default (", 0
menu_default_msg2 db ")", 0
menu_color8_msg db ", 256 colors", 0
menu_color16_msg db ", 64k colors", 0
menu_color24_msg db ", 16m colors", 0
menu_boot_msg1 db "Booting ", 0
menu_boot_msg2 db "...", 0

%endif ; MENU_ASM ;
