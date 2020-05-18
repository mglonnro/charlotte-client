#!/bin/bash

RUNNING=`ps axuw|grep charlotte-client|grep -v grep`
if test -n "$RUNNING"
then
  PID=`ps -o pid,args|grep charlotte-client|grep -v grep|cut -d' ' -f1`
  kill $PID
fi
