#!/bin/bash
for ((  i = 1024 ;  i <= 1044;  i++  ))
do
	./dtndhttest $i 4000 > output/$i.out &
done

