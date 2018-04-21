#!/bin/bash -l
#SBATCH -W
#SBATCH -t 0:10:00
#SBATCH -p GPU-shared
#SBATCH --gres=gpu:p100:1
#SBATCH -J cnn_train

module purge
module load psc_path/1.1 slurm/default
module load gcc mpi/gcc_openmpi
module load tensorflow/1.5_gpu

python cnn_resnet.py