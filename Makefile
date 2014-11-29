LDFLAGS+=-lcurses
CFLAGS+=-std=c11 -Wall -g

all: deemacs

deemacs: deemacs.o input.o
