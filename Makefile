
# -O3 -march=native -m64 -fstack-arrays -Wl-dead_strip

CC=gcc
CFLAGS=-Wall -g -O0
BINS=hfc hfcncurses
all: $(BINS)

.PHONY: libhfc.so
libhfc.so: huffmantree.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ -lc

.PHONY: menu.o
menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -c $^

.PHONY: driver.o
driver.o: driver.c
	$(CC) $(CFLAGS) -c $^

.PHONY: hfc
hfc: driver.o libhfc.so
	$(CC) $(CFLAGS) -o $@ $^ -L. -lhfc

.PHONY: hfcncurses
hfcncurses: menu.o libhfc.so
	$(CC) $(CFLAGS) -lncursesw -o $@ $^ -L. -lhfc

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
	./hfc -c hfc -o test.hfc


