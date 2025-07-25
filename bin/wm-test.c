#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <ec/wm.h>

static int fd = -1; /* channel fd */
static char buffer[ECIO_CHNL_BUFSZ]; /* message response buffer */
static wm_message_t *response = (wm_message_t *)buffer; /* message response */
static size_t rsize; /* response size */

static int send_message(wm_message_t *message, size_t size) {

	while (ec_write(fd, message, size) < 0 && errno == EAGAIN);

	ec_ioctl(fd, ECIO_CHNL_WAITREAD, 0);
	rsize = (size_t)ec_read(fd, buffer, ECIO_CHNL_BUFSZ);

	printf("[client] Response: Type=%u, Size=%u, Result=%u\n", response->type, response->size, response->result);
	if (response->result != WM_SUCCESS)
		return -1;
	return 0;
}

/* run application */
static int run(void) {

	char pathbuf[32];
	snprintf(pathbuf, 32, "/dev/chnl/%d", WM_CHNL);

	fd = ec_open(pathbuf, ECF_READ | ECF_WRITE, 0);
	if (fd < 0) {

		fprintf(stderr, "Can't open '%s': %s\n", pathbuf, strerror(errno));
		return 1;
	}

	/* acknowledge connection */
	wm_message_t acknowledge = {
		.type = WM_REQUEST,
		.size = sizeof(acknowledge),
		.function = WM_FUNCTION_ACKNOWLEDGE,
	};
	if (send_message(&acknowledge, sizeof(acknowledge)) < 0)
		return 1;
	printf("[client] Sent ACKNOWLEDGE message\n");

	ec_timeval_t tv = {
		.sec = 3,
		.nsec = 0,
	};
	ec_sleepns(&tv);

	/* forget connection */
	wm_message_t forget = {
		.type = WM_REQUEST,
		.size = sizeof(forget),
		.function = WM_FUNCTION_FORGET,
	};
	if (send_message(&forget, sizeof(forget)) < 0)
		return 1;
	printf("[client] Sent FORGET message\n");
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	if (fd >= 0) ec_close(fd);
}

int main() {

	int code = run();
	cleanup();
	return code;
}
