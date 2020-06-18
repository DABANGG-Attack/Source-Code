all: ff_stepped_distribution
ff_stepped_distribution: ff_stepped_distribution.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c -lm
all: fr_stepped_distribution
fr_stepped_distribution: fr_stepped_distribution.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
clean:
	rm ff_stepped_distribution fr_stepped_distribution