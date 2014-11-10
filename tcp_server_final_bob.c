/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
***********************************/


#include "headsock.h"
#define TRUE 1
#define FALSE 0
#define NACK 3

#define BACKLOG 10

void str_ser(int sockfd, float errorProbability);                                                        // transmitting and receiving function

int main(void)
{
	int sockfd, con_fd, ret;
	float errorProbability;
	int detorrandom;
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
	
	printf("Input error probability: ");
	scanf("%f", &errorProbability);
	printf("Use deterministic or random error generation? ");
	scanf("%d", &detorrandom);
	if(detorrandom == 0)
		errorProbability /=100;
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
			str_ser(con_fd, errorProbability);                                          //receive packet and response
			close(con_fd);
			exit(0);
		}
		else close(con_fd);                                         //parent process
	}
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd, float errorProbability)
{
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[DATALEN];
	struct ack_so ack;
	struct timeval stop, start;
	int end, n = 0;
	long lseek=0;
	end = 0;
	int acknowledgementNumber = 1;
	int count=0;
	int total_packet_number = 0;
	int error_number = 0;
	int dropped_acks = 0;
	int modulo_error = 0;
	if(errorProbability <=1)
	{
	total_packet_number = BUFSIZE/DATALEN;
	if(BUFSIZE % DATALEN > 0)
		total_packet_number++;
	modulo_error = total_packet_number/(total_packet_number*errorProbability);
	errorProbability = 101; //Set to bigger than 100 so it never factors into the if decision.
	}
	printf("receiving data!\n");

	while(!end)
	{
		if ((n=recv(sockfd, &recvs, DATALEN, 0))==-1) //receive the packet, n is the number of bits.
		{
			printf("error when receiving\n"); 
			ack.num = 3;
			ack.len = -1; //Send a real NACK.
		}
		else //Packet was received properly.
		{
		count++;
//		printf("n is %d\n", n);
		srand(time(NULL)); //Generate seed
		error_number = (rand() % 99)+1; //Between 1 and 100
		if(errorProbability < error_number || count % modulo_error != 0)
		{
		if(recvs[n-1] == '\0')
		{
			n--;
			end = 1;
		}
		
		memcpy((buf+lseek), recvs, n);
		lseek+=n;
		ack.num = acknowledgementNumber;
		if(end ==1)
			n++;
		ack.len = n; //Store the received data to memory

		if(acknowledgementNumber == 0)
			acknowledgementNumber = 1;
		else
			acknowledgementNumber = 0;
		 //If received the packet properly, toggle the ack number.
		}
		else
		{
			ack.num = 3;
			ack.len = -1; //Send a NACK using the error simulator.
		}
		}
		if ((n = send(sockfd, &ack, 8, 0))==-1)
		{
				printf("ACK send error!");								//send the ack
				exit(1);
		}
		else
		{
//			printf("%i %i as ACK sent\n", ack.num, ack.len);
		}

	}	
	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exist\n");
		exit(0);
	}

	
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data written is %d bytes\n", (int)lseek);
}
