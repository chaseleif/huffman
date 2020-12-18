CC=gcc
CFLAGS=-std=c99 -Wall -O3 -march=native -m64
BINS=hfc libhfc.so hfcncurses
all: $(BINS)

.PHONY: libhfc.so driver.o hfc clean clear test driver
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

driver.o: driver.c
	$(CC) $(CFLAGS) -c $^

huffmantree.o: huffmantree.c
	$(CC) $(CFLAGS) -c $^

hfc: driver.o huffmantree.o
	$(CC) $(CFLAGS) -o $@ $^

#hfc: driver.o libhfc.so
#	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

clean:
	rm -f *.o

clear:
	rm -f *.o $(BINS) hfc.restored test.hfc

test: hfc
	make driver

driver: hfc
	./hfc -c hfc -o test.hfc
	./hfc -d test.hfc -o hfc.restored
	diff hfc hfc.restored

# the ncurses practice
cursestree.o: huffmantree.c
	$(CC) $(CFLAGS) -D_DONTFREETREES -o $@ -c $^
menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -D_GNU_SOURCE -c $^
hfcncurses: menu.o cursestree.o
	$(CC) $(CFLAGS) --enable-widec -lncursesw -D_GNU_SOURCE -o $@ $^
curses: hfcncurses
	./hfcncurses
