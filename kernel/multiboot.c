#include <kernel/types.h>
#include <kernel/tty.h>
#include <kernel/string.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fb.h>
#include <kernel/multiboot.h>

#define NBUFSZ 12
static page_frame_id_t frames[NBUFSZ]; /* buffer to store page frame numbers */

static multiboot_info_t *info = NULL;
static multiboot_saved_info_t saved;
static multiboot_cmdline_t cmdline;

/* initialize */
extern void multiboot_init(void) {

	multiboot_map_structure();

	/* read info */
	multiboot_tag_t *tagptr = info->tags;
	size_t tagsz = 0;
	while (tagptr->type != 0) {

		/* command line */
		if (tagptr->type == MULTIBOOT_TAG_CMDLINE) {

			multiboot_cmdline_tag_t *tag = (multiboot_cmdline_tag_t *)tagptr;
			saved.f_cmdline = true;
			saved.cmdline = tag->cmdline;
		}

		/* boot device */
		else if (tagptr->type == MULTIBOOT_TAG_DEV) {

			multiboot_dev_tag_t *tag = (multiboot_dev_tag_t *)tagptr;
			saved.f_bootdev = true;
			saved.biosdev = tag->biosdev;
			saved.part = tag->part;
			saved.subpart = tag->subpart;
		}

		/* memory */
		else if (tagptr->type == MULTIBOOT_TAG_MEM) {

			multiboot_mem_tag_t *tag = (multiboot_mem_tag_t *)tagptr;
			saved.f_memlayout = true;
			saved.memlow = tag->memlow;
			saved.memup = tag->memup;
		}

		/* framebuffer */
		else if (tagptr->type == MULTIBOOT_TAG_FRAMEBUFFER) {

			multiboot_framebuffer_tag_t *tag = (multiboot_framebuffer_tag_t *)tagptr;
			if (tag->type == MULTIBOOT_FB_TYPE_RGB) {

				saved.f_framebuf = true;
				saved.fb_addr = (void *)tag->loaddr;
				saved.fb_pitch = tag->pitch;
				saved.fb_width = tag->width;
				saved.fb_height = tag->height;
				saved.fb_bpp = tag->bpp;
				saved.fb_color = &tag->color;

				/* create format and map framebuffer */
				fb_format_t format;
				format.r.index = (uint32_t)(tag->color.red.fpos/8);
				format.g.index = (uint32_t)(tag->color.green.fpos/8);
				format.b.index = (uint32_t)(tag->color.blue.fpos/8);
				format.bytes = (uint32_t)tag->bpp/3;

				format.r.mask = 0xffffffff % (1 << (uint32_t)tag->color.red.masksz);
				format.g.mask = 0xffffffff % (1 << (uint32_t)tag->color.green.masksz);
				format.b.mask = 0xffffffff % (1 << (uint32_t)tag->color.blue.masksz);

				fb_map(&saved, format);
			}
		}

		/* increment */
		tagsz += ALIGN(tagptr->size, 8);
		tagptr = (void *)info->tags + tagsz;
	}

	/* parse command line */
	if (saved.f_cmdline) {

		const char *arg = saved.cmdline;
		while (arg) {

			const char *next = strchr(arg, ' ');
			size_t len = next? (size_t)(next-arg): strlen(arg);

			/* uart as tty */
			if (!strncmp("uart-tty", arg, len))
				cmdline.uart_tty = true;

			/* no info or warning messages */
			else if (!strncmp("quiet", arg, len))
				cmdline.quiet = true;

			arg = next? next+1: NULL;
		}
	}
}

/* get multiboot structure */
extern void multiboot_map_structure(void) {

	if (info) return;

	page_frame_id_t fr = (uint32_t)multiboot_data_info / 4096;
	uint32_t poff = (uint32_t)multiboot_data_info % 4096;
	
	/* determine frames to allocate */
	size_t nfrs = 1; page_id_t page;
	page_frame_id_t frs[2] = {fr, fr+1};
	page_frame_use(fr);

	page = page_breakp++;
	page_map(page, fr);

	/* structure spans multiple pages */
	if (poff + sizeof(multiboot_info_t) > 4096) {

		page_frame_use(fr+1);
		page_map(page_breakp++, fr+1);
		nfrs++;
	}
	info = PAGE_ADDR(page) + poff;

	/* allocate remaining frames */
	uint32_t frcnt = (poff + info->size) / 4096 + 1;
	frcnt = frcnt > NBUFSZ? NBUFSZ: frcnt;

	for (int i = 0; i < frcnt - nfrs; i++) {

		frames[i] = fr + nfrs + i;
		page_frame_use(frames[i]);
		page_map(page_breakp++, frames[i]);
	}
}

/* get structure */
extern multiboot_info_t *multiboot_get_structure(void) {

	return info;
}

/* get saved info */
extern multiboot_saved_info_t *multiboot_get_saved_info(void) {

	return &saved;
}

/* get command line info */
extern multiboot_cmdline_t *multiboot_get_cmdline(void) {

	return &cmdline;
}
