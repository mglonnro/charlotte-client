BOATID=2zGrCQC2X9X2LbkzMhFm
DEVICE=`ls /dev/serial/by-id/*NMEA*`

(./actisense-serial -r $DEVICE | ./analyzer -json | ./charlotte-client $BOATID) > ./daemon.log 2>&1 &

