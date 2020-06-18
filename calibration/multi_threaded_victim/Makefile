all: h0mae
h0mae: h0mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: h01mae
h01mae: h01mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: h1mae
h1mae: h1mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: h0123mae
h0123mae: h0123mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: h123mae
h123mae: h123mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c
all: mae
mae: mae.c ./cacheutils.h
	gcc -std=gnu11 -O2 -pthread -o $@ $@.c

clean:
	rm h0mae h01mae h1mae h0123mae h123mae mae 