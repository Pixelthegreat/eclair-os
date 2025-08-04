/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECLAIR_DRIVER_AC97_H
#define ECLAIR_DRIVER_AC97_H

#include <kernel/types.h>

#define AC97_VENDORID 0x8086 /* intel */
#define AC97_DEVICEID 0x2415 /* ac97 */

#define AC97_NAM_RESET 0x00 /* reset/capabilities */
#define AC97_NAM_MASTER_VOLUME 0x02 /* master volume */
#define AC97_NAM_AUX_VOLUME 0x04 /* auxiliary volume */
#define AC97_NAM_MIC_VOLUME 0x0e /* microphone volume */
#define AC97_NAM_PCM_VOLUME 0x18 /* pcm output volume? */
#define AC97_NAM_INPUT_DEVICE 0x1a /* input device */
#define AC97_NAM_INPUT_GAIN 0x1c /* input gain */
#define AC97_NAM_MIC_GAIN 0x1e /* microphone gain */
#define AC97_NAM_EXT_CAPB 0x28 /* extended capabilities */
#define AC97_NAM_CTL_EXT_CAPB 0x2a /* control extended capabilities */
#define AC97_NAM_RATE_FRONT 0x2c /* sample rate */
#define AC97_NAM_RATE_SURR 0x2e /* sample rate */
#define AC97_NAM_RATE_LFE 0x30 /* sample rate */
#define AC97_NAM_RATE_LR 0x32 /* sample rate */

#define AC97_NABM_PCM_IN 0x00 /* pcm input box */
#define AC97_NABM_PCM_OUT 0x10 /* pcm output box */
#define AC97_NABM_MIC 0x20 /* microphone box */
#define AC97_NABM_GCR 0x2c /* global control register */
#define AC97_NABM_GSR 0x30 /* global status register */

#define AC97_VOLUME_R5 0x1f /* right volume (5-bit) */
#define AC97_VOLUME_R6 0x3f /* right volume (6-bit) */
#define AC97_VOLUME_L5 0x1f00 /* left volume (5-bit) */
#define AC97_VOLUME_L6 0x3f00 /* right volume (6-bit) */
#define AC97_VOLUME_MUTE 0x8000 /* mute both channels */

#define AC97_NABM_BOX_BDL 0x00 /* buffer descriptor list physical address */
#define AC97_NABM_BOX_NPROC 0x04 /* number of processed buffer descriptor entry */
#define AC97_NABM_BOX_NDESC 0x05 /* number of last valid buffer descriptor entry */
#define AC97_NABM_BOX_TSTAT 0x06 /* transfer status */
#define AC97_NABM_BOX_NTSAMPLES 0x08 /* number of transfered samples in entry */
#define AC97_NABM_BOX_NEXTPROC 0x0a /* number of next processed entry */
#define AC97_NABM_BOX_TCTRL 0x0b /* transfer control */

#define AC97_GCR_IE 0x1 /* interrupt enable */
#define AC97_GCR_CRESET 0x2 /* cold reset */
#define AC97_GCR_WRESET 0x4 /* warm reset */
#define AC97_GCR_SHUTDOWN 0x8 /* shutdown */
#define AC97_GCR_CHANNELS 0x300000 /* channel count (2, 4, 6) */
#define AC97_GCR_MODE 0xc00000 /* output mode (16 or 20 bits) */

#define AC97_TSTAT_DMA 0x1 /* transferring data */
#define AC97_TSTAT_EOT 0x2 /* end of transfer */
#define AC97_TSTAT_LBE 0x4 /* enable interrupt for last buffer entry */
#define AC97_TSTAT_ERR 0x8 /* enable error interrupt */
#define AC97_TSTAT_IOC 0x10 /* enable regular interrupt */

/* buffer descriptor entry */
typedef struct ac97_bdl_entry {
	uint32_t addr; /* physical address */
	uint32_t ctrl; /* number of samples and control bits */
} __attribute__((packed)) ac97_bdl_entry_t;

#define AC97_BDL_STATUS_LAST 0x40000000 /* last entry of buffer */
#define AC97_BDL_STATUS_INT 0x80000000 /* enable interrupt for entry */

/* functions */
extern void ac97_register(void); /* register pci driver */

#endif /* ECLAIR_DRIVER_AC97_H */
