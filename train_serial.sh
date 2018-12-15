#!/bin/bash -l

# Intended for use in an interactive GPU session on Bridges


module purge
module load psc_path/1.1 slurm/default
module load gcc mpi/gcc_openmpi
module load tensorflow/1.5_gpu


while :
do
python cnn_resnet.py
sleep 1
./param_serial -rank 0 -ngames 60
sleep 1
done