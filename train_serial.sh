#!/bin/bash -l

while :
do
python cnn_resnet.py
sleep 1
./record_games -rank 0 -ngames 100 -iter 100
sleep 1
./play_test_games -iter 100 -ngames 50 -name model_0 -player1 monte -player2 rand
sleep 1
done