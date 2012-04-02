#!/bin/bash

for ((port=11000;port < 12000; port+=2 ))
do
   bash lifetime.sh $port &
   sleep 1m
done
