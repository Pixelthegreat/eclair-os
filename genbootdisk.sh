#!/bin/sh
# bootdisk generation script for eclair-os #
bootdisk=bootdisk.img

# exit in case of an error #
exit_on_error() {

	error=$?
	if [ $error != 0 ]; then
		echo "Error $error; Exiting..."
		exit $error
	fi
}

echo Generating file...
dd if=/dev/zero of="$bootdisk" count=1 bs=16M
exit_on_error

# setup file system and partition table #
echo Generating partition table and fs...
build/mkecfs "$bootdisk" -b 1024 -n 'eclair-os' -m 1024
exit_on_error

# mount partition #
echo Mounting partition...
mkdir -pv tmp
build/mntecfs "$bootdisk" tmp -m
exit_on_error

# copy files #
echo Copying system files...
mkdir -pv tmp/boot tmp/boot
cp -v boot/s3b/menu.cfg tmp/boot

# unmount #
echo Unmounting...
umount tmp

# install bootloader #
stat build/boot/stage0.bin 1>/dev/null 2>/dev/null
if [ $? != 0 ]; then

	echo Done.
	exit 0
fi

echo Installing bootloader...
cp build/boot/stage0.bin build/boot.bin
dd if=build/boot/stage1.bin of=build/boot.bin bs=512 seek=1 conv=notrunc
build/bootimage

echo Done.
