# othello
C++ - Using distributed reinforcement learning to play othello

## Running on bridges

To run interactively (not with a job script):
	- Run "module load tensorflow/1.5_gpu"
	- Run "pip install --user posix_ipc"
	- Then run "interact -gpu" to get a hold of gpu
	- Run whatever file needed

Warning, if "module purge" is run, then "module load slurm/default psc_path/1.1" must be run to interact with the GPU.

## Description of files

### cnn_resnet.py
The main model for the MonteCarlo players to use. Uses tensorflow. Contains the train() script that play_and_train.py calls.