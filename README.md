# othello
C++ - Using distributed reinforcement learning to play othello

## Dependancies

### Linux / OSX
Python 2.7
posix_ipc
tensorflow 1.5
c++14

### Windows
No current support

## Description of files

### cnn_resnet.py
The main model for the MonteCarlo players to use. Uses tensorflow. Contains the train() script that play_and_train.py calls.

### param_serial.cpp
Plays a certain number of games on one processor. Command line args are -rank and -ngames. Spawns a python_model_communicator.py to comminucate with.  
Example call:  
./param_serial -rank 0 -ngames 10

### python_model_communicator.py
Runs from param_serial.cpp and communicates through shared memory. Serves requests for tensorflow forward passes.

### play_and_train.py
Calls the hi_mpi_script.sh and cnn_train.sh over and over again performing the full improvement loop.

### engine.hpp + engine.cpp
The engine for the othello game. Handles playing moves, finding legal moves, and tracking the board state.

### player.hpp + player.cpp
Definitions for types of players.

### driver.cpp
Plays two montecarlo players against each other and reports the win.

### play.cpp
Plays two players against each other and reports the win with timing results.
