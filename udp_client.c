#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define MAXBUFSIZE 100

/* You will have to modify the program below */

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

int sendToServer(sock_data *S){
	int nbytes = sendto(S->sock, &S->outgoing, sizeof(S->outgoing), 0, (struct sockaddr *)&S->remote, S->remote_length);
	bzero(S->outgoing,MAXBUFSIZE);
	return nbytes;
}


long long int fileSize(char *msg){
	char str_size[MAXBUFSIZE];
	int i=5; // we can ignore first 5 characters
	while(msg[i] != '\n' && msg[i] != EOF && i < MAXBUFSIZE && msg[i] != ';'){
		str_size[i-5]=msg[i];
		i++;
	}
	return atoi(str_size);
}

char *parseFilename(char *msg){
	char *fn = malloc(MAXBUFSIZE);
	int i=5; // we can ignore first 5
	int j=0;
	int semi = 0;
	while(msg[i] != '\n' && msg[i] != EOF && i < MAXBUFSIZE && msg[i] != ' '){
		if(semi == 1){
			fn[j++]=msg[i];
		}
		if(msg[i] == ';'){
			semi = 1;
		}
		i++;
	}
	return fn;
}

void receiveLs(sock_data *S){
	long long int size = fileSize(S->incoming);
	strcat(S->outgoing,";CTS;");
	sendToServer(S);
	while(size > 0){
		size -= receive(S);
		printf("%s",S->incoming);
	}
	printf("\n");
}

void receiveFile(sock_data *S){
	long long int size = fileSize(S->incoming);
	//printf("num bytes = %lli\n",size);
	char *fn = parseFilename(S->incoming);
	char *newFn = fn; strcat(newFn,".received");
	FILE *fp = fopen(newFn,"wb");
	strcat(S->outgoing,";CTS;");
	sendToServer(S);
	while(size > 0){
		receive(S);
		int writeSize = MAXBUFSIZE; 				//default bytes to write to new file
		if(size < MAXBUFSIZE){writeSize = size;}    //write only the number of bits left if less than MAXBUFSIZE
		int written = fwrite(&S->incoming, 1, writeSize, fp);
		//printf("bytes written = %d\n",written);
		size-=written;
		//printf("bytes remaining: %lli\n",size);
	}
	fclose(fp);
	printf("%s successfully recieved!\n", fn);
}

//This code populates the sockaddr_in struct with the information about our socket
int populateSockAddr(sock_data *S, char *addr, char *port){
	bzero(&S->remote,sizeof(S->remote));					//zero remote
	S->remote.sin_family = AF_INET;                   		//address family (IPv4)
	S->remote.sin_port = htons(atoi(port));        			//htons() sets the port # to network byte order (byte order - section 3.2 in Beej Guide)
	S->remote.sin_addr.s_addr = inet_addr(addr);         //sets IP address

	S->remote_length = sizeof(S->remote);					//set remote length
	bzero(S->incoming,MAXBUFSIZE);							//zero the incoming message

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((S->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
		return -1;											//if the socket gen fails return -1 flag
	}
	return 0;												//return 0 on success
}

//gets user input from terminal and stores in ClientSock.outgoing
void getUserInput(sock_data *S){
	printf("> ");
	int i = 0;
	char c = getchar();
	while (c != '\n' && c != EOF && i != MAXBUFSIZE){
		S->outgoing[i++] = c;
		c = getchar();
	}
}

int main (int argc, char * argv[]){
	if (argc < 3){
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}
	char *addr = argv[1];
	char *port = argv[2];
	// int sock;                               // this will be our socket
	// char incoming[MAXBUFSIZE];				// incoming buffer
	// char outgoing[MAXBUFSIZE];				// outgoing buffer
	//
	// struct sockaddr_in remote;              //"Internet socket address structure"
	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	// bzero(&remote,sizeof(remote));               //zero the struct
	// remote.sin_family = AF_INET;                 //address family
	// remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	// remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address
	//
	// //Causes the system to create a generic socket of type UDP (datagram)
	// if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	// {
	// 	printf("unable to create socket");
	// }

	sock_data ClientSock;
	if(populateSockAddr(&ClientSock,addr,port)>0){
		printf("Error: Unable to create socket");
		exit(1);
	}
	printf("Connected to %s:%s...\n",argv[1],argv[2]);

	for(;;){
		bzero(ClientSock.outgoing,MAXBUFSIZE);					//zero
		bzero(ClientSock.incoming,MAXBUFSIZE);
		getUserInput(&ClientSock);
		sendToServer(&ClientSock);

		receive(&ClientSock);
		char cmd[MAXBUFSIZE];								//stores command GET or PUT
		getCmd(&ClientSock,cmd,4);

		//if server requests to send ls to client
		if(strcmp(cmd,";rls")==0){
			receiveLs(&ClientSock);
		//if server requests to send file
		}else if(strcmp(cmd,";rts")==0){
			receiveFile(&ClientSock);
		}else if(strcmp(cmd,"exit")==0){
			close(ClientSock.sock);
			return 0;
		}else{
			printf("%s\n",ClientSock.incoming);
		}
	}

	// while(strcmp(outgoing, "exit") != 0){
	// 	memset(outgoing,0,MAXBUFSIZE);
	// 	/******************
	// 	  sendto() sends immediately.
	// 	  it will report an error if the message fails to leave the computer
	// 	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	// 	 ******************/
	// 	//char outgoing[] = "apple";
	// 	/*
	// 	int sendto(int sockfd, const void *msg, int len, unsigned int flags,
	//            const struct sockaddr *to, socklen_t tolen);
	// 	*/
	// 	printf("> ");
	// 	int i = 0;
	// 	char c = getchar();
	// 	while (c != '\n' && c != EOF && i != MAXBUFSIZE){
	// 		outgoing[i++] = c;
    //     	c = getchar();
    // 	}
	//
	// 	printf("%s\n",outgoing);
	//
	// 	nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,
	// 			(struct sockaddr *)&remote, sizeof(remote));
	//
	// 	// Blocks till bytes are received
	// 	struct sockaddr_in from_addr;
	// 	socklen_t addr_length = sizeof(struct sockaddr);
	// 	bzero(incoming,sizeof(incoming));
	// 	nbytes = recvfrom(sock, &incoming, sizeof(incoming), 0,
	// 			(struct sockaddr *)&from_addr, &addr_length);
	//
	// 	char cmd[MAXBUFSIZE];						//stores command GET or PUT
	// 	strncpy(cmd, incoming, 5);
	// 	cmd[5] = '\0';
	//
	// 	if(strcmp(cmd,";RLS;")==0){
	// 		long long int size = fileSize(incoming);
	// 		bzero(outgoing,MAXBUFSIZE);
	// 		strcat(outgoing,";CTS;");
	// 		sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, sizeof(remote));
	// 		while(size > 0){
	// 			size -= recvfrom(sock, &incoming, sizeof(incoming), 0,(struct sockaddr *)&from_addr, &addr_length);
	// 			printf("%s",incoming);
	// 		}
	// 		printf("\n");
	// 	}else if(strcmp(cmd,";RTS;")==0){
	// 		long long int size = fileSize(incoming);
	// 		printf("num bytes = %lli\n",size);
	// 		char *fn = getFileName(incoming);
	// 		char *nfn = fn;
	// 		strcat(nfn,".received");
	// 		FILE *fp = fopen(nfn,"wb");
	// 		memset(outgoing,0,MAXBUFSIZE);
	// 		strcat(outgoing,";CTS;");
	// 		nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, sizeof(remote));
	// 		while(size > 0){
	// 			nbytes = recvfrom(sock, &incoming, sizeof(incoming), 0,(struct sockaddr *)&from_addr, &addr_length);
	// 			int writeSize = MAXBUFSIZE; 				//default bytes to write to new file
	// 			if(size < MAXBUFSIZE){writeSize = size;}    //write only the number of bits left if less than MAXBUFSIZE
	// 			int written = fwrite(&incoming, 1, writeSize, fp);
	// 			printf("bytes written = %d\n",written);
	// 			size-=written;
	// 			printf("bytes remaining: %lli\n",size);
	// 		}
	// 		fclose(fp);
	// 		printf("%s successfully recieved\n", fn);
	//
	// 	}else{
	// 		printf("%s\n", incoming);
	// 	}
	//
	// }

	close(ClientSock.sock);
	return 0;

}
