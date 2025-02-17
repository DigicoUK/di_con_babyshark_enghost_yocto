DESCRIPTION = "Starts up the IP Core"
SECTION = "examples"
LICENSE = "CLOSED"

SRC_URI = " \
    file://init-dante-ip-core \
    file://no-dante-drivers-on-startup.conf \
    file://view-dante-logs \
"

S = "${WORKDIR}"

# The ipcore container looks for /etc/os-release
RDEPENDS:${PN} = "os-release"

inherit update-rc.d

INITSCRIPT_NAME = "init-dante-ip-core"
INITSCRIPT_PARAMS = "defaults 90 10"

do_install() {
    install -d ${D}${sysconfdir}/modprobe.d
    install -m 0644 no-dante-drivers-on-startup.conf ${D}${sysconfdir}/modprobe.d

    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-dante-ip-core ${D}${sysconfdir}/init.d

    install -d ${D}${bindir}
    install -m 0755 view-dante-logs ${D}${bindir}
}

FILES:${PN} += " \
    ${sysconfdir}/modprobe.d/* \
    ${sysconfdir}/init.d/* \
"
