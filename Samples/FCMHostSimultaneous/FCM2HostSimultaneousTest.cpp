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

#define NUM_PEERS 8
RakPeerInterface *rakPeer[NUM_PEERS];

int main()
{
	FullyConnectedMesh2 fcm2[NUM_PEERS];
	ConnectionGraph2 cg2[NUM_PEERS];

	for (int i=0; i < NUM_PEERS; i++)
	{
		rakPeer[i]=RakNetworkFactory::GetRakPeerInterface();
		rakPeer[i]->AttachPlugin(&fcm2[i]);
		rakPeer[i]->AttachPlugin(&cg2[i]);
		fcm2[i].SetAutoparticipateConnections(true);
		SocketDescriptor sd;
		sd.port=60000+i;
		rakPeer[i]->Startup(NUM_PEERS,0,&sd,1);
		rakPeer[i]->SetMaximumIncomingConnections(NUM_PEERS);
		rakPeer[i]->SetTimeoutTime(1000,UNASSIGNED_SYSTEM_ADDRESS);
		printf("Our guid is %s\n", rakPeer[i]->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	}

	for (int i=0; i < NUM_PEERS; i++)
	{
		for (int j=0; j < NUM_PEERS; j++)
		{
			if (i==j)
				continue;
			rakPeer[i]->Connect("127.0.0.1", 60000+j, 0, 0 );
		}
	}

	bool quit=false;
	Packet *packet;
	char ch;
	while (!quit)
	{
		for (int i=0; i < NUM_PEERS; i++)
		{
			for (packet = rakPeer[i]->Receive(); packet; rakPeer[i]->DeallocatePacket(packet), packet = rakPeer[i]->Receive())
			{
				switch (packet->data[0])
				{
				case ID_DISCONNECTION_NOTIFICATION:
					// Connection lost normally
					printf("%i. ID_DISCONNECTION_NOTIFICATION\n", i);
					break;


				case ID_NEW_INCOMING_CONNECTION:
					// Somebody connected.  We have their IP now
					printf("%i. ID_NEW_INCOMING_CONNECTION from %s. guid=%s.\n", i, packet->systemAddress.ToString(true), packet->guid.ToString());
					break;

				case ID_CONNECTION_REQUEST_ACCEPTED:
					// Somebody connected.  We have their IP now
					printf("%i. ID_CONNECTION_REQUEST_ACCEPTED from %s. guid=%s.\n", i, packet->systemAddress.ToString(true), packet->guid.ToString());
					break;


				case ID_CONNECTION_LOST:
					// Couldn't deliver a reliable packet - i.e. the other system was abnormally
					// terminated
					printf("%i. ID_CONNECTION_LOST\n", i);
					break;


				case ID_FCM2_NEW_HOST:
					if (packet->systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
						printf("%i. Got new host (ourselves)\n", i);
					else
						printf("%i. Got new host %s\n", i, packet->systemAddress.ToString(true));
					break;
				}
			}
		}

		if (kbhit())
		{
			ch=getch();
			if (ch==' ')
			{
				DataStructures::DefaultIndexType participantList;
				RakNetGUID hostGuid;
				bool weAreHost;
				for (int i=0; i < NUM_PEERS; i++)
				{
					if (rakPeer[i]->IsActive()==false)
						continue;

					fcm2[i].GetParticipantCount(&participantList);
					weAreHost=fcm2[i].IsHostSystem();
					hostGuid=fcm2[i].GetHostSystem();
					if (weAreHost)
						printf("%i. %iP myGuid=%s, hostGuid=%s (Y)\n",i, participantList, rakPeer[i]->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString(), hostGuid.ToString());
					else
						printf("%i. %iP myGuid=%s, hostGuid=%s (N)\n",i, participantList, rakPeer[i]->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString(), hostGuid.ToString());
				}
			}
			if (ch=='d' || ch=='D')
			{
				char str[32];
				printf("Enter system index to disconnect.\n");
				gets(str);
				rakPeer[atoi(str)]->Shutdown(100);
				printf("Done.\n");
			}
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
		}


		RakSleep(30);
	}

	for (int i=0; i < NUM_PEERS; i++)
	{
		RakNetworkFactory::DestroyRakPeerInterface(rakPeer[i]);
	}
	return 0;
}
