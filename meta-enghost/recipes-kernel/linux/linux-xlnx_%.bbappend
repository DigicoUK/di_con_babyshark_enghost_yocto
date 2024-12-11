FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://dante-requirements.cfg \
    file://ethernet-switch.cfg \
    file://i2c-gpio-expander.cfg \
    file://misc.cfg \
    file://slim.cfg \
    file://usb-ulpi.cfg \
"
