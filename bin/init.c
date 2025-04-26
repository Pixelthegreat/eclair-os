#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ec.h>

/* main proc */
extern int main(int argc, const char **argv) {

	for (int i = 0; i < argc; i++)
		printf("Argument %d: '%s'\n", i, argv[i]);

	/* print */
	printf("Hello, world!\nPress enter to cause a kernel panic\n");

	char buf[32];
	fgets(buf, 32, stdin);
	size_t len = strlen(buf);
	if (buf[len-1] == '\n') buf[len-1] = 0;

	printf("Input: %s\n", buf);

	ec_panic("You're not supposed to be here");
}
