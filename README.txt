***************************************************
CSCI 4273 Network Systems
Programming Assignment 1:UDP Socket Programming
Instructor: Sangtae Ha
Student: Cathal Killeen
Date: September 21 2016
***************************************************

This is an implementation of a UDP socket. It includes a server and a client program.
Commands from the server side are:

- get [file_name]: the server checks if the requested file exists and then transfers to the client
    if it exists, or sends an error if it doesn't. The client saves the file as [file_name.received]
- put [file_name]: the client checks if the file exists locally, and then transmits to the server if
    it does, or prints an error if it doesnt. The server will save the file as [file_name.received]
- ls: The server searches all the files it has in its local directory and send a list of
    all these files to the client.
- exit: the client notifies the server that it is exiting and then the client program terminates. The
    server then waits for another client connection.

**** sock_data struct ****
In both of my programs I have defined a sock_data struct which stores all of the relevant data
related to the socket.

**** functions ****
most of the operations have been abstracted into functions on my implementation. Most of these functions
take an instance of sock_data struct as a parameter

**** packet loss ****
my implementation does NOT handle packet loss between the client and server. The program can tell when
a file has been successfully received since the initial handshake specifies the filesize, but I have not
implemented a reliable transfer mechanism

**** makefile ****
type 'make' or 'make socket' to compile the two programs. Type 'make clean' to remove the executables
