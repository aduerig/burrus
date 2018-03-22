
CC = g++
CFLAGS = -std=c++14 -Ofast -flto -funroll-loops -march=native -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
HEADERS = engine.hpp
DRIVER_OBJECTS = engine.cpp driver.cpp


all: driver

driver: $(DRIVER_OBJECTS)
	$(CC) $(CFLAGS) $(HEADERS) $(DRIVER_OBJECTS) -o driver

clean:
	-rm -f driver
	-rm -f callgrind.out.*
	-rm -f out.log
