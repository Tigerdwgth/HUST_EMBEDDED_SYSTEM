CFLAGS=-I./
LDFLAGS=-L../ -lvosk -ldl -lpthread

all: server

server: server.o
	arm-linux-gnueabihf-gcc $^ -o $@ $(LDFLAGS)
	rm *.o; mv server ../

%.o: %.c
	arm-linux-gnueabihf-gcc $(CFLAGS) -c -o $@ $<

