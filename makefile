CC = g++
MPICC = mpic++
CFLAGS = -std=c++14 -O1 -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
LINKER_OPTIONS= -lrt -lpthread


DRIVER_HEADERS = communicator.hpp engine.hpp player.hpp
PLAY_HEADERS = engine.hpp player.hpp
PARAM_HEADERS = engine.hpp player.hpp param_serial.hpp

DRIVER_OBJECTS = communicator.cpp engine.cpp driver.cpp player.cpp
PLAY_OBJECTS = play.cpp engine.cpp player.cpp
PARAM_OBJECTS = engine.cpp player.cpp param_serial.cpp


all: driver

driver: $(DRIVER_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_HEADERS) $(DRIVER_OBJECTS) -o driver $(LINKER_OPTIONS)

play: $(PLAY_OBJECTS)
	$(CC) $(CFLAGS) $(PLAY_HEADERS) $(PLAY_OBJECTS) -o play $(LINKER_OPTIONS)

param: $(PARAM_OBJECTS)
	$(CC) $(CFLAGS) $(PARAM_HEADERS) $(PARAM_OBJECTS) -o param_serial $(LINKER_OPTIONS)

himpi:
	$(MPICC) $(CFLAGS) hi_mpi.cpp -o hi_mpi $(LINKER_OPTIONS)

clean:
	-rm -f driver
	-rm -f play
	-rm -f callgrind.out.*
	-rm -f out.log
	-rm -f tensor_driver
	-rm -f param_serial
