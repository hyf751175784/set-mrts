#!/bin/bash

# usage: sudo ./ssh_server.sh client_ip remote_port local_port

PORT_B1=10001
USER_B=ubuntu
IP_B=1.14.60.24
IP_B_1=42.193.36.160
IP_B_2=139.186.147.186
IP_B_3=139.155.242.23
IP_B_4=114.117.203.221

# 42.193.36.160
# 139.186.147.186
# 139.155.242.23
# 114.117.203.221

ssh -fNR  $PORT_B1:localhost:10000 $USER_B@$IP_B

# ssh -fNR  $PORT_B1:localhost:10000 $USER_B@$IP_B_1
# ssh -fNR  $PORT_B1:localhost:10000 $USER_B@$IP_B_2
# ssh -fNR  $PORT_B1:localhost:10000 $USER_B@$IP_B_3
# ssh -fNR  $PORT_B1:localhost:10000 $USER_B@$IP_B_4

# autossh -M 55557 -fNR $PORT_B1:localhost:$3 $USER_B@$IP_B

# autossh -M 55556 -fNR 139.186.147.186:localhost:10000 $USER_B@$IP_B
