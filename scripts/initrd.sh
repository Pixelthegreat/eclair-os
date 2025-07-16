#!/bin/sh
set -e

INITRD_DIR='build/initrd-staging'
INITRD_OUTPUT='build/initrd.tar'

echo Preparing staging area...
mkdir -pv "$INITRD_DIR"
mkdir -pv "$INITRD_DIR/dev"
cp -uRv build/bin "$INITRD_DIR"
cp -uRTv base "$INITRD_DIR"

chmod +644 "$INITRD_DIR" -R

echo Archiving...
tar -cf "$INITRD_OUTPUT" -C "$INITRD_DIR" . --owner=root:0 --group=root:0 -p
