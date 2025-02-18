# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Akashi Register Interface device driver.
#

# Exclusion if the driver is integrated into the kernel source tree
ifeq ($(CONFIG_AKASHI_KBUILD), y)
	ccflags-y += -include $(KBUILD_PATH)/include/generated/autoconf.h
endif

EXTRA_CFLAGS = -DAKASHI_REG_DYNAMIC_MAJOR -DAKASHI_REG_RX_BUNDLE_ACTIVITY -DAKASHI_REG_METERINGEV -DAKASHI_REG_KA_MISS -DAKASHI_REG_AERR -DAKASHI_REG_KAEV -DCONFIG_AKASHI_REG_DYNAMIC_MAJOR -DCONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY -DCONFIG_AKASHI_REG_METERINGEV -DCONFIG_AKASHI_REG_KA_MISS -DCONFIG_AKASHI_REG_AERR -DCONFIG_AKASHI_REG_KAEV

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

