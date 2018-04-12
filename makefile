CC = g++
MPICC = mpic++
# CFLAGS = -std=c++14 -Ofast -flto -funroll-loops -march=native -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
CFLAGS = -std=c++14 -Ofast -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
DRIVER_HEADERS = engine.hpp player.hpp
PLAY_HEADERS = engine.hpp player.hpp
PARAM_HEADERS = engine.hpp player.hpp

DRIVER_OBJECTS = engine.cpp driver.cpp player.cpp
PLAY_OBJECTS = play.cpp engine.cpp player.cpp
PARAM_OBJECTS = engine.cpp player.cpp param.cpp

all: driver play param

play: $(PLAY_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_HEADERS) $(PLAY_OBJECTS) -o play

driver: $(DRIVER_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_HEADERS) $(DRIVER_OBJECTS) -o driver

tensor_driver:
	$(CC) $(CFLAGS) tensorflow_test.cpp -o tensor_driver

param:
	$(MPICC) $(CFLAGS) $(PARAM_HEADERS) $(PARAM_OBJECTS) -o param

clean:
	-rm -f driver
	-rm -f play
	-rm -f callgrind.out.*
	-rm -f out.log
	-rm -f tensor_driver
	-rm -f param
