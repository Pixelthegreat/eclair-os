#!/bin/sh
# script for setting up bootdisk #
bootdisk=bootdisk.img

# create an initrd #
initrd=0
if [ $1 = "initrd" ]; then

	initrd=1
fi

# check if bootdisk exists #
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

# copy files #
mkdir -pv tmp/boot/grub tmp/dev

cp -uv boot/s3b/menu.cfg tmp/boot
cp -uv build/e.clair tmp/boot

if [ $initrd -eq 0 ]; then

	cp -uRTv build/bin tmp/bin
	cp -uRTv base tmp

# initial ram disk #
else

	scripts/initrd.sh
	cp -v build/initrd.tar tmp/boot
fi

# unmount device #
umount tmp

exit 0
