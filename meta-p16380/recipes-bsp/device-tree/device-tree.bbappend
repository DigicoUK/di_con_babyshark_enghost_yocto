# https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/61669922/Customizing+Device+Trees+in+Xilinx+Yocto

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SYSTEM_DTFILE = "system-top.dts"
SRC_URI:append = " file://${SYSTEM_DTFILE}"

KERNEL_INCLUDE:append = " \
    ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts \
    ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts/* \
    ${STAGING_KERNEL_DIR}/scripts/dtc/include-prefixes \
"
