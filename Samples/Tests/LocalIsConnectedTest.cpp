#include "LocalIsConnectedTest.h"

/*
Description:
Tests

IsLocalIP
SendLoopback
IsConnected
GetLocalIP
GetInternalID

Success conditions:
All tests pass

Failure conditions:
Any test fails

RakPeerInterface Functions used, tested indirectly by its use:
Startup
SetMaximumIncomingConnections
Receive
DeallocatePacket
Send

RakPeerInterface Functions Explicitly Tested:
IsLocalIP
SendLoopback
IsConnected
GetLocalIP
GetInternalID
*/
int LocalIsConnectedTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{

	RakPeerInterface *server,*client;
	destroyList.Clear(false,__FILE__,__LINE__);

	server=RakNetworkFactory::GetRakPeerInterface();
	destroyList.Push(server,__FILE__,__LINE__);
	client=RakNetworkFactory::GetRakPeerInterface();
	destroyList.Push(client,__FILE__,__LINE__);

	client->Startup(1,30,&SocketDescriptor(),1);
	server->Startup(1,30,&SocketDescriptor(60000,0),1);
	server->SetMaximumIncomingConnections(1);

	SystemAddress serverAddress;

	serverAddress.SetBinaryAddress("127.0.0.1");
	serverAddress.port=60000;
	RakNetTime entryTime=RakNet::GetTime();
	bool lastConnect=false;
	if (isVerbose)
		printf("Testing IsConnected\n");

	while(!client->IsConnected (serverAddress,false,false)&&RakNet::GetTime()-entryTime<5000)
	{

		if(!client->IsConnected (serverAddress,true,true))
		{
			lastConnect=client->Connect("127.0.0.1",serverAddress.port,0,0);
		}

		RakSleep(100);

	}

	if (!lastConnect)//Use thise method to only check if the connect function fails, detecting connected client is done next
	{
		if (isVerbose)
			DebugTools::ShowError("Client could not connect after 5 seconds\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 1;
	}

	if(!client->IsConnected (serverAddress,false,false))
	{
		if (isVerbose)
			DebugTools::ShowError("IsConnected did not detect connected client",!noPauses && isVerbose,__LINE__,__FILE__);
		return 2;
	}
	client->CloseConnection (serverAddress,true,0,LOW_PRIORITY); 

	if(!client->IsConnected (serverAddress,false,true))
	{
		DebugTools::ShowError("IsConnected did not detect disconnecting client",!noPauses && isVerbose,__LINE__,__FILE__);
		return 3;
	}

	RakSleep(1000);
	client->Connect("127.0.0.1",serverAddress.port,0,0);

	if(!client->IsConnected (serverAddress,true,false))
	{
		DebugTools::ShowError("IsConnected did not detect connecting client",!noPauses && isVerbose,__LINE__,__FILE__);

		return 4;

	}

	entryTime=RakNet::GetTime();

	while(!client->IsConnected (serverAddress,false,false)&&RakNet::GetTime()-entryTime<5000)
	{

		if(!client->IsConnected (serverAddress,true,true))
		{
			client->Connect("127.0.0.1",serverAddress.port,0,0);
		}

		RakSleep(100);

	}

	if (!client->IsConnected (serverAddress,false,false))
	{
		if (isVerbose)
			DebugTools::ShowError("Client could not connect after 5 seconds\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 1;
	}

	if (isVerbose)
		printf("Testing IsLocalIP\n");

	if (!client->IsLocalIP("127.0.0.1"))
	{
		if (isVerbose)
			DebugTools::ShowError("IsLocalIP failed test\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 5;
	}

	if (isVerbose)
		printf("Testing SendLoopback\n");
	char str[]="AAAAAAAAAA";
	str[0]=(char)(ID_USER_PACKET_ENUM+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);
	client->SendLoopback(str, (int) strlen(str)+1);

	bool recievedPacket=false;
	Packet *packet;

	RakNetTime	stopWaiting = RakNet::GetTimeMS() + 1000;
	while (RakNet::GetTimeMS()<stopWaiting)
	{

		for (packet=client->Receive(); packet; client->DeallocatePacket(packet), packet=client->Receive())
		{

			if (packet->data[0]==ID_USER_PACKET_ENUM+1)
			{

				recievedPacket=true;

			}
		}

	}

	if (!recievedPacket)
	{
		if (isVerbose)
			DebugTools::ShowError("SendLoopback failed test\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 6;
	}

	if (isVerbose)
		printf("Testing GetLocalIP\n");
	const char * localIp=client->GetLocalIP(0);

	if (!client->IsLocalIP(localIp))
	{
		if (isVerbose)
			DebugTools::ShowError("GetLocalIP failed test\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 7;
	}

	if (isVerbose)
		printf("Testing GetInternalID\n");

	SystemAddress localAddress=client->GetInternalID();

	char convertedIp[39];

	sprintf(convertedIp,"%d.%d.%d.%d",  ((localAddress.binaryAddress >> (24 - 8 * 3)) & 0xFF),((localAddress.binaryAddress >> (24 - 16)) & 0xFF),((localAddress.binaryAddress >> (24 - 8 )) & 0xFF),((localAddress.binaryAddress >> (24)) & 0xFF));

	printf("GetInternalID returned %s\n",convertedIp);

	if (!client->IsLocalIP(convertedIp))
	{
		if (isVerbose)
			DebugTools::ShowError("GetInternalID failed test\n",!noPauses && isVerbose,__LINE__,__FILE__);
		return 8;
	}

	return 0;

}

RakNet::RakString LocalIsConnectedTest::GetTestName()
{

	return "LocalIsConnectedTest";

}

RakNet::RakString LocalIsConnectedTest::ErrorCodeToString(int errorCode)
{

	switch (errorCode)
	{

	case 0:
		return "No error";
		break;

	case 1:
		return "Client could not connect after 5 seconds";
		break;
	case 2:
		return "IsConnected did not detect connected client";
		break;
	case 3:
		return "IsConnected did not detect disconnecting client";
		break;
	case 4:
		return "IsConnected did not detect connecting client";
		break;

	case 5:
		return "IsLocalIP failed test";
		break;

	case 6:
		return "Sendloopback failed test";
		break;

	case 7:
		return "GetLocalIP failed test";
		break;

	case 8:
		return "GetInternalID failed test";
		break;

	default:
		return "Undefined Error";
	}

}

LocalIsConnectedTest::LocalIsConnectedTest(void)
{
}

LocalIsConnectedTest::~LocalIsConnectedTest(void)
{
}

void LocalIsConnectedTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}