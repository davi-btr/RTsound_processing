CC=gcc
CFLAGS=-g -Wall

SRCHOME=./src/
HEADHOME=-I./include/
LIBHOME=-L./libs
LDFLAGS=-lsndutils -lfourier -laudiostream -lasound -lm
SRCS=$(SRCHOME)sndprocess.c $(SRCHOME)sndutils.c $(SRCHOME)fourier.c $(SRCHOME)audiostream.c
OBJS=$(SRCS:.c=.o)
PROGS=sndprocess

# .PHONY: all
all:	$(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) $(HEADHOME) -o $(PROGS) $(OBJS) $(LIBHOME) $(LDFLAGS)

# .PHONY: clean
clean:
	rm -f *~ $(PROGS) *.o

