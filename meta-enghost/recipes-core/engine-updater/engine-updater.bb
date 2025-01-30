SUMMARY = "update scripts"
DESCRIPTION = "Write to FPGA's spi flash"
LICENSE = "CLOSED"

COMPATIBLE_MACHINE = "p16380"

PROVIDES:remove = "virtual/dtb"

SRC_URI = " \
    file://fpga-flash.dts \
    file://update-fpga \
"

inherit devicetree

do_install() {
    install -d ${D}/home/root
    install -m 0666 ${B}/fpga-flash.dtbo ${D}/home/root/
    install -m 0777 ${S}/update-fpga ${D}/home/root/
}

RDEPENDS:${PN} = "bash"

FILES:${PN} += "/home/root/*"
