#!/bin/bash

ALI_CVM=47.106.80.141
CLIENT=1.14.60.24
# CLIENT_1=42.193.36.160
# CLIENT_2=139.186.147.186
# CLIENT_3=139.155.242.23
# CLIENT_4=114.117.203.221
# CLIENT=49.51.91.31

# 42.193.36.160
# 139.186.147.186
# 139.155.242.23
# 114.117.203.221

#echo "Reload to server..."
# scp ./config.xml root@$ALI_CVM:/root/exp/config.xml
# scp ./bt_server root@$ALI_CVM:/root/exp/bt_server
# scp ./config_csl.xml root@$ALI_CVM:/root/exp/config_csl.xml
# scp ./csl_server root@$ALI_CVM:/root/exp/csl_server
# scp ./config_csn.xml root@$ALI_CVM:/root/exp/config_csn.xml
# scp ./csn_server root@$ALI_CVM:/root/exp/csn_server

echo "Reload to clients..."
# scp ./bt_exec.sh ubuntu@$CLIENT:/home/ubuntu/exp/bt_exec.sh
# scp ./terminate.sh ubuntu@$CLIENT:/home/ubuntu/exp/terminate.sh
# scp ./auto_update.sh ubuntu@$CLIENT:/home/ubuntu/exp/auto_update.sh
scp ./config.xml ubuntu@$CLIENT:/home/ubuntu/exp/config.xml
# scp ./bt_client ubuntu@$CLIENT:/home/ubuntu/exp/bt_client
# scp ./ut_exec.sh ubuntu@$CLIENT:/home/ubuntu/exp/ut_exec.sh
# scp ./config_ut.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_ut.xml
# scp ./ut_client ubuntu@$CLIENT:/home/ubuntu/exp/ut_client
# scp ./csl_exec.sh ubuntu@$CLIENT:/home/ubuntu/exp/csl_exec.sh
# scp ./config_csl.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_csl.xml
# scp ./csl_client ubuntu@$CLIENT:/home/ubuntu/exp/csl_client
# scp ./csn_exec.sh ubuntu@$CLIENT:/home/ubuntu/exp/csn_exec.sh
# scp ./config_csn.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_csn.xml
# scp ./csn_client ubuntu@$CLIENT:/home/ubuntu/exp/csn_client
# scp ./gard.sh ubuntu@$CLIENT:/home/ubuntu/exp/gard.sh
# scp scripts/ssh/ssh_client.sh ubuntu@$CLIENT:/home/ubuntu/exp/ssh_client.sh
# scp scripts/ssh/ssh_termination.sh ubuntu@$CLIENT:/home/ubuntu/exp/ssh_termination.sh


# scp ./rerun.sh ubuntu@$CLIENT_1:/home/ubuntu/exp/rerun.sh
# scp ./rerun.sh ubuntu@$CLIENT_2:/home/ubuntu/exp/rerun.sh
# scp ./rerun.sh ubuntu@$CLIENT_3:/home/ubuntu/exp/rerun.sh
# scp ./rerun.sh ubuntu@$CLIENT_4:/home/ubuntu/exp/rerun.sh


# scp ./config.xml ubuntu@$CLIENT_1:/home/ubuntu/exp/config.xml
# scp scripts/ssh/ssh_client.sh ubuntu@$CLIENT_1:/home/ubuntu/exp/ssh_client.sh
# scp scripts/ssh/ssh_termination.sh ubuntu@$CLIENT_1:/home/ubuntu/exp/ssh_termination.sh

# scp ./config.xml ubuntu@$CLIENT_2:/home/ubuntu/exp/config.xml
# scp scripts/ssh/ssh_client.sh ubuntu@$CLIENT_2:/home/ubuntu/exp/ssh_client.sh
# scp scripts/ssh/ssh_termination.sh ubuntu@$CLIENT_2:/home/ubuntu/exp/ssh_termination.sh

# scp ./config.xml ubuntu@$CLIENT_3:/home/ubuntu/exp/config.xml
# scp scripts/ssh/ssh_client.sh ubuntu@$CLIENT_3:/home/ubuntu/exp/ssh_client.sh
# scp scripts/ssh/ssh_termination.sh ubuntu@$CLIENT_3:/home/ubuntu/exp/ssh_termination.sh

# scp ./config.xml ubuntu@$CLIENT_4:/home/ubuntu/exp/config.xml
# scp scripts/ssh/ssh_client.sh ubuntu@$CLIENT_4:/home/ubuntu/exp/ssh_client.sh
# scp scripts/ssh/ssh_termination.sh ubuntu@$CLIENT_4:/home/ubuntu/exp/ssh_termination.sh

# scp ./bt_exec.sh ubuntu@139.186.147.186:/home/ubuntu/exp/bt_exec.sh

# scp ./config_1.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_1.xml
# scp ./config_2.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_2.xml
# scp ./config_3.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_3.xml
# scp ./config_4.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_4.xml
# scp ./config_5.xml ubuntu@$CLIENT:/home/ubuntu/exp/config_5.xml