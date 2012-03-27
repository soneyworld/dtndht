#!/bin/bash
IBRDTN_PATH=/home/till/sources/ibrdtn-dht
RAND=$RANDOM
IBRDTN_DAEMON="$IBRDTN_PATH/daemon/src/dtnd -c /home/till/dtnd.conf --nodiscover"
IBRDTN_PING="$IBRDTN_PATH/tools/src/dtnping --nowait --count 1 --src random_lookup_evaluation --size 1"
LOG_FILE="evaluation_random_lookups_$RAND.log"

echo "RUNNING RANDOM LOOKUP EVALUATION: $LOG_FILE"
if [ ! -d $IBRDTN_PATH ]; then
	echo "ibrdtn directory could not be found"
	exit
fi
echo "start ibrdtn daemon process"
$IBRDTN_DAEMON > $LOG_FILE &
IBRDTN_DAEMON_PID=$!
sleep 10s
echo "start dtnping processes for random lookups"
for (( c=0; c<100; c++ ))
do
	$IBRDTN_PING dtn://random_$RANDOM.dtn/random > /dev/null
done
sleep 10m
kill $IBRDTN_DAEMON_PID
wait $IBRDTN_DAEMON_PID
echo "evaluation saved to: $LOG_FILE"
