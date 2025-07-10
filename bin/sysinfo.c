#include <stdio.h>
#include <stdlib.h>
#include <ec.h>

int main() {

	ec_kinfo_t info;
	ec_kinfo(&info);

	printf("OS name: %s\n"
	       "OS version: %hhu.%hhu.%hhu\n"
	       "Free memory: %juK/%juK\n",
	       info.name,
	       info.version[0], info.version[1], info.version[2],
	       (info.mem_total - info.mem_free) >> 10, info.mem_total >> 10);
	return 0;
}
