/*
gcc server.c
./a.out 4444 127.0.0.1(DNS server's IP)
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
struct row{
	char name[256];
	char value[256];
};

struct row cache[3];
int rear = 0;
int SIZE = 3;

int search_send(char type, char *value, int clientsocket){
	int count, i;
	char wrong[256];
	strcpy(wrong, "400:Wrong request type");
	if(type=='1'){
			for(i=0; i<SIZE; i++){
				if(strcmp(cache[i].name, value)==0){
					if((count = send(clientsocket, cache[i].value, 256, 0))<0){
						perror("Error in sending..\n");
						exit(1);
					}
					break;
				}
			}
			if(i==SIZE){
				return -1;
			}
	}
	else if(type=='2'){
		for(i=0; i<SIZE; i++){
			if(strcmp(cache[i].value, value)==0){
				if(send(clientsocket, cache[i].name, 256, 0)<0){
					perror("Error in sending..\n");
					exit(1);
				}
				break;
			}
		}
		if(i==SIZE){
				return -1;
		}
	}
	else{
		if((count = send(clientsocket, wrong, 256, 0))<0){
			perror("Error in sending..\n");
			exit(1);
		}
	}
		return 0;
}


void insert(char *name1, char *value1){
	strcpy(cache[rear].name, name1);
	strcpy(cache[rear].value, value1);
	rear = (rear + 1)%SIZE;
};


void send_type4(int clientsocket){
	char wrong[256];
	strcpy(wrong, "4entry not found in the database");
	if(send(clientsocket, wrong, 256, 0)<0){
		perror("Error in sending..\n");
		exit(1);
	}
}

int main(int argc, char *argv[]){
	int sockid, statusb, statusl, clientLen, clientsocket, recvMsgSize, port, dnsport=4666, DNSsock, n;
	struct hostent *DNSserver;
	port = atoi(argv[1]);
	struct sockaddr_in addrport, clientAddr, DNSAddr;
	char buffer[256];
	if((sockid = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){		//If giving error then use PF_INET instead of AF_INET
		perror("Socket not created\n");
		exit(1);
	}
	addrport.sin_family = AF_INET;
	addrport.sin_port = htons(port);	//(short int) host to network byte order
	addrport.sin_addr.s_addr = htonl(INADDR_ANY);	//(int) network to host byte order
	if((statusb = bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport)))<0){
		perror("Error binding the socket\n");
		exit(1);
	}
	if((statusl = listen(sockid, 12))<0){	//non blocking call
		perror("listen() Failed\n");
		exit(1);
	}
	while(1){
		clientLen = sizeof(clientAddr);
		if((clientsocket = accept(sockid, (struct sockaddr *)&clientAddr, &clientLen))<0){
			perror("accept() failed\n");
			exit(1);
		}
		printf("Connection Established with client\n");
		if ((recvMsgSize = recv(clientsocket, buffer, 256, 0)) < 0){
			perror("recv() failed\n");
			exit(1);
		}
		int i, j;
		char type, value[256];
		type=buffer[0];
		for(i=1; i<strlen(buffer)-1;i++){	//this may be wrong
			value[i-1]=buffer[i];
		}
		value[i-1]='\0';
		printf("Message Received\nRequest_Type : %c, Message:%s\n", type, value);
		if(search_send(type, value, clientsocket)==0){
			close(clientsocket);
			printf("Connection Closed with client\n");
			continue;
		}
		printf("Did not found in cache...requesting DNS server\n");
		if((DNSsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			perror("Socket not created\n");
			exit(1);
		}


		DNSserver = gethostbyname(argv[2]);
		if (DNSserver == NULL) {
      		fprintf(stderr,"ERROR, no such host\n");
      		exit(1);
   		}
		bzero((char *) &DNSAddr, sizeof(DNSAddr));
		DNSAddr.sin_family = AF_INET;
		bcopy((char *)DNSserver->h_addr, (char *)&DNSAddr.sin_addr.s_addr, DNSserver->h_length);
		DNSAddr.sin_port = htons(dnsport);

		/* Now connect to the DNS server */
		if (connect(DNSsock, (struct sockaddr*)&DNSAddr, sizeof(DNSAddr)) < 0) {
		  perror("ERROR connecting to DNS server");
		  exit(1);
		}
		printf("Connection Established with DNS Server\n");
		/*sending same request to DNS  server*/
		if (send(DNSsock, buffer, 256, 0) < 0) {
			perror("ERROR writing to socket");
			exit(1);
		}

		/*receiving from DNS server*/
		bzero(buffer,256);
		if (recv(DNSsock, buffer, 256, 0) < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}
		close(DNSsock);
		char temp1=buffer[0];
		if(temp1=='4'){
			printf("Message Received from DNS server\nResponse_Type : 4, Message : entry not found in the database\n");
			printf("Connection closed with DNS Server\n");
			send_type4(clientsocket);
			printf("Connection closed with Client\n");
			continue;
		}
		else if(temp1=='3'){
			for(i=0; i<255; i++){
				buffer[i]=buffer[i+1];
			}
			printf("Message Received from DNS server\nResponse_Type : %c, Message : %s\n", temp1, buffer);
			printf("Connection closed with DNS Server\n");
			if(type=='1'){
				insert(value, buffer);
			}
			else{
				insert(buffer, value);
			}
			if((send(clientsocket, buffer, 256, 0))<0){
				perror("Error in sending..\n");
				exit(1);
			}
			close(clientsocket);
			printf("Connection Closed with client\n");
		}
	}
	return 0;
}