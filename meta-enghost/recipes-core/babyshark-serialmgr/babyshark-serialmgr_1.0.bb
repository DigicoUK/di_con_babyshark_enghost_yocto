SUMMARY = "Serial manager application for babyshark"
SECTION = "utils"
LICENSE = "CLOSED"

SRC_URI = " \
    git://github.com/DigicoUK/di_con_enghost_babyshark_serialmgr.git;branch=main;protocol=ssh;user=git \
    file://init \
    file://daemon \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake update-rc.d

RDEPENDS:${PN} = "iproute2"

INITSCRIPT_NAME = "init-serialclient"
INITSCRIPT_PARAMS = "defaults 50 50"

do_install:append() {
    # may contain special paths
    cat ${S}/babyshark-serialclient.sh | \
        sed -e 's,/usr/sbin/,${sbindir}/,g' \
            -e 's,/usr/bin/,${bindir}/,g' \
            -e 's,/usr/lib/,${libdir}/,g' \
            -e 's,/etc/,${sysconfdir}/,g' \
            -e 's,/usr/,${prefix}/,g' > ${D}${bindir}/babyshark-serialclient
    chmod 0755 ${D}${bindir}/babyshark-serialclient

    install ${WORKDIR}/daemon ${D}${bindir}/babyshark-serialclient-daemon

    install -d ${D}/etc/init.d
    install -m 0755 ${WORKDIR}/init ${D}${sysconfdir}/init.d/init-serialclient
}

FILES:${PN} += "${bindir}/babyshark-serialclient"
FILES:${PN} += "${bindir}/babyshark-serialclient-daemon"
FILES:${PN} += "/etc/init.d/*"
