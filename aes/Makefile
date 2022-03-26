SSL_INCLUDE=$(shell pwd)/openssl-1.1.0f/build/include/openssl
SSL_LIB=$(shell pwd)/openssl-1.1.0f/build/lib

all: dff dfr server dabangg_noise
dff: dff.cpp
	g++ -I${SSL_INCLUDE} -L${SSL_LIB} -pthread -o $@ $@.cpp  -lcrypto
dfr: dfr.cpp
	g++ -I${SSL_INCLUDE} -L${SSL_LIB} -pthread -o $@ $@.cpp -lcrypto
server: server.cpp
	g++ -I${SSL_INCLUDE} -L${SSL_LIB} -o $@ $@.cpp -lcrypto
dabangg_noise: noise.cpp
	g++ -O2 -pthread -o dabangg_noise noise.cpp

clean:
	rm -f dff dfr server dabangg_noise

clean_data:
	rm -f computed_key.* *.csv