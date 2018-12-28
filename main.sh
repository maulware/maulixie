#!/bin/bash
#
# Connecting to pixies and doing stuff
#
#

COLOR=$1
IP=$2
PORT=$3
: ${IP:="127.0.0.1"}
: ${PORT:="1337"}
: ${COLOR:="993721"}

echo "http://${IP}:${PORT}"

function openSocket()
{
  exec 42<>/dev/tcp/${IP}/${PORT}
  if [ $? -ne 0 ]; then
    echo "Failed to connect: got $?"
    exit 1
  fi
}

CPAR=0
MPAR=42

function pech()
{
  let "CPAR++"
  ( echo "$1" >&42 ) &
  if [ $CPAR -ge $MPAR ]; then
    CPAR=0
    wait
  fi
}

PX=300
PY=300
XMAX=100
YMAX=100
XDIR=1
YDIR=1
function move()
{
  local MOV=30
  local my=0
  PX=$((PX+($MOV*$XDIR)));
  if [ $PX -ge $XMAX ]; then
    PX=$(($XMAX - $MOV))
    my=1
  elif [ $PX -le $MOV ]; then
    PX=$MOV
    my=-1
  fi
  if [ $my -ne 0 ]; then
    echo "Change XDIR to $XDIR"
    XDIR=$((XDIR*-1))
    PY=$((PY+($MOV*$YDIR)))
    if [ $PY -le $MOV ]; then
      PY=$MOV
      YDIR=$((YDIR*-1))
      echo "Change YDIR to $YDIR"
    elif [ $PY -ge $YMAX ]; then
      PY=$(($YMAX - $MOV))
      YDIR=$((YDIR*-1))
      echo "Change YDIR to $YDIR"
    fi
  fi
}

function simple()
{
  for x in $(seq 1 20); do
    for y in $(seq 1 20); do
      pech "PX $x $y $COLOR"
    done
  done
}

function sparse()
{
  local XM=$1
  local YM=$2
  local COLOR=$3
  : ${XM:=30}
  : ${YM:=30}
  : ${COLOR:=857332}
  local SSIZE=7
  for CX in $(seq 1 $SSIZE); do
    for CY in $(seq 1 $SSIZE); do
      for i in $(seq $CX 4 $XM); do
        for j in $(seq $CY 4 $YM); do
          pech "PX $i $j $COLOR"
        done
      done
    done
  done
}

openSocket

pech "OFFSET $PX $PY"
while [ 1 -eq 1 ]; do
  sparse 10 10 $COLOR
#  move
done
