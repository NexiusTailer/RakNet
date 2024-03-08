#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include "BitStream.h"
#include <stdlib.h> // For atoi
#include <cstring> // For strlen
#include "RakNetStatistics.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "MTUSize.h"
#include <stdio.h>
#include "Kbhit.h"

#ifdef _COMPATIBILITY_1
#include "Compatibility1Includes.h" // Developers of a certain platform will know what to do here.
#else
#include "RakSleep.h"
#endif

bool quit;
bool sentPacket=false;

#define BIG_PACKET_SIZE 100000000

RakPeerInterface *client, *server;
char *text;

int main(void)
{
	client=server=0;

	text= new char [BIG_PACKET_SIZE];
	quit=false;
	char ch;

	printf("This is a test I use to test the packet splitting capabilities of RakNet\n");
	printf("All it does is send a large block of data to the feedback loop\n");
	printf("Difficulty: Beginner\n\n");

	printf("Enter 's' to run as server, 'c' to run as client, space to run local.\n");
	ch=' ';
	gets(text);
	ch=text[0];

	if (ch=='c')
	{
		client=RakNetworkFactory::GetRakPeerInterface();
		printf("Working as client\n");
		printf("Enter remote IP: ");
		gets(text);
		if (text[0]==0)
			//strcpy(text, "127.0.0.1");
			strcpy(text, "94.198.81.195"); // dx in Europe
	}
	else if (ch=='s')
	{
		server=RakNetworkFactory::GetRakPeerInterface();
		printf("Working as server\n");
	}
	else
	{
		client=RakNetworkFactory::GetRakPeerInterface();
		server=RakNetworkFactory::GetRakPeerInterface();;
		strcpy(text, "127.0.0.1");
	}
	if (client)
	{
		client->SetTimeoutTime(2000,UNASSIGNED_SYSTEM_ADDRESS);
		SocketDescriptor socketDescriptor(0,0);
		client->Startup(1, 10, &socketDescriptor, 1);
		client->SetSplitMessageProgressInterval(10000); // Get ID_DOWNLOAD_PROGRESS notifications
		client->Connect(text, 60000, 0, 0);
	}
	if (server)
	{
		server->SetTimeoutTime(2000,UNASSIGNED_SYSTEM_ADDRESS);
		SocketDescriptor socketDescriptor(60000,0);
		server->SetMaximumIncomingConnections(4);
		server->Startup(4, 10, &socketDescriptor, 1);
	}
	RakSleep(500);

	// Always apply the network simulator on two systems, never just one, with half the values on each.
	// Otherwise the flow control gets confused.
	//if (client)
	// client->ApplyNetworkSimulator(128000, 0, 0);
	//if (server)
	//	server->ApplyNetworkSimulator(128000, 0, 0);

	RakNetTime start,stop;

	RakNetTime nextStatTime = RakNet::GetTime() + 1000;
	Packet *packet;
	start=RakNet::GetTimeMS();
	while (!quit)
	{
		if (server)
		{
			for (packet = server->Receive(); packet; server->DeallocatePacket(packet), packet=server->Receive())
			{
				if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
				{
					printf("Starting send\n");
					start=RakNet::GetTimeMS();
					if (BIG_PACKET_SIZE<=100000)
					{
						for (int i=0; i < BIG_PACKET_SIZE; i++)
							text[i]=255-(i&255);
					}
					else
						text[0]=(unsigned char) 255;
					server->Send(text, BIG_PACKET_SIZE, LOW_PRIORITY, RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
					// Keep the stat from updating until the messages move to the thread or it quits right away
					nextStatTime=RakNet::GetTime()+1000;
				}
				if (packet->data[0]==ID_CONNECTION_LOST)
					printf("ID_CONNECTION_LOST from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_DISCONNECTION_NOTIFICATION)
					printf("ID_DISCONNECTION_NOTIFICATION from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
					printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
					printf("ID_CONNECTION_REQUEST_ACCEPTED from %s\n", packet->systemAddress.ToString());
			}

			if (kbhit())
			{
				char ch=getch();
				if (ch==' ')
				{
					printf("Sending medium priority message\n");
					char t[1];
					t[0]=(unsigned char) 254;
					server->Send(t, 1, MEDIUM_PRIORITY, RELIABLE_ORDERED, 1, UNASSIGNED_SYSTEM_ADDRESS, true);
				}
			}
		}
		if (client)
		{
			packet = client->Receive();
			while (packet)
			{
				if (packet->data[0]==ID_DOWNLOAD_PROGRESS)
				{
					RakNet::BitStream progressBS(packet->data, packet->length, false);
					progressBS.IgnoreBits(8); // ID_DOWNLOAD_PROGRESS
					unsigned int progress;
					unsigned int total;
					unsigned int partLength;

					// Disable endian swapping on reading this, as it's generated locally in ReliabilityLayer.cpp
					progressBS.ReadBits( (unsigned char* ) &progress, BYTES_TO_BITS(sizeof(progress)), true );
					progressBS.ReadBits( (unsigned char* ) &total, BYTES_TO_BITS(sizeof(total)), true );
					progressBS.ReadBits( (unsigned char* ) &partLength, BYTES_TO_BITS(sizeof(partLength)), true );

					printf("Progress: msgID=%i Progress %i/%i Partsize=%i\n",
						(unsigned char) packet->data[0],
						progress,
						total,
						partLength);
				}
				else if (packet->data[0]==255)
				{
					if (packet->length!=BIG_PACKET_SIZE)
					{
						printf("Test failed. %i bytes (wrong number of bytes).\n", packet->length);
						quit=true;
						break;
					}
					if (BIG_PACKET_SIZE<=100000)
					{
						for (int i=0; i < BIG_PACKET_SIZE; i++)
						{
							if  (packet->data[i]!=255-(i&255))
							{
								printf("Test failed. %i bytes (bad data).\n", packet->length);
								quit=true;
								break;
							}
						}
					}

					if (quit==false)
					{
						printf("Test succeeded. %i bytes.\n", packet->length);
						quit=true;
					}

				}
				else if (packet->data[0]==254)
				{
 					printf("Got high priority message.\n");
				}
				else if (packet->data[0]==ID_CONNECTION_LOST)
					printf("ID_CONNECTION_LOST from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_DISCONNECTION_NOTIFICATION)
					printf("ID_DISCONNECTION_NOTIFICATION from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
					printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
				else if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
				{
					start=RakNet::GetTimeMS();
					printf("ID_CONNECTION_REQUEST_ACCEPTED from %s\n", packet->systemAddress.ToString());
				}

				client->DeallocatePacket(packet);
				packet = client->Receive();
			}
		}

		if (RakNet::GetTime() > nextStatTime)
		{
			nextStatTime=RakNet::GetTime()+1000;
			RakNetStatistics rssSender;
			RakNetStatistics rssReceiver;
			if (server)
			{
				unsigned int i;
				unsigned short numSystems;
				server->GetConnectionList(0,&numSystems);
				if (numSystems>0)
				{
					for (i=0; i < numSystems; i++)
					{
						server->GetStatistics(server->GetSystemAddressFromIndex(i), &rssSender);
						StatisticsToString(&rssSender, text,2);
						printf("==== System %i ====\n", i+1);
						printf("%s\n\n", text);
					}
				}
			}
			if (client && server==0)
			{
				client->GetStatistics(client->GetSystemAddressFromIndex(0), &rssReceiver);
				StatisticsToString(&rssReceiver, text,2);
				printf("%s\n\n", text);
			}
		}

		RakSleep(100);
	}
	stop=RakNet::GetTime();
	double seconds = (double)(stop-start)/1000.0;

	if (server)
	{
		RakNetStatistics *rssSender2=server->GetStatistics(server->GetSystemAddressFromIndex(0));
		StatisticsToString(rssSender2, text, 1);
		printf("%s", text);
	}

	printf("%i bytes per second (%.2f seconds). Press enter to quit\n", (int)((double)(BIG_PACKET_SIZE) / seconds ), seconds) ;
	gets(text);

	delete []text;
	RakNetworkFactory::DestroyRakPeerInterface(client);
	RakNetworkFactory::DestroyRakPeerInterface(server);

	return 0;
}
