#!/bin/bash
ulimit -n 65000
make indexserver
./indexserver 8080