SUMMARY = "Recipe for  build an external akashi-reg Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = " \
    file://Makefile \
    file://Kconfig \
    file://akashi_reg.c \
    file://akashi_reg.h \
    file://COPYING \
"

S = "${WORKDIR}"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
