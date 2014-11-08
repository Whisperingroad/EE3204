/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
***********************************/


#include "headsock.h"
#define TRUE 1
#define FALSE 0
#define NACK 3

#define BACKLOG 10

void str_ser(int sockfd);                                                        // transmitting and receiving function

int main(void)
{
	int sockfd, con_fd, ret;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;

//	char *buf;
	pid_t pid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);          //create socket
	if (sockfd <0)
	{
		printf("error in socket!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYTCP_PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("172.0.0.1");
	bzero(&(my_addr.sin_zero), 8);
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));                //bind socket
	if (ret <0)
	{
		printf("error in binding");
		exit(1);
	}
	
	ret = listen(sockfd, BACKLOG);                              //listen
	if (ret <0) {
		printf("error in listening");
		exit(1);
	}

	while (1)
	{
		printf("waiting for data\n");
		sin_size = sizeof (struct sockaddr_in);
		con_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);            //accept the packet
		if (con_fd <0)
		{
			printf("error in accept\n");
			exit(1);
		}

		if ((pid = fork())==0)                                         // creat acception process
		{
			close(sockfd);
			str_ser(con_fd);                                          //receive packet and response
			close(con_fd);
			exit(0);
		}
		else close(con_fd);                                         //parent process
	}
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd)
{
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[DATALEN];
	struct ack_so ack;
	int end, n = 0;
	long lseek=0;
	end = 0;
	int acknowledgementNumber = 1;
	int count=0;
	int errorProbability = 100;
	
	printf("receiving data!\n");

	while(!end)
	{
		

		if ((n= recv(sockfd, &recvs, DATALEN, 0))==-1)                                   //receive the packet
		{
			printf("error when receiving\n");
			exit(1);
		}

		if (acknowledgementNumber == 1)
			acknowledgementNumber = 0;
		else
			acknowledgementNumber = 1;


		if (recvs[n-1] == '\0')									
		{
			end = 1;
			n --;
		}


		ack.num = acknowledgementNumber;
		ack.len = n;
		count++;


		//to simulate errors
		if ( count% 10 == 0){


			ack.num = NACK;
		}


		if (ack.num != NACK){
			memcpy((buf+lseek), recvs, n);
			lseek += n;
		}

		printf("%i %i as ACK sent\n", ack.num, ack.len);

		if ((n = send(sockfd, &ack, 8, 0))==-1)
		{
				printf("ACK send error!");								//send the ack
				exit(1);
		}
	}	
	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exist\n");
		exit(0);
	}

	
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}
