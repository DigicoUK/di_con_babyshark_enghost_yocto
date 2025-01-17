SUMMARY = "(Q7) Enghost application"
DESCRIPTION = "This is the main Q7 app"
LICENSE = "CLOSED"
SECTION = "utils"
SRC_URI = "git://github.com/DigicoUK/di_con_enghost_q7_application.git;branch=babyshark-test;protocol=ssh;user=git"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

CXXFLAGS += "-fPIC -Wl,--no-as-needed -O3 -DELPP_THREAD_SAFE -pthread -std=c++11 -Wno-unused-variable "

LDFLAGS += "-lpthread"

do_install() {
	install -d ${D}/usr/local/bin
	install -m 0755 ${S}/Q7_app.bin ${D}/usr/local/bin
}

FILES:${PN} += "${ROOT_HOME} usr/local/bin"
FILES:${PN}-dbg += "usr/local/bin/.debug"
