SUMMARY = "Init scripts for enghost"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

RDEPENDS:${PN} = "busybox"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}
    install -m 0755 ${WORKDIR}/init ${D}
    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}
}

FILES:${PN} = "/init ${sysconfdir}/rc.local"
RCONFLICTS:${PN} = "systemd"
