CC = gcc
CFLAGS = -Wall -g -std=c11 -lpthread

SRCS = threads_AS.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = A2

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm *.o $(EXECUTABLE)