#!/bin/bash

CLIENT=114.117.203.221
# CLIENT=49.51.91.31

# 42.193.36.160
# 139.186.147.186
# 139.155.242.23
# 114.117.203.221



scp ubuntu@$CLIENT:/home/ubuntu/exp/results.tar ./exp/results_$CLIENT.tar
