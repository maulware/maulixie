#!/bin/bash
#
#

if [ $(whoami) -ne "root" ]; then
  echo "please root"
  exit 1
fi

for p in $(seq 10 42); do
  ip link add link eth0 address 00:11:11:11:11:${p} eth0.${p} type macvlan
done

for p in $(seq 10 42); do ip link set "eth0.${p}" netns pixie; done

for p in $(seq 10 42); do
  ( ip netns exec pixie ip link set up eth0.${p} && ip netns exec pixie dhclient  -v eth0.${p} ) &
done
