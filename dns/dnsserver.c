/*
gcc dnsserver.c
./a.out 4666
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
struct row{
	char name[256];
	char value[256];
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
	int sockid, statusb, statusl, clientLen, clientsocket, recvMsgSize, port, count;
	struct sockaddr_in addrport, clientAddr;
	char buffer[256];
	port = atoi(argv[1]);
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
		printf("Connection Established with Client\n");
		if ((recvMsgSize = recv(clientsocket, buffer, 256, 0)) < 0){
			perror("recv() failed\n");
			exit(1);
		}
		int i, j, flag;
		
		/*breaking query into type and message*/
		char type, value[256];
		type=buffer[0];
		for(i=1; i<strlen(buffer)-1;i++){	//this may be wrong
			value[i-1]=buffer[i];
		}
		value[i-1]='\0';
		printf("Message Received\nRequest_Type : %c, Message:%s\n", type, value);

		FILE *fp;
		fp = fopen("cache", "r");
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		char query[256], result[256];
		flag=0;
		if(type=='1'){
			while ((read = getline(&line, &len, fp)) != -1) {
				for(i=0; i<read; i++){
					if(line[i]==' '){
						query[i]='\0';
						break;
					}
					query[i]=line[i];
				}
				if(strcmp(query, value)==0){
					result[0]='3';
					j=1;
					for(i++; i<read-1; i++){
						result[j++]=line[i];
					}
					result[j]='\0';
					printf("Requested query successful.. Found : %s\n", result);
					if((count = send(clientsocket, result, 256, 0))<0){
							perror("Error in sending..\n");
							exit(1);
					}
					close(clientsocket);
					flag=1;
					break;
				}
			}
		}
		else{
			while ((read = getline(&line, &len, fp)) != -1) {
				for(i=0; i<read; i++){
					if(line[i]==' '){
						result[i+1]='\0';
						break;
					}
					result[i+1]=line[i];
				}
				j=0;
				for(i++; i<read; i++){
						query[j++]=line[i];
				}
				query[j-1]='\0';
				if(strcmp(query, value)==0){
					result[0]='3';
					printf("Requested query successful.. Found : %s\n", result);
					if((count = send(clientsocket, result, 256, 0))<0){
							perror("Error in sending..\n");
							exit(1);
					}
					close(clientsocket);
					flag=1;
					break;
				}
			}
		}
		if(flag==0){
			printf("Requested query unsuccessful.. NOT FOUND\n");
			send_type4(clientsocket);
		}
		printf("Connection Closed with Client\n");
	}
	return 0;
}