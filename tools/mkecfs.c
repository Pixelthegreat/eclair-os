#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define ECFS_IMPL
#include <ec/ecfs.h>
#include <ec/mbr.h>

#define DEF_BLKSZ 1024

#define MIN(x, y) ((x) < (y)? (x): (y))

enum {
	OPT_HELP = 0,
	OPT_BLKSZ,
	OPT_OSID,
	OPT_MBR,
	OPT_COUNT,

	OPT_HELP_BIT = 0x1,
	OPT_BLKSZ_BIT = 0x2,
	OPT_OSID_BIT = 0x4,
	OPT_MBR_BIT = 0x8,
} flags;

static const char *arg_first = NULL;

static int flag_values[OPT_COUNT] = {
	[OPT_BLKSZ] = DEF_BLKSZ,
};
static const char *flag_osid = NULL;

static size_t blksz = 0; /* block size */
static void *blk = NULL; /* block data */
static size_t nblk = 0; /* number of fs blocks */

static char osid[12] = "generic     "; /* operating system identifier */

static FILE *fp = NULL; /* device/disk image file */
static size_t fsz; /* file size */
static size_t max_fssz; /* maximum fs size */
static size_t fsbase = 0; /* file system base block */

/* parse arguments */
static int parse_args(int argc, const char **argv) {

	for (int i = 1; i < argc; i++) {
		
		const char *arg = argv[i];
		if (arg[0] == '-') {

			/* help */
			if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
				flags |= OPT_HELP_BIT;

			/* block size */
			else if (!strcmp(arg, "-b") || !strcmp(arg, "--block-size")) {

				flags |= OPT_BLKSZ_BIT;
				if (++i >= argc) {

					fprintf(stderr, "%s: Expected argument after '%s'\n", argv[0], arg);
					return -1;
				}
				flag_values[OPT_BLKSZ] = atoi(argv[i]);
			}

			/* operating system identifier */
			else if (!strcmp(arg, "-n") || !strcmp(arg, "--os-name")) {

				flags |= OPT_OSID_BIT;
				if (++i >= argc) {

					fprintf(stderr, "%s: Expected argument after '%s'\n", argv[0], arg);
					return -1;
				}
				flag_osid = argv[i];
			}

			/* include an mbr */
			else if (!strcmp(arg, "-m") || !strcmp(arg, "--mbr-start")) {

				flags |= OPT_MBR_BIT;
				if (++i >= argc) {

					fprintf(stderr, "%s: Expected argument after '%s'\n", argv[0], arg);
					return -1;
				}
				flag_values[OPT_MBR] = atoi(argv[i]);
			}

			/* unrecognized */
			else {

				fprintf(stderr, "%s: Unrecognized option '%s'\n", argv[0], arg);
				return -1;
			}
		}

		else if (!arg_first) arg_first = arg;

		else {

			fprintf(stderr, "%s: Unexpected argument '%s'\n", argv[0], arg);
			return -1;
		}
	}

	if (!arg_first && !(flags & OPT_HELP_BIT)) {

		fprintf(stderr, "%s: Expected device argument\n", argv[0]);
		return -1;
	}
	return 0;
}

/* print help message */
static void print_help(const char *prog) {

	fprintf(stderr, "Usage: %s <device> [options]\n\nOptions:\n"
			"\t-h|--help               Show this help message\n"
			"\t-b|--block-size <size>  Specify the size of a file system block\n"
			"\t-n|--os-name <name>     Specify the operating system name/identifier\n"
			"\t-m|--mbr-start <block>  Create a master boot record and a partition starting at <block>\n",
			prog);
}

/* count number of one bits in a number */
int count_ones(uint32_t v) {

	int n = 0;
	for (int i = 0; i < 32; i++) {
		if (v & (1 << i))
			n++;
	}
	return n;
}

/* write head block */
static size_t write_hblk(ecfs_hblk_t *hblk) {

	memcpy(blk, hblk, sizeof(ecfs_hblk_t));

	fseek(fp, (fsbase * blksz) + ECFS_HBLK_START, SEEK_SET);
	return fwrite(blk, 1, ECFS_HBLK_SIZE, fp);
}

/* write a block */
static size_t write_blk(void *data, size_t index) {

	if (index >= nblk) return 0;

	fseek(fp, blksz * (fsbase + index), SEEK_SET);
	return fwrite(blk, 1, blksz, fp);
}

/* generate file system */
static int genfs(const char *prog) {

	size_t base = blksz == 1024? 2: 1;
	size_t bblk_tlrm = blksz >> 2; /* number of blocks per tlrm block */
	size_t blkasz = blksz * 8; /* size of block area */
	size_t nblk_tlrm = ECFS_ALIGN(ECFS_ALIGN(nblk, blkasz)/blkasz, bblk_tlrm)/bblk_tlrm; /* toplevel reservation map */
	size_t nblk_brb = ECFS_ALIGN(nblk, blkasz)/blkasz;
	size_t nblk_rsvd = base + nblk_tlrm + nblk_brb + 1; /* reserved blocks */
	fprintf(stderr, "%s: Generating structures...\n", prog);

	/* write head block */
	ecfs_hblk_t hblk;
	memcpy(hblk.signature, ecfs_sig, 4);
	memcpy(hblk.osid, osid, 12);
	hblk.blk_tlrm = ecfs_htold((uint32_t)base);
	hblk.nblk_tlrm = ecfs_htold((uint32_t)nblk_tlrm);
	hblk.blk_brb = ecfs_htold((uint32_t)(base + nblk_tlrm));
	hblk.nblk_brb = ecfs_htold((uint32_t)nblk_brb);
	hblk.blk_root = ecfs_htold((uint32_t)(base + nblk_tlrm + nblk_brb));
	hblk.blksz = ecfs_htold((uint32_t)blksz);
	hblk.nblk = ecfs_htold((uint32_t)nblk);
	hblk.nblk_alloc = ecfs_htold((uint32_t)nblk_rsvd);
	hblk.nblk_rsvd = ecfs_htold((uint32_t)nblk_rsvd);
	hblk.revision = ecfs_htold(ECFS_REVISION);

	write_hblk(&hblk);

	/* write tlrm */
	size_t cnt = nblk_rsvd;
	for (size_t i = 0; i < nblk_tlrm; i++) {

		memset(blk, 0, blksz);
		uint32_t *arr = (uint32_t *)blk;
		for (size_t j = 0; j < bblk_tlrm; j++) {
			if (cnt) {

				size_t k = MIN(cnt, bblk_tlrm);
				cnt -= k;

				arr[j] += (uint32_t)k;
			}
			arr[j] = ecfs_htold(arr[j]);
		}
		write_blk(blk, base+i);
	}

	/* write brb */
	for (size_t i = 0; i < nblk_brb; i++) {

		memset(blk, 0, blksz);
		for (size_t j = 0; j < blkasz; j++) {

			size_t p = i * blkasz + j;
			uint8_t *arr = (uint8_t *)blk;

			if (p < nblk_rsvd)
				arr[j/8] |= (1 << (j % 8));
		}
		write_blk(blk, base+nblk_tlrm+i);
	}

	/* write root file entry */
	memset(blk, 0, blksz);
	ecfs_file_t *file = (ecfs_file_t *)blk;

	uint64_t tm = (uint64_t)time(NULL);

	file->flags = ecfs_htold(ECFS_FLAG_DIR);
	file->mask = ecfs_htold(0644); /* or|gr|ur|uw */
	file->atime_lo = ecfs_htold((uint32_t)(tm & 0xffffffff));
	file->atime_hi = ecfs_htold((uint32_t)((tm >> 32) & 0xffffffff));
	file->mtime_lo = file->atime_lo;
	file->mtime_hi = file->atime_hi;
	file->ctime_lo = file->atime_lo;
	file->ctime_hi = file->atime_hi;
	file->fsz = 0;
	file->nblk = 0;

	write_blk(blk, base+nblk_tlrm+nblk_brb);

	/* create master boot record */
	/* TODO: set chs start and end */
	if (flags & OPT_MBR_BIT) {

		fprintf(stderr, "%s: Creating master boot record...\n", prog);

		ec_mbr_t mbr;
		memset(&mbr, 0, sizeof(mbr));

		uint32_t m = (uint32_t)blksz/512;

		mbr.bootsig[0] = 0x55;
		mbr.bootsig[1] = 0xaa;
		mbr.part[0].attr = EC_MBR_ATTR_BOOT;
		mbr.part[0].type = 0xec;
		mbr.part[0].start_lba = ecfs_htold((uint32_t)fsbase * m);
		mbr.part[0].nsect = ecfs_htold((uint32_t)nblk * m);

		fseek(fp, EC_MBR_START, SEEK_SET);
		fwrite(&mbr, 1, sizeof(mbr), fp);
	}

	fprintf(stderr, "%s: Done.\n", prog);
	return 0;
}

int main(int argc, const char **argv) {

	if (parse_args(argc, argv) < 0)
		return EXIT_FAILURE;
	if (flags & OPT_HELP_BIT) {

		print_help(argv[0]);
		return EXIT_SUCCESS;
	}

	/* check block size */
	if (flag_values[OPT_BLKSZ] < 1024 || count_ones(flag_values[OPT_BLKSZ]) != 1) {

		fprintf(stderr, "%s: Invalid block size: %d\n", argv[0], (uint32_t)flag_values[OPT_BLKSZ]);
		return EXIT_FAILURE;
	}
	else if (flag_values[OPT_BLKSZ] > 65536)
		fprintf(stderr, "%s: Warning: Block sizes greater than 65536 are not recommended\n", argv[0]);

	blksz = (size_t)flag_values[OPT_BLKSZ];
	max_fssz = blksz * 4294967296L;

	/* check os id */
	if (flags & OPT_OSID_BIT) {

		size_t len = strlen(flag_osid);
		if (len > 12) {

			fprintf(stderr, "%s: Operating system name/identifier must be at most 12 bytes\n", argv[0]);
			return EXIT_FAILURE;
		}
		memcpy(osid, flag_osid, len);
		memset(osid+len, ' ', 12-len);
	}

	/* check mbr start */
	if (flags & OPT_MBR_BIT && !flag_values[OPT_MBR]) {

		fprintf(stderr, "%s: Partition start must be greater than zero\n", argv[0]);
		return EXIT_FAILURE;
	}
	fsbase = (size_t)flag_values[OPT_MBR];

	size_t nreq = (blksz == 1024? 2: 1) + 3;

	/* open file */
	fp = fopen(arg_first, "rb+");
	if (!fp) {

		fprintf(stderr, "%s: Can't open '%s': %s\n", arg_first, strerror(errno));
		return EXIT_FAILURE;
	}

	fseek(fp, 0, SEEK_END);
	fsz = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* check file size */
	if (fsz < (blksz * nreq)) {

		fclose(fp);
		fprintf(stderr, "%s: Device/image is too small\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (fsz >= max_fssz) {

		fclose(fp);
		fprintf(stderr, "%s: Device/image is too large\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (fsz % blksz)
		fprintf(stderr, "%s: Warning: Total size of device/image doesn't align to chosen block size\n", argv[0]);

	if (flags & OPT_MBR_BIT && (flag_values[OPT_MBR] + nreq) * blksz > fsz) {

		fprintf(stderr, "%s: Partition start block must be within file system requirements\n", argv[0]);
		return EXIT_FAILURE;
	}

	nblk = (fsz / blksz) - fsbase;

	/* generate file system */
	blk = malloc(blksz);
	int res = genfs(argv[0]);
	fclose(fp);
	free(blk);

	if (res < 0) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
