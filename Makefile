CC=gcc
# your platform might need "-lerr" as LDFLAGS
LDFLAGS+=-lcurses -lc
CFLAGS+=-std=c11 -Wall -g

all: deemacs

deemacs: deemacs.o input.o

clean:
	rm *.o deemacs
