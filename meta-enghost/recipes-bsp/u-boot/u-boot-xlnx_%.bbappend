FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://slim.cfg \
    file://digico-settings.cfg \
    file://custom-env.cfg \
    file://ps_init_file.cfg \
    file://digico-uboot-env.env \
    file://ps7_init_gpl.c \
    file://ps7_init_gpl.h \
    file://0001-Add-command-to-run-ps7_post_config.patch \
"

do_configure:append() {
    # https://catonmat.net/sed-one-liners-explained-part-one section 39.
    # Join backslashes to the following line
    sed -e :a -e '/\\$/N; s/\\\n//; ta' ${WORKDIR}/digico-uboot-env.env > ${B}/source/digico-uboot-env.env

    # install ps7_init_gpl.c to src
    install -d ${B}/source/board/xilinx/zynq/zynq-babyshark/
    install ${WORKDIR}/ps7_init_gpl.c ${B}/source/board/xilinx/zynq/zynq-babyshark/
    install ${WORKDIR}/ps7_init_gpl.h ${B}/source/board/xilinx/zynq/
}
