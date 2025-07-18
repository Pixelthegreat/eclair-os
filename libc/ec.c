/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ec.h>

static char pathbuf[EC_PATHSZ];
static char tempbuf[EC_PATHSZ];

/* fix path */
static const char *fixpath(const char *path) {

	if (!path) return NULL;

	size_t len = 0;
	if (path[0] != '/') {

		len = strlen(pathbuf);
		memcpy(tempbuf, pathbuf, len < EC_PATHSZ-2? len: EC_PATHSZ-2);
	}

	/* add each path component */
	const char *next = NULL;
	const char *opath = path;
	for (; path; path = next) {

		const char *end = strchr(path, '/');
		next = end? end+1: NULL;
		size_t plen = end? (size_t)end - (size_t)path: strlen(path);

		if (!plen) continue;

		/* go back one directory */
		if (!strncmp(path, "..", plen)) {

			tempbuf[len] = 0;
			char *prev = strrchr(tempbuf, '/');
			if (prev) len = (size_t)prev - (size_t)tempbuf;
		}

		/* add directory */
		else {
			if (len != 1) tempbuf[len++] = '/';

			size_t rem = EC_PATHSZ-len-2;
			plen = plen < rem? plen: rem;

			memcpy(tempbuf+len, path, plen);
			len += plen;
		}
	}
	if (!len) tempbuf[len++] = '/';
	tempbuf[len++] = 0;
	return tempbuf;
}

extern uint32_t ec_syscall3(uint32_t i, uint32_t a, uint32_t b, uint32_t c) {

	uint32_t ret = 0;
	asm volatile(
		"push %%ebx\n"
		"mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n"
		"pop %%ebx\n"
		"mov %%eax, %4\n"
		: : "m"(i), "m"(a), "m"(b), "m"(c), "m"(ret)
		);
	return ret;
}

extern uint64_t ec_syscall3r2(uint32_t i, uint32_t a, uint32_t b, uint32_t c) {

	uint32_t reta = 0, retb = 0;
	asm volatile(
		"push %%ebx\n"
		"mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n"
		"pop %%ebx\n"
		"mov %%eax, %4\n"
		"mov %%ecx, %5\n"
		: : "m"(i), "m"(a), "m"(b), "m"(c), "m"(reta), "m"(retb)
		);
	return ((uint64_t)retb << 32) | (uint64_t)reta;
}

extern void ec_exit(int code) {

	(void)ec_syscall3(ECN_EXIT, (uint32_t)code, 0, 0);
}

extern int ec_open(const char *path, int flags, ec_mode_t mode) {

	path = fixpath(path);

	__ec_seterrno(int, ec_syscall3(ECN_OPEN, (uint32_t)path, (uint32_t)flags, (uint32_t)mode));
}

extern ec_ssize_t ec_read(int fd, const void *buf, size_t cnt) {

	__ec_seterrno(ec_ssize_t, ec_syscall3(ECN_READ, (uint32_t)fd, (uint32_t)buf, (uint32_t)cnt));
}

extern ec_ssize_t ec_write(int fd, const void *buf, size_t cnt) {

	__ec_seterrno(ec_ssize_t, ec_syscall3(ECN_WRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)cnt));
}

extern ec_off_t ec_lseek(int fd, ec_off_t pos, int whence) {

	__ec_seterrno(ec_off_t, ec_syscall3(ECN_LSEEK, (uint32_t)fd, (uint32_t)pos, (uint32_t)whence));
}

extern int ec_close(int fd) {

	__ec_seterrno(int, ec_syscall3(ECN_CLOSE, (uint32_t)fd, 0, 0));
}

extern int ec_stat(const char *path, ec_stat_t *st) {

	__ec_seterrno(int, ec_syscall3(ECN_STAT, (uint32_t)path, (uint32_t)st, 0));
}

extern int ec_fstat(int fd, ec_stat_t *st) {

	__ec_seterrno(int, ec_syscall3(ECN_FSTAT, (uint32_t)fd, (uint32_t)st, 0));
}

extern int ec_getpid(void) {

	__ec_seterrno(int, ec_syscall3(ECN_GETPID, 0, 0, 0));
}

extern int ec_kill(int pid, int sig) {

	__ec_seterrno(int, ec_syscall3(ECN_KILL, (uint32_t)pid, (uint32_t)sig, 0));
}

extern void *ec_sbrk(intptr_t inc) {

	return (void *)ec_syscall3(ECN_SBRK, (uint32_t)inc, 0, 0);
}

extern int ec_gettimeofday(ec_timeval_t *tv) {

	__ec_seterrno(int, ec_syscall3(ECN_GETTIMEOFDAY, (uint32_t)tv, 0, 0));
}

extern int ec_timens(ec_timeval_t *tv) {

	__ec_seterrno(int, ec_syscall3(ECN_TIMENS, (uint32_t)tv, 0, 0));
}

extern int ec_isatty(int fd) {

	__ec_seterrno(int, ec_syscall3(ECN_ISATTY, (uint32_t)fd, 0, 0));
}

extern int ec_signal(int sig, void (*handler)()) {

	__ec_seterrno(int, ec_syscall3(ECN_SIGNAL, (uint32_t)sig, (uint32_t)handler, 0));
}

extern int ec_panic(const char *msg) {

	__ec_seterrno(int, ec_syscall3(ECN_PANIC, (uint32_t)msg, 0, 0));
}

extern int ec_pexec(const char *path, const char **argv, const char **envp) {

	__ec_seterrno(int, ec_syscall3(ECN_PEXEC, (uint32_t)path, (uint32_t)argv, (uint32_t)envp));
}

extern int ec_pwait(int pid, int *status, ec_timeval_t *timeout) {

	__ec_seterrno(int, ec_syscall3(ECN_PWAIT, (uint32_t)pid, (uint32_t)status, (uint32_t)timeout));
}

extern int ec_sleepns(ec_timeval_t *tv) {

	__ec_seterrno(int, ec_syscall3(ECN_SLEEPNS, (uint32_t)tv, 0, 0));
}

extern int ec_readdir(const char *path, ec_dirent_t *dent) {

	path = fixpath(path);

	__ec_seterrno(int, ec_syscall3(ECN_READDIR, (uint32_t)path, (uint32_t)dent, 0));
}

extern int ec_ioctl(int fd, int op, uintptr_t arg) {

	__ec_seterrno(int, ec_syscall3(ECN_IOCTL, (uint32_t)fd, (uint32_t)op, (uint32_t)arg));
}

extern void ec_kinfo(ec_kinfo_t *info) {

	(void)ec_syscall3(ECN_KINFO, (uint32_t)info, 0, 0);
}

extern int ec_getuser(ec_uinfo_t *info) {

	__ec_seterrno(int, ec_syscall3(ECN_GETUSER, (uint32_t)info, 0, 0));
}

extern int ec_setuser(const char *name, const char *pswd) {

	__ec_seterrno(int, ec_syscall3(ECN_SETUSER, (uint32_t)name, (uint32_t)pswd, 0));
}

extern int ec_chdir(const char *path) {

	if (!path) {

		errno = EINVAL;
		return -1;
	}

	path = fixpath(path);
	ec_stat_t stat;
	if (ec_stat(path, &stat) < 0)
		return -1;
	if (!(stat.flags & ECS_DIR)) {

		errno = ENOTDIR;
		return -1;
	}
	strncpy(pathbuf, path, EC_PATHSZ);
	return 0;
}

extern int ec_getcwd(char *buf, size_t bufsz) {

	strncpy(buf, pathbuf, bufsz);
	return 0;
}
