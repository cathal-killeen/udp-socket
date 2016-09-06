#include <sys/types.h>
#include <sys/socket.h> 		//structures needed for sockets
#include <netinet/in.h>			//constants and structures needed for internet domain addresses
#include <arpa/inet.h>			//definitions for internet operations
#include <netdb.h>				//definitions for network database operations
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100

int main (int argc, char * argv[] )
{


	int sock;                           	//This will be our socket
	struct sockaddr_in sin, remote;     	//"Internet socket address structure"
	socklen_t remote_length;         	//length of the sockaddr_in structure
	int nbytes;                        		//number of bytes we receive in our message
	char buffer[MAXBUFSIZE];            	//a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	bzero(&remote,sizeof(remote));
	sin.sin_family = AF_INET;                   //address family (IPv4)
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order (byte order - section 3.2 in Beej Guide)
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	/*
	int socket(int domain, int type, int protocol);
		*	'domain' specifies the address family e.g. IPv4 or IPv6
		*	'type' specifies the socket type (e.g. SOCK_STREAM, SOCK_DGRAM)
		*	'protocol' should be specified if more than one protocol is supported for the socket type.
			setting to 0 uses default protocol
	 Causes the system to create a generic socket of type UDP (datagram)
	*/
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	/*
	int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
		*	'sockfd' is the socket descriptor returned by socket() func
	*/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	bzero(buffer,sizeof(buffer));
	printf("Listening on port %s...\n", argv[1]);

	/*
	int recvfrom(int sockfd, void *buf, int len, unsigned int flags,
		 struct sockaddr *from, int *fromlen);
	*/
	nbytes = recvfrom(sock, &buffer, MAXBUFSIZE, 0,
	 	(struct sockaddr *)&remote, &remote_length);

	printf("The client says %s\n", buffer);

	char msg[] = "orange";
	/*
	int sendto(int sockfd, const void *msg, int len, unsigned int flags,
           const struct sockaddr *to, socklen_t tolen);
	*/
	nbytes = sendto(sock, &msg, sizeof(msg), 0,
		(struct sockaddr *)&remote, remote_length);

	close(sock);
	return 0;
}
