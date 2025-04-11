#!/bin/sh
# script for setting up bootdisk #
bootdisk=bootdisk.img

stat "$bootdisk" 1>/dev/null 2>/dev/null
if [ $? != 0 ]; then

	echo \'$bootdisk\' does not yet exist!
	exit 1
fi

# exit in case of an error #
exit_on_error() {

	error=$?
	if [ $error != 0 ]; then
		echo "Error $error; Exiting..."
		exit $error
	fi
}

# mount disk image #
mkdir -pv tmp
build/mntecfs "$bootdisk" tmp -m
exit_on_error

mkdir -pv tmp/boot/grub tmp/dev tmp/bin

cp -v boot/s3b/menu.cfg tmp/boot
cp -v build/e.clair tmp/boot
cp -v base/hello.txt tmp
cp -RTv build/bin tmp/bin

# unmount device #
umount tmp

exit 0
