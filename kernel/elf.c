/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <kernel/task.h>
#include <kernel/mm/paging.h>
#include <kernel/vfs/fs.h>
#include <ec/elf.h>
#include <kernel/elf.h>

static fs_node_t *dummy = NULL; /* dummy node to synchronize task loading */
static int res = 0; /* result */

/* entry point */
static void load_entry() {

	/* locate file */
	fs_node_t *node = fs_resolve(task_active->load.path);
	if (!node) {

		kprintf(LOG_WARNING, "[elf] Failed to load executable file '%s'", task_active->load.path);
		res = -1; /* should be -ENOENT */
		task_unlockcli();
		task_terminate();
	}

	/* open file */
	if (node->refcnt && !(node->oflags & FS_READ)) {

		kprintf(LOG_WARNING, "[elf] Failed to open file '%s' as readable", task_active->load.path);
		res = -1; /* should (probably) be -EBUSY */
		task_unlockcli();
		task_terminate();
	}
	if (node->refcnt) fs_open(node, node->oflags);
	else fs_open(node, FS_READ);

	/* validate header */
	elf32_header_t ehdr;
	char mag[4] = ELF_MAG_BYTES;

	if (fs_read(node, 0, sizeof(ehdr), (uint8_t *)&ehdr) != sizeof(ehdr) || !!memcmp(&ehdr.ident, mag, 4)) {

		kprintf(LOG_WARNING, "[elf] Failed to recognize '%s' as an ELF file", task_active->load.path);
		res = -1; /* maybe -EINVAL? */
		task_unlockcli();
		task_terminate();
	}

	if (ehdr.ident[ELF_IDENT_CLASS] != ELF_CLASS_32 || ehdr.ident[ELF_IDENT_DATA] != ELF_DATA_2LSB ||
	    ehdr.type != ELF_TYPE_EXEC || ehdr.machine != ELF_MACHINE_386 || ehdr.version != ELF_VERSION_CURRENT) {

		kprintf(LOG_WARNING, "[elf] Failed to load unsupported ELF file '%s'", task_active->load.path);
		res = -1; /* maybe -EINVAL? */
		task_unlockcli();
		task_terminate();
	}

	/* read program headers */
	for (elf32_half_t i = 0; i < ehdr.phnum; i++) {

		elf32_program_header_t phdr;
		uint32_t offset = (uint32_t)ehdr.phoff + (uint32_t)i * (uint32_t)ehdr.phentsize;
		
		if (fs_read(node, offset, sizeof(phdr), (uint8_t *)&phdr) != sizeof(phdr)) {

			kprintf(LOG_WARNING, "[elf] Failed to read program header %d from '%s'", (int)i, task_active->load.path);
			res = -1; /* maybe -EINVAL? */
			task_unlockcli();
			task_terminate();
		}

		if (phdr.type != ELF_PH_TYPE_LOAD) continue;

		/* allocate memory for program data */
		elf32_addr_t end = phdr.paddr + phdr.memsz;
		if (end > task_active->brkp) {

			uint32_t startp = task_active->brkp >> 12;
			uint32_t endp = ALIGN(end, 4096) >> 12;

			for (uint32_t i = startp; i < endp; i++) {
				if (!page_is_mapped(i))
					page_map_flags(i, page_frame_alloc(), PAGE_FLAG_US);
			}
			task_active->brkp = end;
		}

		/* load program data */
		if (fs_read(node, phdr.offset, (size_t)phdr.filesz, (uint8_t *)phdr.paddr) != (kssize_t)phdr.filesz) {

			kprintf(LOG_WARNING, "[elf] Failed to read program data from '%s'", task_active->load.path);
			res = -1; /* maybe -EINVAL? */
			task_unlockcli();
			task_terminate();
		}

		void *bss_addr = (void *)phdr.paddr + phdr.filesz;
		size_t bss_size = (size_t)(phdr.memsz - phdr.filesz);
		memset(bss_addr, 0, bss_size);
	}

	task_active->entp = ehdr.entry;

	fs_close(node);

	res = 1;
	task_entry();
}

/* load an executable */
extern int elf_load_task(const char *path, const char **argv, const char **envp, bool freeargs) {

	task_lockcli();
	if (!dummy)
		dummy = fs_node_new(NULL, 0);
	task_unlockcli();

	task_acquire(dummy);

	/* create task */
	res = 0;
	task_lockcli();
	task_t *task = task_new(NULL, load_entry);
	int pid = task->id;
	
	task->load.path = path;
	task->argv = argv;
	task->envp = envp;
	task->freeargs = freeargs;
	
	task_unlockcli();
	while (!res);
	int pres = res;
	res = 0;

	/* return result */
	task_release();

	if (pres < 0) return pres;
	return pid;
}
