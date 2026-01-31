CC=gcc
CFLAGS=-Wall -Werror -g -pthread


dine : dawdle.o dine.o
	$(CC) $(CFLAGS) -o dine dawdle.o dine.o -lpthread -lrt

dine.o : dine.c
	$(CC) $(CFLAGS) -c dine.c

dawdle.o : dawdle.c
	$(CC) $(CFLAGS) -c dawdle.c

