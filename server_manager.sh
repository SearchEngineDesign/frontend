#!/bin/bash

make indexserver
nohup ./run_indexserver.sh & > nohup.out
while true; do
  sleep 10
  if pgrep -x "indexserver" > /dev/null
  then
      sleep 100
  else
    echo "Indexserver stalled or stopped."
    nohup ./run_indexserver.sh & > nohup.out
    sleep 10
  fi
done