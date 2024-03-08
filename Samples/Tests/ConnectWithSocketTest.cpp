#include "ConnectWithSocketTest.h"

/*
Description:
virtual bool RakPeerInterface::ConnectWithSocket  	(  	const char *   	 host, 		unsigned short  	remotePort, 		const char *  	passwordData, 		int  	passwordDataLength, 		RakNetSmartPtr< RakNetSocket >  	socket, 		unsigned  	sendConnectionAttemptCount = 7, 		unsigned  	timeBetweenSendConnectionAttemptsMS = 500, 		RakNetTime  	timeoutTime = 0	  	) 	

virtual void RakPeerInterface::GetSockets  	(  	DataStructures::List< RakNetSmartPtr< RakNetSocket > > &   	 sockets  	 )   	 
virtual RakNetSmartPtr<RakNetSocket> RakPeerInterface::GetSocket  	(  	const SystemAddress   	 target  	 )   	 [pure virtual]

Success conditions:

Failure conditions:

RakPeerInterface Functions used, tested indirectly by its use:
Startup
SetMaximumIncomingConnections
Receive
DeallocatePacket
Send
IsConnected

RakPeerInterface Functions Explicitly Tested:
ConnectWithSocket
GetSockets
GetSocket

*/
int ConnectWithSocketTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{
	destroyList.Clear(false,__FILE__,__LINE__);

	RakPeerInterface *server,*client;

	DataStructures::List< RakNetSmartPtr< RakNetSocket > > sockets;
	TestHelpers::StandardClientPrep(client,destroyList);
	TestHelpers::StandardServerPrep(server,destroyList);

	SystemAddress serverAddress;

	serverAddress.SetBinaryAddress("127.0.0.1");
	serverAddress.port=60000;

	printf("Testing normal connect before test\n");
	if (!TestHelpers::WaitAndConnectTwoPeersLocally(client,server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[1-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 1;
	}

	TestHelpers::BroadCastTestPacket(client);

	if (!TestHelpers::WaitForTestPacket(server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[2-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 2;
	}

	printf("Disconnecting client\n");
	CommonFunctions::DisconnectAndWait(client,"127.0.0.1",60000);

	RakNetSmartPtr<RakNetSocket> theSocket;

	client->GetSockets(sockets);

	theSocket=sockets[0];

	RakTimer timer2(5000);

	printf("Testing ConnectWithSocket using socket from GetSockets\n");
	while(!client->IsConnected (serverAddress,false,false)&&!timer2.IsExpired())
	{

		if(!client->IsConnected (serverAddress,true,true))
		{
			client->ConnectWithSocket("127.0.0.1",serverAddress.port,0,0,theSocket);
		}

		RakSleep(100);

	}

	if (!client->IsConnected (serverAddress,false,false))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[3-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 3;
	}

	TestHelpers::BroadCastTestPacket(client);

	if (!TestHelpers::WaitForTestPacket(server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[4-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 4;

	}

	printf("Disconnecting client\n");
	CommonFunctions::DisconnectAndWait(client,"127.0.0.1",60000);

	printf("Testing ConnectWithSocket using socket from GetSocket\n");
	theSocket=client->GetSocket(UNASSIGNED_SYSTEM_ADDRESS);//Get open Socket

	timer2.Start();

	while(!client->IsConnected (serverAddress,false,false)&&!timer2.IsExpired())
	{

		if(!client->IsConnected (serverAddress,true,true))
		{
			client->ConnectWithSocket("127.0.0.1",serverAddress.port,0,0,theSocket);
		}

		RakSleep(100);

	}

	if (!client->IsConnected (serverAddress,false,false))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[5-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 5;
	}

	TestHelpers::BroadCastTestPacket(client);

	if (!TestHelpers::WaitForTestPacket(server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[6-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 6;

	}

	return 0;

}

RakNet::RakString ConnectWithSocketTest::GetTestName()
{

	return "ConnectWithSocketTest";

}

RakNet::RakString ConnectWithSocketTest::ErrorCodeToString(int errorCode)
{

	if (errorCode>0&&(unsigned int)errorCode<=errorList.Size())
	{
		return errorList[errorCode-1];
	}
	else
	{
		return "Undefined Error";
	}	

}

ConnectWithSocketTest::ConnectWithSocketTest(void)
{
	errorList.Push("Client did not connect after 5 seconds",__FILE__,__LINE__);
	errorList.Push("Control test send didn't work",__FILE__,__LINE__);
	errorList.Push("Client did not connect after 5 secods Using ConnectWithSocket, could be GetSockets or ConnectWithSocket problem",__FILE__,__LINE__);
	errorList.Push("Server did not recieve test packet from client",__FILE__,__LINE__);
	errorList.Push("Client did not connect after 5 secods Using ConnectWithSocket, could be GetSocket or ConnectWithSocket problem",__FILE__,__LINE__);
	errorList.Push("Server did not recieve test packet from client",__FILE__,__LINE__);

}

ConnectWithSocketTest::~ConnectWithSocketTest(void)
{
}

void ConnectWithSocketTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}