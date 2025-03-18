/*
 * == Eclair OS system call declarations and documentation ==
 *
 * To use this header as a library, add this line of code:
 *   #define EC_IMPL
 * To JUST ONE source code file, before the #include statement.
 */
#ifndef EC_H
#define EC_H

#include <stddef.h>
#include <stdint.h>

typedef long ec_ssize_t;
typedef long ec_off_t;
typedef int ec_mode_t;

#define ECN_NULL 0

#define ECN_EXIT 1
#define ECN_OPEN 2
#define ECN_READ 3
#define ECN_WRITE 4
#define ECN_LSEEK 5
#define ECN_CLOSE 6
#define ECN_STAT 7
#define ECN_FSTAT 8
#define ECN_GETPID 9
#define ECN_KILL 10
#define ECN_SBRK 11
#define ECN_TIMES 12
#define ECN_GETTIMEOFDAY 13
#define ECN_ISATTY 14

#define ECN_COUNT 15

/* generic system call wrappers */
extern uint32_t ec_syscall3(uint32_t i, uint32_t a, uint32_t b, uint32_t c);
extern uint64_t ec_syscall3r2(uint32_t i, uint32_t a, uint32_t b, uint32_t c);

#ifdef EC_IMPL

/*
 * System call with 3 arguments and a uint32_t return value:
 *   eax = i, ebx = a, ecx = b, edx = c
 *   ret = eax
 */
extern uint32_t ec_syscall3(uint32_t i, uint32_t a, uint32_t b, uint32_t c) {

	uint32_t ret = 0;
	asm volatile(
		"mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n"
		"mov %%eax, %4\n"
		: : "m"(i), "m"(a), "m"(b), "m"(c), "m"(ret)
		);
	return ret;
}

/*
 * System call with 3 arguments and a uint64_t return value:
 *   eax = i, ebx = a, ecx = b, edx = c
 *   ret = (ebx << 32) | eax
 */
extern uint64_t ec_syscall3r2(uint32_t i, uint32_t a, uint32_t b, uint32_t c) {

	uint32_t reta = 0, retb = 0;
	asm volatile(
		"mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n"
		"mov %%eax, %4\n"
		"mov %%ebx, %5\n"
		: : "m"(i), "m"(a), "m"(b), "m"(c), "m"(reta), "m"(retb)
		);
	return ((uint64_t)retb << 32) | (uint64_t)reta;
}

#endif /* EC_IMPL */

/*
 * Exit the current task/process. No return value or arguments.
 */
static inline void ec_exit(void) {

	(void)ec_syscall3(ECN_EXIT, 0, 0, 0);
}

/*
 * Open a file.
 *   ebx/path = Path to file
 *   ecx/flags = Flags (fcntl)
 *   edx/mode = File mode (with O_CREAT only)
 *   eax (return) = File descriptor if successful, negative on error
 */
static inline int ec_open(const char *path, int flags, ec_mode_t mode) {

	return (int)ec_syscall3(ECN_OPEN, (uint32_t)path, (uint32_t)flags, (uint32_t)mode);
}

/*
 * Read from a file.
 *   ebx/fd = File descriptor
 *   ecx/buf = Buffer to fill
 *   edx/cnt = Number of bytes to read
 *   eax (return) = Number of bytes read if successful, negative on error
 */
static inline ec_ssize_t ec_read(int fd, const void *buf, size_t cnt) {

	return (ec_ssize_t)ec_syscall3(ECN_READ, (uint32_t)fd, (uint32_t)buf, (uint32_t)cnt);
}

/*
 * Write to a file.
 *   ebx/fd = File descriptor
 *   ecx/buf = Buffer to copy
 *   edx/cnt = Number of bytes to write
 *   eax (return) = Number of bytes written if successful, negative on error
 */
static inline ec_ssize_t ec_write(int fd, const void *buf, size_t cnt) {

	return (ec_ssize_t)ec_syscall3(ECN_WRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)cnt);
}

/*
 * Go to a position in a file.
 *   ebx/fd = File descriptor
 *   ecx/pos = Position to seek
 *   edx/whence = The relative start position (SEEK_SET, SEEK_CUR, SEEK_END)
 *   eax (return) = The position seeked relative to the beginning of the file
 */
static inline ec_off_t ec_lseek(int fd, ec_off_t pos, int whence) {

	return (ec_off_t)ec_syscall3(ECN_LSEEK, (uint32_t)fd, (uint32_t)pos, (uint32_t)whence);
}

/*
 * Close a file.
 *   ebx/fd = File descriptor
 *   eax (return) = Zero if successful, negative on error
 */
static inline int ec_close(int fd) {

	return (int)ec_syscall3(ECN_CLOSE, (uint32_t)fd, 0, 0);
}

#endif /* EC_H */
