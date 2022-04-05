#!/bin/bash

files=$(pwd)'/output/prefetcher_exp_2/*.txt'

for file in $files; do
  out=$(grep "Finished CPU 0 instructions:" "$file")
  for i in $out
  do
     echo "$i"
     echo ""
  done
  break
done



#oldIFS=$IFS
#IFS=$oldIFS