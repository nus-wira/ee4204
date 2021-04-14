/*******************************
udp_client.c: the source file of the client in udp
********************************/

#include "headsock.h"

void str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len);       
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in


int main(int argc, char *argv[])
{
	// length of message, socket object
  int len, sockfd;
	struct sockaddr_in ser_addr;
	char **pptr; // array of strings
	struct hostent *sh;
	struct in_addr **addrs; // address list (array of addresses)
	FILE *fp;

	if (argc!= 2)
	{
		printf("parameters not match.");
		exit(0);
	}

	if ((sh=gethostbyname(argv[1]))==NULL) {             //get host's information
		printf("error when gethostbyname");
		exit(0);
	}
  
  // AF_INET: Internet address
  // SOCK_DGRAM: datagram, connectionless-mode, unreliable
  // 0: Default UDP
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);             //create socket
	if (sockfd<0)
	{
		printf("error in socket");
		exit(1);
	}
  
  // get address list from host
	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name); // host name
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);			//printf socket information
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(MYUDP_PORT); // host byte order to network byte order
  // copy address list from host into sockaddr struct
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);

	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}

	str_cli1(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);   // receive and send

	close(sockfd);
	exit(0);
}

void str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len)
{
	char *buf; // buffer
	long lsize, ci; // size of file, current index
	char sends[DATALEN]; // string that is sent
	struct ack_so ack; // to receive ACK
	int n, slen; // num bytes sent, num bytes in message 
	float time_inv = 0.0;
	struct timeval sendt, recvt; // to calculate time
	ci = 0;
	int cdu = 1, cp = 0; // current DU size, current packet #

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp); // get size of file
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	gettimeofday(&sendt, NULL);							//get the current time
	
  // copy entire file, keep track using index
	while(ci<= lsize)
	{
    // send DATALEN if enough data left, otherwise just sent the rest of data
		if ((lsize+1-ci) <= DATALEN)
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
    // copy slen bytes from buffer into string to be sent
		memcpy(sends, (buf+ci), slen);
		
    // n: num bytes sent
		n = sendto(sockfd, &sends, slen, 0, addr, addrlen);
		
		if(n == -1) {
			printf("send error!");								//send the data
			exit(1);
		}
		ci += slen; // update current index to after slen bytes sent
		
		// Get ACK only if no. of DU match packet no.
		cp++;
		if (cdu != cp) continue;


		if (n= recvfrom(sockfd, &ack, DATALEN, 0, addr, &addrlen)==-1)                                   //receive the ack
		{
			printf("error when receiving\n");
			exit(1);
		}
		
		if (ack.num != 1|| ack.len != 0)
			printf("error in transmission\n");


		
		// Reset current packet
		cp = 0;
		// Increment current DU #
		cdu++;
		if (cdu > MAXDU) cdu = 1;
		
	}

	// Get time taken
	gettimeofday(&recvt, NULL);
	*len= ci;                                                         //get current time
	tv_sub(&recvt, &sendt);                                                                 // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	float rt = *len / (float) time_inv;
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", time_inv, *len, rt);
	
	fclose(fp);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}