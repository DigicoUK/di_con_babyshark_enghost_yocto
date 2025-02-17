# Simple initramfs image. Mostly used for live images.
SUMMARY = "Small image capable of booting a device."
DESCRIPTION = "Small image capable of booting a device. The kernel includes \
the Minimal RAM-based Initial Root Filesystem (initramfs), which finds the \
first 'init' program more efficiently."

# Do not pollute the initrd image with rootfs features
IMAGE_FEATURES = ""

# Don't allow the initramfs to contain a kernel
PACKAGE_EXCLUDE = "kernel-image-*"

IMAGE_NAME_SUFFIX ?= ""
IMAGE_LINGUAS = ""

LICENSE = "MIT"

IMAGE_FSTYPES = "${INITRAMFS_FSTYPES}"
inherit core-image

IMAGE_FSTYPES:remove = "wic.qemu-sd"

IMAGE_ROOTFS_SIZE = "8192"
IMAGE_ROOTFS_EXTRA_SPACE = "0"

# Use the same restriction as initramfs-module-install
COMPATIBLE_HOST = '(x86_64.*|i.86.*|arm.*|aarch64.*|loongarch64.*)-(linux.*|freebsd.*)'

IMAGE_FEATURES += "ssh-server-dropbear"

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
# packagegroup-core-tools-debug
# usbutils

# Override core-image as it has bloat
IMAGE_INSTALL = " \
    packagegroup-core-boot \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    ${DEBUG_IMAGE_INSTALL} \
    mtd-utils \
    util-linux \
    i2c-tools \
    libgpiod-tools \
    linux-xlnx-udev-rules \
    bash \
    unzip \
    fpga-manager-script \
    iproute2 \
    engine-updater \
    kernel-modules \
    sharc-booter \
    eth-internal-delay-bodge \
    pregenerated-ssh-keys \
    kernel-module-yeng \
    enghost-application \
    rng-tools \
    rng-tools-service \
    babyshark-serialmgr \
    "
# packagegroup-dante-drivers ommitted due to no dante PL for now

BAD_RECOMMENDATIONS += "init-ifupdown ifupdown"

USE_DEVFS = "0"

IMAGE_DEVICE_TABLES:append = " files/enghost_device_table.txt"

INITRAMFS_MAXSIZE = "300000"
