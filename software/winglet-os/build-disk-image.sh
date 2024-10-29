#!/bin/bash

set -e
set -u

if [ -z "$1" ]; then
    echo "Usage: $0 [buildroot image dir]"
    exit 1
fi

IMGDIR="$1"

# Get target paramters and make sure target is okay
IMGFILE="$IMGDIR/winglet-os.img"

# Get input rootfs parameters
ROOTFS="$IMGDIR/rootfs.ext2"
UBOOT_IMG="$IMGDIR/u-boot-sunxi-with-spl.bin"
USERDATA_IMG="$IMGDIR/userdata.ext3"
USERDATA_STAGING="$IMGDIR/../userdata_staging"
ROOTFS_SIZE="$(stat --printf="%s" "$ROOTFS")"
UBOOT_SIZE="$(stat --printf="%s" "$UBOOT_IMG")"
if [ "$ROOTFS_SIZE" -eq 0 ]; then
    echo "rootfs is empty"
    exit 1
fi
if [ "$(($ROOTFS_SIZE%512))" -ne 0 ]; then
    echo "rootfs size does not fit evenly into sector count"
    exit 1
fi

SECTOR_SIZE=512          # 512 byte sectors
GPT_PADDING=34           # Number of sectors to reserve at start/end of gpt partition (should be 34)
ROOTFS_START=2048        # Start rootfs 1 MB into filesystem
USERDATA_SECTORS=32768   # 16 MB Userdata, should be enough to store empty file to tell device to resize on first boot
DD_BS_VAL=1048576        # bs value when calling dd


DD_CNT_PER_SECTOR=$(($DD_BS_VAL / $SECTOR_SIZE))
if [ "$(($DD_BS_VAL % $SECTOR_SIZE))" -ne 0 ]; then
    echo "dd bs must be divisible by sector size"
    exit 1
fi

sector_to_ddcopy() {
    if [ $((($1 * $SECTOR_SIZE) % $DD_BS_VAL)) -ne 0 ]; then
        echo "Sector count $1 does not cleanly fit into dd bs value $DD_BS_VAL"
        exit 1
    fi
    echo "$(($1 / $DD_CNT_PER_SECTOR))"
}

ROOTFS_SECTORS="$(($ROOTFS_SIZE / $SECTOR_SIZE))"
USERDATA_START="$(($ROOTFS_SECTORS + $ROOTFS_START))"
GPT_END="$(($USERDATA_START + $USERDATA_SECTORS))"
DISK_END="$(($GPT_END + $GPT_PADDING))"

# Round DISK_END to the nearest bs cpy size
DISK_END=$(((($DISK_END + $DD_CNT_PER_SECTOR - 1) / $DD_CNT_PER_SECTOR) * $DD_CNT_PER_SECTOR ))

if [ "$UBOOT_SIZE" -gt 851968 ]; then
    # U-Boot can exist from 0x20000 (where sunxi bootrom will see it) to 0xf0000 (where the environment is saved)
    echo "UBoot image does not fit into region"
    exit 1
elif [ "$UBOOT_SIZE" -eq 0 ]; then
    echo "UBoot image is empty"
    exit 1
fi

dd if=/dev/zero "of=$IMGFILE" bs=$DD_BS_VAL count=$(sector_to_ddcopy $DISK_END)

echo "Creating Partition Table"
echo "label: gpt
label-id: $(uuidgen)
device: $IMGFILE
unit: sectors
first-lba: 34
last-lba: $GPT_END
sector-size: 512

${IMGFILE}1 : start=$ROOTFS_START, size=$ROOTFS_SECTORS, type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, uuid=$(uuidgen), name=\"rootfs\"
${IMGFILE}2 : start=$USERDATA_START, size=$USERDATA_SECTORS, type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, uuid=$(uuidgen), name=\"userdata\"" | \
    sfdisk "$IMGFILE"

echo "Copying rootfs..."
dd "if=$ROOTFS" "of=$IMGFILE" seek=$(sector_to_ddcopy $ROOTFS_START) bs=$DD_BS_VAL conv=notrunc

echo "Creating userdata..."
rm -f "$USERDATA_IMG"
mkdir -p "$USERDATA_STAGING"
touch "$USERDATA_STAGING/RESIZE_USERDATA"
echo "This partition is marked to be resized and wiped on first boot. Do not add any files to this partition!" > "$USERDATA_STAGING/info.txt"

mkfs.ext3 -d "$USERDATA_STAGING" -r 1 -N 0 -m 5 -L "winglet-userdata-resize" -I 256 -O ^64bit "$USERDATA_IMG" "$(($USERDATA_SECTORS * $SECTOR_SIZE / 1024))"
dd "if=$USERDATA_IMG" "of=$IMGFILE" seek=$(sector_to_ddcopy $USERDATA_START) bs=$DD_BS_VAL conv=notrunc

rm "$USERDATA_STAGING/RESIZE_USERDATA" "$USERDATA_STAGING/info.txt"
rmdir "$USERDATA_STAGING"

#echo "Writing uboot image"
#dd "if=$UBOOT_IMG" "of=$IMGFILE" bs=4k seek=32 conv=notrunc

echo "Compressing Image..."
gzip -f "$IMGFILE"

echo Done!
