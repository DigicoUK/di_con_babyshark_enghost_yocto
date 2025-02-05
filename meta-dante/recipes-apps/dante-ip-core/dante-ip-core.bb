SUMMARY = "Audiante Dante IPcore application"
SECTION = "PETALINUX/apps"
LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

FILES:${PN} += "/home/root/*"
FILES:${PN} += "/etc/*"
FILES:${PN} += "/data/*"
FILES:${PN} += "/dante/*"
FILES:${PN} += "/*"

RDEPENDS_${PN} += "bash"

INSANE_SKIP_${PN}:append = "already-stripped"


IPCORE_VERSION = "4.2.7.4"

SRC_URI = " \
    file://cap.bin \
    file://blacklist.conf \
    file://ipcore-4.2.7.4_arm_Linux_hc.tgz;unpack=0 \
    file://ethmaddr.conf \
    file://unpack_ipcore.sh \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}/data
    install -d ${D}/dante

    install -d ${D}/dante/config/akashi
    install -m 0666 ${S}/ethmaddr.conf ${D}/dante/config/akashi/ethmaddr.conf

    install -d ${D}/dante/cap
    install -m 0666 ${S}/cap.bin ${D}/dante/cap/cap.bin

    install -d ${D}/home/root
    install -m 0666 ${S}/ipcore-${IPCORE_VERSION}_arm_Linux_hc.tgz ${D}/home/root/ipcore-${IPCORE_VERSION}_arm_Linux_hc.tgz
    install -m 0755 ${S}/unpack_ipcore.sh ${D}/home/root/unpack_ipcore.sh

    install -d ${D}/etc/modprobe.d
    install -m 0666 ${S}/blacklist.conf ${D}/etc/modprobe.d/blacklist.conf
}

