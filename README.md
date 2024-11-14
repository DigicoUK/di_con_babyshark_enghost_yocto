# Yocto layers for babyshark engine host

This repo contains the custom layers and recipes for the babyshark engine host.

- meta-enghost: layer for general recipes relating to the engine host, distro, and output images
- meta-dante: layer for dante configuration, drivers, and IP
- meta-klante-module: BSP layer for klante module
- meta-p16380: BSP layer for babyshark engine board


## Setting up build environment
1. Install system requirements for yocto scarthgap
2. Clone repositories
    - In the _parent_ directory to this repository, run
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
3. Init configuration from template. Cd into `poky/`. Run eg
   `TEMPLATECONF="../di_con_babyshark_enghost_yocto/meta-enghost/conf/templates/klante-module-test/ . ./oe-init-build-env`
   to use the "klante-module-test" configuration template.
4. Build images: `bitbake enghost-image`

# External layer management
The multiple repositories required are managed by a [very simple shell script](./clone-repositories).
It reads the file [repositories.txt](./repositories.txt) and shallow clones
each repository at the listed revision to the destination directory.