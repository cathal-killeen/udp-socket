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

typedef struct{
	int sock;
	struct sockaddr_in sin;
	struct sockaddr_in remote;
	socklen_t remote_length;
	char incoming[MAXBUFSIZE];
	char outgoing[MAXBUFSIZE];
}sock_data;

//convert string to lowercase
void strToLower(char *str){
	int i=0;
	while(str[i] != '\n' && str[i] != EOF && i < MAXBUFSIZE){
		str[i] = tolower(str[i]);
		i++;
	}
}

//extract first n characters from incoming message and check if client made a request (eg. GET PUT LS)
void getCmd(sock_data *S, char *cmd, int n){
	bzero(cmd,MAXBUFSIZE);
	strncpy(cmd, S->incoming, n);
	cmd[n] = '\0';
	strToLower(cmd);
}

//gets the request from client
int receive(sock_data *S){
	return recvfrom(S->sock, &S->incoming, sizeof(S->incoming), 0, (struct sockaddr *)&S->remote, &S->remote_length);
}

int sendToClient(sock_data *S){
	int nbytes = sendto(S->sock, &S->outgoing, sizeof(S->outgoing), 0, (struct sockaddr *)&S->remote, S->remote_length);
	bzero(S->outgoing,MAXBUFSIZE);
	return nbytes;
}

int sendHandshake(sock_data *S, char *type, long long int size, char *filename){
	strcat(S->outgoing,";");
	strcat(S->outgoing,type);
	strcat(S->outgoing,";");
	char numbuf[MAXBUFSIZE];
	sprintf(numbuf, "%lli", size);
	strcat(S->outgoing,numbuf);
	strcat(S->outgoing,";");
	strcat(S->outgoing,filename);

	sendToClient(S);
	receive(S);

	char cmd[5];
	getCmd(S,cmd,5);

	if(strcmp(cmd,";cts;") == 0){
		return 0;
	}
	return -1;
}

//gets a string representation of ls and returns via *msg
void sendLs(sock_data *S){
	FILE *ls = popen("ls", "r");
	char buf[MAXBUFSIZE];
	char paths[MAXBUFSIZE][MAXBUFSIZE];
	int i=0;
	long long int total = 0;
	while (fgets(buf, sizeof(buf), ls) != 0) {
		char *path = strtok(buf, "\n");
		strcat(path,"   ");
		total += strlen(path);
		strcpy(paths[i++],path);
	}
	pclose(ls);
	if(sendHandshake(S, "RLS", total, "ls") == 0){
		int added = 0;
		for(int j=0; j<i;j++){
			if(total <= 0) break;
			if(added + strlen(paths[j]) > MAXBUFSIZE){
				sendToClient(S);
				added = 0;
			}
			strcat(S->outgoing,paths[j]);
			added+=strlen(paths[j]);
		}
		if(added > 0){
			sendToClient(S);
		}
	}else{
		printf("Error sending ls");
	}
}

//extracts file name from client text request
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

//gets the number of bytes in a file
long long int numBytes(FILE *f){
	fseek(f, 0L, SEEK_END);
	int len = ftell(f);
	rewind(f);
	return len;
}

//This code populates the sockaddr_in struct with the information about our socket
int populateSockAddr(sock_data *S, char *port){
	bzero(&S->sin,sizeof(S->sin));                    		//zero the struct
	bzero(&S->remote,sizeof(S->remote));					//zero remote
	S->sin.sin_family = AF_INET;                   			//address family (IPv4)
	S->sin.sin_port = htons(atoi(port));        			//htons() sets the port # to network byte order (byte order - section 3.2 in Beej Guide)
	S->sin.sin_addr.s_addr = INADDR_ANY;           			//supplies the IP address of the local machine

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((S->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
		return -1;											//if the socket gen fails return -1 flag
	}
	return 0;												//return 0 on success
}

//bind the socket to the local address and port we've supplied in the sockaddr_in struct
int bindSocket(sock_data *S, char *port){
	if (bind(S->sock, (struct sockaddr *)&S->sin, sizeof(S->sin)) < 0){
		return -1;											//if bind fails return -1
	}
	S->remote_length = sizeof(S->remote);					//set remote length
	bzero(S->incoming,MAXBUFSIZE);							//zero the incoming message
	printf("Listening on port %s...\n", port);				//success, server will now listen for requests
	return 0;
}

int main (int argc, char * argv[] ){
	if (argc != 2){
		printf ("USAGE: <port>\n");
		exit(1);
	}

	sock_data MySock;										//initialize MySock struct which stores all the relevant sock info
	char *port = argv[1];

	if(populateSockAddr(&MySock,port)>0){
		printf("Error: Unable to create socket");
		exit(1);
	}

	if(bindSocket(&MySock,port)>0){
		printf("Error: Unable to bind the socket");
	}

	//infinite loop - terminated by ctrl+C by the server admin
	for(;;){
		char cmd[MAXBUFSIZE];								//stores command GET or PUT
		bzero(MySock.outgoing,MAXBUFSIZE);					//clear outgoing message from last time

		//receives request from client
		receive(&MySock);
		printf("The client says %s\n", MySock.incoming);
		getCmd(&MySock, cmd, 3);

		//if client requests ls
		if(strcmp(cmd, "ls") == 0){
			sendLs(&MySock);
			//sendto(MySock.sock, &MySock.outgoing, sizeof(MySock.outgoing), 0, (struct sockaddr *)&MySock.remote, MySock.remote_length);

		//if client makes GET request
		}
		//else if(strcmp(cmd,"get") == 0){
		// 	char *fn = getFileName(incoming);
		// 	printf("request type: GET\nfilename: %s\n",fn);
		// 	if(fileExists(fn)){
		// 		printf("%s exists\n",fn);
		// 		FILE *fp = fopen(fn,"rb");
		// 		long long int size = numBytes(fp); 	//get size of file
		// 		printf("bytes = %lli\n",size);
		// 		strcat(outgoing,";RTS;");
		// 		char numbuf[MAXBUFSIZE];
		// 		sprintf(numbuf, "%lli", size);
		// 		strcat(outgoing,numbuf);strcat(outgoing,";");
		// 		strcat(outgoing,fn);
		// 		sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
		// 		recvfrom(sock, &incoming, sizeof(incoming), 0,(struct sockaddr *)&remote, &remote_length);
		// 		if(strcmp(incoming,";CTS;") == 0){
		// 			while(!feof(fp)){
		// 				int readBytes = fread(&outgoing, 1, sizeof(outgoing), fp);
		// 				printf("bytes read = %d\n",readBytes);
		// 				sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
		// 				memset(outgoing,0,MAXBUFSIZE);
		// 			}
		// 		}
		// 		fclose(fp);
		// 		printf("file sent!\n");
		// 	}else{
		// 		printf("%s doesnt exist\n",fn);
		// 		strcat(outgoing,";ERR;DNE;");
		// 		strcat(outgoing,fn);
		// 		sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
		// 	}
		// }else if(strcmp(cmd,"put") == 0){
		// 	strcat(outgoing,"PUT COMMAND");
		// 	printf("GET filename = %s\n",getFileName(incoming));
		// 	sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
		// }else{
		// 	strcat(outgoing,"Unknown command");
		// 	sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, remote_length);
		// }
	}

	close(MySock.sock);
	return 0;
}
