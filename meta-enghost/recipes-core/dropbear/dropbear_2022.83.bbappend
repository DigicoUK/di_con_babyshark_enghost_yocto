# This is a patch for dropbear ssh server's init script to start it within the
# "appcomms" network namespace

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
