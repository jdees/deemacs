CC=gcc
# your platform might need "-lerr" as LDFLAGS
LDFLAGS+=-lcurses -lc -O2
CFLAGS+=-std=c11 -Wall --pedantic -O2

all: deemacs

deemacs: deemacs.o input.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm *.o deemacs
