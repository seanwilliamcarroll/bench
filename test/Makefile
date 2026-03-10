CC = gcc
CXX = g++
CFLAGS = -O0 -fno-omit-frame-pointer
CXXFLAGS = -std=c++20 -O1 -fno-omit-frame-pointer
BINDIR = bin

all: test multi threaded pipeline

$(BINDIR):
	mkdir -p $(BINDIR)

test: test.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/test test.c

multi: multi.c | $(BINDIR)
	$(CC) $(CFLAGS) -lm -o $(BINDIR)/multi multi.c

threaded: threaded.c | $(BINDIR)
	$(CC) $(CFLAGS) -lpthread -lm -o $(BINDIR)/threaded threaded.c

pipeline: pipeline.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -lpthread -lm -o $(BINDIR)/pipeline pipeline.cpp

clean:
	rm -rf $(BINDIR)
