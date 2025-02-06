SUMMARY = "Init scripts for enghost"
LICENSE = "CLOSED"

INHIBIT_DEFAULT_DEPS = "1"

SRC_URI = " \
    file://init-dmesg-logrotate.sh \
    file://init-hostname.sh \
    file://init-misc-system.sh \
    file://init-networking.sh \
    file://logrotate-dmesg.conf \
    file://mount-filesystems.sh \
    file://populate-volatile.sh \
    file://volatiles \
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
    #install -m 0755 init-dmesg-logrotate.sh ${D}${sysconfdir}/init.d
    #install -m 0644 logrotate-dmesg.conf ${D}${sysconfdir}/

    # TODO this is adapted from initscripts, but this is a much simpler system
    # and we should be able to just make the volatile directories in the rootfs
    # manually
    install -d ${D}${sysconfdir}/default
    install -d ${D}${sysconfdir}/default/volatiles
    install -m 0644 ${WORKDIR}/volatiles ${D}${sysconfdir}/default/volatiles/00_core
    install -m 0755 ${WORKDIR}/populate-volatile.sh ${D}${sysconfdir}/init.d

    update-rc.d -r ${D} init-misc-system.sh start 2 S .
    update-rc.d -r ${D} mount-filesystems.sh start 2 S .
    update-rc.d -r ${D} init-networking.sh start 5 S .
	update-rc.d -r ${D} populate-volatile.sh start 37 S .
    update-rc.d -r ${D} init-hostname.sh start 39 S .
    #update-rc.d -r ${D} init-dmesg-logrotate.sh start 39 S .
}


FILES:${PN} = " \
    ${sysconfdir}/init.d/ \
    ${sysconfdir}/rcS.d/* \
    ${sysconfdir}/* \
"
