#!/bin/bash

./run_indexserver.sh
while [ $? -ne 0 ]; do
    ./run_indexserver.sh
done