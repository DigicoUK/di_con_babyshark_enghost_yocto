FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://fsbl-p16380.elf"

FSBL_FILE := "${THISDIR}/files/fsbl-p16380"
FSBL_DEPENDS = ""
FSBL_MCDEPENDS = ""

inherit deploy

# do_deploy() {
#     install -Dm 0644 ${B}/ ${DEPLOYDIR}/${FSBL_BASE_NAME}.elf
#     ln -sf ${FSBL_BASE_NAME}.elf ${DEPLOYDIR}/${FSBL_IMAGE_NAME}.elf
# }

