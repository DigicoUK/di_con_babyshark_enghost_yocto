# Use the same restriction as initramfs-module-install
COMPATIBLE_HOST = '(x86_64.*|i.86.*|arm.*|aarch64.*|loongarch64.*)-(linux.*|freebsd.*)'

IMAGE_FSTYPES:remove = "wic.qemu-sd"

IMAGE_ROOTFS_SIZE = "8192"
IMAGE_ROOTFS_EXTRA_SPACE = "0"

# Do not pollute the initrd image with rootfs features
IMAGE_FEATURES = ""

# Don't allow the initramfs to contain a kernel
PACKAGE_EXCLUDE = "kernel-image-*"

IMAGE_NAME_SUFFIX ?= ""
IMAGE_LINGUAS = ""

IMAGE_FSTYPES = "${INITRAMFS_FSTYPES}"

CORE_IMAGE_EXTRA_INSTALL ?= ""

IMAGE_INSTALL = " \
    packagegroup-core-boot \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    \
    pregenerated-ssh-keys \
    bash \
    dropbear \
    i2c-tools \
    iproute2 \
    kernel-modules \
    libgpiod-tools \
    mtd-utils \
    unzip \
    util-linux \
    engine-updater \
    update-server \
    enghost-udev-rules \
    sharc-booter \
"

BAD_RECOMMENDATIONS += "init-ifupdown ifupdown"
USE_DEVFS = "0"
USE_VT="0"
IMAGE_DEVICE_TABLES:append = " files/enghost_device_table.txt"
INITRAMFS_MAXSIZE = "300000"

MACHINE_HWCODECS ??= ""

INITRAMFS_IMAGE = "enghost-image"
INITRAMFS_IMAGE_BUNDLE = "1"

UBOOT_ENTRYPOINT:zynq  = "0x8000"
UBOOT_LOADADDRESS:zynq = "0x8000"

# This makes kernel.bbclass symlink $deployDir/fitImage to the actual fit
# image. We don't want this as we're creating multiple fitImages that shouldn't
# conflict
# KERNEL_IMAGE_LINK_NAME ?= ""

inherit image
