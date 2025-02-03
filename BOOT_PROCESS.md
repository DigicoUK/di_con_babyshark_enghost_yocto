## Programming FPGA1 bitstream
There are a few options for how to do this, namely:
1. using FSBL, where bitstream is baked in to the boot.bin
2. from u-boot, using the `fpga loadb <...>` command
3. from Linux using the FPGA-manager linux driver and device tree overlay

Option 1 is the easiest to set up, but it couples the bitstream to the FSBL and
u-boot binaries. To update FPGA1 using this method, we would need to either
- Package and update fsbl, u-boot, and FPGA1 bistream all in one. This has the
  disadvantages of unnecessarily coupling a FPGA1 update with a u-boot update
  (u-boot updates should be very rare), as well as requiring us to do a
  potentially less safe update of the boot.bin, since power failure while
  updating will cause a fallback to the backup partition.
- Edit the boot.bin in-place. It is feasible to repackage the boot.bin or
  manually edit it in linux. length fields and checksums would need to be
  patched up. This is clearly a complex and non-standard way of doing things
  and was avoided as such.

Option 2 is doable from the u-boot script. It has the advantage of having the
PL up and running before the linux driver stack runs, so no device tree
overlays are required. The disadvantage is that it requires some patches to
u-boot so that it can run `ps7_post_config` after programming, as is usually
done by the fsbl.

Option 3 is also achievable. The downside is that it would require all
FPGA-dependent devices and system setup to be moved to a device tree overlay,
since they will not be available until the fpga is programmed via fpga-manager.
This includes any EMIO devices, and AXI devices.



I've decided to go with Option 2, as it removes complexity from the linux
image. Device tree overlays are not required, and no patching of boot.bin is
required.
