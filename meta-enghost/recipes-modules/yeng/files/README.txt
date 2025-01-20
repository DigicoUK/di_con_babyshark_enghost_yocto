This is a re-written version of the 'xeng' driver by Paul. The previous driver
created a static character device. This one registers as a platform driver, and
is instantiated by the device tree (see digico,yeng.txt for DT bindings). This
was done to leverage the platform driver support for mapping interrupts, mmio,
and clocks rather than hardcoding it as was done in the previous driver. The
driver should be backwards compatible with blueshark.

For a quick test, send an 'are you there' message and hexdump the response:

echo -ne "\x04\x00\xa5\xff\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\xa6\xff" > /dev/xengstream0 && hexdump /dev/xengstream0

raw "are you there" hex:
0400a5ff0000010000000000000000000400a6ff
