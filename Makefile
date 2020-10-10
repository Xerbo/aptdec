CC = gcc
BIN = /usr/bin
INCLUDES = -I.
PALETTE_DIR=\"$(PWD)/palettes\"
CFLAGS = -O3 -Wall -Wextra -Wno-missing-field-initializers -DPALETTE_DIR=$(PALETTE_DIR) $(INCLUDES)
OBJS = src/main.o src/image.o src/dsp.o src/filter.o src/libs/reg.o src/pngio.o src/libs/median.o src/color.o src/libs/argparse.o

aptdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lpng
	echo "Using the Makefile with GNU automake is now deprecated and will be removed soon, please use CMake instead."

src/libs/reg.o: src/libs/reg.c
src/libs/argparse.o: src/libs/argparse.c
src/color.o:    src/color.c
src/main.o:     src/main.c
src/libs/median.o: src/libs/median.c
src/dsp.o:      src/dsp.c
src/filter.o:   src/filter.c
src/image.o:    src/image.c
src/pngio.o:    src/pngio.c

clean:
	rm -f src/*.o aptdec

install:
	install -m 755 aptdec $(BIN)

uninstall:
	rm $(BIN)/aptdec
