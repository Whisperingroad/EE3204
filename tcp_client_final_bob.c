/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"
#define TRUE 1
#define FALSE 0
#define NACK 3
#define TIMEOUT 500

float str_cli(FILE *fp, int sockfd, long *len);                       //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in

int main(int argc, char **argv)
{
	int sockfd, ret;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("parameters not match");
	}

	sh = gethostbyname(argv[1]);	                                       //get host's information
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

	printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	addrs = (struct in_addr **)sh->h_addr_list;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);                           //create the socket
	if (sockfd <0)
	{
		printf("error in socket");
		exit(1);
	}
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYTCP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));         //connect the socket with the host
	if (ret != 0) {
		printf ("connection failed\n"); 
		close(sockfd); 
		exit(1);
	}
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}

	ti = str_cli(fp, sockfd, &len);                       //perform the transmission and receiving
	rt = (len/(float)ti);                                         //caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);
	close(sockfd);
	fclose(fp);
//}
	exit(0);
}

float str_cli (FILE *fp, int sockfd, long *len)
{
  char *buffer; //Load entire file into this.
  long lsize, position; //lsize specifies the length of the file, position specifies the point at which the file should be next read.
  char packetdata[DATALEN]; //Contains data in the packet.
  struct ack_so ack; //Acknowledgement packet container.
  int srstatus, packetlength; //srstatus is the status of send/receive, packetlength is the length of the packet to be sent.
  float time_inv = 0.0; //Time interval initialization
  struct timeval sendt, recvt;
  int prevMsgAck; //Whether the last message was acknowledged.
  int ackEvenOdd; //Whether this message should expect even or odd acknowledgement.
  int end = 0;
  int ended = 0;
  int send_flag = 1;
  int noTimeout = 0;
  long total_data_sent = 0;
  //Initialize often-changed variables:
  position = -DATALEN;
  prevMsgAck = TRUE; //This is initialized as such since there was no message at the start.
  ackEvenOdd = 1;
  packetlength = DATALEN;

  fseek(fp, 0, SEEK_END); //Put read position on the end of the file.
  lsize = ftell(fp); //Finds out size of file.
  buffer = (char *)malloc(lsize); //Allocates buffer equal to the size of the file.
  if (buffer == NULL) 
    exit(2);

  rewind(fp); //Resets file position to beginning of file.
  fread(buffer,1,lsize,fp); //Store file into buffer.
  
  printf("The file length is %d bytes\n", (int)lsize); 
  printf("The standard packet length is %d bytes\n", DATALEN); //Information.
  buffer[lsize]='\0'; //Add ending character.
  gettimeofday(&sendt, NULL); //Get start time.

  while (ended!=1) //Position is measured out in bytes.
    {
    if(prevMsgAck)
	{
	send_flag = 1;
//	  printf("Previous message was acknowledged.\n");
	  	position +=packetlength; //Advance if this is not the last packet.
//	 printf("This is the current position: %d\n", position); 
	  if((lsize+1-position) <= DATALEN)
	  {
//		printf("This is the final packet.\n");
		end = 1;
	    packetlength = lsize+1-position; //The 1 is '\0'
	  }
	  else
	    packetlength = DATALEN; //This part is for the end part where a packet might not be big enough to meet DATALEN
	}
	
	//Send part.
	if(packetlength!=0 && send_flag == 1)
	{
      memcpy(packetdata, (buffer+position), packetlength);
      srstatus = send(sockfd, &packetdata, packetlength, 0); //Send a packet.
//	  printf("Packet sent from position: %d \n", position);
//	  printf("Packet size is: %d \n", packetlength);

      if(srstatus == -1)
	{
//	  printf("Send error!");
	  send_flag = 1; //Immediately resend if it is a send error.
	} 
	  else //It sent, but receive status is unknown.
	{
		total_data_sent += packetlength;
		send_flag = 0;
	}

	}

	//Receive part.
	  if((srstatus = recv(sockfd, &ack, 8, 0)) == -1)
	{
  //    printf("error when receiving\n");
	  ack.num = -1;
	  ack.len = -1; //If there is an error, make the 'expected ack packet' completely wrong so it will be resent.
	}

      if(ack.num != ackEvenOdd || ack.len != packetlength) //Acknowledgement is wrong.
	{
//		printf("Expected ack.num was %d, received ack.num was %d\n", ackEvenOdd, ack.num);
//		printf("Expected length was %d, received length was %d\n", packetlength, ack.len);
	  if(packetlength == ack.len)
	    {
//	      printf("Error in transmission - wrong packet.\n");
	    }
	else if(ack.len != packetlength && ack.num == ackEvenOdd)
	    {
//	      printf("Packetlength is not correct but ack.num is - corrupt packet.\n");
	    } 
		else
		{
//			if(ack.num == 3)
//				printf("NACK detected.\n");
//				else
//					printf("This is unexpected.\n");
		}
	  prevMsgAck = FALSE;
//	  printf("prevMsgAck is false!\n");
	  send_flag = 1; //Prepare to send again.
	}

      else //Acknowledgement is correct.
	{
//	  printf("Correct acknowledgement received.\n");
	  prevMsgAck = TRUE;
	  if(ackEvenOdd == 1)
	    ackEvenOdd = 0;
	  else
	    ackEvenOdd = 1; //Flip the acknowledgement to prepare for the next incoming ack.
	
	if(end == 1){
//		printf("Done.\n");
		printf("Total data sent out, with or without acknowledgement: %d\n", total_data_sent);
		position+=packetlength;
		ended = 1;
	}
//	else
//	  printf("Next expected ackEvenOdd: %d\n", ackEvenOdd);
	}
 } //End of file while loop

  gettimeofday(&recvt, NULL);
  *len = position;
  tv_sub(&recvt, &sendt);
  time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
  return(time_inv);
}


//Calculate the time interval between out and in
void tv_sub(struct  timeval *out, struct timeval *in)
{
	//out time is less than in
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
