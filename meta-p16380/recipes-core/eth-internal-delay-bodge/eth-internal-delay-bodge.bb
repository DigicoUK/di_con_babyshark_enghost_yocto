SUMMARY = "Configure internal delay on marvell switch"
DESCRIPTION = "This is a hack to manually write switch internal delay registers for port 5"
LICENSE = "CLOSED"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:p16380 = "${MACHINE}"

SRC_URI = "file://init"

inherit update-rc.d

INITSCRIPT_NAME = "eth-switch-internal-delay"
INITSCRIPT_PARAMS = "start 5 S ."

S = "${WORKDIR}"

do_install() {
    install -d ${D}/etc/init.d
	install -m 0755 ${WORKDIR}/init ${D}${sysconfdir}/init.d/eth-switch-internal-delay
}

RDEPENDS:${PN} = " \
    mdio-netlink \
    mdio-tools \
"

FILES:${PN} += "${sysconfdir}/init.d/*"
