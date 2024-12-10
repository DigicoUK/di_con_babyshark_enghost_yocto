FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://slim.cfg \
    file://digico-settings.cfg \
    file://custom-env.cfg \
    file://digico-default-env.env \
"

do_configure:append() {
    # https://catonmat.net/sed-one-liners-explained-part-one section 39.
    # Join backslashes to the following line
    sed -e :a -e '/\\$/N; s/\\\n//; ta' ${WORKDIR}/digico-default-env.env > ${B}/source/digico-default-env.env
}
