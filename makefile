
CC = g++
CFLAGS = -std=c++14 -Ofast -flto -funroll-loops -march=native -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
DRIVER_HEADERS = engine.hpp
PLAY_HEADERS = engine.hpp player.hpp

DRIVER_OBJECTS = engine.cpp driver.cpp
PLAY_OBJECTS = play.cpp engine.cpp player.cpp


all: driver play

driver: $(DRIVER_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_HEADERS) $(DRIVER_OBJECTS) -o driver

play: $(PLAY_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_HEADERS) $(PLAY_OBJECTS) -o play

clean:
	-rm -f driver
	-rm -f play
	-rm -f callgrind.out.*
	-rm -f out.log
