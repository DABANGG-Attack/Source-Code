#!/bin/bash

gcc -pthread -o dff dff.cpp -I/usr/local/include/ssl -L/usr/local/lib -lcrypto;
gcc -pthread -o dfr dfr.cpp -I/usr/local/include/ssl -L/usr/local/lib -lcrypto;
gcc -o server server.cpp -I/usr/local/include/ssl -L/usr/local/lib -lcrypto;



