/*
 * == Eclair OS system call declarations and documentation ==
 *
 * For clarity: Tasks are referred to as processes and
 * tasks interchangeably.
 */
#ifndef EC_H
#define EC_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#define EC_ALIGN(x, sz) (((x) + ((sz)-1)) & ~((sz)-1))

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
#define ECN_TIMENS 12
#define ECN_GETTIMEOFDAY 13
#define ECN_ISATTY 14
#define ECN_SIGNAL 15
#define ECN_PANIC 16
#define ECN_PEXEC 17
#define ECN_PWAIT 18
#define ECN_SLEEPNS 19
#define ECN_READDIR 20

#define ECN_COUNT 21

#define EC_PATHSZ 256

/*
 * System call with 3 arguments and a uint32_t return value:
 *   eax = i, ebx = a, ecx = b, edx = c
 *   ret = eax
 */
extern uint32_t ec_syscall3(uint32_t i, uint32_t a, uint32_t b, uint32_t c);

/*
 * System call with 3 arguments and a uint64_t return value:
 *   eax = i, ebx = a, ecx = b, edx = c
 *   ret = (ecx << 32) | eax
 */
extern uint64_t ec_syscall3r2(uint32_t i, uint32_t a, uint32_t b, uint32_t c);

#define __ec_seterrno(rtype, call) rtype res = (rtype)call;\
	if (res < 0) { errno = -(int)res; return -1; }\
	return res

/*
 * Exit the current task/process. Does not return.
 *   ebx/code = Exit status code
 */
extern void ec_exit(int code);

/*
 * Open a file.
 *   ebx/path = Path to file
 *   ecx/flags = Flags (fcntl)
 *   edx/mode = File mode (with O_CREAT only)
 *   eax (return) = File descriptor if successful, negative on error
 */
#define ECF_READ 0x1
#define ECF_WRITE 0x2
#define ECF_TRUNCATE 0x4
#define ECF_CREATE 0x100

extern int ec_open(const char *path, int flags, ec_mode_t mode);

/*
 * Read from a file.
 *   ebx/fd = File descriptor
 *   ecx/buf = Buffer to fill
 *   edx/cnt = Number of bytes to read
 *   eax (return) = Number of bytes read if successful, negative on error
 */
extern ec_ssize_t ec_read(int fd, const void *buf, size_t cnt);

/*
 * Write to a file.
 *   ebx/fd = File descriptor
 *   ecx/buf = Buffer to copy
 *   edx/cnt = Number of bytes to write
 *   eax (return) = Number of bytes written if successful, negative on error
 */
extern ec_ssize_t ec_write(int fd, const void *buf, size_t cnt);

/*
 * Go to a position in a file.
 *   ebx/fd = File descriptor
 *   ecx/pos = Position to seek
 *   edx/whence = The relative start position (SEEK_SET, SEEK_CUR, SEEK_END)
 *   eax (return) = The position seeked relative to the beginning of the file
 */
extern ec_off_t ec_lseek(int fd, ec_off_t pos, int whence);

/*
 * Close a file.
 *   ebx/fd = File descriptor
 *   eax (return) = Zero if successful, negative on error
 */
extern int ec_close(int fd);

/*
 * Stat a file.
 *   ebx/path = File path
 *   ecx/st = Stat buffer
 *   eax (return) = Zero if successful, negative on error
 */
#define ECS_REG 0x1
#define ECS_DIR 0x2
#define ECS_CHRDEV 0x4
#define ECS_BLKDEV 0x8

typedef struct {
	int dev; /* device id */
	int ino; /* file serial number */
	ec_mode_t mode; /* file mode */
	int flags; /* file flags */
	int nlink; /* number of hard links */
	int uid; /* user owner */
	int gid; /* group owner */
	int rdev; /* special file device id */
	long size; /* file size */
	long long atime; /* access time */
	long long mtime; /* modification time */
	long long ctime; /* status change time */
	int blksize; /* block size */
	int blocks; /* number of blocks */
} ec_stat_t;

extern int ec_stat(const char *path, ec_stat_t *st);

/*
 * Stat an open file.
 *   ebx/fd = File descriptor
 *   ecx = Stat buffer
 *   eax (return) = Zero if successful, negative on error
 */
extern int ec_fstat(int fd, ec_stat_t *st);

/*
 * Get current process id.
 *   eax (return) = Process id
 */
extern int ec_getpid(void);

/*
 * Raise a signal on a process.
 *   ebx/pid = Process id
 *   ecx/sig = Signal number
 *   eax (return) = Zero if successful, negative on error
 */
extern int ec_kill(int pid, int sig);

/*
 * Increase or decrease breakpoint.
 *   ebx/inc = Increment
 *   eax (return) = Breakpoint address if successful, NULL on error
 */
extern void *ec_sbrk(intptr_t inc);

/*
 * Get epoch time.
 *   ebx/tv = Time info to fill
 *   eax (return) = Zero if successful, negative on error
 * The time value on success should be in Greenwich Meantime (GMT).
 * The normal gettimeofday function will properly convert between .nsec and .tv_usec.
 */
typedef struct ec_timeval {
	uint64_t sec; /* seconds */
	uint64_t nsec; /* nanoseconds */
} ec_timeval_t;

extern int ec_gettimeofday(ec_timeval_t *tv);

/*
 * Get arbitrary timestamp in nanosecond resolution.
 *   ebx/tv = Time info to fill
 *   eax (return) = Zero if successful, negative on error
 * The times syscall is not implemented here due to a lack of task time accounting.
 */
extern int ec_timens(ec_timeval_t *tv);

/*
 * Check if a file is a teletype.
 *   ebx/fd = File descriptor
 *   eax (return) = Zero if not a tty, one if a tty, negative on error
 */
extern int ec_isatty(int fd);

/*
 * Set a signal handler.
 *   ebx/sig = Signal number
 *   ecx/handler = Signal handler
 *   eax (return) = Zero if successful, negative on error
 */
extern int ec_signal(int sig, void (*handler)());

/*
 * Cause a kernel panic.
 *   ebx/msg = Panic message
 *   eax (return) = Doesn't return if successful, negative on error
 * This system call can only be called by the init process (pid = 1).
 */
extern int ec_panic(const char *msg);

/*
 * Execute a process.
 *   ebx/path = Path to executable binary
 *   ecx/argv = Arguments
 *   edx/envp = Environment or NULL
 *   eax (return) = Process id of task if successful, negative on error
 * If envp is NULL, then the current task's environment is used.
 *
 * This system call loads a process but does not wait for it to finish executing.
 * Additionally, unlike the Unix execve family, this does not replace the current process image.
 */
extern int ec_pexec(const char *path, const char **argv, const char **envp);

/*
 * Wait for a process.
 *   ebx/pid = ID of process
 *   ecx/status = Wait status bitfield
 *   edx/timeout = Timeout of system call or NULL
 *   eax (return) = Zero if successful, negative on error
 *
 * If timeout is NULL, then this system call will wait until the task changes state.
 * If the timeout is 0, then it will not wait for a status change.
 * Otherwise, then it will wait until either the task changes state or the timeout is reached.
 *
 * If pid is invalid, the status will be set to ECW_EXITED regardless.
 */
#define ECW_EXITCODE 0xff
#define ECW_EXITED 0x100
#define ECW_TIMEOUT 0x200

#define ECW_TOEXITCODE(st) ((st) & ECW_EXITCODE)
#define ECW_ISEXITED(st) ((st) & ECW_EXITED)
#define ECW_ISTIMEOUT(st) ((st) & ECW_TIMEOUT)

extern int ec_pwait(int pid, int *status, ec_timeval_t *timeout);

/*
 * Sleep with a nanosecond resolution.
 *   ebx/tv = Time info
 *   eax (return) = Zero if successful, negative on error
 */
extern int ec_sleepns(ec_timeval_t *tv);

/*
 * Read information about directory entries from a directory.
 *   ebx/path = Directory path or NULL
 *   ecx/dent = Directory entry structure to fill
 *   eax (return) = Zero if successful, one if reached the end of the directory, negative on error
 */
#define ECD_NAMESZ 128

typedef struct ec_dirent {
	char name[ECD_NAMESZ]; /* file name */
	int flags; /* file flags */
	void *_data;
} ec_dirent_t;

extern int ec_readdir(const char *path, ec_dirent_t *dent);

/*
 * Change current process working directory.
 *   path = Directory path
 *   return = Zero if successful, negative on error
 */
extern int ec_chdir(const char *path);

/*
 * Get current process working directory.
 *   buf = Buffer to write
 *   bufsz = Buffer size
 *   return = Zero if successful, negative on error
 */
extern int ec_getcwd(char *buf, size_t bufsz);

#endif /* EC_H */
