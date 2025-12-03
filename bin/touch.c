#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>

int main(int argc, const char **argv) {

	if (argc != 2) {

		fprintf(stderr, "Invalid arguments\nUsage: %s <path>\n", argv[0]);
		return 1;
	}

	const char *path = argv[1];
	int fd = ec_open(path, ECF_WRITE | ECF_CREATE, 0644);
	if (fd < 0) {

		fprintf(stderr, "Can't touch file '%s': %s\n", path, strerror(errno));
		return 1;
	}
	ec_close(fd);
	return 0;
}
