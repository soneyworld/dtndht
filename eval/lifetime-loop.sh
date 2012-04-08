#!/bin/bash

for ((port=12000;port < 13000; port+=2 ))
do
   bash lifetime.sh $port &
   sleep 1m
done
