#!/bin/bash

ALI_CVM=47.106.80.141
CLIENT=118.24.250.149

echo "Reload to server..."
scp ./config.xml root@$ALI_CVM:/root/exp/config.xml
scp ./bt_server root@$ALI_CVM:/root/exp/bt_server

echo "Reload to clients..."
scp ./bt_exec.sh root@$Lab_1:/root/exp/bt_exec.sh
scp ./config.xml root@$Lab_1:/root/exp/config.xml
scp ./bt_client root@$Lab_1:/root/exp/bt_client
