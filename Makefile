CC=gcc
CFLAGS=-O3 -march=native -m64 -funroll-loops
BINS=hfc

all: $(BINS)

.PHONY: huffmantree.o
huffmantree.o: huffmantree.c
	$(CC) $(CFLAGS) -pthread -c $^

.PHONY: menu.o
menu.o: menu.c
	$(CC) $(CFLAGS) -pthread -lncursesw -c $^

.PHONY: hfc
hfc: menu.o huffmantree.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lncursesw

.PHONY: clean
clean:
	rm -f *.o

.PHONY: test
test: hfc
	./hfc
