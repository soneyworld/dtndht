#!/bin/bash
TOOLS_PATH=/home/till/sources/dht/tools
PORT=9999
RAND=$RANDOM
ANNOUNCE="$TOOLS_PATH/src/dhtannounce"
LOOKUP="$TOOLS_PATH/src/dhtlookup"
#LOG_FILE="lifetime_$(date +%Y%m%d)_$RAND.log"
LOG_FILE="logfile_$RAND"
echo "RUNNING EVALUATION WITH RANDOM NUMBER: $RAND to $LOG_FILE"
if [ ! -d $TOOLS_PATH ]; then
	echo "tools directory could not be found"
	exit
fi
echo "start announce process for dtn://test.dtn/$PORT/$RAND"
$ANNOUNCE -4 -p $PORT -a dtn://test.dtn/$PORT/$RAND > $LOG_FILE
echo "start lookup process for dtn://test.dtn/$PORT/$RAND"
$LOOKUP -c -4 -p $PORT+1 -l dtn://test.dtn/$PORT/$RAND >> $LOG_FILE &
PID=$!
#sleep 1m
kill -s USR1 $PID
sleep 5m
kill $PID
