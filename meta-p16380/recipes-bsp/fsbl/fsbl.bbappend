FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://fsbl-p16380.elf"

FSBL_FILE := "${THISDIR}/files/fsbl-p16380"
FSBL_DEPENDS = ""
FSBL_MCDEPENDS = ""
