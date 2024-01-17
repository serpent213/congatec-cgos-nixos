# Define the C compiler to use
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall

# Define any directories containing header files
INCLUDES =

# Define the C source files
SRCS = ./cgexporter.c

# Define the C object files
OBJS = $(SRCS:.c=.o)

LIBS = -l cgos -l microhttpd -l bsd

# Define the executable file
MAIN = cgexporter

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
