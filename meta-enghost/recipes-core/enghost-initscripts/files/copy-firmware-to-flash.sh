#!/bin/sh

ROOTFS_FIRMWARE="/home/root/bundled-firmware"
FLASH_FIRMWARE="/enginefirmware"

if [ ! -d "$ROOTFS_FIRMWARE" ]; then
    echo "Source directory $ROOTFS_FIRMWARE does not exist."
    exit 1
fi

if [ ! -d "$FLASH_FIRMWARE" ]; then
    echo "Destination directory $FLASH_FIRMWARE does not exist."
    exit 1
fi

for file in "$ROOTFS_FIRMWARE"/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")

        if [ ! -e "$FLASH_FIRMWARE/$filename" ]; then
            cp "$file" "$FLASH_FIRMWARE"
            echo "Repopulated $filename from $ROOTFS_FIRMWARE to $FLASH_FIRMWARE."
        fi
    fi
done
