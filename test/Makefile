CC = gcc
CFLAGS = -O0 -fno-omit-frame-pointer

test: test.c
	$(CC) $(CFLAGS) -o test test.c

multi: multi.c
	$(CC) $(CFLAGS) -lm -o multi multi.c

clean:
	rm -f test multi
