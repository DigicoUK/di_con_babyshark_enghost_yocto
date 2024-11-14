# https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/59605045/Adding+a+Hardware+Platform+to+a+Xilinx+Yocto+Layer

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

HDF_BASE:klante-module = "file://"
HDF_PATH:klante-module = "klante-fourier.xsa"
# HDF_PATH:klante-module = "HC_SingleDDR_Fourier.xsa"
# HDF_PATH:klante-module = "klante-fourier-hc-top-plus-mmc.xsa"
# HDF_PATH:klante-module = "klante-standard-wourier-release.xsa"
# HDF_PATH:klante-module = "klante-standard-mmc-wourier-release-2024.xsa"