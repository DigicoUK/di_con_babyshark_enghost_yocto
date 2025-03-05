# Simple initramfs image. Mostly used for live images.
SUMMARY = "Main linux image for babyshark"
DESCRIPTION = "Includes base and runtime services"
LICENSE = "MIT"

inherit engine-host-image

IMAGE_FEATURES += " \
    allow-empty-password \
    allow-root-login \
    empty-root-password \
    serial-autologin-root \
"

DEBUG_IMAGE_INSTALL = " \
    e2fsprogs \
    tcpdump \
    devmem2 \
    ethtool \
    spidev-test \
    picocom \
"

# Override core-image as it has bloat
IMAGE_INSTALL += " \
    ${DEBUG_IMAGE_INSTALL} \
    kernel-module-yeng \
    enghost-application \
    babyshark-serialmgr \
    packagegroup-dante \
    "

# KERNEL_IMAGE_LINK_NAME = "main"
