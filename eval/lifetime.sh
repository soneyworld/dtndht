#!/bin/bash
TOOLS_PATH=/home/till/sources/dht/tools
PORT=$1
RAND=$RANDOM
ANNOUNCE="$TOOLS_PATH/src/dhtannounce"
LOOKUP="$TOOLS_PATH/src/dhtlookup"
LOG_FILE="evaluation_lifetime_$(date +%Y%m%d)_$PORT.log"
#LOG_FILE="evaluation_lifetime_$PORT.log"
echo "RUNNING EVALUATION WITH RANDOM NUMBER: $RAND" > $LOG_FILE
if [ ! -d $TOOLS_PATH ]; then
	echo "tools directory could not be found"
	exit
fi
$ANNOUNCE -4 -p $PORT -a dtn://test.dtn/$PORT/$RAND >> $LOG_FILE
$LOOKUP -c -4 -p $PORT+1 -l dtn://test.dtn/$PORT/$RAND >> $LOG_FILE &
PID=$!
for (( i = 0; i < 12 ; i++))
do
	sleep 5m
	kill -s USR1 $PID
done

sleep 5m
kill -s INT $PID
wait $PID
