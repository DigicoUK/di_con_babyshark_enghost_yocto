SUMMARY = "Udev rules file"
DESCRIPTION = "udev rules"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "\
    file://80-net-name-slot.rules \
    file://79-rename-zynq-ps-eth.rules \
    file://85-mtd-flash-links.rules \
"

S = "${WORKDIR}"

do_install () {
    install -d ${D}${sysconfdir}/udev/rules.d
    # It needs to be named this to override the default persistent-ethernet-name rule
    install -m 0644 80-net-name-slot.rules ${D}${sysconfdir}/udev/rules.d/
    install -m 0644 79-rename-zynq-ps-eth.rules ${D}${sysconfdir}/udev/rules.d/
    install -m 0644 85-mtd-flash-links.rules  ${D}${sysconfdir}/udev/rules.d/
}

FILES:${PN} += "${sysconfdir}/udev/rules.d/*"

