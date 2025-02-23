# Makefile for gush

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = gush
OBJS = main.o parser.o builtins.o exec.o history.o utils.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c shell.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c shell.h
	$(CC) $(CFLAGS) -c parser.c

builtins.o: builtins.c shell.h
	$(CC) $(CFLAGS) -c builtins.c

exec.o: exec.c shell.h
	$(CC) $(CFLAGS) -c exec.c

history.o: history.c shell.h
	$(CC) $(CFLAGS) -c history.c

utils.o: utils.c shell.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(TARGET) $(OBJS)
