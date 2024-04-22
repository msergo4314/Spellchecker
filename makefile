CC = gcc
# CFLAGS = -fopenmp -Wall -g -std=c11 -lpthread
# CFLAGS = -Wall -std=c11 -lpthread -pedantic -g
CFLAGS = -Wall -O1 -std=c11 -pedantic -lpthread -g # optimized by the compiler

SRCS = spellchecker.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = spellchecker.exe

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) header.h
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f *.o $(EXECUTABLE) debug.txt username.out