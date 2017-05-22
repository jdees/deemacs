CC=gcc
# your platform might need "-lerr" as LDFLAGS
LDFLAGS+=-lcurses -lc
CFLAGS+=-std=c11 -Wall -g --pedantic

all: deemacs

deemacs: deemacs.o input.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm *.o deemacs
