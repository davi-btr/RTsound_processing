CC=gcc
CFLAGS=-g -Wall

HEADHOME=-I./include/
LIBHOME=-L./libs
LDFLAGS=-lsndutils -lfourier -laudiostream -lasound -lm
SRCS=sndprocess.c sndutils.c fourier.c audiostream.c
OBJS=$(SRCS:.c=.o)
PROGS=sndprocess

# .PHONY: all
all:	$(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) $(HEADHOME) -o $(PROGS) $(OBJS) $(LIBHOME) $(LDFLAGS)

# .PHONY: clean
clean:
	rm -f *~ $(PROGS) *.o

