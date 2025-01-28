#!/bin/sh

# first create the mountpoints
mkdir -p /config
mkdir -p /data
mkdir -p /dante
mkdir -p /enginefirmware
mkdir -p /systempersist
mkdir -p /immutable
mkdir -p /engineparams

mount -at nonfs,nosmbfs,noncpfs 2>/dev/null
