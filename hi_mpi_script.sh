#!/bin/bash -l
#SBATCH -N 1
#SBATCH -n 8
#SBATCH -t 0:10:00
#SBATCH -p GPU-shared
#SBATCH --gres=gpu:p100:2
#SBATCH -J hi_mpi
 
# This job requests use of one GPU node.
# This requests 32 processors on said GPU node.
# It requests both p100 GPUs connected to that node.
# if you change -p GPU to -p GPU-shared then other jobs might be running on the same node.
# however, with the numbers given, we're basically requesting access to an entire GPU node for ourselves.
# Not sure how fast that would pick up.
# We might be better off changing -n to a more modest number, and -p GPU to -p GPU-shared
 
# One P100 node contains 2 p100 GPUs, 2 Intel Xeon E5-2683 v4 CPUs, 128 GB RAM
# -- Each of those two CPUs contains 16 cores
 
# One K80 node contains 4 k80 GPUs, 2 Intel Xeon E5-2695 v3 CPUs, 128 GB RAM
# -- Each of those two CPUs contains 14 cores
export  I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0

module purge
module load psc_path/1.1 slurm/default
module load gcc mpi/gcc_openmpi
module load tensorflow/1.5_gpu

# -n arg should be equal to N * ntasks-per-node

while :
do
sleep 1
mpiexec -n 8 ./hi_mpi -ngames 10000
sleep 1
python cnn_resnet.py
done