# Babyshark boot and update processes

## Quick start
Note, if you are updating from the interim system, you should update the
bootloader, FPGA1 PS and FPGA1 PL all in one go from the USB. This is because
FPGA1 PL has been separated from the PS image and requires extra machinery to
start up.

During this first migration, updating only one of the components without the
others may cause PS to fail to boot because it's now relying on an updated
bootloader to program the PL.

### Update PS
1. `<buildhost>$ bitbake enghost-image`
2. `scp digico@<buildhost>:<poky>/build/deploy/images/p16380/fitImage <my-usb-drive>`
3. Put drive in engine
4. ssh or serial console in to engine
5. `root@p16380# engine-update usb enghost`
6. Now remove drive and reboot, engine will boot from flash

### Update other things, including FPGA1 PL, other FPGAs, sharcs, xmos
1. Place `FPGA1.bit`/`FPGA2.bit`/`sharc1.bin`, etc on usb drive
2. Put drive in engine
3. ssh or serial console in to engine
4. `root@p16380# engine-update usb fpga1` (replace fpga1 with device name as
   needed)

FPGA1 requires zynq reboot to take effect. All other devices will individually
reboot after update.

### Update uboot or fsbl
1. `<buildhost>$ bitbake xilinx-bootbin`
2. `scp digico@<buildhost>:<poky>/build/deploy/images/p16380/boot.bin <my-usb-drive>`
4. `root@p16380# engine-update usb bootloader`

## Virgin programming
Generate a Xilinx boot.bin using `bitbake xilinx-bootbin`. In the image deploy
directory, `boot.bin` should be generated. You should program this to flash
address zero using vitis.

You should also erase the entire flash, either via vitis, or by using the
u-boot console once the boot.bin is programmed. Careful not to erase the
boot.bin you just programmed :^).

TODO(liam): provide a u-boot command to automate this. For now, you can run `sf probe 0`
followed by `sf erase 0x400000 0x7c00000` (Eg. erase 128MiB excluding the first
4MiB which u-boot is running from).

Once the boot.bin is programmed, the rest of the system can be bootstrapped
without vitits. The boot.bin will boot from USB if it exists, so a usb drive
containing a fitimage and all the other required firmware blobs (shrarc, fpgas,
dante...) can be used, along with the `factory-program` script detailed below.

Alternatively at this point you can update individual components as detailed in
[quick start](#quick-start).

## Updating
There is a script that can be invoked via ssh or serial console:
`engine-update`.

```
Usage: engine-update usb|ramfs sharc1|sharc2|sharc3|fpga1|fpga2|fpga3|xmos|enghost|enghost_backup|bootloader|bootloader_backup|dante
       engine-update custom sharc1|sharc2|sharc3|fpga1|fpga2|fpga3|xmos|enghost|enghost_backup|bootloader|bootloader_backup|dante <filename>
```

### Via USB
Copy the file to a usb drive. It must have the canonical name, eg `FPGA1.bit`
for fpga 1. Then run `engine-update usb <device>`, eg `engine-update usb fpga1`.

### Via SCP
Scp the file (with canonical name) to the engine host's `/home/root`. Then run
`engine-updatae ramfs <device>`


## Factory programming
There is a script `factory-program`, which just runs `engine-update usb ${device}`
for every possible device. Put all the firmwares on a USB and run this.

TODO(liam): This should erase the flash as well?

## Normal boot flow
1. boot.bin located at start of qspi flash is the default boot device
2. Zynq bootrom loads boot.bin which contains u-boot
3. u-boot loads kernel from linuxmain flash partition
4. Linux boots

## Filesystem layout
### QSPI Flash
See also: `scripts/babyshark-flash-partitions.txt`

| start (MiB) | Length (MiB) | /dev/mtdblockN | usage           | fs        | moutpoint       |
| ----------- | ------------ | -------------- | --------------- | --------- | --------------- |
| 0           | 4            | 0              | ubootmain       | raw flash | n/a             |
| 4           | 16           | 0              | linuxmain       | raw flash | n/a             |
| 20          | 4            | 0              | ubootbackup     | raw flash | n/a             |
| 24          | 8            | 0              | linuxbackup     | raw flash | n/a             |
| 32          | 14           | 0              | fpga1bitstream  | raw flash | n/a             |
| 46          | 4            | 0              | firmware        | jffs2     | /enginefirmware |
| 50          | 9            | 0              | dantedata       | jffs2     | /data           |
| 59          | 2            | 0              | dantedante      | jffs2     | /dante          |
| 61          | 2            | 0              | system          | jffs2     | /systempersist  |
| 63          | 1            | 0              | systemimmutable | jffs2     | /immutable      |
| 64          | 16           | 0              | danteipcore     | jffs2     | /danteipcore    |

#### ubootmain/ubootbackup
Contains a xilinx boot.bin with FSBL and u-boot. See [boot script](#boot-script).

#### linuxmain/linuxbackup
Contains a fitImage with device tree, rootfs, and kernel.

#### fpga1bitstream
Contains the bitstream for the PL. Note that this is in the raw `.bit` format,
not the bootable `.bin` format that is generated by xilinx bootgen. This will
be programmed to the PL in u-boot.

#### firmware
Contains firmware images for on-demand boot devices. This includes the sharcs
and xmos.

#### dantedata/dantedante
These persistent partitions are required by dante.

#### system/systemimmutable
Misc persistent data used by the engine host. `systemimmutable` should be
mounted read-only and contain lifetime-immutable data such as the MAC address
and board ID.

#### danteipcore
Contains the packed dante IP core package (eg `ipcore-XXX.tgz`).

### eMMC

| start (MiB) | Length (GiB) | usage        | fs   | moutpoint     |
| ----------- | ------------ | ------------ | --   | ------------- |
| 0           | 29           | engineparams | ext4 | /engineparams |

For now, the entire device is formatted as ext4 and mounted for engine parameter storage.

## Boot script

See the bootscript [here](./meta-enghost/recipes-bsp/u-boot/u-boot-xlnx/digico-uboot-env.env).

### Current bootscript (during development)
1. If the safeboot button is pressed, stop everything and go to uboot prompt.
2. Attempt to program the PL. This reads from the `fpga1bitstream` flash
   partition and programs it. A patch to u-boot has been added to re-run
   `ps7_post_config` after programming.
3. If programming fails, continue anyway as best effort. The kernel command
   line `digico_no_pl=1` is added to the kernel command line, as well as a
   cmdline paramter to blacklist the engine comms kernel module. This is
   because the engine comms kernel module ("yeng") will attempt to read and
   write to the AXI bus which goes nowhere. TODO(liam): we need to also
   blacklist dante startup.
4. Try boot from USB. Mount the USB as FAT
    - Scan for bootscripts (`boot.scr`). If any is found, run it
    - Scan for fitImages (`fitImage` or `image.ub`). If any is found, boot it
5. Try boot from eMMC. Mount the eMMC as FAT
    - Scan for bootscripts (`boot.scr`). If any is found, run it
    - Scan for fitImages (`fitImage` or `image.ub`). If any is found, boot it
6. Try to boot from `linuxmain`. Attempt to load a fitImage from this partition
   and boot from it.
7. Try to boot from `linuxbackup`. Attempt to load a fitImage from this partition
   and boot from it.
8. Die.


For all boot paths, the option `digico_boot_device=${device}` is added to the
kernel command line. `${device}` may be usb, mmc, qspi_main, or qspi_backup.

## Programming FPGA1 bitstream considerations
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
