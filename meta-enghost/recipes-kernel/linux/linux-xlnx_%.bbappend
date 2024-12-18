FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# TODO(liam) what is fragment.cfg
SRC_URI += "file://dante-requirements.cfg \
            file://ethernet-switch.cfg \
            file://i2c-gpio-expander.cfg \
            file://misc.cfg \
            file://slim.cfg \
            file://usb-ulpi.cfg \
            file://fragment.cfg \
            file://0001-Ignore-FCS-errors-if-FCS-offload-is-not-enabled-enab.patch \
            "
