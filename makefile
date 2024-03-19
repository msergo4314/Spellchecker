CC = gcc
# CFLAGS = -fopenmp -Wall -g -std=c11 -lpthread
CFLAGS = -Wall -std=c11 -lpthread
# CFLAGS = -O3 -Wall-g -std=c11 -lpthread # agressively optimized by the compiler

SRCS = threads_AS.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = A2.exe

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm *.o $(EXECUTABLE).exe