CFLAGS = -g -Wall -O3
LDLIBS = -lcrypt

dht-example: dht-example.o dht.o

dht-bootstrap: bootstrap.o

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core bootstrap.o
