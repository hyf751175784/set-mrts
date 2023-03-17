#!/bin/bash

EXP_DIR=/home/ubuntu/exp
SRC_DIR=/home/ubuntu/works/SET-MRTS/src

cd $SRC_DIR
git pull origin lab-dev:lab-dev
make bt_client
cp bt_client $EXP_DIR
cd $EXP_DIR