#ifndef ECLAIR_DRIVER_ATA_H
#define ECLAIR_DRIVER_ATA_H

#define ATA_PORT_PRIMARY 0x1f0
#define ATA_PORT_SECONDARY 0x170

/* offsets from the ports defined above */
#define ATA_PORT_DATA 0
#define ATA_PORT_ERROR 1
#define ATA_PORT_SECTOR_COUNT 2
#define ATA_PORT_LBA_LOW 3
#define ATA_PORT_LBA_MID 4
#define ATA_PORT_LBA_HIGH 5
#define ATA_PORT_DEVICE 6
#define ATA_PORT_COMMAND 7

/* status flags */
#define ATA_STATUS_ERR 0x1
#define ATA_STATUS_DRQ 0x8
#define ATA_STATUS_CMD 0x10
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_BSY 0x80

/* device commands */
#define ATA_COMMAND_IDENTIFY 0xec

/* functions */
extern void ata_init(void); /* initialize */

#endif /* ECLAIR_DRIVER_ATA_H */
