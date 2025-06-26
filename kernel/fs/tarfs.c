#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <ec/tar.h>
#include <kernel/fs/tarfs.h>

static char pathbuf[FS_NAMESZ];

/* read octal value */
static uint32_t readoct(const char *val, size_t sz) {

	uint32_t res = 0;
	for (size_t i = 0; i < sz && val[i]; i++) {

		res <<= 3;
		res += (uint32_t)val[i]-'0';
	}
	return res;
}

/* resolve directory from path */
static fs_node_t *resolve(fs_node_t *root, const char *path, const char **name) {

	if (!strcmp(path, "./")) return NULL;

	fs_node_t *prev = NULL;
	fs_node_t *node = root;
	if (!strncmp(path, "./", 2)) path += 2;

	size_t len = 0;
	const char *oldpath = NULL;
	*pathbuf = 0;

	while (path && node) {

		while (node->ptr) node = node->ptr;

		oldpath = path;
		path = strchr(path, '/');
		len = path? (size_t)(path-oldpath): strlen(oldpath);
		path = path? path+1: NULL;

		strncpy(pathbuf, oldpath, len);

		prev = node;
		node = fs_finddir(node, pathbuf);
	}
	*name = pathbuf;
	return prev;
}

/* create node for file */
static void create_node(fs_node_t *root, tar_header_t *header) {

	uint32_t flags = 0;
	if (header->typeflag == TAR_REGTYPE) flags = FS_FILE;
	else if (header->typeflag == TAR_DIRTYPE) flags = FS_DIRECTORY;
	else return;

	/* find parent directory */
	const char *name = NULL;
	fs_node_t *dir = resolve(root, header->name, &name);

	if (!name || !dir) return;

	/* set attributes */
	fs_node_t *node = fs_node_new(dir, flags);
	node->data = header;

	node->mask = readoct(header->mode, 8);
	node->uid = readoct(header->uid, 8);
	node->gid = readoct(header->gid, 8);
	node->len = readoct(header->size, 12);

	if (flags & FS_DIRECTORY) {

		fs_dirent_t *cur = fs_dirent_new(".");
		fs_dirent_t *par = fs_dirent_new("..");

		fs_node_add_dirent(node, cur);
		fs_node_add_dirent(node, par);
	}

	/* add to directory */
	fs_dirent_t *dirent = fs_dirent_new(name);
	dirent->node = node;

	fs_node_add_dirent(dir, dirent);
}

/* read from file */
static kssize_t tarfs_read(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buf) {

	uint32_t moffset = MIN(offset, node->len);
	uint32_t msize = MIN(moffset+(uint32_t)size, node->len)-moffset;

	memcpy(buf, node->data+TAR_HEADER_SIZE+moffset, (size_t)msize);
	return (kssize_t)msize;
}

/* stat a file */
static void tarfs_stat(fs_node_t *node, ec_stat_t *st) {

	tar_header_t *header = (tar_header_t *)node->data;
	long long time = (long long)readoct(header->mtime, 12);

	st->atime = time;
	st->mtime = time;
	st->ctime = time;
}

/* mount filesystem */
extern fs_node_t *tarfs_mount(fs_node_t *mountp, void *addr) {

	fs_node_t *node = fs_node_new(NULL, FS_DIRECTORY);
	node->data = addr;

	node->read = tarfs_read;
	node->stat = tarfs_stat;

	fs_dirent_t *cur = fs_dirent_new(".");
	fs_dirent_t *par = fs_dirent_new("..");

	fs_node_add_dirent(node, cur);
	fs_node_add_dirent(node, par);

	/* read entries and create nodes */
	tar_header_t *header = (tar_header_t *)addr;
	while (*header->name) {

		uint32_t size = readoct(header->size, 12);
		uint32_t asize = ALIGN(size, TAR_HEADER_SIZE);

		create_node(node, header);

		header = (tar_header_t *)((void *)header + TAR_HEADER_SIZE + asize);
	}

	mountp->ptr = node;
	return node;
}
