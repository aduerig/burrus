# othello
C++ - Using distributed reinforcement learning to play othello

## Running on bridges

To run interactively (not with a job script):  
	- Run "module load tensorflow/1.5_gpu gcc"  
	- Run "pip install --user posix_ipc"  
	- Then run "interact -gpu" to get a hold of gpu  
	- Run "make"  
	- Run whatever file needed

Warning, if "module purge" is run, then "module load slurm/default psc_path/1.1" must be run to interact with the GPU.

## Description of files

### cnn_resnet.py
The main model for the MonteCarlo players to use. Uses tensorflow. Contains the train() script that play_and_train.py calls.

### engine.hpp + engine.cpp
The engine for the othello game. Handles playing moves, finding legal moves, and tracking the board state.

### param_serial.cpp
Plays a certain number of games on one processor. Command line args are -rank and -ngames.  
Example call:  
./param_serial -rank 0 -ngames 10

### driver.cpp
Plays two montecarlo players against each other and reports the win.