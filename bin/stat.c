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

	/* get file info */
	ec_stat_t st;
	if (ec_stat(argv[1], &st) < 0) {

		fprintf(stderr, "Can't stat file '%s': %s\n", argv[1], strerror(errno));
		return 1;
	}

	printf("Device: %d\n"
	       "Inode: %d\n"
	       "Mode: %d\n"
	       "Flags: %d\n"
	       "Hard links: %d\n"
	       "User ID: %d\n"
	       "Group ID: %d\n"
	       "Size: %d\n"
	       "Block size: %d\n"
	       "Blocks: %d\n",
	       st.dev, st.ino, st.mode, st.flags,
	       st.nlink, st.uid, st.gid, (int)st.size,
	       st.blksize, st.blocks);
	return 0;
}
