#!/bin/bash

# create version file
. mkversion.sh $
# create version file
. mkversion.sh $@@
if [ ! -d "m4" ]; then
	mkdir "m4"
fi
automake --add-missing
autoreconf --force --install
