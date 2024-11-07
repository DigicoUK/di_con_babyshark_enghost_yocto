SUMMARY = "Recipe for  build an external akashi-temac Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

INHIBIT_PACKAGE_STRIP = "1"

SRC_URI = " \
    file://COPYING \
    file://Makefile \
    file://Kconfig \
    file://akashi_temac.c \
    file://akashi_temac.h \
    file://digico_kconfig_defines.h \
    file://smi_config.h \
    file://switch_lib.h \
    file://switch_lib_linux.c \
    file://switch_lib_reg.h \
    file://switch_lib_shared.c \
    file://switch_lib_uboot.c \
    file://switch_lib_utils.c \
    file://zynq/switch_lib_aud_zynq.h \
    file://zynq_interface.c \
    file://zynq_ps_mac.c \
    file://zynq_ps_mac.h \
"

S = "${WORKDIR}"