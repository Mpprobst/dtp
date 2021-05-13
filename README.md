# dtp
Data Transmission Protocol project for CS571 - Networks at the University of Kentucky written by Michael Probst

## Files
This repository contains the following files:
- UDPEchoClient-Timeout.c: implementation of a client that sends packets to the server containing a portion of a larger message.
- UDPEchoServer.c: implementation of a server that handles clients which send packets of data containing portions of a larger message.
- DieWithError.c: a handler for killing the server and client when an error occurs.
- packet.c: definition of the data packets sent by client.
- ack.c: definition of the ACK packets sent by the server.
- makefile: used to compile the project

## Usage
To run this project, first enter the command `make all` in the project directory. This creates the executable files `server` and `client`.
Next, open a second terminal on either the same host or a new one.
One one terminal, run the server with the command `./server <PORT> <PACKETS TO DROP>`
`<PORT>` is the port on which the server will run. 7 is a typical echo port.
`<PACKETS TO DROP>` should be a list of integers separated by whitespace.

On the other terminal, run the client with the command `./client <SERVER IP> <SERVER PORT>`
`<SERVER IP` should be IP address of the host running the server. If the server is running on the same host as the client, enter the IP address: 127.0.0.1
`<SERVER PORT>` should be the same port which you defined when starting the server program. This is optional and will default to port 7
