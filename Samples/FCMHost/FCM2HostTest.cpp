#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"
#include "RakNetTypes.h"
#include "RakSleep.h"
#include "FullyConnectedMesh2.h"
#include "ConnectionGraph2.h"
#include <assert.h>
#include "SocketLayer.h"
#include "Kbhit.h"
#include "PacketLogger.h"

RakPeerInterface *rakPeer;

int main()
{
	FullyConnectedMesh2 fcm2;
	ConnectionGraph2 cg2;
	rakPeer=RakNetworkFactory::GetRakPeerInterface();
	rakPeer->AttachPlugin(&fcm2);
	rakPeer->AttachPlugin(&cg2);
	fcm2.SetAutoparticipateConnections(true);
	SocketDescriptor sd;
	sd.port=60000;
	while (SocketLayer::IsPortInUse(sd.port)==true)
		sd.port++;
	rakPeer->Startup(8,0,&sd,1);
	rakPeer->SetMaximumIncomingConnections(8);
	rakPeer->SetTimeoutTime(1000,UNASSIGNED_SYSTEM_ADDRESS);
	printf("Our guid is %s\n", rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());

//	PacketLogger packetLogger;
//	rakPeer->AttachPlugin(&packetLogger);
//	packetLogger.SetLogDirectMessages(false);

	bool quit=false;
	Packet *packet;
	char ch;
	while (!quit)
	{
		for (packet = rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet = rakPeer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				break;


			case ID_NEW_INCOMING_CONNECTION:
				// Somebody connected.  We have their IP now
				printf("ID_NEW_INCOMING_CONNECTION from %s. guid=%s.\n", packet->systemAddress.ToString(true), packet->guid.ToString());
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				// Somebody connected.  We have their IP now
				printf("ID_CONNECTION_REQUEST_ACCEPTED from %s. guid=%s.\n", packet->systemAddress.ToString(true), packet->guid.ToString());
				break;


			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				printf("ID_CONNECTION_LOST\n");
				break;

			case ID_ADVERTISE_SYSTEM:
				rakPeer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.port,0,0);
				break;

			case ID_FCM2_NEW_HOST:
				if (packet->systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
					printf("Got new host (ourselves)\n");
				else
					printf("Got new host %s\n", packet->systemAddress.ToString(true));
				break;
			}
		}

		if (kbhit())
		{
			ch=getch();
			if (ch==' ')
			{
				DataStructures::DefaultIndexType participantList;
				fcm2.GetParticipantCount(&participantList);
				printf("participantList=%i\n",participantList);
			}
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
		}

		RakSleep(30);
		for (int i=0; i < 32; i++)
		{
			if (rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS,0).port!=60000+i)
				rakPeer->AdvertiseSystem("255.255.255.255", 60000+i, 0,0,0);
		}
	}

	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
	return 0;
}
