# Makefile for Tic Tac Toe Project
# Type:
#     	make		to compile mapto program
#     	make clean	to remove object files and executables
#     	make progname	to make just progname


CC = /usr/bin/gcc
CXX = /usr/bin/g++
OBJS = *.o
EXECS = tictactoe
CFLAGS = -Wall -g

all: $(EXECS)
.PHONY: all

tictactoe.o:

tictactoe: tictactoe.o
	$(CC) $(CFLAGS) -o tictactoe tictactoe.o -lncurses -lpthread

.PHONY: clean
clean:
	-rm -f $(OBJS)

.PHONY: cleanall
cleanall:
	-rm -f $(OBJS) $(EXECS)
