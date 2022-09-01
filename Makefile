# CPPFLAGS=-g
CC=gcc
CFLAGS=-g -Wall # -pedantic -Wno-variadic-macros
# LDLIBS=-lasound -lm

HEADS=-I./#include/
LIBHOME=-L./#libs
LIBS=-lsndutils -lfourier -laudiostream -lasound -lm
SRCS=sndprocess.c sndutils.c fourier.c audiostream.c
OBJS=$(SRCS:.c=.o)
PROGS=sndprocess

# .PHONY: all
all:	$(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) $(HEADS) -o $(PROGS) $(OBJS) $(LIBHOME) $(LIBS)

.PHONY: clean
clean:
	rm -f *~ $(PROGS) *.o

