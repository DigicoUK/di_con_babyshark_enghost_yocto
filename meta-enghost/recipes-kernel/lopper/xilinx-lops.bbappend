# This recipe requires "lopper" which is in meta-virtualization. No need to pull
# in that layer for no reason, instead exclude xilinx-lops

EXCLUDE_FROM_WORLD = "1"