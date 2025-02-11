#!/bin/sh


if [ -e /proc ] && ! [ -e /proc/mounts ]; then
  mount -t proc proc /proc
fi

if [ -e /sys ] && grep -q sysfs /proc/filesystems && ! [ -e /sys/class ]; then
  mount -t sysfs sysfs /sys
fi

if [ -e /sys/kernel/debug ] && grep -q debugfs /proc/filesystems; then
  mount -t debugfs debugfs /sys/kernel/debug
fi

if [ -e /sys/kernel/config ] && grep -q configfs /proc/filesystems; then
  mount -t configfs configfs /sys/kernel/config
fi

if [ -e /sys/firmware/efi/efivars ] && grep -q efivarfs /proc/filesystems; then
  mount -t efivarfs efivarfs /sys/firmware/efi/efivars
fi

if ! [ -e /dev/zero ] && [ -e /dev ] && grep -q devtmpfs /proc/filesystems; then
  mount -n -t devtmpfs devtmpfs /dev
fi

# GID of the `tty' group
TTYGRP=5

# Set to 600 to have `mesg n' be the default
TTYMODE=620

if grep -q devpts /proc/filesystems
then
	#
	#	Create multiplexor device.
	#
	test -c /dev/ptmx || mknod -m 666 /dev/ptmx c 5 2

	#
	#	Mount /dev/pts if needed.
	#
	if ! grep -q devpts /proc/mounts
	then
		mkdir -p /dev/pts
		mount -t devpts devpts /dev/pts -ogid=${TTYGRP},mode=${TTYMODE}
	fi
fi

# setup alignment traps
if [ -e /proc/cpu/alignment ]; then
   echo "3" > /proc/cpu/alignment
fi


# Populate volatiles
#
