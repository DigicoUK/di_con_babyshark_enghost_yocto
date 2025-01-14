DESCRIPTION = "Custom fstab/motd"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# NOTE: we need to use the digico sub-folder in files. This is because poky
# overrides MOTD using "poky" FILESOVERRIDES, and this must be higher priority
# (more specific) in FILESPATH than that. So we use the 'digico' FILESOVERRIDE
# (derived from DISTROOVERRIDES)
