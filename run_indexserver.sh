#!/bin/bash
export LD_LIBRARY_PATH=/home/tkmaher/searchengine/dynamicRanker/onnxruntime-linux-x64-gpu-1.21.1/lib:$LD_LIBRARY_PATH
ulimit -n 65000
make indexserver
./indexserver 8080