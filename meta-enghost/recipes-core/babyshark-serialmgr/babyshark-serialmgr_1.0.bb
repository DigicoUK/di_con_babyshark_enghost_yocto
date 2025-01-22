SUMMARY = "Serial manager application for babyshark"
SECTION = "utils"
LICENSE = "CLOSED"

SRC_URI = "git://github.com/DigicoUK/di_con_enghost_babyshark_serialmgr.git;branch=main;protocol=ssh;user=git"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake
