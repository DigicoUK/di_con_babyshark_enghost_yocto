SUMMARY = "PL-PS interface rewritten as platform driver"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=cf540fc7d35b5777e36051280b3a911c"

inherit module

SRC_URI = " \
    file://yeng.c \
    file://Makefile \
    file://digico,yeng.txt \
    file://COPYING \
"

S = "${WORKDIR}"
