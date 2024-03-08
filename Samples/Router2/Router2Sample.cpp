#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include <stdio.h>
#include "Kbhit.h"
#include <string.h>
#include <stdlib.h>
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "Router2.h"
#include "RakSleep.h"
#include "GetTime.h"
#include "Rand.h"
#include "RakAssert.h"
#include "SocketLayer.h"
#include "Getche.h"

using namespace RakNet;

// Global just to make the sample easier to write
RakNetGUID endpointGuid;
RakPeerInterface *endpoint=0, *router=0, *sender=0;
RakPeerInterface *rakPeer;
Router2 *router2Plugin;

void ReadAllPackets(void)
{
	char str[64], str2[64];
	Packet *packet;
	for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
	{
		packet->guid.ToString(str);
		packet->systemAddress.ToString(true,str2);
		if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
		{
			printf("ID_NEW_INCOMING_CONNECTION from %s on %s\n", str, str2);

			// If this is the router, then send the guid of the endpoint to the sender
			if (router)
			{
				endpointGuid=rakPeer->GetGUIDFromIndex(0);
				RakAssert(endpointGuid!=UNASSIGNED_RAKNET_GUID);
				RakNet::BitStream bsOut;
				bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
				bsOut.Write(endpointGuid);
				rakPeer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
			}
		}
		else if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
		{
			if (sender!=0 && packet->guid==endpointGuid)
			{
				printf("Sender got ID_CONNECTION_REQUEST_ACCEPTED to endpoint on %s.\n", str2);
			}
			else
			{
				printf("ID_CONNECTION_REQUEST_ACCEPTED from %s on %s\n", str, str2);
			}
		}
		else if (packet->data[0]==ID_USER_PACKET_ENUM)
		{
			// Only sender should get this message
			RakAssert(sender!=0);

			// This should be returned once for each router, so just process the first one
			if (endpointGuid==UNASSIGNED_RAKNET_GUID)
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(MessageID));
				bsIn.Read(endpointGuid);
				endpointGuid.ToString(str);
				printf("Sender got endpoint guid of %s. Attempting routing.\n", str);
				router2Plugin->EstablishRouting(endpointGuid);
			}
		}
		else if (packet->data[0]==ID_ROUTER_2_FORWARDING_NO_PATH)
		{
			// Only sender should get this message
			RakAssert(sender!=0);

			printf("No path to endpoint exists. Routing failed.\n");
		}
		else if (packet->data[0]==ID_CONNECTION_LOST)
		{
			if (sender!=0 && packet->guid==endpointGuid)
			{
				printf("Sender got ID_CONNECTION_LOST to endpoint.\n");
			}
			else
			{
				printf("ID_CONNECTION_LOST from %s\n", str);
			}
		}
		else if (packet->data[0]==ID_ROUTER_2_FORWARDING_ESTABLISHED)
		{
			// Only sender should get this message
			RakAssert(sender!=0);

			printf("Routing through %s successful. Connecting to endpoint.\n", str);

			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID endpointGuid;
			bs.Read(endpointGuid);
			unsigned short sourceToDestPort;
			bs.Read(sourceToDestPort);
			char ipAddressString[32];
			packet->systemAddress.ToString(false, ipAddressString);
			rakPeer->Connect(ipAddressString, sourceToDestPort, 0,0);
		}
		else if (packet->data[0]==ID_ROUTER_2_REROUTED)
		{
			// Only sender should get this message
			RakAssert(sender!=0);

			// You could read the endpointGuid and sourceToDestPoint if you wanted to
			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID endpointGuid2;
			bs.Read(endpointGuid2);
			RakAssert(endpointGuid2==endpointGuid);
			unsigned short sourceToDestPort;
			bs.Read(sourceToDestPort);
			SystemAddress intermediateAddress=packet->systemAddress;
			intermediateAddress.port=sourceToDestPort;

			printf("Sender connection to endpoint rerouted through %s\n", str);
			intermediateAddress.ToString(true, str);
			printf("Sending reroute message to endpoint on %s\n", str);

			// Test sending a message to the endpoint
			RakNet::BitStream bsOut;
			MessageID id = ID_USER_PACKET_ENUM+1;
			bsOut.Write(id);
			rakPeer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,endpointGuid,false);
		}
		else if (packet->data[0]==ID_USER_PACKET_ENUM+1)
		{
			RakAssert(endpoint!=0);
			printf("Endpoint got test message after rerouting.\n");
		}
	}
}

int main(void)
{
	printf("Demonstration of the router2 plugin.\n");
	printf("The router2 plugin allows you to connect to a system, routing messages through\n");
	printf("a third system. This is useful if you can connect to the second system, but not\n");
	printf("the third, due to NAT issues.\n");
	printf("1. Start the endpoint\n");
	printf("2. Start 1 or more routers, and connect them to the endpoint\n");
	printf("3. Start the sender\n");
	printf("4. After the sender connects to the endpoint, close the router\n");
	printf("Difficulty: Advanced\n\n");

	printf("Run as:\n(E)ndpoint\n(R)outer\n(S)ender\n");
	endpointGuid=UNASSIGNED_RAKNET_GUID;
	const unsigned short ENDPOINT_PORT=1234;
	const unsigned short ROUTER_PORT=1235;
	char ch = getche();
	char str[64];
	printf("\n");
	if (ch=='e' || ch=='E')
	{
		endpoint=RakNetworkFactory::GetRakPeerInterface();
		rakPeer=endpoint;
		endpoint->SetMaximumIncomingConnections(32);
		SocketDescriptor sd(ENDPOINT_PORT,0);
		endpoint->Startup(32,0,&sd,1);
	}
	else if (ch=='r' || ch=='R')
	{
		router=RakNetworkFactory::GetRakPeerInterface();
		rakPeer=router;
		router->SetMaximumIncomingConnections(32);
		SocketDescriptor sd;
		sd.port=ROUTER_PORT;
		while (SocketLayer::IsPortInUse(sd.port)==true)
			sd.port++;
		router->Startup(32,0,&sd,1);

		printf("Enter endpoint IP address: ");
		gets(str);
		if (str[0]==0)
			strcpy(str, "127.0.0.1");

		// Connect to the endpoint
		router->Connect(str,ENDPOINT_PORT,0,0);

		// To make things easy, just wait for the connection
		RakSleep(1000);

		// Check who we connected to
		SystemAddress remoteSystems[1];
		unsigned short numberOfSystems=1;
		router->GetConnectionList((SystemAddress*) remoteSystems,&numberOfSystems);
		if (numberOfSystems==0)
		{
			printf("Didn't connect to the endpoint. Quitting\n");
			return 1;
		}
	}
	else if (ch=='s' || ch=='S')
	{
		sender=RakNetworkFactory::GetRakPeerInterface();
		rakPeer=sender;
		SocketDescriptor sd(0,0);
		sender->Startup(32,0,&sd,1);

		printf("Enter router IP address: ");
		gets(str);
		if (str[0]==0)
			strcpy(str, "127.0.0.1");

		// Try connecting to up to 8 routers
		for (unsigned short i=0; i < 8; i++)
		{
			sender->Connect(str,ROUTER_PORT+i,0,0);
		}

		// To make things easy, just wait for all connections
		RakSleep(1000);

		// Check who we connected to
		SystemAddress remoteSystems[8];
		unsigned short numberOfSystems=8;
		sender->GetConnectionList((SystemAddress*) remoteSystems,&numberOfSystems);
		if (numberOfSystems==0)
		{
			printf("Didn't connect to any routers. Quitting\n");
			return 1;
		}
	}
	else
	{
		printf("Unknown mode. Quitting\n");
		return 1;
	}

	rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString(str);
	printf("My GUID is %s\n", str);

	rakPeer->SetTimeoutTime(3000,UNASSIGNED_SYSTEM_ADDRESS);
	router2Plugin = new Router2;
	rakPeer->AttachPlugin(router2Plugin);
	router2Plugin->SetMaximumForwardingRequests(1);

	// Print connection update messages
	ReadAllPackets();

	// The sender needs to read out the endpointGuid
	RakSleep(500);
	ReadAllPackets();

	printf("Sample running. Press 'q' to quit\n");
	while (1)
	{
		if (kbhit())
		{
			if (getch()=='q')
				break;
		}

		RakSleep(30);
		ReadAllPackets();
	}

	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
	delete router2Plugin;
}
