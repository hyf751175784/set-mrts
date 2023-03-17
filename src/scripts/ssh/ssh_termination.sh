#!/bin/bash

# ssh -fNR 10001:localhost:10000 ubuntu@139.186.147.186
# `ps -ef | grep "ssh -fN" | awk '{print $2}'`


for PID in `ps -ef | grep "ssh -fN" | awk '{print $2}'`
do
	kill -9 $PID
done

