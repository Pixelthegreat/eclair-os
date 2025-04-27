#include <stdlib.h>
#include <string.h>
#include <ec.h>

int errno = 0;

extern const char **environ;

extern int main(int argc, const char **argv);

extern int __libc_init_heap(void);
extern void __libc_fini_heap(void);
extern int __libc_init_file(void);
extern void __libc_fini_file(void);

/* environment */
#define MAX_EXITH 32
static void (*exith[MAX_EXITH])();
static int nexith = MAX_EXITH;

/* initialize library */
extern int __libc_init(void) {

	if (__libc_init_heap() < 0)
		return -1;
	if (__libc_init_file() < 0)
		return -1;
	return 0;
}

/* clean up library */
extern void __libc_fini(void) {

	__libc_fini_file();
	__libc_fini_heap();
}

/* initialize library, call main and exit */
extern void __libc_main(int argc, const char **argv) {

	// atexit(_fini);

	if (__libc_init() < 0)
		abort();

	// _init();

	exit(main(argc, argv));
}

/* environment functions */
extern void abort(void) {

	ec_exit(1);
}

extern int atexit(void (*func)(void)) {

	if (!nexith) return -1;

	exith[--nexith] = func;
	return 0;
}

extern void exit(int status) {

	for (; nexith < MAX_EXITH; nexith++)
		exith[nexith]();
	_Exit(status);
}

extern void _Exit(int status) {

	__libc_fini();
	ec_exit(status);
}

extern char *getenv(const char *name) {

	size_t i = 0;
	while (environ[i]) {

		const char *env = environ[i++];
		
		const char *end = strchr(env, '=');
		if (!end) continue;

		size_t len = (size_t)(end - env);
		if (!strncmp(env, name, len))
			return (char *)end+1;
	}
	return NULL;
}
