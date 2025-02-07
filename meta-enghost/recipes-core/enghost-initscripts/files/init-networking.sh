#!/bin/sh

INTERFACE="eth0"

MAC_FILE="/immutable/macaddress"
if [ -f "$MAC_FILE" ]; then
    ip link set dev "$INTERFACE" address "$(cat /immutable/macaddress)"
else
    echo "$MAC_FILE does not exist"
fi
