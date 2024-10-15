# https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/61669922/Customizing+Device+Trees+in+Xilinx+Yocto

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
 
SYSTEM_USER_DTSI ?= "system-user.dtsi"
 
SRC_URI:append = " file://${SYSTEM_USER_DTSI}"
 
do_configure:append() {
        cp ${WORKDIR}/${SYSTEM_USER_DTSI} ${B}/device-tree
        echo "/include/ \"${SYSTEM_USER_DTSI}\"" >> ${B}/device-tree/system-top.dts
}