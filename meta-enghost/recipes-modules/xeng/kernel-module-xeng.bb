SUMMARY = "Example of how to build an external Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = " \
    file://fpga_access.c \
    file://fpga_access.h \
    file://xeng.c \
    file://xeng.h \
    file://Makefile \
    file://COPYING \
"

S = "${WORKDIR}"

RPROVIDES_${PN} += "kernel-module-xeng"
