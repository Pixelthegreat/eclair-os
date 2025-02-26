%ifndef MEMORY_ASM
%define MEMORY_ASM

%define MEMORY_MAX 0xffff

memory_breakp dw stage1_end

; allocate memory ;
; ax = size of allocation ;
; si (return) = address ;
allocate:
	push di
	mov di, word[memory_breakp]
	mov si, di
	
	add di, ax
	cmp di, MEMORY_MAX
	jg .fail
	
	mov word[memory_breakp], di
	pop di
	ret
.fail:
	mov si, memory_error_msg
	call print_error

; copy memory ;
; ax = count ;
; si = source ;
; di = destination ;
; es = source segment ;
; gs = destination segment ;
memcpy:
	push ax
	push si
	push di
.loop:
	cmp ax, 0
	je .done
	dec ax
	
	push ax
	mov ah, byte[es:si]
	mov byte[gs:di], ah
	pop ax
	
	inc si
	inc di
	jmp .loop
.done:
	pop di
	pop si
	pop ax
	ret

; initialize memory ;
; ax = count ;
; si = buffer ;
; bh = byte ;
memset:
	push ax
	push si
.loop:
	cmp ax, 0
	je .done
	dec ax
	
	mov byte[si], bh
	
	inc si
	inc di
	jmp .loop
.done:
	pop si
	pop ax
	ret

; compare memory ;
; ax = count ;
; si = first ;
; di = second ;
; ax (return) = difference ;
memcmp:
	push si
	push di
	push bx
.loop:
	cmp ax, 0
	je .done
	dec ax
	
	mov bl, byte[si]
	mov bh, byte[di]
	
	cmp bl, bh
	jne .diff
	
	inc si
	inc di
	jmp .loop
.diff:
	mov al, byte[si]
	mov ah, byte[di]
	sub al, ah
	mov ah, 0
.done:
	pop bx
	pop di
	pop si
	ret

; get length of string ;
; si = string ;
; ax (return) = length ;
strlen:
	push si
	mov ax, 0
.loop:
	push ax
	mov ah, byte[si]
	cmp ah, 0
	pop ax
	je .done
	
	inc ax
	inc si
	jmp .loop
.done:
	pop si
	ret

; compare strings up to a certain amount ;
; ax = count ;
; si = first ;
; di = second ;
; ax (return) = difference ;
strncmp:
	push si
	push di
	push bx
.loop:
	cmp ax, 0
	je .done
	dec ax
	
	mov bl, byte[si]
	mov bh, byte[di]
	
	cmp bl, 0
	je .done
	cmp bh, 0
	je .done
	cmp bl, bh
	jne .done
	
	inc si
	inc di
	jmp .loop
.done:
	mov al, byte[si]
	mov ah, byte[di]
	sub al, ah
	mov ah, 0
	
	pop bx
	pop di
	pop si
	ret

; locate character in string ;
; si = string ;
; ah = character ;
; si (return) = string at character position ;
strchrnl:
	push ax
.loop:
	lodsb
	cmp al, 0
	je .none
	cmp al, ah
	je .done
	cmp al, 10
	je .none
	jmp .loop
.none:
	mov si, 0
.done:
	pop ax
	ret

; convert string to integer until character ;
; si = string ;
; ah = stop character ;
; ax (return) = integer ;
atoich:
	push bx
	mov bx, 0
.loop:
	lodsb
	cmp al, 0
	je .none
	cmp al, ah
	je .done
	cmp al, 10
	je .done
	
	sub al, '0'
	push ax
	mov ax, bx
	mov bx, 10
	mul bx
	mov bx, ax
	pop ax
	
	push ax
	mov ah, 0
	add bx, ax
	pop ax
	
	jmp .loop
.none:
	mov si, 0
.done:
	mov ax, bx
	pop bx
	ret

; copy string to newline ;
; ax = count ;
; si = source ;
; di = destination ;
strcpynl:
	push si
	push di
	push ax
	dec ax
.loop:
	cmp ax, 0
	je .done
	dec ax
	
	push ax
	mov al, byte[si]
	cmp al, 0
	je .done
	cmp al, 10
	je .done
	
	mov byte[di], al
	pop ax
	
	inc si
	inc di
	jmp .loop
.done:
	pop ax
	pop ax
	push ax
	mov byte[di], 0
	pop ax
	
	pop di
	pop si
	ret

memory_error_msg db "Failed to allocate enough memory!", 10, 0

%endif ; MEMORY_ASM ;
