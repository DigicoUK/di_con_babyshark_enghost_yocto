#!/bin/sh

echo "booting sharc 1"
sharc-booter /home/root/sharc1.bin 0
echo "booting sharc 2"
sharc-booter /home/root/sharc2.bin 1
echo "booting sharc 3 (aka 4)"
sharc-booter /home/root/sharc4.bin 2
