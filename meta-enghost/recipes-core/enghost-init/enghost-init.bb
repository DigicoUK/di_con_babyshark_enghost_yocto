SUMMARY = "Init scripts for enghost"
LICENSE = "CLOSED"

SRC_URI = " \
    file://init-networking \
"

INITSCRIPT_PACKAGES = "${PN}-networking"
INITSCRIPT_NAME:${PN}-networking = "init-networking"
INITSCRIPT_PARAMS:${PN}-networking = "defaults 5 95"

inherit update-rc.d

RDEPENDS:${PN}-networking = "iproute2"

S = "${WORKDIR}"

do_install() {
    install -d ${D}/etc/init.d
    install -m 0755 init-networking ${D}${sysconfdir}/init.d
}

PACKAGES =+ "${PN}-networking"

FILES:${PN}-networking = "${sysconfdir}/init.d/init-networking"
