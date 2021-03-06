#!/bin/bash
#
# use: $0 [dev]
#

if [ "$(whoami)" != "root" ]; then
  echo "please root"
  exit 1
fi

spawn=$1
prefix=$2
: ${spawn:=eth0}
: ${prefix:="00:12:11:11:11"}

ip netns | grep -q pixie || echo "create netns pixie" && ip netns add pixie

menge=14

ip netns exec pixie ip a | grep -q $spawn || (
  echo "create off spawn of $spawn and set in netns pixie"
  for p in $(seq 10 $menge); do
    ip link add link ${spawn} address ${prefix}:${p} ${spawn}.${p} type macvlan
  done
  for p in $(seq 10 $menge); do ip link set "${spawn}.${p}" netns pixie; done
)

echo "set up links and dhclient IP"
for p in $(seq 10 $menge); do
  echo -n "${spawn}.${p}"
  ( ip netns exec pixie ip link set up ${spawn}.${p} && ip netns exec pixie dhclient  -v ${spawn}.${p} ) &
done
