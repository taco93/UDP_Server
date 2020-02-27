#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>

/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>


#include "protocol.h"

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Too few arguments\nExpected: <server-ip> <server-port>");
		exit(0);
	}
	//Variabler som kommer användas deklareras
	uint16_t sockFD;
	struct addrinfo hints, * servinfo, * p;
	struct sockaddr_in servAddress;
	socklen_t servAddressSize = sizeof(servAddress);
	uint16_t returnValue;
	int numBytes;
	struct calcMessage calcMsg;
	calcMsg.protocol = 17;
	calcMsg.type = 22;
	calcMsg.message = 0;
	calcMsg.major_version = 1;
	calcMsg.minor_version = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	struct calcProtocol calculate;
	if ((returnValue = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returnValue));
		exit(0);
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}


		break;
	}
	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	numBytes = sendto(sockFD, &calcMsg, sizeof(calcMsg), 0, p->ai_addr, p->ai_addrlen);
	printf("[<]Sent %d bytes\n", numBytes);
	numBytes = recvfrom(sockFD, &calculate, sizeof(calculate),0, (struct sockaddr*) &servAddress, &servAddressSize);
	printf("[>]Recieved %d bytes, %d", numBytes, calculate.arith);

	close(sockFD);
  /* Do magic */
  

}
