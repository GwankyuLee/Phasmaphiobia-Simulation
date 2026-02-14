
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

SRCS = main.c functions.c helpers.c
OBJS = $(SRCS:.c=.o)
HDRS = defs.h functions.h helpers.h
TARGET = simulation

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

#make valgrind
#compiles the program and runs it with Memcheck to find memory leaks
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

#make helgrind
#compiles the program and runs it with Helgrind to find threading data races
helgrind: all
	valgrind --tool=helgrind --history-level=approx ./$(TARGET)

# Compile source files
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.csv
