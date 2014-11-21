#!/bin/bash

# mkmod.sh
# Making an image file which contains driver modules(.ko files)
# The image file will be in $TMP_DIR/$BIN_NAME (usr/tmp-mod/modules.img)
# CAUTION: This script MUST be run in the root directory of linux kernel source.
# Author: Wonil Choi (wonil22.choi@samsung.com)

TMP_DIR="usr/tmp-mod"
BIN_NAME="modules.img"

make modules
if [ "$?" != "0" ]; then
	echo "Failed to make modules"
	exit 1
fi
make modules_install ARCH=arm INSTALL_MOD_PATH=${TMP_DIR}
if [ "$?" != "0" ]; then
	echo "Failed to make modules_install"
	exit 1
fi

# modules image size is dynamically determined
BIN_SIZE=`du -s ${TMP_DIR}/lib | awk {'printf $1;'}`
let BIN_SIZE=${BIN_SIZE}+1024+512 # 1 MB journal + 512 KB buffer

cd ${TMP_DIR}/lib
mkdir -p tmp
dd if=/dev/zero of=${BIN_NAME} count=${BIN_SIZE} bs=1024
mkfs.ext4 -q -F -t ext4 -b 1024 ${BIN_NAME}
sudo -n mount -t ext4 ${BIN_NAME} ./tmp -o loop
if [ "$?" != "0" ]; then
	echo "Failed to mount (or sudo)"
	exit 1
fi
cp modules/* ./tmp -rf
sudo -n chown root:root ./tmp -R
sync
sudo -n umount ./tmp
if [ "$?" != "0" ]; then
	echo "Failed to umount (or sudo)"
	exit 1
fi
mv ${BIN_NAME} ../
cd ../
rm lib -rf

