SUMMARY = "Init scripts for enghost"
LICENSE = "CLOSED"

INHIBIT_DEFAULT_DEPS = "1"

SRC_URI = " \
    file://init-dmesg-logrotate \
    file://init-hostname \
    file://init-misc-system \
    file://init-ps-ethernet-mac-address \
    file://logrotate-dmesg.conf \
    file://mount-filesystems \
    file://populate-volatile \
    file://volatiles \
"

DEPENDS:append = " update-rc.d-native"
RDEPENDS:${PN} = "iproute2"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-ps-ethernet-mac-address ${D}${sysconfdir}/init.d
    install -m 0755 mount-filesystems ${D}${sysconfdir}/init.d
    install -m 0755 init-misc-system ${D}${sysconfdir}/init.d
    install -m 0755 init-hostname ${D}${sysconfdir}/init.d
    #install -m 0755 init-dmesg-logrotate ${D}${sysconfdir}/init.d
    #install -m 0644 logrotate-dmesg.conf ${D}${sysconfdir}/

    # TODO this is adapted from initscripts, but this is a much simpler system
    # and we should be able to just make the volatile directories in the rootfs
    # manually
    install -d ${D}${sysconfdir}/default
    install -d ${D}${sysconfdir}/default/volatiles
    install -m 0644 ${WORKDIR}/volatiles ${D}${sysconfdir}/default/volatiles/00_core
    install -m 0755 ${WORKDIR}/populate-volatile ${D}${sysconfdir}/init.d

    update-rc.d -r ${D} init-misc-system start 2 S .
    update-rc.d -r ${D} mount-filesystems start 2 S .
    update-rc.d -r ${D} init-ps-ethernet-mac-address start 5 S .
	update-rc.d -r ${D} populate-volatile start 37 S .
    update-rc.d -r ${D} init-hostname start 39 S .
    #update-rc.d -r ${D} init-dmesg-logrotate start 39 S .
}


FILES:${PN} = " \
    ${sysconfdir}/init.d/ \
    ${sysconfdir}/rcS.d/* \
    ${sysconfdir}/* \
"
