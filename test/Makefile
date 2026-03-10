CC = gcc
CXX = g++
CFLAGS = -O0 -fno-omit-frame-pointer
CXXFLAGS = -std=c++20 -O1 -fno-omit-frame-pointer

all: test multi threaded pipeline

test: test.c
	$(CC) $(CFLAGS) -o test test.c

multi: multi.c
	$(CC) $(CFLAGS) -lm -o multi multi.c

threaded: threaded.c
	$(CC) $(CFLAGS) -lpthread -lm -o threaded threaded.c

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -lpthread -lm -o pipeline pipeline.cpp

clean:
	rm -f test multi threaded pipeline
