#!/bin/sh

ip addr add 192.168.1.5/24 dev eth0
ip link set eth0 up

