OBJECTS=UDPEchoClient-Timeout.o UDPEchoServer.o DieWithError.o
EXE=Server Client

Server: UDPEchoServer.c DieWithError.c
	gcc -o Server UDPEchoServer.c DieWithError.c

Client: UPDEchoClient-Timeout.c DieWithError.c
	gcc -o Server UDPEchoClient-Timeout.c DieWithError.c

clean:
	rm edit $(OBJECTS) $(EXE)
