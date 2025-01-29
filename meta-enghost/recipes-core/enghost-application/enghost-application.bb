SUMMARY = "(Q7) Enghost application"
DESCRIPTION = "This is the main Q7 app"
LICENSE = "CLOSED"
SECTION = "utils"
SRC_URI = "git://github.com/DigicoUK/di_con_enghost_q7_application.git;branch=babyshark-test;protocol=ssh;user=git"
#SRC_URI = "git://github.com/DigicoUK/di_con_enghost_q7_application.git;protocol=ssh;user=git"
#SRCREV = "89d8bc8700b500ce9989c66400f6e0456db26a1c"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

CXXFLAGS += "-fPIC -Wl,--no-as-needed -O3 -DELPP_THREAD_SAFE -pthread -std=c++11 -Wno-unused-variable "

LDFLAGS += "-lpthread"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/Q7_app.bin ${D}${bindir}
}

FILES:${PN} += "${ROOT_HOME} ${bindir}/*"
FILES:${PN}-dbg += "usr/local/bin/.debug"
