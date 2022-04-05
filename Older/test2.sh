#!/bin/bash


echo "Got $1"

nex=$1
nex=$((nex+1))
./test.sh "$nex"