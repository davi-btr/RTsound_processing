CPPFLAGS=-g
CFLAGS=-O0 -Wall -pedantic -Wno-variadic-macros
LDLIBS=-lasound -lm

PROGS= test sndprocess

.PHONY: all
all: $(PROGS)

.PHONY: clean
clean:
	rm -f *~ $(PROGS) *.o recorded.dat

