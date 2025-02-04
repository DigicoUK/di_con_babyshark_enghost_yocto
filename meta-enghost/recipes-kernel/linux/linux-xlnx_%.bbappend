FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# TODO(liam) what is fragment.cfg
SRC_URI += "file://dante-requirements.cfg \
            file://ethernet-switch.cfg \
            file://i2c-adc.cfg \
            file://i2c-gpio-expander.cfg \
            file://misc.cfg \
            file://slim.cfg \
            file://usb-ulpi.cfg \
            file://fragment.cfg \
            file://crypto-random.cfg \
            file://0001-Report-correct-frame-size-of-1536-for-BIG-FRAMES.patch \
            "
