/**************************************
udp_ser.c: the source file of the server in udp transmission
**************************************/
#include "headsock.h"

void str_ser1(int sockfd);                                                           // transmitting and receiving function

int main(int argc, char *argv[])
{
	int sockfd; // socket
	struct sockaddr_in my_addr; // address
  
  // AF_INET: Internet address
  // SOCK_DGRAM: datagram, connectionless-mode, unreliable
  // 0: Default UDP
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {			//create socket
		printf("error in socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET; // Internet address
	my_addr.sin_port = htons(MYUDP_PORT); // host byte order to network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // in case system has multiple IPs
	bzero(&(my_addr.sin_zero), 8); // Writes 8 bytes of '\0'
  // Assigns address my_addr to socket sockfd
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {           //bind socket
		printf("error in binding");
		exit(1);
	}
	printf("start receiving\n");
	while(1) {
		str_ser1(sockfd);                        // send and receive
	}
	close(sockfd);
	exit(0);
}

void str_ser1(int sockfd)
{
	char buf[BUFSIZE]; // buffer
	FILE *fp; // filepointer
	char recvs[DATALEN]; // received data
	struct ack_so ack; // ack packet
	int end = 0, n = 0, len; // end of file bool, num bytes of received string, length of address
	long lseek=0; // index to buffer
	int cdu = 1, cp = 0; // current DU size, current packet #
	
	struct sockaddr_in addr;

	len = sizeof (struct sockaddr_in);

	while (!end) {
    // n = num bytes received
    // addr: sockaddr struct to store 
		if ((n=recvfrom(sockfd, &recvs, DATALEN, 0, (struct sockaddr *)&addr, &len)) == -1) {      //receive the packet
			printf("error receiving");
			exit(1);
		}

		if (recvs[n-1] == '\0')									//if it is the end of the file
		{
			end = 1; // flag reached end
			n --;
		}
		memcpy((buf+lseek), recvs, n); // copy from recvs into buffer, n bytes
		lseek += n; // add n to buffer pointer
		
		// Send ACK only if no. of DU match packet no.
		cp++;
		if (cdu != cp) continue;

		ack.num = 1;
		ack.len = 0;
    // ack: message that is sent
    // 2 bytes for ACK packet, 0 flag
    // destination address struct, length of struct
		if ((n = sendto(sockfd, &ack, 2, 0, (struct sockaddr *)&addr, len))==-1)
		{
				printf("send error!");								//send the ack
				exit(1);
		}

		// Reset current packet
		cp = 0;
		// Increment current DU #
		cdu++;
		if (cdu > MAXDU) cdu = 1;
	}
	
	if ((fp = fopen ("myUDPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exist\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);

}
