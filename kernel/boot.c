#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/multiboot.h>
#include <kernel/mm/paging.h>
#include <kernel/driver/fb.h>
#include <kernel/boot.h>

static boot_info_t *info = NULL;

static boot_saved_info_t saved;
static boot_cmdline_t cmdline;

static uint32_t memmap_reserved_frames = 0;

boot_protocol_t boot_protocol;

/* map structure */
static void map_structure(void) {

	if (info || !boot_data_info) return;

	page_frame_id_t fr = (uint32_t)boot_data_info / 4096;
	page_frame_use(fr);

	page_id_t page = page_breakp++;
	page_map(page, fr);

	info = PAGE_ADDR(page);
	uint32_t cnt = info->size / 4096 + 1;

	for (uint32_t i = 1; i < cnt; i++) {

		fr++;
		page_frame_use(fr);
		page_map(page_breakp++, fr);
	}
}

/* parse command line */
static void parse_cmdline(void) {

	cmdline.init_profile[0] = 0;
	if (saved.f_cmdline) {

		const char *arg = saved.cmdline;
		const char *next;
		for (; arg; arg = next? next+1: NULL) {

			next = strchr(arg, ' ');
			size_t len = next? (size_t)(next-arg): strlen(arg);

			if (!len) continue;

			/* uart as tty */
			if (!strncmp("uart-tty", arg, len))
				cmdline.uart_tty = true;

			/* no info or warning messages */
			else if (!strncmp("quiet", arg, len))
				cmdline.quiet = true;

			/* profile for init to load */
			else if (!strncmp("init-profile", arg, MIN(len, 12))) {

				if (arg[12] == '=') {

					const char *param = arg+13;
					size_t plen = len-13;

					strncpy(cmdline.init_profile, param, BOOT_CMDLINE_PARAM_MAX_CHARS);
				}
			}
		}
	}
}

/* initialize */
extern void boot_init(void) {

	if (multiboot_init())
		boot_protocol = BOOT_PROTOCOL_MULTIBOOT;
	else {
		
		boot_protocol = BOOT_PROTOCOL_S3BOOT;

		map_structure();
		if (!info) return;

		/* calculate checksum */
		uint32_t checksum = 0;
		for (uint32_t i = 4; i < info->size; i++)
			checksum += (uint32_t)(((uint8_t *)info)[i]);

		if (checksum != info->checksum) return;

		/* command line */
		if (info->offsets[BOOT_STRUCT_CMDLINE]) {

			saved.f_cmdline = true;
			saved.cmdline = (const char *)((void *)info + info->offsets[BOOT_STRUCT_CMDLINE]);
		}

		/* memory map */
		if (info->offsets[BOOT_STRUCT_MEMMAP]) {

			boot_memmap_entry_t *entry = (boot_memmap_entry_t *)((void *)info + info->offsets[BOOT_STRUCT_MEMMAP]);
			while (entry->type != BOOT_MEMMAP_ENTRY_NULL) {

				/* mark entries as used */
				if (entry->type == BOOT_MEMMAP_ENTRY_UNUSABLE) {

					uint32_t startf = entry->start >> 12;
					uint32_t endf = ALIGN(entry->end, 0x1000) >> 12;

					for (uint32_t f = startf; f < endf; f++) {

						page_frame_use(f);
						memmap_reserved_frames++;
					}
				}
				entry++;
			}
		}

		/* framebuffer */
		if (info->offsets[BOOT_STRUCT_FRAMEBUF]) {

			boot_framebuf_t *fb = (boot_framebuf_t *)((void *)info + info->offsets[BOOT_STRUCT_FRAMEBUF]);
			if (fb->type == BOOT_FRAMEBUF_TYPE_RGB) {

				saved.f_framebuf = true;
				saved.fb_addr = (void *)fb->addr;
				saved.fb_pitch = fb->pitch;
				saved.fb_width = fb->width;
				saved.fb_height = fb->height;
				saved.fb_bpp = fb->depth;

				/* map framebuffer */
				fb_format_t format;
				format.r.masksz = (uint32_t)fb->rmask_sz;
				format.r.pos = (uint32_t)fb->rmask_pos;
				format.g.masksz = (uint32_t)fb->gmask_sz;
				format.g.pos = (uint32_t)fb->gmask_pos;
				format.b.masksz = (uint32_t)fb->bmask_sz;
				format.b.pos = (uint32_t)fb->bmask_pos;
				format.bytes = ALIGN(fb->depth, 8)/8;

				fb_map(&saved, format);
			}
		}
	}
	parse_cmdline();
}

/* log boot info */
extern void boot_log(void) {

	kprintf(LOG_INFO, "[boot] Reserved frames: 0x%x", memmap_reserved_frames);
	
	boot_memmap_entry_t *entry = (boot_memmap_entry_t *)((void *)info + info->offsets[BOOT_STRUCT_MEMMAP]);
	/*while (entry->type != BOOT_MEMMAP_ENTRY_NULL) {

		kprintf(LOG_INFO, "[boot] type=0x%x, start=0x%x, end=0x%x", entry->type, entry->start, entry->end);
		entry++;
	}*/
}

/* get page mapped info */
extern boot_info_t *boot_get_info(void) {

	return info;
}

/* get useful saved info */
extern boot_saved_info_t *boot_get_saved_info(void) {
	
	return &saved;
}

/* get command line info */
extern boot_cmdline_t *boot_get_cmdline(void) {

	return &cmdline;
}
