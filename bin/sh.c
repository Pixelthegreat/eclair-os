#include <stdio.h>
#include <stdlib.h>
#include <ec.h>

int main() {

	printf("Hello, world!\n");

	ec_timeval_t tv = {
		.sec = 2,
		.nsec = 0,
	};
	ec_sleepns(&tv);

	printf("Exiting...\n");
	return 123;
}
