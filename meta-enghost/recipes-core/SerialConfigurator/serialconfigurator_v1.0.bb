SUMMARY = "Quantum 7 Serial port application"
DESCRIPTION = "This application manages the Serial Port command from the server"
LICENSE = "CLOSED"
SECTION = "utils"

SRC_URI = "git://github.com/DigicoUK/di_con_enghost_q7_serialmgr.git;branch=babyshark;protocol=ssh;user=git"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

RDEPENDS:${PN} += "python3-core python3-pyserial"

do_install() {
    install -d ${D}/home/root/logs
    install -d ${D}/usr/local/bin
    install -m 0755 ${S}/*.py ${D}/usr/local/bin
    install -m 0755 ${S}/*.ini ${D}/usr/local/bin
}

FILES:${PN} += " \
    ${ROOT_HOME} \
    /usr/local/bin/* \
    ${ROOT_HOME}/logs\
"
