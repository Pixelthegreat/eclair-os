#include <stddef.h>

#define EC_IMPL
#include <ec.h>

static const char **argv = NULL;
static const char **envp = NULL;

/* get string length */
static size_t m_strlen(const char *s) {

	size_t n = 0;
	while (*s++) n++;
	return n;
}

/* compare strings */
static int m_strncmp(const char *a, const char *b, size_t n) {

	if (!n--) return 0;
	for (; *a && *b && n && *a == *b; a++, b++, n--);
	return *(unsigned char *)a - *(unsigned char *)b;
}

/* find character in string */
extern char *m_strchr(const char *str, int ch) {

	while (*str) {
		if (*str == (char)ch)
			return (char *)str;
		str++;
	}
	return NULL;
}

/* print string */
static void m_prints(const char *s) {

	ec_write(1, s, m_strlen(s));
}

/* print string with newline */
static void m_println(const char *s) {

	m_prints(s);
	ec_write(1, "\n", 1);
}

/* get environment variable */
static const char *m_getenv(const char *name) {

	size_t i = 0;
	while (envp[i]) {

		const char *env = envp[i++];
		
		const char *end = m_strchr(env, '=');
		if (!end) continue;

		size_t len = (size_t)(end - env);
		if (!m_strncmp(env, name, len))
			return end+1;
	}
	return NULL;
}

/* main proc */
extern void _start() {

	argv = *((const char ***)0x8008);
	envp = *((const char ***)0x800c);

	/* open stdin, stdout and stderr */
	const char *p_stdin = m_getenv("EC_STDIN");
	if (!p_stdin) ec_panic("Failed to getenv 'EC_STDIN'");

	const char *p_stdout = m_getenv("EC_STDOUT");
	if (!p_stdout) ec_panic("Failed to getenv 'EC_STDOUT'");

	const char *p_stderr = m_getenv("EC_STDERR");
	if (!p_stderr) ec_panic("Failed to getenv 'EC_STDERR'");

	if (ec_open(p_stdin, ECF_READ, 0) != 0)
		ec_panic("Failed to open stdin");
	if (ec_open(p_stdout, ECF_WRITE, 0) != 1)
		ec_panic("Failed to open stdout");
	if (ec_open(p_stderr, ECF_WRITE, 0) != 2)
		ec_panic("Failed to open stderr");

	/* print */
	m_prints("stdin: ");
	m_println(p_stdin);
	m_prints("stdout: ");
	m_println(p_stdout);
	m_prints("stderr: ");
	m_println(p_stderr);

	m_prints("Hello, world!\nPress enter to cause a kernel panic\n");
	char buf[32];
	ec_read(0, buf, 32);

	ec_panic("You're not supposed to be here");
}
