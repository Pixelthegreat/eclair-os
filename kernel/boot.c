#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/multiboot.h>
#include <kernel/mm/paging.h>
#include <kernel/boot.h>

static boot_info_t *info = NULL;

static boot_saved_info_t saved;
static boot_cmdline_t cmdline;

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
	}
	parse_cmdline();
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
