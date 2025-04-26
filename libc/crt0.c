#include <stdlib.h>

#define EC_IMPL
#include <ec.h>

const char **__libc_argv = NULL;
const char **__libc_envp = NULL;

const char **environ = NULL;

extern void __libc_main(int argc, const char **argv);

static void sigsegv() {

	ec_write(1, "Segmentation fault\n", 19);
	abort();
}

extern void _start() {

	__libc_argv = *((const char ***)0x8008);
	__libc_envp = *((const char ***)0x800C);
	environ = __libc_envp;

	int argc = 0;
	while (__libc_argv[argc]) argc++;

	/* open stdin, stdout and stderr */
	ec_open(getenv("EC_STDIN"), ECF_READ, 0);
	ec_open(getenv("EC_STDOUT"), ECF_WRITE, 0);
	ec_open(getenv("EC_STDERR"), ECF_WRITE, 0);

	ec_signal(5, sigsegv);

	__libc_main(argc, __libc_argv);
}
