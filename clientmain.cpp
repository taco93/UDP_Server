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
void convertMsgToNet(calcMessage* calcMsg)
{

	calcMsg->protocol = htons(calcMsg->protocol);
	calcMsg->type = htons(calcMsg->type);
	calcMsg->message = htons(calcMsg->message);
	calcMsg->major_version = htons(calcMsg->major_version);
	calcMsg->minor_version = htons(calcMsg->minor_version);

}
void convertMsgToHost(calcMessage* calcMsg)
{

	calcMsg->protocol = ntohs(calcMsg->protocol);
	calcMsg->type = ntohs(calcMsg->type);
	calcMsg->message = ntohs(calcMsg->message);
	calcMsg->major_version = ntohs(calcMsg->major_version);
	calcMsg->minor_version = ntohs(calcMsg->minor_version);

}
void convertToHost(calcProtocol* calcProto)
{
	calcProto->arith = ntohs(calcProto->arith);
	calcProto->id = ntohs(calcProto->id);
	if (calcProto->arith < 5)
	{
		calcProto->inValue1 = ntohl(calcProto->inValue1);
		calcProto->inValue2 = ntohl(calcProto->inValue2);

	}
}
void convertToNetwork(calcProtocol* calcProto)
{
	calcProto->arith = htons(calcProto->arith);
	calcProto->id = htons(calcProto->id);
	if (calcProto->arith < 5)
	{
		calcProto->inValue1 = htonl(calcProto->inValue1);
		calcProto->inValue2 = htonl(calcProto->inValue2);
		calcProto->inResult = htonl(calcProto->inResult);
	}
}
void calculateResult(struct calcProtocol* calc)
{
	switch (calc->arith)
	{
	case 1:
		calc->inResult = calc->inValue1 + calc->inValue2;
		break;
	case 2:
		calc->inResult = calc->inValue1 - calc->inValue2;
		break;
	case 3:
		calc->inResult = calc->inValue1 * calc->inValue2;
		break;
	case 4:
		calc->inResult = calc->inValue1 / calc->inValue2;
		break;
	case 5:
		calc->flResult = calc->flValue1 + calc->flValue2;
		break;
	case 6:
		calc->flResult = calc->flValue1 - calc->flValue2;
		break;
	case 7:
		calc->flResult = calc->flValue1 * calc->flValue2;
		break;
	case 8:
		calc->flResult = calc->flValue1 / calc->flValue2;
		break;
	default:
		break;
	}
}
int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("Too few arguments\nExpected: <server-ip> <server-port>");
		exit(0);
	}
	//Variabler som kommer anv�ndas deklareras
	uint16_t sockFD;
	struct addrinfo hints, * servinfo, * p;
	struct sockaddr_in servAddress;
	socklen_t servAddressSize = sizeof(servAddress);
	uint16_t returnValue;
	int numBytes;
	struct calcMessage* serverCalcMsg;
	struct calcMessage calcMsg;
	calcMsg.protocol = 17;
	calcMsg.type = 22;
	calcMsg.message = 0;
	calcMsg.major_version = 1;
	calcMsg.minor_version = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	struct calcProtocol* calculate;
	struct timeval timeO;
	timeO.tv_sec = 2;
	timeO.tv_usec = 0;
	int timeOutCounter = 2;
	bool success = false;
	void* ptrTemp[100];

	memset(&calculate, 0, sizeof(calculate));
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
	if (setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeO, sizeof(timeO)) == -1)
	{
		printf("Error setting socket timeout: %s\n", gai_strerror(errno));
		exit(0);
	}
	convertMsgToNet(&calcMsg);
	numBytes = sendto(sockFD, &calcMsg, sizeof(calcMessage), 0, p->ai_addr, p->ai_addrlen);
	printf("[<]Sent %d bytes\n", numBytes);

	for (int i = 0; i <= timeOutCounter && !success; i++)
	{
		numBytes = recvfrom(sockFD, &ptrTemp, sizeof(ptrTemp), 0, (struct sockaddr*) & servAddress, &servAddressSize);
		if (numBytes == -1 && i != timeOutCounter)
		{
			numBytes = sendto(sockFD, &calcMsg, sizeof(calcMsg), 0, p->ai_addr, p->ai_addrlen);
			printf("No response, resending\n[<]Sent %d bytes\n", numBytes);

		}
		else if (numBytes != -1)
		{
			if (numBytes == sizeof(calcProtocol))
			{
				calculate = (struct calcProtocol*) ptrTemp;
				printf("[>]Recieved %d bytes\n", numBytes);
				convertToHost(calculate);
				calculateResult(calculate);
				printf("My ID: %d\n", calculate->id);
				if (calculate->arith < 5)
				{
					printf("Result: %d\n", calculate->inResult);

				}
				else
				{
					printf("Result: %8.8g\n", calculate->flResult);

				}
				convertToNetwork(calculate);
				numBytes = sendto(sockFD, calculate, sizeof(calcProtocol), 0, p->ai_addr, p->ai_addrlen);
				printf("[<]Sent %d bytes\n", numBytes);


			}
			else if (numBytes == sizeof(calcMessage))
			{


				printf("Error:Protocol not supported\n");
				close(sockFD);
				exit(0);
			}
			success = true;
		}
	}
	if (!success)
	{
		printf("Timed Out\n");
		close(sockFD);
		exit(0);
	}
	success = false;
	for (int i = 0; i < timeOutCounter +1 && !success; i++)
	{
		numBytes = recvfrom(sockFD, &ptrTemp, sizeof(ptrTemp), 0, (struct sockaddr*) & servAddress, &servAddressSize);
		if (numBytes == -1 && i != timeOutCounter)
		{
			numBytes = sendto(sockFD, calculate, sizeof(calcProtocol), 0, p->ai_addr, p->ai_addrlen);
			printf("No response, resending\n[<]Sent %d bytes\n", numBytes);

		}
		else if (numBytes != -1)
		{
			if (numBytes == sizeof(calcMessage))
			{
				serverCalcMsg = (struct calcMessage*) ptrTemp;
				convertMsgToHost(serverCalcMsg);
				if (serverCalcMsg->message == 1)
				{
					printf("OK!\n");
					close(sockFD);
					exit(0);
				}
				else if (serverCalcMsg->message == 2)
				{
					printf("NOT OK!\n");
					close(sockFD);					
					exit(0);

				}
			}
			else if (numBytes == sizeof(calcProtocol))
			{

				printf("[>]Recieved %d bytes\n", numBytes);
				printf("Wrong response\n");
				convertToHost(calculate);
				calculateResult(calculate);
				printf("My ID: %d\n", calculate->id);
				convertToNetwork(calculate);
				numBytes = sendto(sockFD, calculate, sizeof(calcProtocol), 0, p->ai_addr, p->ai_addrlen);
				printf("[<]Sent %d bytes\n", numBytes);

				i = 0;

			}
		}

	}
	if (!success)
	{
		printf("Timed Out\n");
		close(sockFD);
		exit(0);
	}
	close(sockFD);
	/* Do magic */


}
