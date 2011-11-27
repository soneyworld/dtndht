#!/bin/bash

# create version file
. mkversion.sh $
# create version file
. mkversion.sh $@@

autoreconf --force --install
