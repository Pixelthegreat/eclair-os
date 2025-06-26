/*
 * Common data structures, types and macros for the GNU Tar archive format
 */
#ifndef EC_TAR_H
#define EC_TAR_H

#include <stdint.h>

#define TAR_REGTYPE '0'
#define TAR_DIRTYPE '5'

/* file header */
#define TAR_HEADER_SIZE 512
typedef struct tar_header {
	char name[100]; /* file name */
	char mode[8]; /* file mode */
	char uid[8]; /* owning user id */
	char gid[8]; /* owning group id */
	char size[12]; /* file size */
	char mtime[12]; /* modificiation time */
	char chksum[8]; /* checksum */
	char typeflag; /* file type */
	char linkname[100]; /* symbolic link name */
	char magic[6]; /* ??? */
	char version[2]; /* version */
	char uname[32]; /* user name */
	char gname[32]; /* group name */
	char devmajor[8]; /* major device number */
	char devminor[8]; /* minor device number */
	char prefix[155]; /* ... */
} __attribute__((packed)) tar_header_t;

#endif /* EC_TAR_H */
