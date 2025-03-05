SUMMARY = "Program the Dante PLL as we buy the unprogrammed variants that the IP core can't handle"
SECTION = "utils"
LICENSE = "CLOSED"

SRC_URI = "file://init-pll"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "init-pll"
INITSCRIPT_PARAMS = "start 40 S ."

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-pll ${D}${sysconfdir}/init.d
}

FILES:${PN} += "${sysconfdir}/init.d/*"
