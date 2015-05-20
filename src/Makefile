# Makefile

CC = g++
CFLAGS = -Wall -O3

CSRC = agent.cpp pipe.cpp
HSRC = pipe.h
OBJ = $(CSRC:.c=.o)

%o:%c $(HSRC)
	$(CC) $(CFLAGS) -c $<

# additional targets
.PHONY: clean

agent: $(OBJ)
	$(CC) -lm $(CFLAGS) -o agent $(OBJ)
