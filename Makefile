
INCLUDES=-I.

CFLAGS= -O4 $(INCLUDES)

OBJS= main.o image.o dsp.o filter.o 

atpdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

main.o:	main.c cmap.h
dsp.o:	dsp.c filtercoeff.h filter.h
filter.o: filter.c filter.h
image.o: image.c satcal.h

clean:
	rm -f *.o atpdec
