#Boris Skurikhin
#1007339
#Assignment 1 - Module 1

CC = gcc
CFLAGS = -Wall -ansi -pedantic -std=c11 -Iinclude/

all: list parser

parser: src/CalendarParser.c
	$(CC) $(CFLAGS) src/CalendarParser.c -o bin/parser.out
list: src/LinkedListAPI.c
	$(CC) $(CFLAGS) src/LinkedListAPI.c -c -o bin/list.out
clean:
	rm bin/parser.out
	rm bin/list.out
