FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://fragment.cfg \
"

# file://dante-requirements.cfg
