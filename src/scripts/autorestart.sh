#!/bin/bash

while [ '' == '' ]
do
client_num=`ps aux | grep -E './client' | grep -v grep | wc -l`
echo $client_num

if [ "$client_num" -lt "4" ]
then
	nohup ./client &
fi

sleep 30

done
