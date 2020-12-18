CC=gcc
CFLAGS=-std=c99 -Wall -O3 -march=native -m64
BINS=hfc libhfc.so
all: $(BINS)

.PHONY: libhfc.so driver.o hfc clean clear test driver
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

driver.o: driver.c
	$(CC) $(CFLAGS) -c $^

hfc: driver.o libhfc.so
	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

clean:
	rm -f *.o

clear:
	rm -f *.o $(BINS)

test: hfc
	make driver

driver: hfc
	./hfc -c hfc -o test.hfc
	./hfc -d test.hfc -o hfc.restored
	diff hfc hfc.restored

# the ncurses practice
#menu.o: menu.c
#	$(CC) $(CFLAGS) -lncursesw -D_GNU_SOURCE -c $^
#hfcncurses: menu.o libhfc.so
#	$(CC) $(CFLAGS) --enable-widec -lncursesw -D_GNU_SOURCE -o $@ $^ -L. -lhfc
#curses: hfcncurses
#	./hfcncurses
