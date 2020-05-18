#!/bin/bash

BOATID=`grep BOATID config|cut -d'=' -f2`
WIFICMD=`grep WIFICMD config|cut -d'=' -f2`
DEVICE=`ls /dev/serial/by-id/*NMEA*`
VER=0.0.2

echo "Charlotte Client v$VER"
if grep -q "replace" config
then
  echo "Please update your config file before starting!"
  exit 1
fi

echo "Boat id $BOATID."
echo "Serial device $DEVICE."

RUNNING=`ps axuw|grep charlotte-client|grep -v grep`
if test -n "$RUNNING"
then
  echo "Already running? $RUNNING"
  exit 1
fi

echo "Starting client with serial device."
(./actisense-serial -r $DEVICE | ./analyzer -json | ./charlotte-client $BOATID) > ./daemon.log 2>&1 &

#echo "Starting client with wifi device."
#(nc -kul 1457 | ./analyzer -json | ./charlotte-client $BOATID) > ./daemon.log 2>&1 &
    
