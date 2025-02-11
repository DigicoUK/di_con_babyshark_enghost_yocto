SUMMARY = "Busybox UDHCPC config and initscripts for dante interfaces"
DESCRIPTION = "Provides udhcpc script files and startup jobs"
LICENSE = "CLOSED"

S = "${WORKDIR}"

SRC_URI = " \
    file://host0.script \
    file://host1.script \
    file://init-udhcpc \
"

inherit update-rc.d

INITSCRIPT_NAME = "init-udhcpc"
# dante starts at 90, this should be after the interface comes up
INITSCRIPT_PARAMS = "defaults 91 9"

RDEPENDS:${PN} += "busybox-udhcpc"

do_install() {
    install -d ${D}${sysconfdir}/udhcpc.d/
    install -m 0755 host0.script ${D}${sysconfdir}/udhcpc.d/
    install -m 0755 host1.script ${D}${sysconfdir}/udhcpc.d/

    install -d ${D}${sysconfdir}/init.d
    install -m 0755 init-udhcpc ${D}${sysconfdir}/init.d
}

FILES:${PN} += " \
    ${sysconfdir}/udhcpc.d/ \
    ${sysconfdir}/init.d/* \
"
