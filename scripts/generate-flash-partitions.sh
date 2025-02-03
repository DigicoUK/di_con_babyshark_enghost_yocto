#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: generate-flash-partitions.sh <partitions.txt>"
    exit 1
fi

addr=0
echo -e "Device tree:\n\n"
(
    read -r
    while IFS=" " read -r name mib; do
        echo "partition@${name} {"
        echo "    label = \"${name}\";"
        siz=$(( mib * 0x100000 ))
        printf "    reg = <0x%x 0x%x>;\n" "${addr}" "${siz}"
        addr=$(( "${addr}" + "${siz}" ))
        echo "};"
    done
) < "$1"
