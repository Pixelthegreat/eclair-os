#!/bin/sh
# bootdisk generation script for eclair-os (script adapted from strange-os) #

MODULES='boot biosdisk part_msdos ext2 configfile normal multiboot2'

echo generating file...
dd if=/dev/zero of=bootdisk.img count=1 bs=16M

echo generating partition table...
sudo parted --script bootdisk.img mklabel msdos mkpart p ext2 1 16 set 1 boot on

# map disk image #
echo mapping partitions...

looppart=`sudo kpartx -l bootdisk.img | awk '{ print $1; exit }'`
loopdev=`sudo kpartx -l bootdisk.img | awk '{ print $5; exit }'`
sudo kpartx -a bootdisk.img
sleep 1

# setup fat32 fs #
echo generating fs...
sudo mkfs.ext2 "/dev/mapper/$looppart"

# mount partition #
echo mounting partition...
mkdir -pv tmp
sudo mount "/dev/mapper/$looppart" tmp

# copy files #
echo copying system files...
sudo mkdir -pv tmp/boot tmp/boot/grub
sudo cp -v boot/grub.cfg tmp/boot/grub

# install grub #
echo installing grub...
echo "(hd0) $loopdev" > boot/device.map
sudo grub-install --no-floppy --target=i386-pc --grub-mkdevicemap=boot/device.map --root-directory=tmp --boot-directory=tmp/boot --install-modules="$MODULES" --modules="$MODULES" $loopdev

# unmount #
echo unmounting...
sudo umount tmp

# unmap #
echo unmapping...
sudo kpartx -d bootdisk.img

echo done.
