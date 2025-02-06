SUMMARY = "Boot the SHARCs"
DESCRIPTION = "This tool will feed the SHARCs"
LICENSE = "CLOSED"

S = "${WORKDIR}"

SRC_URI = " \
    file://Makefile \
    file://sharc-booter.c \
    file://init-boot-sharcs \
"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:p16380 = "${MACHINE}"

inherit update-rc.d

INITSCRIPT_NAME = "init-boot-sharcs"
INITSCRIPT_PARAMS = "start 50 S ."

do_install() {
    install -d ${D}${bindir}
    install -m 0755 sharc-booter ${D}${bindir}

    install -d ${D}/etc/init.d
    install -m 0755 init-boot-sharcs ${D}/etc/init.d
}

DEPENDS += "libgpiod"
RDEPENDS:${PN} += "libgpiod (>= 2.1)"

FILES:${PN} += "${bindir}/* /etc/init.d/*"
