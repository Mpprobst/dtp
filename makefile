OBJECTS=UDPEchoClient-Timeout.o UDPEchoServer.o DieWithError.o
EXE=Server Client

Server: UDPEchoServer.c DieWithError.c
	gcc -o Server -w UDPEchoServer.c DieWithError.c

Client: UDPEchoClient-Timeout.c DieWithError.c
	gcc -o Client -w UDPEchoClient-Timeout.c DieWithError.c

clean:
	-rm $(OBJECTS) $(EXE)
