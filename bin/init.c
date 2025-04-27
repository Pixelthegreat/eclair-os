#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ec.h>

/* main proc */
extern int main(int argc, const char **argv) {

	int pid = ec_pexec("/bin/sh", argv, NULL);
	if (pid >= 0) {

		int status = 0;
		while (!ECW_ISEXITED(status)) {

			ec_timeval_t tv = {.sec = 1, .nsec = 500000000};
			ec_pwait(pid, &status, &tv);
		}
		printf("Exited with code %d\n", ECW_TOEXITCODE(status));
	}

	ec_panic("You're not supposed to be here");
}
