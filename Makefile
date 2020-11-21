CC=gcc
CFLAGS=-std=c99 -Wall -O3 -march=native -m64
BINS=hfc hfcncurses libhfc.so
all: $(BINS)

.PHONY: libhfc.so driver.o menu.o hfc hfcncurses clean clear test curses driver
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

driver.o: driver.c
	$(CC) $(CFLAGS) -c $^

menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -D_GNU_SOURCE -c $^

hfc: driver.o libhfc.so
	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

hfcncurses: menu.o libhfc.so
	$(CC) $(CFLAGS) --enable-widec -lncursesw -D_GNU_SOURCE -o $@ $^ -L. -lhfc

clean:
	rm -f *.o

clear:
	rm -f *.o $(BINS)

test: hfc
	make driver

curses: hfcncurses
	./hfcncurses

driver: hfc
	./hfc -v -c hfc -o test.hfc
	./hfc -v -d test.hfc -o hfc.restored
	diff hfc hfc.restored

