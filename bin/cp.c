#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define COPYBUFSZ 512
static char copybuf[COPYBUFSZ];

int main(int argc, const char **argv) {

	if (argc != 3) {

		fprintf(stderr, "Invalid arguments\nUsage: %s <from> <to>\n", argv[0]);
		return 1;
	}

	/* open source and destination */
	FILE *from_fp = fopen(argv[1], "rb");
	if (!from_fp) {

		fprintf(stderr, "Can't open file '%s': %s\n", argv[1], strerror(errno));
		return 1;
	}
	FILE *to_fp = fopen(argv[2], "wb");
	if (!to_fp) {

		fprintf(stderr, "Can't open file '%s': %s\n", argv[2], strerror(errno));
		fclose(from_fp);
		return 1;
	}

	/* copy data */
	size_t nread = COPYBUFSZ;
	while (nread == COPYBUFSZ) {

		nread = fread(copybuf, 1, COPYBUFSZ, from_fp);
		fwrite(copybuf, 1, nread, to_fp);
	}

	fclose(to_fp);
	fclose(from_fp);
	return 0;
}
