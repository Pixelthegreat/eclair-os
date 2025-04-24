#include <stddef.h>

#define EC_IMPL
#include <ec.h>

static int ttyfd = -1;

static size_t m_strlen(const char *s) {

	size_t res = 0;
	while (*s++) res++;
	return res;
}

static void m_printi(int d) {

	if (!d) {
		ec_write(ttyfd, "0", 1);
		return;
	}

	char buf1[32], buf2[32];
	size_t n1 = 0, n2 = 0;
	int sign = d < 0;
	if (sign) d = -d;

	while (d) {
		buf1[n1++] = (char)(d % 10) + '0';
		d /= 10;
	}
	if (sign) buf2[n2++] = '-';
	while (n1--) buf2[n2++] = buf1[n1];

	ec_write(ttyfd, buf2, n2);
}

static void segvh() {

	ec_write(ttyfd, "Segmentation fault\n", 19);
	ec_exit();
}

static void run() {

	const char *msg = NULL; size_t len = 0;

	if ((ttyfd = ec_open("/dev/tty0", 0x3, 0)) < 0)
		return;

	ec_signal(5, segvh);

	/* print time */
	ec_timeval_t tv;
	ec_gettimeofday(&tv);

	msg = "Seconds since epoch: ";
	len = m_strlen(msg);
	ec_write(ttyfd, msg, len);

	m_printi((int)tv.sec);

	/* write message */
	msg = "\nHello world from /bin/init!\n";
	len = m_strlen(msg);
	ec_write(ttyfd, msg, len);

	/* read input */
	char buf[32];
	ec_ssize_t blen = ec_read(ttyfd, buf, 32);

	ec_write(ttyfd, buf, (size_t)blen-1);
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
