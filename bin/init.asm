%define ERR 0xffffffff
%define BUF_SIZE 32
	
	[global _start]

section .text
_start:
	mov eax, 2 ; open ;
	mov ebx, fname ; path ;
	mov ecx, 0x3 ; flags: FS_READ | FS_WRITE ;
	mov edx, 0 ; mask ;
	int 0x80
	
	cmp eax, ERR
	je .end
	
	mov dword[ttyfd], eax
	
	; write message ;
	mov eax, 4 ; write ;
	mov ebx, dword[ttyfd]
	mov ecx, msg
	mov edx, msg_len
	int 0x80
	
	cmp eax, ERR
	je .end
	
	; read input ;
	mov eax, 3 ; read ;
	mov ebx, dword[ttyfd]
	mov ecx, buf
	mov edx, BUF_SIZE
	int 0x80
	
	cmp eax, ERR
	je .end
	
	mov edx, eax
	dec edx
	mov eax, 4 ; write ;
	int 0x80
	
	cmp eax, ERR
	je .end
	
	; close tty file ;
	mov eax, 6 ; close ;
	mov ebx, dword[ttyfd]
	int 0x80
	
	cmp eax, ERR
	je .end
.end:
	mov eax, 1 ; exit ;
	int 0x80
	
	hlt

; program data ;
section .data
	fname db "/dev/tty0", 0
	msg db "Hello world from /bin/init!", 10
	msg_len equ $-msg

section .bss
	ttyfd resd 1
	buf resb BUF_SIZE
