# Yocto layers for babyshark engine host

This repo contains the custom layers and recipes for the babyshark engine host.

- meta-enghost: layer for general recipes relating to the engine host, distro, and output images
- meta-dante: layer for dante configuration, drivers, and IP
- meta-klante-module: BSP layer for klante module
- meta-p16380: BSP layer for babyshark engine board


## Setting up build environment
1. Install [system requirements for yocto scarthgap](https://docs.yoctoproject.org/scarthgap/ref-manual/system-requirements.html#ubuntu-and-debian)
2. mkdir a new directory
3. Clone repositories
    - In the toplevel directory created in the previous step, clone this
      repository to it's own subfolder
    - From the toplevel directory, run
      `di_con_babyshark_enghost_yocto/clone-repositories`
    - This will create necessary sibling directories with other required yocto
      layers, along with the yocto base source repo.
The project should look like this
```
parent (run clone-repositories from this working directory)
|- di_con_babyshark_enghost_yocto/
|- poky/
|- meta-*/ (other required layers)
```
3. Init configuration from template. Cd into `poky/`. Run
```
TEMPLATECONF="../di_con_babyshark_enghost_yocto/meta-enghost/conf/templates/babyshark/" . ./oe-init-build-env
```
4. Build image: `bitbake enghost-image`
5. Use image artifacts: in `build/tmp/deploy/images/p16380/` you should have
    - boot.bin: FSBL and U-boot bootable zynq image
    - fitImage: u-boot fitimage containing devicetree, kernel, and rootfs


## External layer management
The multiple repositories required are managed by a [very simple shell script](./clone-repositories).
It reads the file [repositories.txt](./repositories.txt) and shallow clones
each repository at the listed revision to the destination directory.

## Building, booting and updating
See [build boot update docs](./docs/boot_fs_and_update.md).
