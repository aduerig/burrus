#!/usr/bin/env bash

# Linker opts should be blank for OS X, FreeBSD and OpenSolaris
#LINKER_OPTIONS=""

# On Linux, we must link with realtime and thread libraries
LINKER_OPTIONS="-lrt -lpthread"

g++ -Wall -std=c++11 -o new_premise new_premise.cpp -L. $LINKER_OPTIONS