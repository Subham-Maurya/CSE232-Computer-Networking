# Makefile for compiling C files

all: server_select client server_single server_multi

server_select: server_select.c
	gcc server_select.c -o server_select

client: client.c
	gcc client.c -o client

server_single: server_single.c
	gcc server_single.c -o server_single

server_multi: server_multi.c
	gcc server_multi.c -o server_multi

clean:
	rm -f server_select client server_single server_multi
