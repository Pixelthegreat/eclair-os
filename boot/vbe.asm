;
; Copyright 2025, Elliot Kohlmyer
;
; SPDX-License-Identifier: BSD-3-Clause
;
%ifndef VBE_ASM
%define VBE_ASM

%define VBE_EDID_SIZE 128

%define VBE_OK 0x004f

; main vbe info ;
struc vbe_info
	.signature resb 4
	.version resw 1
	.oem_name resd 1
	.capabilities resd 1
	.modes_offset resw 1
	.modes_segment resw 1
	.size_blocks resw 1
	.oem_sw_rev resw 1
	.oem_vendor_name resd 1
	.oem_product_name resd 1
	.oem_product_rev resd 1
	.reserved resb 222
	.oem_data resb 256
endstruc

; vbe mode info ;
struc vbe_mode_info
	.attributes resw 1
	.window_a resb 1
	.window_b resb 1
	.granularity resw 1
	.size resw 1
	.segment_a resw 1
	.segment_b resw 1
	.win_func_ptr resd 1
	.bytes_per_scanline resw 1
	.width resw 1
	.height resw 1
	.char_width resb 1
	.char_height resb 1
	.planes resb 1
	.bits_per_pixel resb 1
	.banks resb 1
	.mem_model resb 1
	.bank_size resb 1
	.image_pages resb 1
	.rsvd1 resb 1
	.red_mask_sz resb 1
	.red_field_pos resb 1
	.green_mask_sz resb 1
	.green_field_pos resb 1
	.blue_mask_sz resb 1
	.blue_field_pos resb 1
	.rsvd_mask_sz resb 1
	.rsvd_mask_pos resb 1
	.dc_mode_info resb 1
	.lfb_addr resd 1
	.offscreen_mem_offset resd 1
	.offscreen_mem_size resw 1
	.rsvd2 resb 206
endstruc

; saved mode info ;
struc vbe_res
	.width resw 1
	.height resw 1
	.pk8 resw 1
	.pk16 resw 1
	.pk24 resw 1
	.pk32 resw 1
	.dc16 resw 1
	.dc24 resw 1
	.dc32 resw 1
endstruc

; video mode attributes ;
%define VBE_MODE_ATTRIB_SUPPORT 0x1
%define VBE_MODE_ATTRIB_LFB 0x80

%define VBE_MODE_LFB 0x4000

; memory models ;
%define VBE_MEM_MODEL_PACKED 0x04
%define VBE_MEM_MODEL_DC 0x06
%define VBE_MEM_MODEL_YUV 0x07

; find entry for resolution ;
; ax = width ;
; bx = height ;
; si (return) = entry ;
vbe_find_res:
	push cx
	
	mov si, word[vbe_res_area]
	cmp si, 0
	je .new
	
	mov cx, 0
.loop:
	cmp cx, word[vbe_res_count]
	je .new
	
	cmp word[si+vbe_res.width], ax
	jne .next
	cmp word[si+vbe_res.height], bx
	jne .next
	
	jmp .end
.next:
	inc cx
	add si, vbe_res_size
	jmp .loop
.new:
	push ax
	mov ax, vbe_res_size
	call allocate
	pop ax
	
	mov word[si+vbe_res.width], ax
	mov word[si+vbe_res.height], bx
	mov word[si+vbe_res.pk8], 0xffff
	mov word[si+vbe_res.pk16], 0xffff
	mov word[si+vbe_res.pk24], 0xffff
	mov word[si+vbe_res.pk32], 0xffff
	mov word[si+vbe_res.dc16], 0xffff
	mov word[si+vbe_res.dc24], 0xffff
	mov word[si+vbe_res.dc32], 0xffff
	
	inc word[vbe_res_count]
	
	cmp word[vbe_res_area], 0
	jne .end
	
	mov word[vbe_res_area], si
.end:
	pop cx
	ret

; get highest bpp for mode ;
; si = resolution info ;
; ax (return) = bpp ;
; bx (return) = vbe mode ;
vbe_get_res_highest_bpp:
	mov ax, 0
	
	mov ax, 32
	mov bx, word[si+vbe_res.dc32]
	cmp bx, 0xffff
	jne .end
	mov bx, word[si+vbe_res.pk32]
	cmp bx, 0xffff
	jne .end
	
	mov ax, 24
	mov bx, word[si+vbe_res.dc24]
	cmp bx, 0xffff
	jne .end
	mov bx, word[si+vbe_res.pk24]
	cmp bx, 0xffff
	jne .end
	
	mov ax, 16
	mov bx, word[si+vbe_res.dc16]
	cmp bx, 0xffff
	jne .end
	mov bx, word[si+vbe_res.pk16]
	cmp bx, 0xffff
	jne .end
	
	mov ax, 8
	mov bx, word[si+vbe_res.pk8]
	cmp bx, 0xffff
	jne .end
	
	mov ax, 0
.end:
	ret

; load vbe mode information ;
; ax = mode number ;
vbe_load_mode:
	pusha
	
	mov si, word[vbe_mode_info_area]
	
	push si
	mov cx, ax
	mov ax, 0x4f01
	mov di, si
	
	push cx
	int 0x10
	pop cx ; video mode number now stored here ;
	
	cmp ax, VBE_OK
	pop si
	jne .end
	
	push ax
	mov ax, word[si+vbe_mode_info.mem_model]
	cmp ax, VBE_MEM_MODEL_DC
	pop ax
	je .next1
	push ax
	mov ax, word[si+vbe_mode_info.mem_model]
	cmp ax, VBE_MEM_MODEL_PACKED
	pop ax
	je .next1
	
	jmp .end
.next1:
	mov ax, word[si+vbe_mode_info.attributes]
	
	push ax
	and ax, VBE_MODE_ATTRIB_LFB
	cmp ax, 0
	pop ax
	je .end
.next2:
	push ax
	and ax, VBE_MODE_ATTRIB_SUPPORT
	cmp ax, 0
	pop ax
	je .end
	
	mov ax, word[si+vbe_mode_info.width]
	mov bx, word[si+vbe_mode_info.height]
	
	push si
	call vbe_find_res
	mov di, si
	pop si
	
	mov bx, word[si+vbe_mode_info.mem_model]
	cmp bx, VBE_MEM_MODEL_PACKED
	je .packed
	
	mov bl, byte[si+vbe_mode_info.bits_per_pixel]
	cmp bl, 16
	je .dc16
	cmp bl, 24
	je .dc24
	cmp bl, 32
	je .dc32
	
	jmp .end
.dc16:
	mov word[di+vbe_res.dc16], cx
	jmp .end
.dc24:
	mov word[di+vbe_res.dc24], cx
	jmp .end
.dc32:
	mov word[di+vbe_res.dc32], cx
	jmp .end
.packed:
	mov bl, byte[si+vbe_mode_info.bits_per_pixel]
	cmp bl, 8
	je .pk8
	cmp bl, 16
	je .pk16
	cmp bl, 24
	je .pk24
	cmp bl, 32
	je .pk32
	
	jmp .end
.pk8:
	mov word[di+vbe_res.pk8], cx
	jmp .end
.pk16:
	mov word[di+vbe_res.pk16], cx
	jmp .end
.pk24:
	mov word[di+vbe_res.pk24], cx
	jmp .end
.pk32:
	mov word[di+vbe_res.pk32], cx
	jmp .end
.end:
	popa
	ret

; locate preferred resolution / mode ;
vbe_load_pref_res:
	pushad
	
	mov si, word[vbe_res_area]
	
	mov eax, 0
	mov ebx, 0
	mov ecx, 0
	mov edx, 0
	cmp word[vbe_no_edid], 1
	je .loop_highest
	
	push si
	mov si, word[vbe_edid_area]
	
	mov al, byte[si+0x3a]
	and ax, 0xf0
	shl ax, 4
	mov al, byte[si+0x38]
	
	mov bl, byte[si+0x3d]
	and bx, 0xf0
	shl bx, 4
	mov bl, byte[si+0x3b]
	pop si
.loop_closest:
	cmp cx, word[vbe_res_count]
	je .end_closest
	
	cmp word[si+vbe_res.width], ax
	jne .next_closest
	
	cmp word[si+vbe_res.height], bx
	jne .next_closest
	
	jmp .end_closest
.next_closest:
	inc cx
	add si, vbe_res_size
	jmp .loop_closest
.loop_highest:
	cmp cx, word[vbe_res_count]
	je .end_highest
	
	mov eax, 0
	mov ax, word[si+vbe_res.width]
	push ebx
	mov ebx, 0
	mov bx, word[si+vbe_res.height]
	mul ebx
	pop ebx
	
	cmp eax, ebx
	jle .next_highest
	
	mov ebx, eax
	mov dx, cx
.next_highest:
	inc cx
	add si, vbe_res_size
	jmp .loop_highest
.end_closest:
	cmp cx, word[vbe_res_count]
	je .go_highest
	
	jmp .end
.go_highest:
	mov cx, 0
	jmp .loop_highest
.end_highest:
	mov cx, dx
.end:
	mov word[vbe_pref_res], cx
	
	popad
	ret

; load vbe information ;
vbe_load:
	pusha
	
	mov ax, VBE_EDID_SIZE
	call allocate
	mov word[vbe_edid_area], si
	
	mov ax, 0x4f15
	mov bl, 0x01
	mov di, si
	mov cx, 0
	mov dx, 0
	int 0x10
	cmp ax, VBE_OK
	je .next1
	
	mov byte[vbe_no_edid], 1
.next1:
	mov ax, vbe_mode_info_size
	call allocate
	mov word[vbe_mode_info_area], si
	
	mov ax, vbe_info_size
	call allocate
	mov word[vbe_info_area], si
	
	mov byte[si], 'V'
	mov byte[si+1], 'B'
	mov byte[si+2], 'E'
	mov byte[si+3], '2'
	
	mov ax, 0x4f00
	mov di, si
	int 0x10
	cmp ax, VBE_OK
	jne .error
	
	mov es, word[si+vbe_info.modes_segment]
	mov di, word[si+vbe_info.modes_offset]
.loop:
	mov ax, word[es:di]
	cmp ax, 0xffff
	je .done
	
	call vbe_load_mode
	
	add di, 2
	jmp .loop
.done:
	mov ax, 0
	mov es, ax
	
	call vbe_load_pref_res
	
	popa
	ret
.error:
	mov si, vbe_error_msg
	call print_error
.monitor_error:
	mov si, vbe_monitor_error_msg
	call print_error

; set vbe mode ;
; ax = resolution index ;
vbe_set_mode:
	pusha
	
	cmp ax, word[vbe_res_count]
	jge .end
	
	mov si, word[vbe_res_area]
	mov bx, vbe_res_size
	mul bx
	add si, ax
	call vbe_get_res_highest_bpp
	
	cmp bx, 0xffff
	je .end
	
	mov word[vbe_selected_mode.depth], ax
	
	mov ax, word[si+vbe_res.width]
	mov word[vbe_selected_mode.width], ax
	
	mov ax, word[si+vbe_res.height]
	mov word[vbe_selected_mode.height], ax
	
	mov si, word[vbe_mode_info_area]
	
	push si
	push bx
	mov ax, 0x4f01
	mov cx, bx
	mov di, si
	int 0x10
	cmp ax, VBE_OK
	pop bx
	pop si
	jne .error
	
	mov ah, byte[si+vbe_mode_info.red_mask_sz]
	mov byte[vbe_selected_mode.red_mask_sz], ah
	
	mov ah, byte[si+vbe_mode_info.red_field_pos]
	mov byte[vbe_selected_mode.red_field_pos], ah
	
	mov ah, byte[si+vbe_mode_info.green_mask_sz]
	mov byte[vbe_selected_mode.green_mask_sz], ah
	
	mov ah, byte[si+vbe_mode_info.green_field_pos]
	mov byte[vbe_selected_mode.green_field_pos], ah
	
	mov ah, byte[si+vbe_mode_info.blue_mask_sz]
	mov byte[vbe_selected_mode.blue_mask_sz], ah
	
	mov ah, byte[si+vbe_mode_info.blue_field_pos]
	mov byte[vbe_selected_mode.blue_field_pos], ah
	
	mov ax, word[si+vbe_mode_info.bytes_per_scanline]
	mov word[vbe_selected_mode.pitch], ax
	
	mov eax, dword[si+vbe_mode_info.lfb_addr]
	mov dword[vbe_selected_mode.addr], eax
	
	; set mode ;
	mov ax, 0x4f02
	or bx, VBE_MODE_LFB
	int 0x10
	cmp ax, VBE_OK
	jne .error
.end:
	popa
	ret
.error:
	mov si, vbe_mode_error_msg
	call print_error

; data ;
vbe_edid_area dw 0
vbe_info_area dw 0
vbe_mode_info_area dw 0
vbe_res_area dw 0
vbe_res_count dw 0

vbe_no_edid db 0 ; EDID was not supported ;
vbe_pref_res dw 0 ; preferred resolution mode ;

; selected mode info ;
vbe_selected_mode:
	.width dw 0
	.height dw 0
	.depth dw 0
	.pitch dw 0
	.addr dd 0
	.red_mask_sz db 1
	.red_field_pos db 1
	.green_mask_sz db 1
	.green_field_pos db 1
	.blue_mask_sz db 1
	.blue_field_pos db 1

vbe_error_msg db "Failed to retrieve video information!", 10, 0
vbe_monitor_error_msg db "Failed to retrieve monitor information!", 10, 0
vbe_mode_error_msg db "Failed to set video mode!", 10, 0

%endif ; VBE_ASM ;
