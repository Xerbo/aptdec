CC = gcc
BIN = /usr/bin
INCLUDES = -I.
PALETTE_DIR=\"$(PWD)/palettes\"
CFLAGS = -O3 -Wall -Wextra -Wno-missing-field-initializers -DPALETTE_DIR=$(PALETTE_DIR) $(INCLUDES)
OBJS = src/main.o src/image.o src/dsp.o src/filter.o src/reg.o src/pngio.o src/median.o src/color.o

aptdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng

reg.o:    src/reg.c
color.o:  src/color.c
main.o:   src/main.c
media.o:  src/median.c
dsp.o:    src/dsp.c
filter.o: src/filter.c
image.o:  src/image.c
pngio.o:  src/pngio.c

clean:
	rm -f src/*.o aptdec

install:
	install -m 755 aptdec $(BIN)

uninstall:
	rm $(BIN)/aptdec
