#include <stddef.h>

#define EC_IMPL
#include <ec.h>

static size_t m_strlen(const char *s) {

	size_t res = 0;
	while (*s++) res++;
	return res;
}

static int ttyfd = -1;

static void run() {

	if ((ttyfd = ec_open("/dev/tty0", 0x3, 0)) < 0)
		return;

	/* write message */
	const char *msg = "Hello world from /bin/init!\n";
	size_t len = m_strlen(msg);
	if (ec_write(ttyfd, msg, len) != len)
		return;

	/* read input */
	char buf[32];
	ec_ssize_t blen = ec_read(ttyfd, buf, 32);
	if (blen < 0) return;

	if (ec_write(ttyfd, buf, (size_t)blen-1) < 0)
		return;
}

static void cleanup() {

	if (ttyfd >= 0) ec_close(ttyfd);
}

/* main proc */
extern void _start() {

	run();
	cleanup();
	ec_exit();
}
