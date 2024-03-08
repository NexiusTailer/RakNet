#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"
#include "RakNetTypes.h"
#include "FCMHost.h"
#include "RakSleep.h"
#include <assert.h>

FCMHost fcmHostPlugin1, fcmHostPlugin2, fcmHostPlugin3;
RakPeerInterface *peer1, *peer2, *peer3;

void PrintHost(RakNetGUID g)
{
	if (g==peer1->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		printf("Peer1");
	else if (g==peer2->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		printf("Peer2");
	else
		printf("Peer3");
}
void PrintHosts(void)
{
	printf("Peer1Host=");
	PrintHost(fcmHostPlugin1.GetHost());
	printf("\n");

	printf("Peer2Host=");
	PrintHost(fcmHostPlugin2.GetHost());
	printf("\n");

	printf("Peer3Host=");
	PrintHost(fcmHostPlugin3.GetHost());
	printf("\n");
}

void WaitAndReceive()
{
	Packet *p;
	RakNetTime time;
	time = RakNet::GetTime();
	while (RakNet::GetTime() < time+500)
	{
		while (p=peer3->Receive())
		{
			if (p->data[0]==ID_FCM_HOST_CHANGED)
			{
				printf("Peer 3 notified us of host change to ");
				PrintHost(fcmHostPlugin3.GetHost());
				printf("\n");
			}
			peer3->DeallocatePacket(p);
		}
		while (p=peer2->Receive())
		{
			if (p->data[0]==ID_FCM_HOST_CHANGED)
			{
				printf("Peer 2 notified us of host change to ");
				PrintHost(fcmHostPlugin2.GetHost());
				printf("\n");
			}
			peer2->DeallocatePacket(p);
		}
		while (p=peer1->Receive())
		{
			if (p->data[0]==ID_FCM_HOST_CHANGED)
			{
				printf("Peer 1 notified us of host change to ");
				PrintHost(fcmHostPlugin1.GetHost());
				printf("\n");
			}
			peer1->DeallocatePacket(p);
		}
		RakSleep(0);
	}
}

void main(void)
{
	peer1=RakNetworkFactory::GetRakPeerInterface();
	printf("Peer1 started with GUID %s\n", peer1->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	RakSleep(30);
	peer2=RakNetworkFactory::GetRakPeerInterface();
	printf("Peer2 started with GUID %s\n", peer2->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	RakSleep(30);
	peer3=RakNetworkFactory::GetRakPeerInterface();
	printf("Peer3 started with GUID %s\n", peer3->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	RakSleep(30);
	peer1->AttachPlugin(&fcmHostPlugin1);
	peer2->AttachPlugin(&fcmHostPlugin2);
	peer3->AttachPlugin(&fcmHostPlugin3);
	fcmHostPlugin1.SetAutoAddNewConnections(true,0);
	fcmHostPlugin2.SetAutoAddNewConnections(true,0);
	fcmHostPlugin3.SetAutoAddNewConnections(true,0);
	peer1->Startup(3,0,&SocketDescriptor(60000,0),1);
	peer2->Startup(3,0,&SocketDescriptor(60001,0),1);
	peer3->Startup(3,0,&SocketDescriptor(60002,0),1);
	peer1->SetMaximumIncomingConnections(3);	
	peer2->SetMaximumIncomingConnections(3);
	peer3->SetMaximumIncomingConnections(3);

	printf("Starting condition:\n");
	PrintHosts();
	
	printf("\n3 connected to 2:\n");
	peer3->Connect("127.0.0.1", 60001, 0,0,0);
	WaitAndReceive();
	PrintHosts();

	printf("\n2 connected to 1:\n");
	peer2->Connect("127.0.0.1", 60000, 0,0,0);
	WaitAndReceive();
	PrintHosts();

	printf("\n3 connected to 1:\n");
	peer3->Connect("127.0.0.1", 60000, 0,0,0);
	WaitAndReceive();
	PrintHosts();

	printf("\n2 disconnected:\n");
	peer2->Shutdown(100,0);
	WaitAndReceive();
	PrintHosts();

	printf("\n3 disconnected:\n");
	peer3->Shutdown(100,0);
	WaitAndReceive();
	PrintHosts();

	RakNetworkFactory::DestroyRakPeerInterface(peer1);
	RakNetworkFactory::DestroyRakPeerInterface(peer2);
	RakNetworkFactory::DestroyRakPeerInterface(peer3);
}
