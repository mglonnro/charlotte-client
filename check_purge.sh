#!/bin/bash

for f in logs/done/*.gz
do
 echo "Processing $f"
 BN=`basename $f`
 URL="https://community.nakedsailor.blog/timescaledb/boat/2zGrCQC2X9X2LbkzMhFm/hash/$BN"
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
