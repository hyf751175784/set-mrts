#!/bin/bash
if [ "$#" -eq "1" ];
then
	num=$1
else
	num=4
fi					

nohup ./bt_server -c config_1.xml &
sleep 5
for((n=0;n<$num;n++))
do
	nohup ./bt_client -c config_1.xml &
done


nohup ./bt_server -c config_2.xml &
sleep 5
for((n=0;n<$num;n++))
do
	nohup ./bt_client -c config_2.xml &
done


nohup ./bt_server -c config_3.xml &
sleep 5
for((n=0;n<$num;n++))
do
	nohup ./bt_client -c config_3.xml &
done


nohup ./bt_server -c config_4.xml &
sleep 5
for((n=0;n<$num;n++))
do
	nohup ./bt_client -c config_4.xml &
done


nohup ./bt_server -c config_5.xml &
sleep 5
for((n=0;n<$num;n++))
do
	nohup ./bt_client -c config_5.xml &
done