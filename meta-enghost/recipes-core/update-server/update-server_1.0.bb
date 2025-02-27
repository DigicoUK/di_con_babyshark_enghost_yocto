SUMMARY = "CoAP update server for babyshark"
SECTION = "utils"
LICENSE = "CLOSED"

SRC_URI = " \
    git://github.com/DigicoUK/di_con_enghost_update_server.git;branch=main;protocol=ssh;user=git \
    file://coap-update-server-daemon \
    file://init-coap-update-server \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake update-rc.d

DEPENDS += "libgpiod"
RDEPENDS:${PN} += "libgpiod (>= 2.1)"

DEPENDS += "libcoap"
RDEPENDS:${PN} += "libcoap (>= 4.3.4)"

INITSCRIPT_NAME = "init-coap-update-server"
INITSCRIPT_PARAMS = "defaults 50 50"

do_install:append() {
    install ${WORKDIR}/coap-update-server-daemon ${D}${bindir}/

    install -d ${D}/etc/init.d
    install -m 0755 ${WORKDIR}/init-coap-update-server ${D}${sysconfdir}/init.d/
}

FILES:${PN} += " \
    ${bindir}/* \
    /etc/init.d/* \
"
