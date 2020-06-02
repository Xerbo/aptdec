CC = gcc
BIN = /usr/bin
INCLUDES = -I.
CFLAGS = -O3 -g -Wall -Wextra $(INCLUDES)
OBJS = main.o image.o dsp.o filter.o reg.o pngio.o median.o color.o

aptdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

reg.o:    reg.c
color.o:  color.c
main.o:   main.c
media.o:  median.c
dsp.o:    dsp.c
filter.o: filter.c
image.o:  image.c
pngio.o:  pngio.c

clean:
	rm -f *.o aptdec

install:
	install -m 755 aptdec $(BIN)

uninstall:
	rm $(BIN)/aptdec
