
INCLUDES=-I.

CFLAGS= -O3 -Wall $(INCLUDES)

OBJS= main.o image.o dsp.o filter.o reg.o fcolor.o

atpdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

main.o:	main.c version.h temppalette.h offsets.h
dsp.o:	dsp.c filtercoeff.h filter.h
filter.o: filter.c filter.h
image.o: image.c satcal.h offsets.h
fcolor.o : fcolor.c offsets.h

clean:
	rm -f *.o atpdec
