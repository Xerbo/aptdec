
INCLUDES=-I.

CFLAGS= -O3 $(INCLUDES)

OBJS= main.o image.o dsp.o filter.o reg.o fcolor.o

atpdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

main.o:	main.c
dsp.o:	dsp.c filtercoeff.h filter.h
filter.o: filter.c filter.h
image.o: image.c satcal.h
fcolor.o : fcolor.c

clean:
	rm -f *.o atpdec
