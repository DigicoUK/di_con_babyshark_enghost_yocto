# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Akashi Temac Ethernet device driver.
#

# Exclusion if the driver is integrated into the kernel source tree
ifeq ($(CONFIG_AKASHI_KBUILD), y)
	ccflags-y += -include $(KBUILD_PATH)/include/generated/autoconf.h
endif

ccflags-y += -DLIB_SWITCH_ZYNQ_PLATFORM -DSWITCH_LIB_LINUX -D__AUD_ZYNQ_KERNEL__

obj-m += akashi-temac.o

akashi-temac-m := akashi_temac.o zynq_interface.o switch_lib_linux.o switch_lib_shared.o switch_lib_uboot.o switch_lib_utils.o
# akashi-temac-y := zynq_ps_mac.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers