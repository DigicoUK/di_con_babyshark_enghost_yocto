# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Akashi Register Interface device driver.
#

# Exclusion if the driver is integrated into the kernel source tree
ifeq ($(CONFIG_AKASHI_KBUILD), y)
	ccflags-y += -include $(KBUILD_PATH)/include/generated/autoconf.h
endif

obj-m += akashi-reg.o

akashi-reg-m := akashi_reg.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

