# Makefile para testes básicos
all: clean
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client
	gcc -Wall server.c common.o -o server

test: clean
	gcc -Wall -c common.c
	gcc -Wall test_client.c common.o -o client
	gcc -Wall test_server.c common.o -o server

aline: clean
	gcc -Wall -c teste2.c -o common.o
	gcc -Wall teste.c common.o -o client
	gcc -Wall teste3.c common.o -o server

clean:
	rm -f common.o client server test_client test_server
