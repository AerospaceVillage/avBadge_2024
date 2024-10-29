#!/bin/sh

set -e

if [ -z "${TARGET_DIR}" ] || [ -z "${PROJECT_VARIANT_DIR}" ] || [ -z "$2" ]; then
	echo "This script must be ran as part of buildroot"
	exit 1
fi

echo Customizing OS Release File...
OS_RELEASE="${TARGET_DIR}/usr/lib/os-release"

# Override the PRETTY_NAME paramter in the OS_RELEASE file
sed -i "s/PRETTY_NAME=.*/PRETTY_NAME=\"Winglet OS $2\"/g" "$OS_RELEASE"

# Set all additional fields in the os-release file
( \
	echo "IMAGE_ID=winglet-os"; \
	echo "IMAGE_VERSION=$(date '+%Y%m%dT%H%M%S%z')"; \
	COMMIT_HASH=$(git -C "$PROJECT_VARIANT_DIR" rev-parse --verify HEAD) 2> /dev/null && echo "IMAGE_HASH=$COMMIT_HASH"; \
	echo 'VENDOR_NAME="Aerospace Village"'; \
	echo 'VENDOR_URL="https://www.aerospacevillage.org/"' \
) >> "$OS_RELEASE"

# Create uboot environment image in /var so it'll be copied to the second partition during flash
echo Creating uboot env...
mkenvimage -s 0x10000 -o "${TARGET_DIR}/var/uboot.env" "${PROJECT_VARIANT_DIR}/uboot.env"

echo Extracting Tiles
mkdir -p "${TARGET_DIR}/opt/winglet-gui/maps"
tar -C "${TARGET_DIR}/opt/winglet-gui/maps"  --strip-components=1 -xf "${PROJECT_VARIANT_DIR}/../LVTiles.tar.xz"

echo Additional Fixups...
# Change root home directory to /var/root
sed -i 's#:/root#:/var/root#g' "${TARGET_DIR}/etc/passwd"
# Create ssh folder in /var/lib (Since we can't commit empty folders)
mkdir -p "${TARGET_DIR}/var/lib/ssh"
