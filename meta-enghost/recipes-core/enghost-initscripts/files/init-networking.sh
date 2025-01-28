#!/bin/sh

INTERFACE="eth0"

MAC_FILE="/immutable/macaddress"
if [ -f "$MAC_FILE" ]; then
    ip link set dev "$INTERFACE" address "$(cat /immutable/macaddress)"
else
    echo "$MAC_FILE does not exist"
fi

ip addr add 192.168.1.5/24 dev "$INTERFACE"
ip link set "$INTERFACE" up

