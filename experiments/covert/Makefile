all: dff
dff: dff.c 
	gcc -o $@ $@.c
all: dfr
dfr: dfr.c
	gcc -o $@ $@.c
all: sender
dfr: sender.c
	gcc -o $@ $@.c
clean:
	rm dff dfr sender