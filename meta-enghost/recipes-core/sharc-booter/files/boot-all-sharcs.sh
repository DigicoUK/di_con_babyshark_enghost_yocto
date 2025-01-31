#!/bin/sh

# bootsharc filename cs-index
checkbootsharc() {
    if [ ! -f "$1" ]; then
        echo "Sharc firmware $1 does not exist"
        return 1
    fi
    echo "Booting $1 at chipselect $2"
    sharc-booter "$1" "$2"
}

case $1 in
    start)
        checkbootsharc /enginefirmware/sharc1.bin 0
        checkbootsharc /enginefirmware/sharc2.bin 1
        checkbootsharc /enginefirmware/sharc3.bin 2
        ;;
    *)
        ;;
esac

