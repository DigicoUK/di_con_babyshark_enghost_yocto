DESCRIPTION = "Target package group for dante stuff"

inherit packagegroup

RDEPENDS:${PN} = " \
    dante-ip-core \
    dante-dhcpcd-config \
    program-si5351-pll \
    kernel-module-akashi-reg \
    kernel-module-akashi-temac \
"
