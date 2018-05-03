# othello
C++ - Using distributed reinforcement learning to play othello

## Dependancies

### Linux / OSX
Python 3.5.2

posix_ipc

tensorflow 1.8

c++14

### Windows
No current support

## Description of files

### cnn_resnet.py
The main model for the MonteCarlo players to use. Uses tensorflow. Contains the train() script that play_and_train.py calls.

### record_games.cpp
Plays a certain number of games on one processor. Command line args are -rank and -ngames. Spawns a python_model_communicator.py to comminucate with.  
Example call:  
./param_serial -rank 0 -ngames 10

### python_model_communicator.py
Runs from record_games.cpp and communicates through shared memory. Serves requests for tensorflow forward passes.

### train_serial.sh
performs the full improvement loop.

### engine.hpp + engine.cpp
The engine for the othello game. Handles playing moves, finding legal moves, and tracking the board state.

### player.hpp + player.cpp + montecarlo.cpp + minimax.cpp + human.cpp + random.cpp
Definitions for types of players.

### play_test_games.cpp
Plays two players against each other and reports the win.

### play.cpp
***currently doesnt work***
Plays two players against each other and reports the win with timing results.
