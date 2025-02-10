DESCRIPTION = "Starts up the IP Core"
SECTION = "examples"
LICENSE = "CLOSED"

SRC_URI = " \
    file://dante-ip-core \
    file://no-dante-drivers-on-startup.conf \
"

S = "${WORKDIR}"

# The ipcore container looks for /etc/os-release
RDEPENDS:${PN} = "os-release"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 dante-ip-core ${D}${bindir}

	install -d ${D}${sysconfdir}/modprobe.d
	install -m 0644 no-dante-drivers-on-startup.conf ${D}${sysconfdir}/modprobe.d
}
