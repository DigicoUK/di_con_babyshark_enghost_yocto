DESCRIPTION = "Custom fstab"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://fstab \
    file://motd \
"

do_install:append(){
    install -m 0644 ${WORKDIR}/fstab ${D}${sysconfdir}/
    install -m 0644 ${WORKDIR}/motd ${D}${sysconfdir}/motd
}