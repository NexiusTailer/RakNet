
#include "RakPeerInterface.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include <cstdio>
#include <memory.h>
#include <cstring>
#include <stdlib.h>
#include "Rand.h"
#include "RakNetStatistics.h"
#include "RakSleep.h"
#include "RakMemoryOverride.h"
#include <stdio.h>
#include "Gets.h"

using namespace RakNet;

#ifdef _WIN32
#include "Kbhit.h"
#include "WindowsIncludes.h" // Sleep
#else
#include <unistd.h> // usleep
#endif

FILE *fp;
int memoryUsage=0;
void *LoggedMalloc(size_t size, const char *file, unsigned int line)
{
	memoryUsage+=size;
	if (fp)
		fprintf(fp,"Alloc %s:%i %i bytes %i total\n", file,line,size,memoryUsage);
	char *p = (char*) malloc(size+sizeof(size));
	memcpy(p,&size,sizeof(size));
	return p+sizeof(size);
}
void LoggedFree(void *p, const char *file, unsigned int line)
{
	char *realP=(char*)p-sizeof(size_t);
	size_t allocatedSize;
	memcpy(&allocatedSize,realP,sizeof(size_t));
	memoryUsage-=allocatedSize;
	if (fp)
		fprintf(fp,"Free %s:%i %i bytes %i total\n", file,line,allocatedSize,memoryUsage);
	free(realP);
}
void* LoggedRealloc(void *p, size_t size, const char *file, unsigned int line)
{
	char *realP=(char*)p-sizeof(size_t);
	size_t allocatedSize;
	memcpy(&allocatedSize,realP,sizeof(size_t));
	memoryUsage-=allocatedSize;
	memoryUsage+=size;
	p = realloc(realP,size+sizeof(size));
	memcpy(p,&size,sizeof(size));
	if (fp)
		fprintf(fp,"Realloc %s:%i %i to %i bytes %i total\n", file,line,allocatedSize,size,memoryUsage);
	return (char*)p+sizeof(size);
}
int main(int argc, char **argv)
{
	RakPeerInterface *sender, *receiver;
	unsigned int packetNumber[32], receivedPacketNumber;
	RakNet::Time receivedTime;
	char str[256];
	char ip[32];
	RakNet::Time sendInterval, nextSend, currentTime, quitTime;
	unsigned short remotePort, localPort;
	unsigned char streamNumber;
	RakNet::BitStream bitStream;
	RakNet::Packet *packet;
	bool doSend=false;

	for (int i=0; i < 32; i++)
		packetNumber[i]=0;

	printf("This project tests RakNet's reliable ordered sending system.\n");
	printf("Difficulty: Beginner\n\n");

	printf("Act as (s)ender or (r)eceiver?\n");
	Gets(str, sizeof(str));
	if (str[0]==0)
		return 1;

	if (argc==2)
	{
		fp = fopen(argv[1],"wt");
		SetMalloc_Ex(LoggedMalloc);
		SetRealloc_Ex(LoggedRealloc);
		SetFree_Ex(LoggedFree);
	}
	else
		fp=0;

	if (str[0]=='s' || str[0]=='S')
	{
		sender = RakNet::RakPeerInterface::GetInstance();
//		sender->ApplyNetworkSimulator(.1, 100, 50);

		receiver = 0;

		printf("Enter number of ms to pass between sends: ");
		Gets(str, sizeof(str));
		if (str[0]==0)
			sendInterval=30;
		else
			sendInterval=atoi(str);

		printf("Enter remote IP: ");
		Gets(ip, sizeof(ip));
		if (ip[0]==0)
			strcpy(ip, "127.0.0.1");
	//		strcpy(ip, "94.198.81.195");
		
		printf("Enter remote port: ");
		Gets(str, sizeof(str));
		if (str[0]==0)
			strcpy(str, "60000");
		remotePort=atoi(str);

		printf("Enter local port: ");
		Gets(str, sizeof(str));
		if (str[0]==0)
			strcpy(str, "0");
		localPort=atoi(str);


		printf("Connecting...\n");
		RakNet::SocketDescriptor socketDescriptor(localPort,0);
		sender->Startup(1, &socketDescriptor, 1);
		sender->Connect(ip, remotePort, 0, 0);
	}
	else
	{
		receiver = RakNet::RakPeerInterface::GetInstance();
	//	receiver->ApplyNetworkSimulator(.1, 100, 50);
		sender=0;

		printf("Enter local port: ");
		Gets(str, sizeof(str));
		if (str[0]==0)
			strcpy(str, "60000");
		localPort=atoi(str);

		printf("Waiting for connections...\n");
		RakNet::SocketDescriptor socketDescriptor(localPort,0);
		receiver->Startup(1, &socketDescriptor, 1);
		receiver->SetMaximumIncomingConnections(32);
	}
	
	printf("How long to run this test for, in seconds?\n");
	Gets(str, sizeof(str));
	if (str[0]==0)
		strcpy(str, "12000");
	
	currentTime = RakNet::GetTimeMS();
	quitTime = atoi(str) * 1000 + currentTime;

	nextSend=currentTime;

	printf("Test running.\n");

	//while (currentTime < quitTime)
	while (1)
	{
#ifdef _WIN32
		if (kbhit())
		{
			char ch=getch();
			if (ch=='q')
				return 1;
			else if (ch==' ')
			{
				RakNetStatistics *rss;
				char message[2048];
				if (sender)
					rss=sender->GetStatistics(sender->GetSystemAddressFromIndex(0));
				else
					rss=receiver->GetStatistics(receiver->GetSystemAddressFromIndex(0));
				StatisticsToString(rss, message, 2);
				printf("%s", message);
			}
		}
#endif
		if (sender)
		{
			uint32_t msgNumber;
			packet = sender->Receive();
			while (packet)
			{
				// PARSE TYPES
				switch(packet->data[0])
				{
				case ID_CONNECTION_REQUEST_ACCEPTED:
					printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
					doSend=true;
					nextSend=currentTime;
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
					break;
				case ID_DISCONNECTION_NOTIFICATION:
					printf("ID_DISCONNECTION_NOTIFICATION\n");
					break;
				case ID_CONNECTION_LOST:
					printf("ID_CONNECTION_LOST\n");
					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					printf("Connection attempt failed\n");
					break;
				case ID_SND_RECEIPT_ACKED:
					memcpy(&msgNumber, packet->data+1, 4);
					printf("Msg #%i was delivered.\n", msgNumber);
					break;
				case ID_SND_RECEIPT_LOSS:
					memcpy(&msgNumber, packet->data+1, 4);
					printf("Msg #%i was probably not delivered.\n", msgNumber);
					break;
				}

				sender->DeallocatePacket(packet);
				packet = sender->Receive();
			}
			
			while (doSend && currentTime > nextSend)
			{
				streamNumber=0;
			//	streamNumber = randomMT() % 32;
				// Do the send
				bitStream.Reset();
				bitStream.Write((unsigned char) (ID_TIMESTAMP));
				bitStream.Write(RakNet::GetTime());
				bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+1));
				bitStream.Write(packetNumber[streamNumber]++);
				bitStream.Write(streamNumber);
				char *pad;
				int padLength = (randomMT() % 5000) + 1;
				//int padLength = (randomMT() % 128) + 1;
				pad = new char [padLength];
				bitStream.Write(pad, padLength);
				delete [] pad;
				// Send on a random priority with a random stream
				// if (sender->Send(&bitStream, HIGH_PRIORITY, (PacketReliability) (RELIABLE + (randomMT() %2)) ,streamNumber, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true)==false)
				if (sender->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,streamNumber, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true)==0)
					packetNumber[streamNumber]--; // Didn't finish connecting yet?

// 				if (sender->Send(&bitStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT ,streamNumber, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true)==0)
// 					packetNumber[streamNumber]--; // Didn't finish connecting yet?

				
				if (sender)
				{
					RakNetStatistics *rssSender;
					rssSender=sender->GetStatistics(sender->GetSystemAddressFromIndex(0));
					//printf("Snd: %i. %i waiting on ack. KBPS=%.1f. Ploss=%.1f. Bandwidth=%f.\n", packetNumber[streamNumber], rssSender->messagesOnResendQueue,rssSender->bitsPerSecondSent/1000, 100.0f * ( float ) rssSender->messagesTotalBitsResent / ( float ) rssSender->totalBitsSent, rssSender->estimatedLinkCapacityMBPS);

					printf("Snd: %i at time %"PRINTF_64_BIT_MODIFIER"u with length %i\n", packetNumber[streamNumber], currentTime, bitStream.GetNumberOfBytesUsed());
				}

				nextSend+=sendInterval;

				// Test halting
			//	if (rand()%20==0)
			//		nextSend+=1000;
			}
		}
		else
		{
			packet = receiver->Receive();
			while (packet)
			{
				switch(packet->data[0])
				{
				case ID_NEW_INCOMING_CONNECTION:
					printf("ID_NEW_INCOMING_CONNECTION\n");
					break;
				case ID_DISCONNECTION_NOTIFICATION:
					printf("ID_DISCONNECTION_NOTIFICATION\n");
					break;
				case ID_CONNECTION_LOST:
					printf("ID_CONNECTION_LOST\n");
					break;
				case ID_TIMESTAMP:
					bitStream.Reset();
					bitStream.Write((char*)packet->data, packet->length);
					bitStream.IgnoreBits(8); // Ignore ID_TIMESTAMP
					bitStream.Read(receivedTime);
					bitStream.IgnoreBits(8); // Ignore ID_USER_ENUM+1
					bitStream.Read(receivedPacketNumber);
					bitStream.Read(streamNumber);

					if (receivedPacketNumber!=packetNumber[streamNumber])
						printf("ERROR! Expecting %i got %i (channel %i).",packetNumber[streamNumber], receivedPacketNumber, streamNumber);
					else
						printf("Got %i.Channel %i.Len %i.", packetNumber[streamNumber], streamNumber, packet->length);

					printf("Sent=%"PRINTF_64_BIT_MODIFIER"u Received=%"PRINTF_64_BIT_MODIFIER"u Diff=%i.\n", receivedTime, currentTime, (int)(currentTime - receivedTime));

					packetNumber[streamNumber]++;
					break;
				}

				
				receiver->DeallocatePacket(packet);
				packet = receiver->Receive();
			}
		}

// DO NOT COMMENT OUT THIS SLEEP!
// This sleep keeps RakNet responsive
#ifdef _WIN32
		Sleep(0);
#else
		usleep(0);
#endif
		currentTime=RakNet::GetTimeMS();
	}

	printf("Press any key to continue\n");
	Gets(str, sizeof(str));

	if (sender)
		RakNet::RakPeerInterface::DestroyInstance(sender);
	if (receiver)
		RakNet::RakPeerInterface::DestroyInstance(receiver);

	if (fp)
		fclose(fp);

	return 1;
}
