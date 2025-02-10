DESCRIPTION = "Target package group for dante stuff"

inherit packagegroup

RDEPENDS:${PN} = " \
    kernel-module-akashi-reg \
    kernel-module-akashi-temac \
    dante-ip-core \
"
