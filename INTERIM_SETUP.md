## Interim boot sequence
For now, we are using a basic boot sequence to get things up and running.

1. boot.bin resides in QSPI flash. It contains
    - FSBL
    - FPGA1 bitstream
    - uboot.elf, with device tree baked in. There is a [boot script baked in to u-boot](./meta-enghost/recipes-bsp/u-boot/u-boot-xlnx/digico-default-env.env) that will search for a file named `fitImage` or `image.ub` in either eMMC or USB drive.
2. From the eMMC or USB drive, u-boot will boot a fitImage. The fitimage contains
    - Kernel device tree (this is the same one used for u-boot)
    - Kernel
    - Rootfs (cpio initramfs)

## Tasks
### Building
```
bitbake enghost-image
```
All of the outputs are found in build/tmp/deploy/images/p16380/.
### Updating kernel/kernel devicetree/rootfs contents
Rebuild the image. In the image output directory, copy `fitImage` to a usb drive or program it on to the eMMC module using the SD card adaper. Plug it in to the engine and power cycle.
### Programming virgin board
Take the boot.bin generated from the build and program the flash using vitis.
### Updating u-boot
If you hold down the "SAFE BOOT" button on power on, you will get to the u-boot
prompt. From here you can update the boot.bin in the qspi by running `run
do_replace_bootbin`. This will look for a `boot.bin` file located on the USB
drive and program it in to the QSPI.
### Update FPGA1 bitstream
Change the XSA located in [this folder](./meta-p16380/recipes-bsp/hdf/files). Rebuild the image, and update the boot.bin as described above. This is a bit annoying I know but we will move to fpga overlay device in the future.
### Update the device tree
Edit [system-user.dtsi](./meta-p16380/recipes-bsp/device-tree/files/system-user.dtsi). This will affect u-boot and linux.
### Change KConfig
Run `bitbake virtual/kernel -c kernel_configme`. Run `bitbake virtual/kernel -c menuconfig` and make changes. Run `bitbake virtual/kernel -c diffconfig`. Take the resultant .cfg fragment and place it in [the config bbappend](./meta-enghost/recipes-kernel/linux/linux-xlnx).
## Misc notes
- Before using the USB host from u-boot, you must pull it out of reset using `gpio set 9;`.
