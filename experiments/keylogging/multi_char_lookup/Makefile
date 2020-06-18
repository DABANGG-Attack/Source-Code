all: dff
dff: dff.c ../cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: dfr
dfr: dfr.c ../cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
clean:
	rm dff dfr