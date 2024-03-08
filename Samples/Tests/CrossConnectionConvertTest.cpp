#include "CrossConnectionConvertTest.h"

/*
Description: Tests what happens if two instances of RakNet connect to each other at the same time. This has caused handshaking problems in the past.

Success conditions:
Everything connects and sends normally.

Failure conditions:
Expected values from ping/pong do not occur within expected time.
*/
int CrossConnectionConvertTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{

	static const unsigned short SERVER_PORT=1234;
	//	char serverMode[32];
	char serverIP[64];

	strcpy(serverIP,"127.0.0.1");	

	char clientIP[64];
	RakPeerInterface *server,*client;
	unsigned short clientPort;
	bool gotNotification;
	server=RakNetworkFactory::GetRakPeerInterface();
	destroyList.Clear(false,__FILE__,__LINE__);
	destroyList.Push(server,__FILE__,__LINE__);
	client=RakNetworkFactory::GetRakPeerInterface();
	destroyList.Push(client,__FILE__,__LINE__);

	

	server->Startup(1,0,&SocketDescriptor(SERVER_PORT,0), 1);
	server->SetMaximumIncomingConnections(1);

	client->Startup(1,0,&SocketDescriptor(0,0), 1);

	client->Ping(serverIP,SERVER_PORT,false);

	//	PacketLogger pl;
	//	pl.LogHeader();
	//	rakPeer->AttachPlugin(&pl);

	RakNetTime connectionAttemptTime=0,connectionResultDeterminationTime=0,nextTestStartTime=0;

	RakNetTime entryTime=RakNet::GetTime();//Loop entry time

	bool printedYet=false;
	while(RakNet::GetTime()-entryTime<10000)//Run for 10 Secoonds
	{

		Packet *p;

		printedYet=false;

		for (p=server->Receive(); p; server->DeallocatePacket(p), p=server->Receive())
		{

			if (isVerbose&&!printedYet)
			{
				printf("Server:\n");
				printedYet=true;
			}
			if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
			{

				if (isVerbose)
					printf("ID_NEW_INCOMING_CONNECTION\n");
				gotNotification=true;
			}
			else if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{

				if (isVerbose)
					printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
				gotNotification=true;
			}
			else if (p->data[0]==ID_PING)
			{

				if (isVerbose)
					printf("ID_PING\n");
				connectionAttemptTime=RakNet::GetTime()+1000;
				p->systemAddress.ToString(false,clientIP);
				clientPort=p->systemAddress.port;
				gotNotification=false;
			}
			else if (p->data[0]==ID_PONG)
			{

				if (isVerbose)
					printf("ID_PONG\n");
				RakNetTime sendPingTime;
				RakNet::BitStream bs(p->data,p->length,false);
				bs.IgnoreBytes(1);
				bs.Read(sendPingTime);
				RakNetTime rtt = RakNet::GetTime() - sendPingTime;
				if (rtt/2<=500)
					connectionAttemptTime=RakNet::GetTime()+1000-rtt/2;
				else
					connectionAttemptTime=RakNet::GetTime();
				gotNotification=false;
			}
		}

		printedYet=false;
		for (p=client->Receive(); p; client->DeallocatePacket(p), p=client->Receive())
		{

			if (isVerbose&&!printedYet)
			{
				printf("Client:\n");
				printedYet=true;
			}
			if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
			{

				if (isVerbose)
					printf("ID_NEW_INCOMING_CONNECTION\n");
				gotNotification=true;
			}
			else if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{

				if (isVerbose)
					printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
				gotNotification=true;
			}
			else if (p->data[0]==ID_PING)
			{

				if (isVerbose)
					printf("ID_PING\n");
				connectionAttemptTime=RakNet::GetTime()+1000;
				p->systemAddress.ToString(false,clientIP);
				clientPort=p->systemAddress.port;
				gotNotification=false;
			}
			else if (p->data[0]==ID_PONG)
			{

				if (isVerbose)
					printf("ID_PONG\n");
				RakNetTime sendPingTime;
				RakNet::BitStream bs(p->data,p->length,false);
				bs.IgnoreBytes(1);
				bs.Read(sendPingTime);
				RakNetTime rtt = RakNet::GetTime() - sendPingTime;
				if (rtt/2<=500)
					connectionAttemptTime=RakNet::GetTime()+1000-rtt/2;
				else
					connectionAttemptTime=RakNet::GetTime();
				gotNotification=false;
			}
		}

		if (connectionAttemptTime!=0 && RakNet::GetTime()>=connectionAttemptTime)
		{

			if (isVerbose)
				printf("Attemping connection\n");
			connectionAttemptTime=0;

			server->Connect(clientIP,clientPort,0,0);
			client->Connect(serverIP,SERVER_PORT,0,0);

			connectionResultDeterminationTime=RakNet::GetTime()+2000;
		}
		if (connectionResultDeterminationTime!=0 && RakNet::GetTime()>=connectionResultDeterminationTime)
		{
			connectionResultDeterminationTime=0;
			if (gotNotification==false)
			{
				DebugTools::ShowError("Did not recieve expected response. \n",!noPauses && isVerbose,__LINE__,__FILE__);
				return 1;
			}

			SystemAddress sa;
			sa.SetBinaryAddress(serverIP);
			sa.port=SERVER_PORT;
			client->CancelConnectionAttempt(sa);

			sa.SetBinaryAddress(clientIP);
			sa.port=clientPort;
			server->CancelConnectionAttempt(sa);

			server->CloseConnection(server->GetSystemAddressFromIndex(0),true,0);
			client->CloseConnection(client->GetSystemAddressFromIndex(0),true,0);

			//if (isServer==false)
			nextTestStartTime=RakNet::GetTime()+1000;

		}
		if (nextTestStartTime!=0 && RakNet::GetTime()>=nextTestStartTime)
		{
			client->Ping(serverIP,SERVER_PORT,false);
			nextTestStartTime=0;
		}
		RakSleep(0);

	}
	if (isVerbose)
		printf("Test succeeded.\n");

	return 0;

}

RakNet::RakString CrossConnectionConvertTest::GetTestName()
{

	return "CrossConnectionConvertTest";

}

RakNet::RakString CrossConnectionConvertTest::ErrorCodeToString(int errorCode)
{

	switch (errorCode)
	{

	case 0:
		return "No error";
		break;

	case 1:
		return "Did not recieve expected response";
		break;

	default:
		return "Undefined Error";
	}

}

void CrossConnectionConvertTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}

CrossConnectionConvertTest::CrossConnectionConvertTest(void)
{
}

CrossConnectionConvertTest::~CrossConnectionConvertTest(void)
{
}