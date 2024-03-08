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

int main(void)
{
	printf("Demonstration of the router2 plugin.\n");
	printf("The router2 plugin allows you to connect to a system, routing messages through a third system\n");
	printf("This is useful if you can connect to the second system, but not the third, due to NAT issues.\n");
	printf("Difficulty: Advanced\n\n");

	RakPeerInterface *source, *router, *endpoint;
	RakNet::Router2 sourceR2, routerR2, endpointR2;
	source = RakNetworkFactory::GetRakPeerInterface();
	router = RakNetworkFactory::GetRakPeerInterface();
	endpoint = RakNetworkFactory::GetRakPeerInterface();
	source->AttachPlugin(&sourceR2);
	router->AttachPlugin(&routerR2);
	endpoint->AttachPlugin(&endpointR2);
	routerR2.SetMaximumForwardingRequests(8);

	SocketDescriptor sdSource(1234,0);
	SocketDescriptor sdRouter(1235,0);
	SocketDescriptor sdEndpoint(1236,0);
	source->Startup(8,0,&sdSource,1);
	router->Startup(8,0,&sdRouter,1);
	router->SetMaximumIncomingConnections(8);
	endpoint->Startup(8,0,&sdEndpoint,1);
	endpoint->SetMaximumIncomingConnections(8);
	source->Connect("127.0.0.1",1235,0,0);
	endpoint->Connect("127.0.0.1",1235,0,0);
	RakSleep(100);
	RakNetGUID endpointGuid = endpoint->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	printf("endpointGuid=%s\n",endpointGuid.ToString());
	printf("routerGuid=%s\n",router->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	sourceR2.Connect(endpointGuid);
	Packet *packet;
	bool testPassed=false;

	RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 5000;
	while (RakNet::GetTimeMS()<stopWaiting)
	{
		for (packet=source->Receive(); packet; source->DeallocatePacket(packet), packet=source->Receive())
		{
			if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED && packet->guid==endpointGuid)
				testPassed=true;
		}

		for (packet=router->Receive(); packet; router->DeallocatePacket(packet), packet=router->Receive())
			;

		for (packet=endpoint->Receive(); packet; endpoint->DeallocatePacket(packet), packet=endpoint->Receive())
			;
		RakSleep(0);
	}

	if (testPassed)
		printf("Test passed\n");
	else
		printf("Test failed\n");

	RakSleep(1000);
	RakNetworkFactory::DestroyRakPeerInterface(source);
	RakNetworkFactory::DestroyRakPeerInterface(router);
	RakNetworkFactory::DestroyRakPeerInterface(endpoint);

	return 1;
}
