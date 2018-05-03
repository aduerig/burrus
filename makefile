CC = g++
MPICC = mpic++
CFLAGS = -std=c++14 -O1 -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
LINKER_OPTIONS= -lrt -lpthread


PLAY_TEST_GAMES_HEADERS = communicator.hpp engine.hpp player.hpp
PLAY_HEADERS = communicator.hpp engine.hpp player.hpp
RECORD_HEADERS = communicator.hpp engine.hpp player.hpp

PLAY_TEST_GAMES_OBJECTS = communicator.cpp engine.cpp play_test_games.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp
PLAY_OBJECTS = communicator.cpp play.cpp engine.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp
RECORD_OBJECTS = communicator.cpp engine.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp record_games.cpp


all: play_test_games record_games

play_test_games: $(PLAY_TEST_GAMES_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_TEST_GAMES_HEADERS) $(PLAY_TEST_GAMES_OBJECTS) -o play_test_games $(LINKER_OPTIONS)

play: $(PLAY_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_HEADERS) $(PLAY_OBJECTS) -o play $(LINKER_OPTIONS)

record_games: $(RECORD_OBJECTS)
	$(CC) $(CFLAGS) $(RECORD_HEADERS) $(RECORD_OBJECTS) -o record_games $(LINKER_OPTIONS)

clean:
	-rm -f play_test_games
	-rm -f play
	-rm -f record_games
	-rm -f callgrind.out.*
	-rm -f out.log
