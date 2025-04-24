#ifndef ECLAIR_VFS_FS_H
#define ECLAIR_VFS_FS_H

#include <kernel/types.h>
#include <ec.h>

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
#define FS_TRUNCATE 0x4
#define FS_CREATE 0x100

#define FS_ISRW(fl) (((fl) & FS_READ) && ((fl) & FS_WRITE))

/* file operations */
typedef kssize_t (*fs_read_t)(struct fs_node *, uint32_t, size_t, uint8_t *);
typedef kssize_t (*fs_write_t)(struct fs_node *, uint32_t, size_t, uint8_t *);
typedef void (*fs_open_t)(struct fs_node *, uint32_t);
typedef void (*fs_close_t)(struct fs_node *);
typedef bool (*fs_filldir_t)(struct fs_node *);
typedef bool (*fs_isheld_t)(struct fs_node *);
typedef struct fs_node *(*fs_create_t)(struct fs_node *, const char *, uint32_t, uint32_t);
typedef struct fs_node *(*fs_mount_t)(struct fs_node *, struct fs_node *);
typedef void (*fs_stat_t)(struct fs_node *, ec_stat_t *);
typedef bool (*fs_isatty_t)(struct fs_node *);

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
	bool held; /* task is holding resource */
	struct fs_node *parent; /* parent node */
	struct fs_node *ptr; /* alias pointer for mountpoints and symlinks */
	fs_dirent_t *first; /* first directory entry */
	fs_dirent_t *last; /* last directory entry */
	fs_read_t read; /* read from file */
	fs_write_t write; /* write to file */
	fs_open_t open; /* open file */
	fs_close_t close; /* close file */
	fs_filldir_t filldir; /* fill directory node with entries */
	fs_isheld_t isheld; /* check if file resource is held by a task */
	fs_create_t create; /* create a node as a child */
	fs_mount_t mount; /* mount device node */
	fs_stat_t stat; /* get file info */
	fs_isatty_t isatty; /* check if file is a teletype */
} fs_node_t;

extern fs_node_t *fs_root; /* root node */

/* functions */
extern void fs_init(void); /* initialize vfs */

extern fs_dirent_t *fs_dirent_new(const char *name); /* create new directory entry */

extern fs_node_t *fs_node_new(fs_node_t *parent, uint32_t flags); /* create new node */
extern fs_node_t *fs_node_new_ext(fs_node_t *parent, uint32_t flags, size_t sz); /* create new node with size */

extern void fs_node_add_dirent(fs_node_t *node, fs_dirent_t *dent); /* add dirent */
extern void fs_node_print(fs_node_t *node); /* print node tree */

extern kssize_t fs_read(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf); /* read from file */
extern kssize_t fs_write(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf); /* write to file */
extern void fs_open(fs_node_t *node, uint32_t flags); /* open file */
extern void fs_close(fs_node_t *node); /* close file */
extern fs_dirent_t *fs_readdir(fs_node_t *node, uint32_t idx); /* read directory entry */
extern fs_node_t *fs_finddir(fs_node_t *node, const char *name); /* find in directory */
extern bool fs_isheld(fs_node_t *node); /* check if resource is held/busy */
extern fs_node_t *fs_create(fs_node_t *node, const char *name, uint32_t flags, uint32_t mask); /* create a node as a child */
extern fs_node_t *fs_mount(fs_node_t *node, fs_node_t *device); /* mount device node */
extern void fs_stat(fs_node_t *node, ec_stat_t *st); /* get file info */
extern bool fs_isatty(fs_node_t *node); /* check if file is a teletype */

extern fs_node_t *fs_resolve_full(const char *path, bool *create, const char **fname); /* resolve a path to a node */
extern fs_node_t *fs_resolve(const char *path); /* resolve a path to a node strictly */

#endif /* ECLAIR_VFS_FS_H */
