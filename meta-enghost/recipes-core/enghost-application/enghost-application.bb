SUMMARY = "(Q7) Enghost application"
DESCRIPTION = "This is the main Q7 app"
LICENSE = "CLOSED"
SECTION = "utils"
SRC_URI = " \
    git://github.com/DigicoUK/di_con_enghost_q7_application.git;branch=babyshark-test;protocol=ssh;user=git \
    file://babyshark-appcomms-tcp-server \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/babyshark-appcomms-tcp-server ${D}${bindir}
}

FILES:${PN}-dbg += "${bindir}/.debug"
