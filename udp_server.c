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
#include <ctype.h>
#include <stdbool.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100

void strToLower(char *str){
	//convert to lowercase
	int i=0;
	while(str[i] != '\n' && str[i] != EOF && i < MAXBUFSIZE){
		str[i] = tolower(str[i]);
		i++;
	}
}

//gets a string rep of ls and returns via *msg
void get_ls(char *msg){
	FILE *ls = popen("ls", "r");
	char buf[256];
	while (fgets(buf, sizeof(buf), ls) != 0) {
		strcat(msg,strtok(buf, "\n"));
		strcat(msg,"    ");
	}
	pclose(ls);
}


// bool does_file_exist(char *msg){
// 	FILE *ls = popen("ls", "r");
// 	char buf[256];
// 	bool exists = false;
// 	while (fgets(buf, sizeof(buf), ls) != 0) {
// 		strcat(msg,strtok(buf, "\n"));
// 		strcat(msg,"    ");
// 		if(strcmp(strtok(buf, "\n")))
// 	}
// 	pclose(ls);
// }

//extracts file name from cmd and returns via fn
//cmd has the format eg 'get filename' or 'put filename'
void get_file_name(char *cmd ,char *fn){
	int start = sizeof(cmd);
	bool end = false;
	for(int i=0;i<sizeof(cmd);i++){
		if(cmd[i] == ' '){
			if(strlen(fn) == 0){
				start = i+1;
			}else{
				end = true;
			}
		}
		if(i >= start && !end){
			fn = append(fn, cmd[i]);
		}
	}
}


int main (int argc, char * argv[] )
{
	int sock;                           	//This will be our socket
	struct sockaddr_in sin, remote;     	//"Internet socket address structure"
	socklen_t remote_length;         		//length of the sockaddr_in structure
	int nbytes;                        		//number of bytes we receive in our message
	char incoming[MAXBUFSIZE];            	//a buffer to store our received message
	char outgoing[MAXBUFSIZE];				//a buffer to store outgoing message
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
	bzero(incoming,sizeof(incoming));
	printf("Listening on port %s...\n", argv[1]);

	//infinite loop - terminated by ctrl+C by the server admin
	for(;;){
		memset(outgoing,0,MAXBUFSIZE);				//clear outgoing message from last time
		/*
		int recvfrom(int sockfd, void *buf, int len, unsigned int flags,
			 struct sockaddr *from, int *fromlen);
		*/
		nbytes = recvfrom(sock, &incoming, MAXBUFSIZE, 0,
		 	(struct sockaddr *)&remote, &remote_length);

		printf("The client says %s\n\n", incoming);

		char cmd[MAXBUFSIZE];

		strncpy(cmd, incoming, 3);
		cmd[3] = '\0';
		printf("%s -> ",cmd);
		strToLower(cmd);
		printf("%s\n",cmd);

		if(strcmp(incoming, "ls") == 0){
			get_ls(outgoing);
		}else if(strcmp(cmd,"get") == 0){
			printf("GET command\n");
			strcat(outgoing,"GET COMMAND");
		}else if(strcmp(cmd,"put") == 0){
			printf("PUT command\n");
			strcat(outgoing,"PUT COMMAND");
		}else{
			strcat(outgoing,"Unknown command");
		}

		nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,
			(struct sockaddr *)&remote, remote_length);


		//msg = "orange";
		/*
		int sendto(int sockfd, const void *msg, int len, unsigned int flags,
	           const struct sockaddr *to, socklen_t tolen);
		*/
		//nbytes = sendto(sock, &msg, sizeof(msg), 0,
		//	(struct sockaddr *)&remote, remote_length);
	}

	close(sock);
	return 0;
}
