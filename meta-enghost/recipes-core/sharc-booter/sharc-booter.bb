SUMMARY = "Boot the SHARCs "
DESCRIPTION = "This tool will feed the SHARCs"
LICENSE = "CLOSED"

S = "${WORKDIR}"

SRC_URI = " \
    file://Makefile \
    file://sharc-booter.c \
    file://sharc1.bin \
    file://sharc2.bin \
    file://sharc4.bin \
    file://boot-all-sharcs.sh \
"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:p16380 = "${MACHINE}"

inherit update-rc.d

INITSCRIPT_NAME = "boot-all-sharcs.sh"
INITSCRIPT_PARAMS = "defaults 10 90"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 sharc-booter ${D}${bindir}

    install -d ${D}/etc/init.d
    install -m 0755 boot-all-sharcs.sh ${D}/etc/init.d

    install -d ${D}/home/root
    install -m 0666 ${S}/sharc1.bin ${D}/home/root/sharc1.bin
    install -m 0666 ${S}/sharc2.bin ${D}/home/root/sharc2.bin
    install -m 0666 ${S}/sharc4.bin ${D}/home/root/sharc4.bin
}

DEPENDS += "libgpiod"
RDEPENDS:${PN} += "libgpiod (>= 2.1)"

FILES:${PN} += "/home/root/* /etc/init.d/*"
