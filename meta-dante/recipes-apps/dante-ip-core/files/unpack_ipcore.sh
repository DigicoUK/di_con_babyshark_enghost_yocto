#!/bin/sh

IPCORE_TGZ="$1"

tar xf "$IPCORE_TGZ"

# replace config kernel module locations with proper kernel name
sed -i "s:/lib/modules/[^/]*/extra/:/lib/modules/$(uname -r)/extra/:" ipcore/dante_package/dante_data/capability/config.*.json

touch /run/dante_manager.pid