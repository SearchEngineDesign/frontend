#!/bin/bash
ulimit -n 65000
make 
./server 8000 ./.www