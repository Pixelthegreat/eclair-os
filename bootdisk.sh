#!/bin/sh
# script for setting up bootdisk #

cd build

# copy image #
cp ../bootdisk.img ./

# setup loopback device #
looppart=`sudo kpartx -l bootdisk.img | awk -e '{ print $1; exit }'`
loopdev=`sudo kpartx -l bootdisk.img | awk -e '{ print $5; exit }'`
sudo kpartx -a bootdisk.img

# mount disk image #
mkdir -pv ../tmp
sudo mount "/dev/mapper/$looppart" ../tmp

sudo mkdir -pv ../tmp/boot ../tmp/boot/grub
sudo cp -v ../boot/grub.cfg ../tmp/boot/grub
sudo cp -v ./e.clair ../tmp/boot

# unmount device #
sudo umount ../tmp

sudo kpartx -d bootdisk.img

# exit #
cd ..
exit 0
