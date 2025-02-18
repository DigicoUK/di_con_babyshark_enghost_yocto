SUMMARY = "Busybox UDHCPC config and initscripts for dante interfaces"
DESCRIPTION = "Provides udhcpc script files and startup jobs"
LICENSE = "CLOSED"

S = "${WORKDIR}"

SRC_URI = " \
    file://init-dhcpcd \
"

inherit update-rc.d

INITSCRIPT_NAME = "init-dhcpcd"
INITSCRIPT_PARAMS = "start 41 S ."

RDEPENDS:${PN} += "dhcpcd"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-dhcpcd ${D}${sysconfdir}/init.d
}

FILES:${PN} += " \
    ${sysconfdir}/init.d/* \
    ${sysconfdir}/* \
"
