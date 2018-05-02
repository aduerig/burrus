#!/bin/bash -l

while :
do
python cnn_resnet.py
sleep 1
./record_games -rank 0 -ngames 5
sleep 1
done