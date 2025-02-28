#!/bin/sh
# bootdisk generation script for eclair-os #

# exit in case of an error #
exit_on_error() {

	error=$?
	if [ $error != 0 ]; then
		echo "Error $error; Exiting..."
		exit $error
	fi
}

echo Generating file...
dd if=/dev/zero of=bootdisk.img count=1 bs=16M
exit_on_error

echo Generating partition table...
sudo parted --script bootdisk.img mklabel msdos mkpart p ext2 1 16 set 1 boot on
exit_on_error

# map disk image #
echo Mapping partitions...

looppart=`sudo kpartx -l bootdisk.img | awk '{ print $1; exit }'`
loopdev=`sudo kpartx -l bootdisk.img | awk '{ print $5; exit }'`
sudo kpartx -a bootdisk.img
exit_on_error
sleep 1

# setup fat32 fs #
echo Generating fs...
sudo mkfs.ext2 "/dev/mapper/$looppart"
exit_on_error

# mount partition #
echo Mounting partition...
mkdir -pv tmp
sudo mount "/dev/mapper/$looppart" tmp

# copy files #
echo Copying system files...
sudo mkdir -pv tmp/boot tmp/boot
sudo cp -v boot/s3b/menu.cfg tmp/boot

# unmount #
echo Unmounting...
sudo umount tmp

# unmap #
echo Unmapping...
sudo kpartx -d bootdisk.img

# install bootloader #
echo Installing bootloader...
cp build/boot/stage0.bin build/boot.bin
dd if=build/boot/stage1.bin of=build/boot.bin bs=512 seek=1 conv=notrunc
build/bootimage

echo Done.
