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

IMAGE_FSTYPES = "cpio.gz"

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
# INITRAMFS_MAXSIZE = "300000"

MACHINE_HWCODECS ??= ""

# INITRAMFS_IMAGE = "enghost-image"
# INITRAMFS_IMAGE_BUNDLE = "1"

UBOOT_ENTRYPOINT:zynq  = "0x8000"
UBOOT_LOADADDRESS:zynq = "0x8000"

inherit image

python __anonymous () {
    #check if there are any dtb providers
    providerdtb = d.getVar("PREFERRED_PROVIDER_virtual/dtb")
    d.appendVarFlag('do_assemble_digico_fitimage', 'depends', ' virtual/dtb:do_populate_sysroot')
    d.setVar('EXTERNAL_KERNEL_DEVICETREE', "${RECIPE_SYSROOT}/boot/devicetree")
}

do_assemble_digico_fitimage() {
    bbwarn "do_assemble_digico_fitimage"
    #if echo ${KERNEL_IMAGETYPES} | grep -wq "fitImage"; then
    #    cd ${B}
    #    fitimage_assemble fit-image.its fitImage-none ""
    #    if [ "${INITRAMFS_IMAGE_BUNDLE}" != "1" ]; then
    #        ln -sf fitImage-none ${B}/${KERNEL_OUTPUT_DIR}/fitImage
    #    fi
    #fi
}

# In image.bbclass, image's do_build task depends on virtual/kernel:do_deploy,
# so we should already have a kernel at this point
addtask assemble_digico_fitimage after do_image_complete before do_build
