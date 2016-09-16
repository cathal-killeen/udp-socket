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

#define MAXBUFSIZE 100

/* You will have to modify the program below */

int fileSize(char *msg){
	char str_size[MAXBUFSIZE];
	int i=5; // we can ignore first 5 characters
	while(msg[i] != '\n' && msg[i] != EOF && i < MAXBUFSIZE && msg[i] != ';'){
		str_size[i-5]=msg[i];
		i++;
	}
	return atoi(str_size);
}

char *getFileName(char *msg){
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

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               // this will be our socket
	char incoming[MAXBUFSIZE];				// incoming buffer
	char outgoing[MAXBUFSIZE];				// outgoing buffer

	struct sockaddr_in remote;              //"Internet socket address structure"

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	printf("Connected to %s:%s...\n",argv[1],argv[2]);

	while(strcmp(outgoing, "exit") != 0){
		memset(outgoing,0,MAXBUFSIZE);
		/******************
		  sendto() sends immediately.
		  it will report an error if the message fails to leave the computer
		  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
		 ******************/
		//char outgoing[] = "apple";
		/*
		int sendto(int sockfd, const void *msg, int len, unsigned int flags,
	           const struct sockaddr *to, socklen_t tolen);
		*/
		printf("> ");
		int i = 0;
		char c = getchar();
		while (c != '\n' && c != EOF && i != MAXBUFSIZE){
			outgoing[i++] = c;
        	c = getchar();
    	}

		printf("%s\n",outgoing);

		nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,
				(struct sockaddr *)&remote, sizeof(remote));

		// Blocks till bytes are received
		struct sockaddr_in from_addr;
		socklen_t addr_length = sizeof(struct sockaddr);
		bzero(incoming,sizeof(incoming));
		nbytes = recvfrom(sock, &incoming, sizeof(incoming), 0,
				(struct sockaddr *)&from_addr, &addr_length);

		char cmd[MAXBUFSIZE];						//stores command GET or PUT
		strncpy(cmd, incoming, 5);
		cmd[5] = '\0';

		if(strcmp(cmd,";RTS;")==0){
			int numPackets = fileSize(incoming);
			printf("num packets = %d\n",numPackets);
			char *fn = getFileName(incoming);
			char *nfn = fn;
			strcat(nfn,".received");
			FILE *fp = fopen(nfn,"w");
			memset(outgoing,0,MAXBUFSIZE);
			strcat(outgoing,";CTS;");
			nbytes = sendto(sock, &outgoing, sizeof(outgoing), 0,(struct sockaddr *)&remote, sizeof(remote));
			while(numPackets > 0){
				nbytes = recvfrom(sock, &incoming, sizeof(incoming), 0,(struct sockaddr *)&from_addr, &addr_length);
				fputs(incoming, fp);
				numPackets--;
			}
			fclose(fp);
			printf("%s successfully recieved\n", fn);

		}else{
			printf("%s\n", incoming);
		}

	}

	close(sock);
	return 0;

}
