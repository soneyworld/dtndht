#!/bin/bash

# create version file
. mkversion.sh $@
if [ ! -d "m4" ]; then
	mkdir "m4"
fi
aclocal
automake --add-missing
autoreconf --install
