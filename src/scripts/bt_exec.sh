#!/bin/bash
if [ "$#" -eq "1" ];
then
	default_num=$1
else
	default_num=4
fi								

for((n=0;n<$default_num;n++))
do
	nohup ./bt_client &
	sleep 1
done
