CC = g++
CFLAGS = -std=c++14 -Ofast -Wall -Wno-unused-variable -Wno-unused-value -Wno-comment -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-delete-non-virtual-dtor -g
TESTS_HEADERS = ../engine.hpp
TESTS_OBJECTS = ../engine.cpp tests.cpp


all: tests

tests: $(TESTS_OBJECTS)
	$(CC) $(CFLAGS) $(TESTS_HEADERS) $(TESTS_OBJECTS) -o tests

clean:
	-rm -f tests
