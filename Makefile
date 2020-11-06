# -march=native -m64
CC=gcc
CFLAGS=-std=c99 -Wall -g -O0
BINS=hfc hfcncurses
all: $(BINS)

.PHONY: libhfc.so driver.o menu.o hfc hfcncurses clean test curses driver
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

	$(CC) $(CFLAGS) -c $^

menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -D_GNU_SOURCE -c $^

hfc: driver.o libhfc.so
	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

hfcncurses: menu.o libhfc.so
	$(CC) $(CFLAGS) --enable-widec -lncursesw -D_GNU_SOURCE -o $@ $^ -L. -lhfc

clean:
	rm -f *.o

test: hfc
	make driver

curses: hfcncurses
	./hfcncurses

driver: hfc
	./hfc -v -c hfc -o test.hfc
	./hfc -v -d test.hfc -o hfc.restored

