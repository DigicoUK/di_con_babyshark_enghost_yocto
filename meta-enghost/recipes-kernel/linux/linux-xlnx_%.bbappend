FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://slim.cfg \
    file://dante_requirements.cfg \
    file://i2c_gpio_expander.cfg \
"
