# https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/61669922/Customizing+Device+Trees+in+Xilinx+Yocto

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SYSTEM_USER_DTSI ?= "system-user.dtsi"

SRC_URI:append = " file://${SYSTEM_USER_DTSI}"

KERNEL_INCLUDE:append = " \
    ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts \
    ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts/* \
    ${STAGING_KERNEL_DIR}/scripts/dtc/include-prefixes \
"

do_configure:append() {
        cp ${WORKDIR}/${SYSTEM_USER_DTSI} ${B}/device-tree
        echo "#include \"${SYSTEM_USER_DTSI}\"" >> ${B}/device-tree/system-top.dts
}
