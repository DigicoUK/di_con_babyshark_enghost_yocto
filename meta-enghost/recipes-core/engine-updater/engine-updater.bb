SUMMARY = "update scripts"
DESCRIPTION = "Write to FPGA's spi flash"
LICENSE = "CLOSED"

COMPATIBLE_MACHINE = "p16380"

PROVIDES:remove = "virtual/dtb"

SRC_URI = " \
    file://external-fpga-flash.dts \
    file://engine-update \
"

inherit devicetree

do_install() {
    install -d ${D}${libdir}/engine-updater
    install -m 0666 ${B}/external-fpga-flash.dtbo ${D}${libdir}/engine-updater

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/engine-update ${D}${bindir}
}

RDEPENDS:${PN} = "bash"

FILES:${PN} += "${libdir}/engine-updater/* ${bindir}/*"
