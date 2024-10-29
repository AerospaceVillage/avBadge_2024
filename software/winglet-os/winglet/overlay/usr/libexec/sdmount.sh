#!/bin/sh
#
# Called from mdev
# Attemp to mount sd card block device
#

MOUNT="/bin/mount"
PMOUNT="/usr/bin/pmount"
UMOUNT="/bin/umount"

ACTION=$1
name=$2

MMCDEV=$(ls /sys/devices/platform/soc/4020000.mmc/mmc_host/)
SDCARD_FIRSTPART="mmcblk${MMCDEV#"mmc"}p1"

if ! [ "$name" = "$SDCARD_FIRSTPART" ]; then
	exit 0  # Not sdcard, don't automount, let OS deal with it
fi

if [ -z "$USER" ]; then
	exit 0
fi


DEVNAME="/dev/$name"
TARGET="/mnt/sd"

if [ $ACTION == "add" ] && [ -n "$DEVNAME" ]; then
	if cat /proc/mounts | awk '{print $1}' | grep -q "^$DEVNAME$"; then
		# Already mounted
		exit 0
	fi
	mkdir -p "$TARGET"
	if ! $MOUNT -t auto -o sync "$DEVNAME" "$TARGET"; then
		rmdir /mnt/sd
	fi
fi

if [ $ACTION == "remove" ] && [ -n "$DEVNAME" ] && [ -d "$TARGET" ]; then
	umount "$TARGET"
	rmdir "$TARGET"
fi
