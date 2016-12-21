#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

#define MAX_SIZE 512

int main()
{

	int socketfd,nbytes; 
	socklen_t from_len;
	int portno = 2000;
	struct sockaddr_in server_addr, from_addr;
	char buff[MAX_SIZE];
	char reply[MAX_SIZE] ;
	int msg_len;

	socketfd = socket(AF_INET,SOCK_DGRAM,0 );
	if (socketfd < 0)
		printf(" Error in obtaining socketfd");

	server_addr.sin_family = AF_INET;
     	server_addr.sin_addr.s_addr = INADDR_ANY;
     	server_addr.sin_port = htons(portno);

	if(bind(socketfd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0)
		printf(" Error in binding the socket");
	
	from_len = sizeof(from_addr);
	printf("Server Initiated: \n");
	while(1)
	{
		bzero(buff,MAX_SIZE);
		printf("Waiting for request:\n");		
		nbytes = recvfrom(socketfd, buff, sizeof(buff), 0, (struct sockaddr*)&from_addr, &from_len);
		if (nbytes < 0 )
		{
			printf("Error in recvfrom");
		}			
		printf("Data received at server: %s\n",buff);
		msg_len = strlen(buff);
		snprintf(reply,sizeof(reply),"%d",msg_len);

		sendto(socketfd, reply, strlen(reply), 0,(struct sockaddr*)&from_addr,from_len);
		memset(buff,'\0',sizeof(buff));

		printf("Length of Data: %d\n",msg_len);
	}

	
	return 0;
}
