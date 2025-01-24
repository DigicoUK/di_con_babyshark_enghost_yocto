SUMMARY = "Init scripts for enghost"
LICENSE = "CLOSED"

INHIBIT_DEFAULT_DEPS = "1"

SRC_URI = " \
    file://init-networking.sh \
    file://mount-filesystems.sh \
    file://lowlevel-system-init.sh \
"

DEPENDS:append = " update-rc.d-native"
RDEPENDS:${PN} = "iproute2"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-networking.sh ${D}${sysconfdir}/init.d
    install -m 0755 mount-filesystems.sh ${D}${sysconfdir}/init.d
    install -m 0755 lowlevel-system-init.sh ${D}${sysconfdir}/init.d

    update-rc.d -r ${D} lowlevel-system-init.sh start 2 S .
    update-rc.d -r ${D} mount-filesystems.sh start 2 S .
    update-rc.d -r ${D} init-networking.sh start 5 S .
}


FILES:${PN} = " \
    ${sysconfdir}/init.d/ \
    ${sysconfdir}/rcS.d/* \
"
