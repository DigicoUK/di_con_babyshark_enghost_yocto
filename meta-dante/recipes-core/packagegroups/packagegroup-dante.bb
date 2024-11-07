DESCRIPTION = "Target package groups for dante stuff"

inherit packagegroup

PACKAGES = " \
    ${PN}-drivers \
    ${PN}-ipcore \
"

RDEPENDS:${PN}-drivers = " \
    kernel-module-akashi-reg \
    kernel-module-akashi-temac \
"

# todo add IPCORE
RDEPENDS:${PN}-ipcore = " \
    kernel-module-akashi-reg \
    kernel-module-akashi-temac \
    dante-ip-core \
"