#!/bin/bash

RUNNING=`ps axuw|grep charlotte-client|grep -v grep`
if test -n "$RUNNING"
then
  PID=`ps -ax -o pid,args|grep charlotte-client|grep -v grep|sed -e 's/^[[:space:]]*//'|cut -d' ' -f1`
  kill $PID
fi
