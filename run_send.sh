#!/bin/bash
#
# best run in netns with all eth0.* lans

echo "starting for all eth0.*"

for IP in $(ip a | grep eth0 | grep inet | cut -d' ' -f 6 | cut -d'/' -f 1); do (./a.out $IP &); sleep 1.1; done

