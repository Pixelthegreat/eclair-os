#!/bin/sh
# script for setting up bootdisk #

# setup loopback device #
looppart=`sudo kpartx -l bootdisk.img | awk '{ print $1; exit }'`
loopdev=`sudo kpartx -l bootdisk.img | awk '{ print $5; exit }'`
sudo kpartx -a bootdisk.img

# mount disk image #
mkdir -pv tmp
sudo mount "/dev/mapper/$looppart" tmp

sudo mkdir -pv tmp/boot/grub
#sudo cp -v boot/grub/grub.cfg tmp/boot/grub
sudo cp -v boot/s3b/menu.cfg tmp/boot
sudo cp -v build/e.clair tmp/boot
sudo cp -v base/hello.txt tmp

# unmount device #
sudo umount tmp

sudo kpartx -d bootdisk.img

exit 0
