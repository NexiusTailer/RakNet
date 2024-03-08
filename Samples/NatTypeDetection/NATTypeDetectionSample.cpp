#include "RakPeerInterface.h"
#include "RakSleep.h"
#include <stdio.h>
#include <stdlib.h>
#include "RakNetworkFactory.h"
#include <string.h>
#include "Kbhit.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "NatTypeDetectionServer.h"
#include "NatTypeDetectionClient.h"
#include "SocketLayer.h"

static const unsigned short NAT_TYPE_DETECTION_SERVER_PORT=60481;

// For the client, so I don't have to type it if I just press enter
static const char *DEFAULT_NAT_TYPE_DETECTION_SERVER_IP1="94.198.81.195";

static const char *NAT_TYPE_DETECTION_SERVER_PASSWORD="";

void RunServer()
{
	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	RakNet::NatTypeDetectionServer natTypeDetectionServer;
	rakPeer->AttachPlugin(&natTypeDetectionServer);
	char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ];
	unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
	SocketLayer::Instance()->GetMyIP( ipList, binaryAddresses );

	for (int i=0; i<5; i++)
	{
		if (ipList[i][0]==0 && i < MAXIMUM_NUMBER_OF_INTERNAL_IDS)
		{
			printf("Not enough IP addresses to bind to.\n");
			return;
		}

		printf("%i. Using %s\n", i+1, ipList[i]);
	}


	SocketDescriptor sd(NAT_TYPE_DETECTION_SERVER_PORT, ipList[0]);
	rakPeer->Startup(128,0,&sd, 1);
	rakPeer->SetMaximumIncomingConnections(128);
	natTypeDetectionServer.Startup(ipList[1], ipList[2], ipList[3]);

	printf("Running. Press 'q' to quit\n");
	Packet *p;
	while (1)
	{
		if (kbhit())
		{
			char ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				return;
			}
		}

		for (p=rakPeer->Receive(); p; rakPeer->DeallocatePacket(p), p=rakPeer->Receive())
		{
			switch (p->data[0])
			{
			case ID_DISCONNECTION_NOTIFICATION:
				printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));;
				break;

			case ID_NEW_INCOMING_CONNECTION:
				printf("ID_NEW_INCOMING_CONNECTION from %s\n", p->systemAddress.ToString(true));
				break;

			case ID_CONNECTION_LOST:
				printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));;
				break;
			}
			
		}

		RakSleep(30);
	}
	
}

void RunClient()
{
	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	RakNet::NatTypeDetectionClient natTypeDetectionClient;
	rakPeer->AttachPlugin(&natTypeDetectionClient);
	SocketDescriptor sd(0,0);
	rakPeer->Startup(1,0,&sd,1);
	printf("Enter first server IP: ");
	char str[256];
	gets(str);
	if (str[0]==0)
		strcpy(str,DEFAULT_NAT_TYPE_DETECTION_SERVER_IP1);
	printf("Connecting to server.\n");
	rakPeer->Connect(str, NAT_TYPE_DETECTION_SERVER_PORT, NAT_TYPE_DETECTION_SERVER_PASSWORD, (int) strlen(NAT_TYPE_DETECTION_SERVER_PASSWORD));
	RakSleep(1000);
	Packet *p;
	SystemAddress serverAddress=UNASSIGNED_SYSTEM_ADDRESS;
	for (p=rakPeer->Receive(); p; rakPeer->DeallocatePacket(p), p=rakPeer->Receive())
	{
		if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			serverAddress=p->systemAddress;
	}
	if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
	{
		printf("Failed to connect to server\n");
		return;
	}
	else
		printf("Connected to server.\n");
	printf("(D)etect NAT type\n");
	printf("(Q)uit\n");
	while (1)
	{
		if (kbhit())
		{
			char ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				return;
			}
			else
			{
				printf("Detecting NAT type...\n");
				natTypeDetectionClient.DetectNATType(serverAddress);
			}
		}

		for (p=rakPeer->Receive(); p; rakPeer->DeallocatePacket(p), p=rakPeer->Receive())
		{
			if (p->data[0]==ID_NAT_TYPE_DETECTION_RESULT)
			{
				serverAddress=p->systemAddress;
				RakNet::NATTypeDetectionResult r = (RakNet::NATTypeDetectionResult) p->data[1];
				printf("NAT Type is %s (%s)\n", NATTypeDetectionResultToString(r), NATTypeDetectionResultToStringFriendly(r));
				printf("Using NATPunchthrough can connect to systems using:\n");
				for (int i=0; i < (int) RakNet::NAT_TYPE_COUNT; i++)
				{
					if (CanConnect(r,(RakNet::NATTypeDetectionResult)i))
					{
						if (i!=0)
							printf(", ");
						printf("%s", NATTypeDetectionResultToString((RakNet::NATTypeDetectionResult)i));
					}
				}
				printf("\n");
				if (r==RakNet::NAT_TYPE_PORT_RESTRICTED || r==RakNet::NAT_TYPE_SYMMETRIC)
				{
					// For UPNP, see Samples\UDPProxy
					printf("Note: Your router must support UPNP or have the user manually forward ports.\n");
					printf("Otherwise not all connections may complete.\n");
				}
				else
					printf("Test done.\n");
				printf("Press 'q' to quit.\n");
			}
			else if (p->data[0]==ID_CONNECTION_LOST)
			{
				printf("Connection lost\nPress 'q' to quit.");
			}
			else if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
			{
				printf("Connection lost\nPress 'q' to quit.");
			}
		}

		RakSleep(30);
	}
}

int main(void)
{
	printf("NAT type detection sample.\n");
	printf("Demonstrates how to use NatTypeDetectionClient/Server.\n");
	printf("Requires a non-NAT server with at least four IP addresses.\n");
	printf("A free server is provided by default at 94.198.81.195\n");

	printf("Run as (s)erver or (c)lient?");
	char ch = getch();
	printf("\n");
	if (ch=='s' || ch=='S')
	{
		RunServer();
	}
	else
	{
		RunClient();
	}
}
