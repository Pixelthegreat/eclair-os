#ifndef ECLAIR_VFS_FS_H
#define ECLAIR_VFS_FS_H

#include <e.clair/types.h>

struct fs_node;

/* file types (matches ext2 for simplicity) */
#define FS_FILE 0x1
#define FS_DIRECTORY 0x2
#define FS_CHARDEVICE 0x4
#define FS_BLOCKDEVICE 0x8
#define FS_PIPE 0x10
#define FS_SYMLINK 0x20
#define FS_MOUNTPOINT 0x40
#define FS_SOCKET 0x80

/* file open flags */
#define FS_READ 0x1
#define FS_WRITE 0x2

/* file operations */
typedef ssize_t (*fs_read_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef ssize_t (*fs_write_t)(struct fs_node *, off_t, size_t, uint8_t *);
typedef void (*fs_open_t)(struct fs_node *, uint32_t flags);
typedef void (*fs_close_t)(struct fs_node *);
typedef bool (*fs_filldir_t)(struct fs_node *);

#define FS_NAMESZ 128

/* directory entry */
typedef struct fs_dirent {
	char name[FS_NAMESZ]; /* entry name */
	struct fs_node *node; /* file system node */
	struct fs_dirent *prev; /* previous sibling */
	struct fs_dirent *next; /* next sibling */
} fs_dirent_t;

/* file node */
typedef struct fs_node {
	void *data; /* user data pointer */
	uint32_t mask; /* permissions mask */
	uint32_t uid; /* user id */
	uint32_t gid; /* group id */
	uint32_t flags; /* node flags */
	uint32_t inode; /* inode number */
	uint32_t len; /* size of file */
	uint32_t impl; /* file system implementation info */
	uint32_t oflags; /* open flags */
	int refcnt; /* reference count for open files */
	void *odata; /* user data pointer for open files */
	struct fs_node *parent; /* parent node */
	struct fs_node *ptr; /* alias pointer for mountpoints and symlinks */
	fs_dirent_t *first; /* first directory entry */
	fs_dirent_t *last; /* last directory entry */
	fs_read_t read; /* read from file */
	fs_write_t write; /* write to file */
	fs_open_t open; /* open file */
	fs_close_t close; /* close file */
	fs_filldir_t filldir; /* fill directory node with entries */
} fs_node_t;

extern fs_node_t *fs_root; /* root node */

/* functions */
extern void fs_init(void); /* initialize vfs */

extern fs_dirent_t *fs_dirent_new(const char *name); /* create new directory entry */

extern fs_node_t *fs_node_new(fs_node_t *parent, uint32_t flags); /* create new node */
extern void fs_node_add_dirent(fs_node_t *node, fs_dirent_t *dent); /* add dirent */
extern void fs_node_print(fs_node_t *node); /* print node tree */

extern ssize_t fs_read(fs_node_t *node, off_t offset, size_t nbytes, uint8_t *buf); /* read from file */
extern ssize_t fs_write(fs_node_t *node, off_t offset, size_t nbytes, uint8_t *buf); /* write to file */
extern void fs_open(fs_node_t *node, uint32_t flags); /* open file */
extern void fs_close(fs_node_t *node); /* close file */
extern fs_dirent_t *fs_readdir(fs_node_t *node, uint32_t idx); /* read directory entry */
extern fs_node_t *fs_finddir(fs_node_t *node, const char *name); /* find in directory */

#endif /* ECLAIR_VFS_FS_H */
