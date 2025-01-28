SUMMARY = "Init scripts for enghost"
LICENSE = "CLOSED"

INHIBIT_DEFAULT_DEPS = "1"

SRC_URI = " \
    file://init-hostname.sh \
    file://init-misc-system.sh \
    file://init-networking.sh \
    file://mount-filesystems.sh \
    file://init-dmesg-logrotate.sh \
    file://logrotate-dmesg.conf \
"

DEPENDS:append = " update-rc.d-native"
RDEPENDS:${PN} = "iproute2"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-networking.sh ${D}${sysconfdir}/init.d
    install -m 0755 mount-filesystems.sh ${D}${sysconfdir}/init.d
    install -m 0755 init-misc-system.sh ${D}${sysconfdir}/init.d
    install -m 0755 init-hostname.sh ${D}${sysconfdir}/init.d
    install -m 0755 init-dmesg-logrotate.sh ${D}${sysconfdir}/init.d
    install -m 0644 ${WORKDIR}/logrotate-dmesg.conf ${D}${sysconfdir}/

    update-rc.d -r ${D} init-misc-system.sh start 2 S .
    update-rc.d -r ${D} mount-filesystems.sh start 2 S .
    update-rc.d -r ${D} init-networking.sh start 5 S .
    update-rc.d -r ${D} init-hostname.sh start 39 S .
    update-rc.d -r ${D} init-dmesg-logrotate.sh start 39 S .
}


FILES:${PN} = " \
    ${sysconfdir}/init.d/ \
    ${sysconfdir}/rcS.d/* \
    ${sysconfdir}/* \
"
