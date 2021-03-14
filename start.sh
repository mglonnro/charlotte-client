#!/bin/bash

VER=0.0.3
echo "Charlotte Client v$VER"

if ! test -e config 
then
  echo "Please run ./install.sh before starting."
  exit 1
fi

BOATID=`grep BOATID config|cut -d'=' -f2`
APIKEY=`grep APIKEY config|cut -d'=' -f2`
DATADIR=`grep DATA config|cut -d'=' -f2`
LOGSDIR=`grep LOGS config|cut -d'=' -f2`
WIFICMD=`grep WIFICMD config|cut -d'=' -f2`
DEVICE=`ls /dev/serial/by-id/*NMEA*`

if grep -q "replace" config
then
  echo "Please update your config file before starting!"
  exit 1
fi

mkdir -p $LOGSDIR
mkdir -p $DATADIR
mkdir -p $DATADIR/done

echo "Boat id: $BOATID"
echo "Serial device: $DEVICE"
echo "Data dir: $DATADIR"
echo "Logs dir: $LOGSDIR"

RUNNING=`ps axuw|grep charlotte-client|grep -v grep`
if test -n "$RUNNING"
then
  echo "Already running? $RUNNING"
  exit 1
fi

echo "Starting client with serial device."
#(./actisense-serial -r $DEVICE | ./charlotte-logger logs --upload-to $BOATID --retry 60 | ./analyzer -json | ./charlotte-client $BOATID) 
(./actisense-serial -r $DEVICE | ./charlotte-logger $DATADIR --upload-to $BOATID --retry 60 | ./analyzer -json | ./charlotte-client $BOATID) > $LOGSDIR/daemon-`date +%y-%m-%d_%H:%M:%S`.log 2>&1 

#echo "Starting client with wifi device."
#(nc -kul 1457 | ./analyzer -json | ./charlotte-client $BOATID) > ./daemon.log 2>&1 &
