#include <stdio.h>
#include <stdlib.h>
#include <ec.h>

int main() {

	ec_kinfo_t info;
	ec_kinfo(&info);

	printf("OS name: %s\n"
	       "OS version: %hhu.%hhu.%hhu\n"
	       "Memory usage: %juK/%juK\n\n",
	       info.name,
	       info.version[0], info.version[1], info.version[2],
	       (info.mem_total - info.mem_free) >> 10, info.mem_total >> 10);

	for (int i = 0; i < 8; i++)
		printf("\e[4%dm ", i);
	for (int i = 0; i < 8; i++)
		printf("\e[2;4%dm ", i);
	printf("\e[0m\n");
	return 0;
}
