SUMMARY = "Pre-generated keys for ssh login"
LICENSE = "CLOSED"

# TODO(liam): need to fix this up when a persistent flash filesystem is set up.
# We should not be shipping rsa host key with the image as that defeats the
# purpose of ssh host auth! it can be generated on first boot and then stored
# in persistent storage.
#
# It's done this way for now to avoid MITM warning from ssh due to new host
# auth each reboot

SRC_URI = " \
    file://enghost_ssh_rsa.pub \
    file://dropbear_rsa_host_key \
"

INHIBIT_DEFAULT_DEPS = "1"

S = "${WORKDIR}"

do_install () {
    # concatenate all required public keys here:
    cat ${S}/enghost_ssh_rsa.pub > ${S}/authorized_keys

    install -d ${D}${sysconfdir}/dropbear
    install -m 0600 ${S}/authorized_keys ${D}${sysconfdir}/dropbear
    install -m 0600 ${S}/dropbear_rsa_host_key ${D}${sysconfdir}/dropbear
    chmod 0700 ${D}${sysconfdir}/dropbear
}

FILES:${PN} += "${sysconfdir}/dropbear/authorized_keys ${sysconfdir}/dropbear/dropbear_rsa_host_key"
