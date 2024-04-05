CC = gcc
# CFLAGS = -fopenmp -Wall -g -std=c11 -lpthread
CFLAGS = -Wall -std=c11 -lpthread
# CFLAGS = -O3 -Wall-g -std=c11 -lpthread # agressively optimized by the compiler

SRCS = spellchecker.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = spellchecker.exe

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f *.o $(EXECUTABLE) debug.txt username.out