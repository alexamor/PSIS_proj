# Compiler and flags
CC = /usr/bin/gcc
CFLAGS = -Wall
LIBS = -pthread -lSDL2 -lSDL2_ttf

# All targets rule
all: server client bot

# Individual program rules
server : server.o board_library.o UI_library.o
	$(CC) -o $@ $^ $(LIBS)

client : client.o board_library.o UI_library.o
	$(CC) -o $@ $^ $(LIBS)

bot : bot.o board_library.o UI_library.o
	$(CC) -o $@ $^ $(LIBS)

# Source file rules
board_library.c : board_library.h
UI_library.c : UI_library.h

# Compile rule, (.c to .o)
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c
