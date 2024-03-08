#include "RPC4Plugin.h"
#include "RakPeerInterface.h"

#include <stdio.h>
#include "Kbhit.h"
#include <string.h>
#include <stdlib.h>
#include "RakSleep.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"

using namespace RakNet;

void CFunc( RakNet::BitStream *bitStream, Packet *packet )
{
	if (packet->systemAddress==RakNet::UNASSIGNED_SYSTEM_ADDRESS)
		printf("Localhost call: ");
	else
		printf("Remote call: \n");
	RakNet::RakString data;
	int offset=bitStream->GetReadOffset();
	bool read = bitStream->ReadCompressed(data);
	RakAssert(read);
	printf("%s\n", data.C_String());
}
int main(void)
{
	printf("Demonstration of the RPC4 plugin.\n");
	printf("Difficulty: Beginner\n\n");

	RakPeerInterface *rakPeer1, *rakPeer2;
	rakPeer1=RakNet::RakPeerInterface::GetInstance();
	rakPeer2=RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd1(1234,0);
	RakNet::SocketDescriptor sd2(1235,0);
	rakPeer1->Startup(8,&sd1,1);
	rakPeer2->Startup(8,&sd2,1);
	rakPeer1->SetMaximumIncomingConnections(8);
	rakPeer2->Connect("127.0.0.1", sd1.port, 0, 0);
	RakSleep(100);
	RPC4 rpc1, rpc2;
	rakPeer1->AttachPlugin(&rpc1);
	rakPeer2->AttachPlugin(&rpc2);
	rpc1.RegisterFunction("CFunc", CFunc);
	rpc1.CallLoopback("blah1", 0);
	RakNet::BitStream testBs;
	testBs.WriteCompressed("testData");
	rpc2.CallLoopback("blah2", &testBs);
	rpc2.Call("CFunc", &testBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakPeer2->GetSystemAddressFromIndex(0),false);
	RakNet::Packet *packet;
	packet = rakPeer1->Receive();
	RakAssert(packet->data[0]==ID_NEW_INCOMING_CONNECTION);
	rakPeer1->DeallocatePacket(packet);
	packet = rakPeer1->Receive();
	RakAssert(packet->data[0]==ID_RPC_REMOTE_ERROR);
	RakAssert(packet->data[1]==RPC_ERROR_FUNCTION_NOT_REGISTERED);
	RakAssert(strcmp((const char*) packet->data+2,"blah1")==0);
	rakPeer1->DeallocatePacket(packet);
	packet = rakPeer2->Receive();
	RakAssert(packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED);
	rakPeer2->DeallocatePacket(packet);
	packet = rakPeer2->Receive();
	RakAssert(packet->data[0]==ID_RPC_REMOTE_ERROR);
	RakAssert(packet->data[1]==RPC_ERROR_FUNCTION_NOT_REGISTERED);
	RakAssert(strcmp((const char*) packet->data+2,"blah2")==0);
	RakSleep(100);
	for (packet=rakPeer1->Receive(); packet; rakPeer1->DeallocatePacket(packet), rakPeer1->Receive())
		;
	for (packet=rakPeer2->Receive(); packet; rakPeer2->DeallocatePacket(packet), rakPeer2->Receive())
		;

	rakPeer1->Shutdown(100,0);
	rakPeer2->Shutdown(100,0);
	RakNet::RakPeerInterface::DestroyInstance(rakPeer1);
	RakNet::RakPeerInterface::DestroyInstance(rakPeer2);

	return 1;
}
