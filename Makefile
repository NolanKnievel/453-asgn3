CC=gcc
CFLAGS=-Wall -Werror -g -pthread



dine: dine.c
	$(CC) $(CFLAGS) dine.c -o dine -lrt

clean:
	rm -f dine
