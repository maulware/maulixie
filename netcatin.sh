#!/bin/bash

if ! [ -f quader ]; then
  echo "OFFSET 30 1000" > quader
  for x in $(seq 0 40); do for y in $(seq 0 40); do echo "PX $x $y 878712" >> quader; done; done
fi
#! [ -f /tmp/netting ] && mkfifo /tmp/netting
#while [ 1 -eq 1 ]; do
 #cat quader > /tmp/netting
  nc 94.45.232.48 1234 < quader 
 echo .
#done
