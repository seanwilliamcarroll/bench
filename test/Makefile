CC = gcc
CFLAGS = -O0 -fno-omit-frame-pointer

all: test multi threaded

test: test.c
	$(CC) $(CFLAGS) -o test test.c

multi: multi.c
	$(CC) $(CFLAGS) -lm -o multi multi.c

threaded: threaded.c
	$(CC) $(CFLAGS) -lpthread -lm -o threaded threaded.c

clean:
	rm -f test multi threaded
