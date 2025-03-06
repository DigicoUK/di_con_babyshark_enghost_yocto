SUMMARY = "Serial manager application for babyshark"
SECTION = "utils"
LICENSE = "CLOSED"

SRC_URI = " \
    git://github.com/DigicoUK/di_con_enghost_babyshark_serialmgr.git;branch=main;protocol=ssh;user=git \
    file://babyshark-serialclient \
    file://daemon \
    file://init \
    file://nuke-appcomms \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake update-rc.d

DEPENDS += "libgpiod"
RDEPENDS:${PN} += "iproute2"
RDEPENDS:${PN} += "libgpiod (>= 2.1)"

INITSCRIPT_NAME = "init-serialclient"
INITSCRIPT_PARAMS = "defaults 50 50"

do_install:append() {
    # may contain special paths
    cat ${WORKDIR}/babyshark-serialclient | \
        sed -e 's,/usr/sbin/,${sbindir}/,g' \
            -e 's,/usr/bin/,${bindir}/,g' \
            -e 's,/usr/lib/,${libdir}/,g' \
            -e 's,/etc/,${sysconfdir}/,g' \
            -e 's,/usr/,${prefix}/,g' > ${D}${bindir}/babyshark-serialclient
    chmod 0755 ${D}${bindir}/babyshark-serialclient

    install ${WORKDIR}/daemon ${D}${bindir}/babyshark-serialclient-daemon
    install ${WORKDIR}/nuke-appcomms ${D}${bindir}

    install -d ${D}/etc/init.d
    install -m 0755 ${WORKDIR}/init ${D}${sysconfdir}/init.d/init-serialclient
}

FILES:${PN} += " \
    ${bindir}/babyshark-serialclient \
    ${bindir}/babyshark-serialclient-daemon \
    /etc/init.d/* \
"
