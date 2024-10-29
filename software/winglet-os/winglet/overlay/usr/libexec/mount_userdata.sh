#!/bin/sh

# ========================================
# Utility Functions
# ========================================

mount_userdata() {
    local TARGET=/var

    if [ "$1" = "tmpmnt" ]; then
        TARGET=/mnt
    fi

    mount -t ext3 "$USERPART" "$TARGET"
}

unmount_userdata() {
    sync
    umount "$USERPART"
}

emerg_shell()
{
    if ! [ "$1" = "noprompt" ]; then
        echo "Failed to Mount Userdata Partition!" >&2
    fi
    echo
    echo '!!!! FALLING BACK TO EMERGENCY SHELL !!!!'
    echo " > Exit this shell to reboot the machine"
    echo
    sh
    reboot -f
}
trap emerg_shell ERR

# ========================================
# Decode Environment
# ========================================

get_kernel_cmdline()
{
    for line in $(xargs -n1 -a /proc/cmdline); do
        if [ "$(echo "$line" | cut -d'='  -f1)" = "$1" ]; then
            # Echo to stdout, so we can fetch it
            echo "$line" | cut -d'=' -f2-
            return
        fi
    done
    return 1
}

# Read the user partition UUID
# This needs to be done by UUID as linux doesn't gaurentee consistent naming of mmc partition naming
# and there are cases where the number swaps. And according to Linux, that's acceptable behavior
# *cough* (https://lore.kernel.org/lkml/1461951139-6109-1-git-send-email-dianders@chromium.org/T/#u)
# So UUIDs are used instead

USERUUID="$(get_kernel_cmdline userdata_uuid)"
if [ -z "$USERUUID" ]; then
    echo "Missing userdata_uuid in kernel cmdline"
    emerg_shell
fi

existing_mount=$(lsblk -o PARTUUID,MOUNTPOINT | awk -v u="$USERUUID" '$1 == u {print $2}')
if [ -n "$existing_mount" ]; then
    echo "User Partition Already Mounted to '$existing_mount'!"
    emerg_shell
fi

USERPART=$(blkid --match-token "PARTUUID=$USERUUID" -o device)
if ! [ -b "$USERPART" ]; then
    echo "Could not locate user partition from PARTUUID='$USERUUID'" >&2
    emerg_shell
fi

# ========================================
# Mount Procedure
# ========================================

fsck -p "$USERPART" || true

if mount_userdata; then
    # Check if magic file to perform filesystem resize is present
    if [ -f /var/RESIZE_USERDATA ]; then
        # Need the ROOTDEV to perform filesystem operations
        ROOTDEV="/dev/$(lsblk -ndo pkname "$USERPART")"
        if ! [ -b "$ROOTDEV" ]; then
            echo "Could not locate root device from partition $USERPART" >&2
            emerg_shell
        fi

        # Sanity check filesystem layout matches what we expect before overwriting partition tables
        if ! [ "${USERPART: -1}" = "2" ]; then
            echo "User data partition expected to be partition #2, not $USERPART"
            emerg_shell
        fi

        # Need to wipe on the next reboot, since we can't partprobe the same partition as the rootfs
        # Set the correct magic files to wipe on next reboot
        rm /var/RESIZE_USERDATA
        touch /var/WIPE_USERDATA
        unmount_userdata

        echo
        echo '!!! USERDATA RESIZE REQUESTED! RESIZING THEN WIPING USERDATA !!!'
        echo '!!! USERDATA RESIZE REQUESTED! !!!' > /dev/tty0
        echo '!!! RESIZING THEN WIPING USERDATA !!!' > /dev/tty0
        echo > /dev/tty0
        echo 'Resize in progress... Will reboot after' > /dev/tty0

        # Perform filesystem resize
        sgdisk -e $ROOTDEV              # Expand to fill partition
        sgdisk -d 2 $ROOTDEV            # Delete userdata partition
        sgdisk -N 2 $ROOTDEV            # Recreate, filling as much space as possible
        sgdisk -c 2:userdata $ROOTDEV   # Set partition label
        sgdisk -G $ROOTDEV              # Regenerate UUIDs since this came from an image
        sync
        reboot -f
    fi

    if ! [ -f /var/WIPE_USERDATA ]; then
        # Mount successful, continue boot
        exit 0
    fi
    echo
    echo '!!! MANUAL USER DATA WIPE REQUESTED !!!'
    echo '!!! MANUAL USER DATA WIPE REQUESTED !!!' > /dev/tty0
    echo > /dev/tty0
    echo 'Wipe in progress... Will reboot after' > /dev/tty0
    unmount_userdata
else
    echo Failed to mount Userdata! Unable to continue boot!
    echo 'Userdata mount failed! See debug console for more info' > /dev/tty0
    while true; do
        read -p "Wipe/reinitialize userdata part? [y/n] " yn
        case $yn in
            [Yy]* ) break;;
            [Nn]* ) emerg_shell noprompt;;
            * ) echo "Please answer yes or no.";;
        esac
    done
    echo
fi

# Filesystem Wipe Procedure
echo "Wiping User Data Partition..."
mkfs.ext3 -F -L "winglet-userdata" -O ^64bit "$USERPART"
mount_userdata tmpmnt
cp -a /var/. /mnt
sync
unmount_userdata
echo "Wipe Complete! Rebooting System"
reboot -f
