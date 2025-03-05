SUMMARY = "Backup linux image for babyshark"
DESCRIPTION = "Includes base only"
LICENSE = "CLOSED"

inherit engine-host-image

IMAGE_FEATURES += " \
    allow-empty-password \
    allow-root-login \
    empty-root-password \
    serial-autologin-root \
"

IMAGE_INSTALL += ""

# KERNEL_IMAGE_LINK_NAME = "main"
