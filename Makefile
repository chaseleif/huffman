CC=gcc
CFLAGS=-Wall -O3 -march=native -m64
BINS=hfc hfcncurses
all: $(BINS)

.PHONY: libhfc.so
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

.PHONY: driver.o
driver.o: driver.c
	$(CC) $(CFLAGS) -c $^

.PHONY: menu.o
menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -D_GNU_SOURCE -c $^

.PHONY: hfc
hfc: driver.o libhfc.so
	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

.PHONY: hfcncurses
hfcncurses: menu.o libhfc.so
	$(CC) $(CFLAGS) --enable-widec -lncursesw -D_GNU_SOURCE -o $@ $^ -L. -lhfc

.PHONY: clean
clean:
	rm -f *.o

.PHONY: test
test: hfc
	make driver

.PHONY: curses
curses: hfcncurses
	./hfcncurses

.PHONY: driver
driver: hfc
	./hfc -v -c hfc -o test.hfc
	./hfc -v -d test.hfc -o hfc.restored


