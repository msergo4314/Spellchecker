CC = gcc #clang also may work
# CFLAGS = -fopenmp -Wall -g -std=c11 -lpthread
CFLAGS = -Wall -std=c11 -pedantic -lpthread -g # g flag is very good for valgrind

SRCS = spellchecker.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = spellchecker

all: $(EXECUTABLE) makefile header.h

$(EXECUTABLE): $(OBJS) header.h
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f *.o $(EXECUTABLE) debug.txt username.out