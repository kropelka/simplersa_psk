CC=gcc
CFLAGS=-Wall -std=c99 -O2 -I/usr/local/include -L/usr/local/lib -D_BSD_SOURCE -g
LDLIBS=-lm -lgmp -lnewt

all: simplersa

simplersa: rsa_keygen.o rsa_decode.o simplersa.o main_menu.o
	$(CC) $(CFLAGS) -o simplersa simplersa.o rsa_keygen.o rsa_decode.o main_menu.o $(LDLIBS)

simplersa.o: simplersa.c rsa_keygen.h
	$(CC) $(CFLAGS) -c simplersa.c

rsa_keygen.o: rsa_keygen.c
	$(CC) $(CFLAGS) -c rsa_keygen.c

rsa_decode.o: rsa_decode.c rsa_decode.h
	$(CC) $(CFLAGS) -c rsa_decode.c

main_menu.o: main_menu.c main_menu.h
	$(CC) $(CFLAGS) -c main_menu.c
