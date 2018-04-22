#!/bin/bash -l
#SBATCH -N 4
#SBATCH -n 4
#SBATCH -t 0:30:00
#SBATCH -p GPU
#SBATCH --gres=gpu:p100:2
#SBATCH -J hi_mpi
#SBATCH -o hi_mpi.out

export  I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0

module purge
module load psc_path/1.1 slurm/default
module load gcc mpi/gcc_openmpi
module load tensorflow/1.5_gpu


while :
do
python cnn_resnet.py
sleep 1
./param_serial -rank 0 -ngames 50
mpiexec -npernode 1 ./param_serial -rank 0 -ngames 20
sleep 1
done