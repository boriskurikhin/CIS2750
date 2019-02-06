#Boris Skurikhin
#1007339
#Assignment 1 - Module 1

CC = gcc
CFLAGS = -Wall -g -std=c11 -Iinclude/

all: list parser

list: src/LinkedListAPI.c
	$(CC) -c -fPIC $(CFLAGS) src/LinkedListAPI.c -o bin/list.o
	$(CC) -shared -o bin/liblist.so bin/list.o
parser: src/CalendarParser.c
	$(CC) -c -fPIC $(CFLAGS) src/CalendarParser.c -o bin/parser.o
	$(CC) -shared -o bin/libcal.so bin/parser.o bin/list.o
clean:
	rm bin/*.o
	rm bin/*.so
