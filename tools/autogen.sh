#!/bin/bash

if [ ! -d "m4" ]; then
	mkdir "m4"
fi
aclocal
automake --add-missing
autoreconf --force --install
