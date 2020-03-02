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

struct worker
{
	char address[100];
	uint32_t port;
	struct calcProtocol calc;
	struct timeval lastMessage;
};
int capacity = 20;
int nrOfWorkers = 0;
worker** workers = new worker * [capacity] { nullptr };

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum) {
	// As anybody can call the handler, its good coding to check the signal number that called it.
	if (signum == SIGALRM)
	{
		printf("Checking inactive workers\n");
		struct timeval now;
		gettimeofday(&now, NULL);
		for (int i = 0; i < nrOfWorkers; i++)
		{
			if (now.tv_sec - workers[i]->lastMessage.tv_sec > 10)
			{
				printf("Worker with ID %d has not responded to the job, killing worker\n", workers[i]->calc.id);
				delete workers[i];
				for (int y = i; y < nrOfWorkers; y++)
				{
					workers[y] = workers[y + 1];
				}
				nrOfWorkers--;
				i--;
			}
		}
		if (loopCount > 200) {
			printf("I had enough.\n");
			terminate = 1;
		}
	}

	return;
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
void expand(struct worker** workers, int nrOfWorkers, int& maxFollowers)
{
	maxFollowers += 10;
	struct worker** temp = new worker * [maxFollowers] {nullptr};
	for (int i = 0; i < nrOfWorkers; i++)
	{
		temp[i] = workers[i];
	}
	delete[]workers;
	workers = temp;
	temp = nullptr;
}
int main(int argc, char* argv[]) {
	if (argc < 2)
	{
		printf("Too few arguments\nExpected: <port>");
		exit(0);
	}
	/* Do more magic */
	const double EPSILON = 0.001;
	initCalcLib();

	char currentAddr[100];
	struct addrinfo guide, * serverInfo, * p;
	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof(their_addr);
	uint8_t sockFD;
	struct timeval timeO;
	timeO.tv_sec = 5;
	timeO.tv_usec = 0;
	struct calcMessage* calcMsg;
	struct calcProtocol* calcProto;
	int numBytes;
	int currentClientID = 1;
	void* ptrTemp[100];
	memset(&guide, 0, sizeof(guide));
	guide.ai_family = AF_INET;
	guide.ai_socktype = SOCK_DGRAM;
	guide.ai_flags = AI_PASSIVE;
	uint8_t returnValue;
	char clientIP[100];
	int clientIP_Len = sizeof(clientIP);
	bool alreadyExists = false;
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
	if (setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeO, sizeof(timeO)) == -1)
	{
		printf("Error setting socket timeout: %s\n", gai_strerror(errno));
		exit(0);
	}

	/* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
	signal(SIGALRM, checkJobbList);
	setitimer(ITIMER_REAL, &alarmTime, NULL); // Start/register the alarm. 


	while (terminate == 0)
	{
		if (nrOfWorkers >= capacity)
		{
			expand(workers, nrOfWorkers, capacity);
		}
		printf("This is the main loop, %d time.\n", loopCount);
		printf("Nr of workers: %d\n", nrOfWorkers);
		memset(&calcProto, 0, sizeof(calcProto));
		memset(&ptrTemp, 0, sizeof(ptrTemp));
		memset(&calcMsg, 0, sizeof(calcMsg));
		memset(&clientIP, 0, sizeof(clientIP));
		memset(&currentAddr, 0, sizeof(currentAddr));
		memset(&their_addr, 0, sizeof(their_addr));
		numBytes = recvfrom(sockFD, &ptrTemp, sizeof(ptrTemp), 0, (struct sockaddr*) & their_addr, &addr_len);
		if (numBytes != -1)
		{

			inet_ntop(AF_INET, &their_addr.sin_addr, clientIP, clientIP_Len);
			printf("---Client connected from %s:%d---\n", clientIP, ntohs(their_addr.sin_port));
			printf("[>]Recieved %d bytes\n", numBytes);
			if (numBytes == sizeof(calcMessage))
			{
				printf("It's a calcMessage\n");
				calcMsg = (struct calcMessage*) ptrTemp;
				printf("Protocol: %d\n", calcMsg->protocol);
				for (int i = 0; i < nrOfWorkers; i++)
				{
					if (strcmp(clientIP, workers[i]->address) == 0 && workers[i]->port == ntohs(their_addr.sin_port))
					{
						printf("Worker already exists, resending job\n");
						if (workers[i]->calc.arith < 5)
						{
							workers[i]->calc.inResult = 0;
						}
						else
						{
							workers[i]->calc.flResult = 0;
						}
						workers[i]->calc.inValue1 = htons(workers[i]->calc.inValue1);
						workers[i]->calc.inValue2 = htons(workers[i]->calc.inValue2);
						numBytes = sendto(sockFD, &workers[i]->calc, sizeof(calcProtocol), 0, (struct sockaddr*) & their_addr, addr_len);
						printf("[<]Sent %d bytes\n", numBytes);
						gettimeofday(&workers[i]->lastMessage, NULL);
						workers[i]->calc.inValue1 = ntohs(workers[i]->calc.inValue1);
						workers[i]->calc.inValue2 = ntohs(workers[i]->calc.inValue2);
						calculateResult(&workers[i]->calc);
						alreadyExists = true;
					}

				}
				if (!alreadyExists)
				{

					if (calcMsg->protocol == 17 && calcMsg->major_version == 1 &&
						calcMsg->message == 0 && calcMsg->minor_version == 0 && calcMsg->type == 22)
					{

						printf("Protocol supported!\n");
						workers[nrOfWorkers] = new struct worker;
						printf("Created Worker\n");
						sprintf(workers[nrOfWorkers]->address, "%s", clientIP);
						workers[nrOfWorkers]->port = ntohs(their_addr.sin_port);
						printf("Address of worker: %s\n", workers[nrOfWorkers]->address);
						printf("Port of worker: %d\n", workers[nrOfWorkers]->port);
						workers[nrOfWorkers]->calc.type = 1;
						char* oper = randomType();
						//Sätter in variabler i worker (operator, values, id)
						if (strchr(oper, 'f') != NULL)
						{
							printf("Operation: %s\n", oper);

							if (strcmp(oper, "fdiv") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 8;

							}
							else if (strcmp(oper, "fsub") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 6;

							}
							else if (strcmp(oper, "fadd") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 5;

							}
							else if (strcmp(oper, "fmul") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 7;

							}
							workers[nrOfWorkers]->calc.flValue1 = randomFloat();
							printf("Val 1: %8.8g\n", workers[nrOfWorkers]->calc.flValue1);

							workers[nrOfWorkers]->calc.flValue2 = randomFloat();
							printf("Val 2: %8.8g\n", workers[nrOfWorkers]->calc.flValue2);

						}
						else
						{
							printf("Operation: %s\n", oper);
							if (strcmp(oper, "sub") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 2;

							}
							else if (strcmp(oper, "add") == 0)
							{

								workers[nrOfWorkers]->calc.arith = 1;
							}
							else if (strcmp(oper, "mul") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 3;

							}
							else if (strcmp(oper, "div") == 0)
							{
								workers[nrOfWorkers]->calc.arith = 4;

							}
							workers[nrOfWorkers]->calc.inValue1 = randomInt();
							printf("Val 1: %d\n", workers[nrOfWorkers]->calc.inValue1);
							workers[nrOfWorkers]->calc.inValue2 = randomInt();
							if (workers[nrOfWorkers]->calc.inValue2 == 0 && strcmp(oper, "div") == 0)
							{
								workers[nrOfWorkers]->calc.inValue2 = 1;
							}
							printf("Val 2: %d\n", workers[nrOfWorkers]->calc.inValue2);
							workers[nrOfWorkers]->calc.inValue1 = htons(workers[nrOfWorkers]->calc.inValue1);
							workers[nrOfWorkers]->calc.inValue2 = htons(workers[nrOfWorkers]->calc.inValue2);

						}

						workers[nrOfWorkers]->calc.id = currentClientID++;
						printf("Current ID: %d\n", workers[nrOfWorkers]->calc.id);
						workers[nrOfWorkers]->calc.type = 1;
						gettimeofday(&workers[nrOfWorkers]->lastMessage, NULL);
						printf("Seconds: %d\n", workers[nrOfWorkers]->lastMessage.tv_sec);
						numBytes = sendto(sockFD, &workers[nrOfWorkers]->calc, sizeof(calcProtocol), 0, (struct sockaddr*) & their_addr, addr_len);
						printf("[<]Sent %d bytes\n", numBytes);
						workers[nrOfWorkers]->calc.inValue1 = ntohs(workers[nrOfWorkers]->calc.inValue1);
						workers[nrOfWorkers]->calc.inValue2 = ntohs(workers[nrOfWorkers]->calc.inValue2);
						
						calculateResult(&workers[nrOfWorkers]->calc);
						if (strchr(oper, 'f') != NULL)
						{
							printf("Result of worker %d: %8.8g\n", nrOfWorkers + 1, workers[nrOfWorkers]->calc.flResult);

						}
						else
						{
							printf("Result of worker %d: %d\n", nrOfWorkers + 1, workers[nrOfWorkers]->calc.inResult);

						}
						nrOfWorkers++;
						printf("Nr of workers: %d\n", nrOfWorkers);

					}
					else
					{
						printf("Protocol NOT supported!\n");
						struct calcMessage wrongProto;
						wrongProto.type = 2;
						wrongProto.message = 2;
						wrongProto.major_version = 1;
						wrongProto.minor_version = 0;
						numBytes = sendto(sockFD, &wrongProto, sizeof(wrongProto), 0, (struct sockaddr*) & their_addr, addr_len);
						printf("[<]Sent %d bytes\n", numBytes);
					}
				}
				alreadyExists = false;
			}
			else if (numBytes == sizeof(calcProtocol))
			{
				printf("It's a calcProtocol\n");
				calcProto = (struct calcProtocol*) ptrTemp;
				printf("ID: %d\n", calcProto->id);
				if (calcProto->arith < 5)
				{
					calcProto->inResult = ntohs(calcProto->inResult);

					printf("Result of client: %d\n", calcProto->inResult);

				}
				else
				{
					printf("Result of client: %8.8g\n", calcProto->flResult);

				}
				if (nrOfWorkers >= 1)
				{

					for (int i = 0; i < nrOfWorkers; i++)
					{
						if (calcProto->id == workers[i]->calc.id)
						{
							printf("ID found in joblist\n");
							if (strcmp(clientIP, workers[i]->address) == 0 && ntohs(their_addr.sin_port) == workers[i]->port)
							{
								printf("Addresses and ports match!\n");
								if (calcProto->arith < 5)
								{
									if (calcProto->inResult == workers[i]->calc.inResult)
									{
										printf("---SUCCESS---\n");
										struct calcMessage correctMsg;
										correctMsg.type = 1;
										correctMsg.protocol = 17;
										correctMsg.message = 1;
										correctMsg.major_version = 1;
										correctMsg.minor_version = 0;
										numBytes = sendto(sockFD, &correctMsg, sizeof(correctMsg), 0, (struct sockaddr*) & their_addr, addr_len);
										printf("[<]Sent %d bytes\n", numBytes);
									}
									else
									{
										printf("---FAILURE---\n");
										struct calcMessage wrongMsg;
										wrongMsg.type = 1;
										wrongMsg.protocol = 17;
										wrongMsg.message = 2;
										wrongMsg.major_version = 1;
										wrongMsg.minor_version = 0;
										numBytes = sendto(sockFD, &wrongMsg, sizeof(wrongMsg), 0, (struct sockaddr*) & their_addr, addr_len);
										printf("[<]Sent %d bytes\n", numBytes);
									}
								}
								else
								{
									if (abs(calcProto->flResult - workers[i]->calc.flResult) < EPSILON)
									{
										printf("---SUCCESS---\n");
										struct calcMessage correctMsg;
										correctMsg.type = 1;
										correctMsg.protocol = 17;
										correctMsg.message = 1;
										correctMsg.major_version = 1;
										correctMsg.minor_version = 0;
										numBytes = sendto(sockFD, &correctMsg, sizeof(correctMsg), 0, (struct sockaddr*) & their_addr, addr_len);
										printf("[<]Sent %d bytes\n", numBytes);
									}
									else
									{
										printf("---FAILURE---\n");
										struct calcMessage wrongMsg;
										wrongMsg.type = 1;
										wrongMsg.protocol = 17;
										wrongMsg.message = 2;
										wrongMsg.major_version = 1;
										wrongMsg.minor_version = 0;
										numBytes = sendto(sockFD, &wrongMsg, sizeof(wrongMsg), 0, (struct sockaddr*) & their_addr, addr_len);
										printf("[<]Sent %d bytes\n", numBytes);
									}

								}
								delete workers[i];
								for (int y = i; y < nrOfWorkers; y++)
								{
									workers[y] = workers[y + 1];
								}
								nrOfWorkers--;
							}
							else
							{
								printf("Shady client, ID doesn't match address on file\n");
							}
						}


					}
				}
				else
				{
					printf("No workers\n");
				}

			}
		}
		else
		{
			printf("No clients connecting...\n");
		}


		usleep(500000);
		loopCount++;




	}
	printf("done.\n");
	close(sockFD);
	return(0);
}
