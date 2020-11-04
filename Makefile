CC=gcc
CFLAGS=-g -O0 -march=native -m64
BINS=hfc

all: $(BINS)

.PHONY: huffmantree.o
huffmantree.o: huffmantree.c
	$(CC) $(CFLAGS) -c $^

.PHONY: menu.o
menu.o: menu.c
	$(CC) $(CFLAGS) -lncursesw -c $^

.PHONY: hfc
hfc: menu.o huffmantree.o
	$(CC) $(CFLAGS) -o $@ $^ -lncursesw

.PHONY: clean
clean:
	rm -f *.o

.PHONY: test
test: hfc
	./hfc hfc test
