#!/bin/bash

inp=$1
inp=$((inp+0))
k=$(((inp / 3) + 1))
j=$((inp % 3))
echo "$j and $k"

if [[ $j -eq 2 ]] && [[ $k -eq 3 ]]
then
  echo "We are done! Ba-bye moon man!"
  exit 0
fi

./test2.sh "$inp"

echo "END OF SCRIPT for $inp"