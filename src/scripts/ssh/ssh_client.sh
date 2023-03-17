#!/bin/bash

PORT_B1=10001
PORT_B2=10000
USER_B=ubuntu

# for PID in `ps -ef | grep "ssh -fCN" | awk '{print $2}'`
# do
# 	kill -9 $PID
# done

ssh -fNL  *:$PORT_B2:localhost:$PORT_B1 ubuntu@localhost
