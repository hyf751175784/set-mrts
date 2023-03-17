#!/bin/bash

while [ '' == '' ]
do
ssh_agent_1=`ps aux | grep -E 'ssh \-fCNR 10000' | grep -v grep | wc -l`
echo $ssh_agent_1

if [ '$ssh_agent_1' == '0' ]
then
	
fi

sleep 10

done
