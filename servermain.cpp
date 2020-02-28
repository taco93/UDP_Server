#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>

/* You will to add includes here */
#define THISPORT "1969"

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"


using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount = 0;
int terminate = 0;


/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum) {
	// As anybody can call the handler, its good coding to check the signal number that called it.

	printf("Let me be, I want to sleep.\n");

	if (loopCount > 20) {
		printf("I had enough.\n");
		terminate = 1;
	}

	return;
}



void calculateResult(struct calcProtocol *calc)
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

int main(int argc, char* argv[]) {
	if (argc < 2)
	{
		printf("Too few arguments\nExpected: <port>");
		exit(0);
	}
	/* Do more magic */
	initCalcLib();
	const double EPSILON = 0.001;
	struct addrinfo guide, * serverInfo, * p;
	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof(their_addr);
	uint8_t sockFD;
	struct calcMessage thisCalc;
	thisCalc.protocol = 17;
	thisCalc.type = 22;
	thisCalc.message = 0;
	thisCalc.major_version = 1;
	thisCalc.minor_version = 0;
	struct calcMessage calcMsg;
	int numBytes;
	memset(&guide, 0, sizeof(guide));
	guide.ai_family = AF_INET;
	guide.ai_socktype = SOCK_DGRAM;
	guide.ai_flags = AI_PASSIVE;
	uint8_t returnValue;
	char clientMsg[100];
	char clientIP[100];
	int clientIP_Len = sizeof(clientIP);
	if ((returnValue = getaddrinfo(NULL, argv[1], &guide, &serverInfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returnValue));
		exit(0);
	}
	for (p = serverInfo; p != NULL; p = p->ai_next)
	{

		if ((sockFD = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1)
		{
			printf("listener: socket: %s\n", gai_strerror(errno));
			continue;
		}

		if (bind(sockFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockFD);
			printf("listener error: bind: %s\n", gai_strerror(errno));
			continue;
		}

		break;
	}
	/*
	   Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set.
	*/
	struct itimerval alarmTime;
	alarmTime.it_interval.tv_sec = 10;
	alarmTime.it_interval.tv_usec = 10;
	alarmTime.it_value.tv_sec = 10;
	alarmTime.it_value.tv_usec = 10;

	/* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
	signal(SIGALRM, checkJobbList);
	setitimer(ITIMER_REAL, &alarmTime, NULL); // Start/register the alarm. 


	while (terminate == 0)
		printf("This is the main loop, %d time.\n", loopCount);
	{
		memset(&calcMsg, 0, sizeof(calcMsg));
		memset(&clientIP, 0, clientIP_Len);
		numBytes = recvfrom(sockFD, &calcMsg, sizeof(calcMsg), 0, (struct sockaddr*) & their_addr, &addr_len);
		inet_ntop(AF_INET, &their_addr.sin_addr, clientIP, clientIP_Len);
		printf("---Client connected from %s:%d---\n", clientIP, ntohs(their_addr.sin_port));
		printf("[>]Recieved %d bytes, %d\n", numBytes, calcMsg.protocol);
		if (calcMsg.protocol == thisCalc.protocol)
		{
			struct calcProtocol calculate;
			struct calcProtocol clientCalc;
			calculate.type = 1;
			char* oper = randomType();
			printf("%s\n", oper);
			if (strchr(oper, 'f') != NULL)
			{
				if (strcmp(oper, "fdiv") == 0)
				{
					calculate.arith = 8;

				}
				else if (strcmp(oper, "fsub") == 0)
				{
					calculate.arith = 6;

				}
				else if (strcmp(oper, "fadd") == 0)
				{
					calculate.arith = 5;

				}
				else if (strcmp(oper, "fmul") == 0)
				{
					calculate.arith = 7;

				}
				calculate.flValue1 = randomFloat();
				printf("Val 1: %8.8g\n", calculate.flValue1);

				calculate.flValue2 = randomFloat();
				printf("Val 2: %8.8g\n", calculate.flValue2);

				clientCalc= calculate;
			}
			else
			{
				printf("Operation: %s\n", oper);
				if (strcmp(oper, "sub") == 0)
				{
					calculate.arith = 2;

				}
				else if (strcmp(oper, "add") == 0)
				{

					calculate.arith = 1;
				}
				else if (strcmp(oper, "mul") == 0)
				{
					calculate.arith = 3;

				}
				else if (strcmp(oper, "div") == 0)
				{
					calculate.arith = 4;

				}
				calculate.inValue1 = randomInt();
				printf("Val 1: %d\n", calculate.inValue1);
				calculate.inValue2 = randomInt();
				printf("Val 2: %d\n", calculate.inValue2);
				clientCalc = calculate;

			}

			numBytes = sendto(sockFD, &clientCalc, sizeof(clientCalc), 0, (struct sockaddr*) & their_addr, addr_len);
			printf("[<]Sent %d bytes\n", numBytes);
			calculateResult(&calculate);
			numBytes = recvfrom(sockFD, &clientCalc, sizeof(clientCalc), 0, (struct sockaddr*) & their_addr, &addr_len);
			if (strchr(oper, 'f') != NULL)
			{
				printf("Server result: %8.8g\n", calculate.flResult);
				printf("[>]Recieved %d bytes, %8.8g\n", numBytes, clientCalc.flResult);
				if (abs(calculate.flResult - clientCalc.flResult) < EPSILON)
				{

					printf("---SUCCESS---\n");
				}
			}
			else
			{
				printf("Server result: %d\n", calculate.inResult);

				printf("[>]Recieved %d bytes, %d\n", numBytes, clientCalc.inResult);
				if (calculate.inResult == clientCalc.inResult)
				{
					printf("---SUCCESS---\n");
				}

			}


		}
		else
		{
			calcMsg.type = 2;
			calcMsg.message = 2;
			calcMsg.major_version = 1;
			calcMsg.minor_version = 0;
			numBytes = sendto(sockFD, &calcMsg, sizeof(calcMsg), 0, (struct sockaddr*) & their_addr, addr_len);
			printf("[<]Sent %d bytes\n", numBytes);

		}
		sleep(1);
		loopCount++;
	}

	printf("done.\n");
	close(sockFD);
	return(0);



}
