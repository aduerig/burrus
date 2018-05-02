CC = g++
MPICC = mpic++
CFLAGS = -std=c++14 -O1 -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
LINKER_OPTIONS= -lrt -lpthread


DRIVER_HEADERS = communicator.hpp engine.hpp player.hpp
PLAY_HEADERS = communicator.hpp engine.hpp player.hpp
RECORD_HEADERS = communicator.hpp engine.hpp player.hpp

DRIVER_OBJECTS = communicator.cpp engine.cpp driver.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp
PLAY_OBJECTS = communicator.cpp play.cpp engine.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp
RECORD_OBJECTS = communicator.cpp engine.cpp player.cpp human.cpp random.cpp minimax.cpp montecarlo.cpp record_games.cpp


all: driver record_games

driver: $(DRIVER_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_HEADERS) $(DRIVER_OBJECTS) -o driver $(LINKER_OPTIONS)

play: $(PLAY_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_HEADERS) $(PLAY_OBJECTS) -o play $(LINKER_OPTIONS)

record_games: $(RECORD_OBJECTS)
	$(CC) $(CFLAGS) $(RECORD_HEADERS) $(RECORD_OBJECTS) -o record_games $(LINKER_OPTIONS)

clean:
	-rm -f driver
	-rm -f play
	-rm -f record_games
	-rm -f callgrind.out.*
	-rm -f out.log
