#!/bin/bash

if ! [ -f quader ]; then
  echo "OFFSET 30 1000" > quader
  for x in $(seq 0 40); do for y in $(seq 0 40); do echo "PX $x $y 878712" >> quader; done; done
fi
if ! [ -f /tmp/netting ]; then
  mkfifo /tmp/netting
fi

while [ 1 -eq 1 ]; do
 cat quader > /tmp/netting
 echo .
done
