#!/bin/bash

if ! test -e config
then
  echo "Please run ./install.sh before starting."
  exit 1
fi

if grep -q "replace" config
then
  echo "Please update your config file before starting!"
  exit 1
fi

DATADIR=`grep DATA config|cut -d'=' -f2`
BOATID=`grep BOATID config|cut -d'=' -f2`
APIKEY=`grep APIKEY config|cut -d'=' -f2`

for f in $DATADIR/done/*.gz
do
 echo "Processing $f"
 BN=`basename $f`
 URL="https://community.nakedsailor.blog/api.beta/boat/$BOATID/hash/$BN"
 echo $URL
 B64HASH=$(curl -s $URL)
 REMOTEHASH=`echo "$B64HASH" | base64 -d | hexdump -ve '1/1 "%.2x"'`
 MYHASH=`md5sum $f|cut -d' ' -f1`
 echo "  Hash: -$REMOTEHASH- == -$MYHASH-"
 if test "$MYHASH" = "$REMOTEHASH"
 then
    echo "  Deleting"
    rm $f
 fi
done
