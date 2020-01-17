CC = gcc
BIN = /usr/bin
INCLUDES = -I.
CFLAGS = -O3 -DNDEBUG -Wall -Wextra $(INCLUDES)
OBJS = main.o image.o dsp.o filter.o reg.o fcolor.o pngio.o median.o color.o

aptdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

color.o:  color.c
main.o:	  main.c   offsets.h messages.h
dsp.o:	  dsp.c    filtercoeff.h filter.h
filter.o: filter.c filter.h
image.o:  image.c  offsets.h messages.h offsets.h
fcolor.o: fcolor.c offsets.h
pngio.o:  pngio.c  offsets.h messages.h

clean:
	rm -f *.o aptdec

install:
	install -m 755 aptdec $(BIN)

uninstall:
	rm $(BIN)/aptdec
