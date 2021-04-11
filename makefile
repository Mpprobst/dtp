OBJECTS=UDPEchoClient-Timeout.o UDPEchoServer.o DieWithError.o ack.o packet.o
EXE=server client

all: server client

server: UDPEchoServer.c DieWithError.c
	gcc -o server -w UDPEchoServer.c DieWithError.c packet.c ack.c

client: UDPEchoClient-Timeout.c DieWithError.c
	gcc -o client -w UDPEchoClient-Timeout.c DieWithError.c packet.c ack.c

clean:
	-rm $(OBJECTS) $(EXE)
