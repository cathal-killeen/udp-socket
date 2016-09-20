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
#include <unistd.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100

//convert string to lowercase
void strToLower(char *str){
	int i=0;
	while(str[i] != '\n' && str[i] != EOF && i < MAXBUFSIZE){
		str[i] = tolower(str[i]);
		i++;
	}
}

//gets a string representation of ls and returns via *msg
void getLs(char *msg){
	FILE *ls = popen("ls", "r");
	char buf[256];
	while (fgets(buf, sizeof(buf), ls) != 0) {
		strcat(msg,strtok(buf, "\n"));
		strcat(msg,"    ");
	}
	pclose(ls);
}

//extracts file name from request and returns via fn
//req has the format eg 'get filename' or 'put filename'
char *getFileName(char *req){
	char *fn = malloc(MAXBUFSIZE);
	int i=4; // we can ignore first 4 characters since they will be 'get ' or 'put ' - note space
	while(req[i] != '\n' && req[i] != EOF && i < MAXBUFSIZE && req[i] != ' '){
		fn[i-4]=req[i];
		i++;
	}
	return fn;
}

//checks to see if requested file exists
bool fileExists(char *fn){
	if(access( fn, F_OK ) != -1 ) {
    	// file exists
		return true;
	} else {
    	// file doesn't exist
		return false;
	}
}

int numPackets(FILE *f){
	int packets;
	char buf[MAXBUFSIZE];
	for(packets = 0; fgets(buf, MAXBUFSIZE, f) != NULL; ++packets);
	rewind(f);
	return packets;
}

long long int numBytes(FILE *f){
	fseek(f, 0L, SEEK_END);
	int len = ftell(f);
	rewind(f);
	return len;
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

	char cmd[MAXBUFSIZE];						//stores command GET or PUT

	//infinite loop - terminated by ctrl+C by the server admin
	for(;;){
		memset(outgoing,0,MAXBUFSIZE);				//clear outgoing message from last time
		memset(cmd,0,MAXBUFSIZE);

		nbytes = recvfrom(sock, &incoming, MAXBUFSIZE, 0,
		 	(struct sockaddr *)&remote, &remote_length);

		printf("The client says %s\n", incoming);


		strncpy(cmd, incoming, 3);
		cmd[3] = '\0';
		strToLower(cmd);

		if(strcmp(cmd, "ls") == 0){
			getLs(outgoing);
		}else if(strcmp(cmd,"get") == 0){
			char *fn = getFileName(incoming);
			printf("GET filename = %s\n",fn);
			if(fileExists(fn)){
				printf("%s exists\n",fn);
				FILE *fp = fopen(fn,"rb");
				//long long int size = numBytes(fp);
				int packets = numPackets(fp);
				//printf("LENGTH = %d",len);
				printf("packets = %d\n",packets);
				strcat(outgoing,";RTS;");
				char numbuf[MAXBUFSIZE];
				sprintf(numbuf, "%d", packets);
				strcat(outgoing,numbuf);strcat(outgoing,";");
				strcat(outgoing,fn);
				nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
				nbytes = recvfrom(sock, &incoming, MAXBUFSIZE, 0,(struct sockaddr *)&remote, &remote_length);
				if(strcmp(incoming,";CTS;") == 0){
					while(!feof(fp)){
						fread(&outgoing, 1, MAXBUFSIZE, fp);
						nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
						memset(outgoing,0,MAXBUFSIZE);
					}
				}
				fclose(fp);
				printf("file sent!\n");
			}else{
				printf("%s doesnt exist\n",fn);
			}
			nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,
				(struct sockaddr *)&remote, remote_length);

		}else if(strcmp(cmd,"put") == 0){
			strcat(outgoing,"PUT COMMAND");
			printf("GET filename = %s\n",getFileName(incoming));
		}else{
			strcat(outgoing,"Unknown command");
		}

		//send packet
		nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,
			(struct sockaddr *)&remote, remote_length);
	}

	close(sock);
	return 0;
}
