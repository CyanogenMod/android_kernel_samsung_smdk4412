#!/bin/bash

UIMAGE_PATH="arch/arm/boot/uImage"

if [ -e ${UIMAGE_PATH} ]; then
	cp ${UIMAGE_PATH} .

	SIG_MAGIC="KeRn"
	SIG_DATE=`date +%Y%m%d%H`
	SIG_PROD="SLP_Fraser"
	SIG_BOARD="proxima"
	KERN_ORG_SIZE=`stat -c %s uImage`
	KERN_PAD_SIZE=`expr 512 - $KERN_ORG_SIZE % 512`

	if [ "$KERN_PAD_SIZE" != "512" ]; then
		head -c $KERN_PAD_SIZE /dev/zero >> uImage
	fi

	echo -n $SIG_MAGIC > slp-header
	head -c $((12 - ${#SIG_MAGIC})) /dev/zero >> slp-header

	echo -n $SIG_DATE >> slp-header
	head -c $((12 - ${#SIG_DATE})) /dev/zero >> slp-header

	echo -n $SIG_PROD >> slp-header
	head -c $((24 - ${#SIG_PROD})) /dev/zero >> slp-header

	echo -n $SIG_BOARD >> slp-header
	head -c $((464 - ${#SIG_BOARD})) /dev/zero >> slp-header

	cat slp-header >> uImage

	mv uImage arch/arm/boot/uImage
	rm slp-header
fi
