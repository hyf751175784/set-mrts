#!/bin/bash

PRO_NAME=$1
CEILING=$2

while true ; do

  NUM=`ps aux | grep -w ./$PRO_NAME | grep -v grep |wc -l`
  echo $NUM
  if [ "${NUM}" -lt "$CEILING" ];then
    nohup ./bt_client &
  fi

  sleep 30s

done

exit 0