#!/bin/sh
#

set -e

# create version file
./mkversion.sh $@

if [ ! -d "m4" ]; then
	mkdir "m4"
fi

autoreconf -i
