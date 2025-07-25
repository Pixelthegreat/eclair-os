/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/task.h>
#include <kernel/mm/heap.h>
#include <ec/device.h>
#include <kernel/vfs/chnlfs.h>

#define COUNT 10
struct {
	int owner; /* channel owner */
	int source; /* source id */
	int dest; /* destination id */
	int lockw; /* write lock id */
	void *buf; /* buffer */
	size_t size; /* message size */
} info[COUNT];

/* open channel */
static void open_fs(fs_node_t *node, uint32_t flags) {

	if (node->refcnt > 0) return;

	int id = (int)node->inode;

	info[id].owner = (int)task_active->id;
	info[id].source = -1;
	info[id].dest = -1;
	info[id].lockw = -1;
	if (!info[id].buf) {

		task_lockcli();
		info[id].buf = kmalloc(ECIO_CHNL_BUFSZ);
		task_unlockcli();
	}
}

/* read message */
static kssize_t read_fs(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	int id = (int)node->inode;

	if ((int)task_active->id != info[id].dest)
		return 0;

	/* copy message */
	if (nbytes > info[id].size) {

		nbytes = info[id].size;
		info[id].size = 0;
	}
	else info[id].size -= nbytes;

	memcpy(buf, info[id].buf, nbytes);

	/* reset */
	if (!info[id].size) {
		info[id].source = -1;
		info[id].dest = -1;
	}
	return (kssize_t)nbytes;
}

/* write message */
static kssize_t write_fs(fs_node_t *node, uint32_t offset, size_t nbytes, uint8_t *buf) {

	int id = (int)node->inode;

	if ((info[id].source >= 0 && info[id].source != (int)task_active->id) ||
	    (info[id].lockw >= 0 && info[id].lockw != (int)task_active->id))
		return -EAGAIN;
	if (info[id].dest < 0)
		info[id].dest = info[id].owner;
	info[id].source = (int)task_active->id;

	nbytes = nbytes > ECIO_CHNL_BUFSZ? ECIO_CHNL_BUFSZ: nbytes;
	memcpy(info[id].buf, buf, nbytes);
	info[id].size = nbytes;

	/* wake up destination task */
	task_lockcli();
	task_t *task = task_get(info[id].dest);
	if (task && task->state == TASK_SLEEPING) {

		task->stale = true;
		task->waketime = 0;
	}
	task_unlockcli();

	return (kssize_t)nbytes;
}

/* io control */
static int ioctl_fs(fs_node_t *node, int op, uintptr_t arg) {

	int id = (int)node->inode;

	switch (op) {
		/* set destination pid */
		case ECIO_CHNL_SETDEST:
			int pid = (int)arg;
			if (info[id].dest >= 0) return -EAGAIN;

			info[id].source = (int)task_active->id;
			info[id].dest = pid;
			return 0;
		/* wait to read message */
		case ECIO_CHNL_WAITREAD:
			if (info[id].dest == (int)task_active->id)
				return 0;

			ec_timeval_t *tv = (ec_timeval_t *)arg;

			task_release();
			if (!tv) task_nano_sleep_until(UINT64_MAX);
			else task_nano_sleep((tv->sec * 1000000000) + tv->nsec);

			task_active->stale = false;
			task_acquire(node);
			if (task_active->stale) return -EINTR;

			if (info[id].dest != (int)task_active->id)
				return -EAGAIN;
			return 0;
		/* get pid of message source */
		case ECIO_CHNL_GETSOURCE:
			return info[id].source;
		/* lock for writing */
		case ECIO_CHNL_LOCKW:
			if (info[id].lockw >= 0)
				return -EAGAIN;
			info[id].lockw = (int)task_active->id;
			return 0;
		/* unlock for writing */
		case ECIO_CHNL_UNLOCKW:
			if (info[id].lockw != (int)task_active->id)
				return -EPERM;
			info[id].lockw = -1;
			return 0;
		/* otherwise */
		default:
			return -ENOSYS;
	}
}

/* initialize channel file system */
extern void chnlfs_init(fs_node_t *node) {

	for (int i = 0; i < COUNT; i++) {

		char buf[2] = {(char)i+'0', 0};
		fs_dirent_t *dent = fs_dirent_new(buf);

		fs_node_t *child = fs_node_new(node, FS_CHARDEVICE);
		dent->node = child;
		child->open = open_fs;
		child->read = read_fs;
		child->write = write_fs;
		child->ioctl = ioctl_fs;
		child->inode = (uint32_t)i;
		child->mask = 0666;

		fs_node_add_dirent(node, dent);
	}
	kprintf(LOG_INFO, "[chnlfs] Initialized channel filesystem");
}
