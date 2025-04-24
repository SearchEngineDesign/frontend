#!/bin/bash
ulimit -n 65000
make clean 
make 
./server 8000 ./.www