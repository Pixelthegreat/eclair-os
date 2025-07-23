#include <stdio.h>
#include <stdlib.h>
#include <ec.h>

int main(int argc, const char **argv) {

	if (argc != 2) {

		fprintf(stderr, "Invalid arguments\nUsage: %s <seconds>\n", argv[0]);
		return 1;
	}
	const char *stime = argv[1];

	ec_timeval_t tv = {
		.sec = 0,
		.nsec = 0,
	};
	while (*stime >= '0' && *stime <= '9')
		tv.sec = (tv.sec * 10) + (uint64_t)((*stime++) - '0');

	if (ec_sleepns(&tv) < 0)
		return 1;
	return 0;
}
