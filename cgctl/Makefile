# Define the C compiler to use
CC = gcc

# Define any compile-time flags
CFLAGS = -std=c99 -Wall

# Define any directories containing header files
INCLUDES = -I./include

# Define the C source files
SRCS = ./cgctl.c

# Define the C object files 
OBJS = $(SRCS:.c=.o)

LIBS = -l cgos -l jansson

# Define the executable file 
MAIN = cgctl

.PHONY: clean

all:    $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -s -o $(MAIN) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN)

install: $(MAIN)
	cp $(MAIN) /usr/local/bin/
