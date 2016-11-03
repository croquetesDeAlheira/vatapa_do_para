all: client server

client: client.o
	gcc client.o -o client

server: server.o
	gcc server.o -o server

server.o: server.c inet.h
	gcc -Wall -c server.c -o server.o

client.o: client.c inet.h
	gcc -Wall -c client.c -o client.o

clean:
	rm -f *.o *~ server client
