#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>

#define READBUFSZ 512
static char readbuf[READBUFSZ];

int main(int argc, const char **argv) {

	FILE *fp = stdin;
	if (argc >= 2) {
		
		fp = fopen(argv[1], "r");
		if (!fp) {

			fprintf(stderr, "%s: Can't open '%s': %s\n", argv[0], argv[1], strerror(errno));
			return 1;
		}
	}

	/* read contents */
	size_t nread = READBUFSZ;
	while (nread == READBUFSZ) {

		nread = fread(readbuf, 1, READBUFSZ, fp);
		fwrite(readbuf, 1, nread, stdout);
	}

	if (fp != stdin) fclose(fp);
	return 0;
}
