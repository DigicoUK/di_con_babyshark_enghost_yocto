DESCRIPTION = "Starts up the IP Core"
SECTION = "examples"
LICENSE = "CLOSED"

SRC_URI = " \
    file://run-dante-ip-core \
"

S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 run-dante-ip-core ${D}${bindir}
}
