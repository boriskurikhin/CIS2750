#Boris Skurikhin
#1007339
#Assignment 1 - Module 1

CC = gcc
CFLAGS = -Wall -ansi -pedantic -std=c11 -Iinclude/

all: list parser

list: src/LinkedListAPI.c
	$(CC) $(CFLAGS) -g src/LinkedListAPI.c -c -o bin/list.o
parser: src/CalendarParser.c bin/list.o
	$(CC) $(CFLAGS) -g src/CalendarParser.c bin/list.o -o bin/parser.out
clean:
	rm bin/parser.out
	rm bin/list.out
