SUMMARY = "Boot the SHARCs "
DESCRIPTION = "This tool will feed the SHARCs"
LICENSE = "CLOSED"

S = "${WORKDIR}"

SRC_URI = " \
    file://Makefile \
    file://sharc-booter.c \
    file://sharc1.bin \
"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:p16380 = "${MACHINE}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 sharc-booter ${D}${bindir}

    install -d ${D}/home/root
    install -m 0666 ${S}/sharc1.bin ${D}/home/root/sharc1.bin
}

DEPENDS += "libgpiod"
RDEPENDS:${PN} += "libgpiod (>= 2.1)"

FILES:${PN} += "/home/root/*"
