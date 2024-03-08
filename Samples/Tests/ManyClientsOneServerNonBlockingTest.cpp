#include "ManyClientsOneServerNonBlockingTest.h"

/*
What is being done here is having 256 clients connect to one server, disconnect, connect again.

Do this for about 10 seconds. Then allow them all to connect for one last time.

This one has a nonblocking recieve so doesn't wait for connects or anything.
Just rapid connecting disconnecting.

Good ideas for changes:
After the last check run a eightpeers like test an add the conditions
of that test as well.

Make sure that if we initiate the connection we get a proper message
and if not we get a proper message. Add proper conditions.

Randomize sending the disconnect notes

Success conditions:
All connected normally.

Failure conditions:
Doesn't reconnect normally.

During the very first connect loop any connect returns false.

Connect function returns false and peer is not connected to anything.

*/
int ManyClientsOneServerNonBlockingTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{

	const int clientNum= 256;

	RakPeerInterface *clientList[clientNum];//A list of clients
	RakPeerInterface *server;//The server
	SystemAddress currentSystem;

	//SystemAddress currentSystem;

	Packet *packet;
	destroyList.Clear(false,__FILE__,__LINE__);

	//Initializations of the arrays
	for (int i=0;i<clientNum;i++)
	{
		clientList[i]=RakNetworkFactory::GetRakPeerInterface();
		destroyList.Push(clientList[i],__FILE__,__LINE__);

		clientList[i]->Startup(1,30,&SocketDescriptor(), 1);

	}

	server=RakNetworkFactory::GetRakPeerInterface();
	destroyList.Push(server,__FILE__,__LINE__);
	server->Startup(clientNum, 30, &SocketDescriptor(60000,0), 1);
	server->SetMaximumIncomingConnections(clientNum);

	//Connect all the clients to the server

	for (int i=0;i<clientNum;i++)
	{

		if (!clientList[i]->Connect("127.0.0.1", 60000, 0,0))
		{

			if (isVerbose)
				DebugTools::ShowError("Problem while calling connect.\n",!noPauses && isVerbose,__LINE__,__FILE__);

			return 1;//This fails the test, don't bother going on.

		}

	}

	RakNetTime entryTime=RakNet::GetTime();//Loop entry time

	DataStructures::List< SystemAddress  > systemList;
	DataStructures::List< RakNetGUID > guidList;

	if (isVerbose)
		printf("Entering disconnect loop \n");

	while(RakNet::GetTime()-entryTime<10000)//Run for 10 Secoonds
	{

		//Disconnect all clients IF connected to any from client side
		for (int i=0;i<clientNum;i++)
		{

			clientList[i]->GetSystemList(systemList,guidList);//Get connectionlist
			int len=systemList.Size();

			for (int j=0;j<len;j++)//Disconnect them all
			{

				clientList[i]->CloseConnection (systemList[j],true,0,LOW_PRIORITY); 	
			}

		}

		//RakSleep(100);

		//Connect

		for (int i=0;i<clientNum;i++)
		{

			currentSystem.SetBinaryAddress("127.0.0.1");
			currentSystem.port=60000;
			if(!clientList[i]->IsConnected (currentSystem,true,true) )//Are we connected or is there a pending operation ?
			{

				if (!clientList[i]->Connect("127.0.0.1", 60000, 0,0))
				{

					if (isVerbose)
						DebugTools::ShowError("Problem while calling connect. \n",!noPauses && isVerbose,__LINE__,__FILE__);

					return 1;//This fails the test, don't bother going on.

				}
			}

		}

		//Server receive

		packet=server->Receive();

		if (isVerbose&&packet)
			printf("For server\n");

		while(packet)
		{

			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("Another client has disconnected.\n");

				break;
			case ID_REMOTE_CONNECTION_LOST:
				if (isVerbose)
					printf("Another client has lost the connection.\n");

				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("Another client has connected.\n");
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				if (isVerbose)              
					printf("Our connection request has been accepted.\n");

				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				if (isVerbose)
					printf("A connection has failed.\n");

				break;

			case ID_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("A connection is incoming.\n");

				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				if (isVerbose)              
					printf("The server is full.\n");

				break;

			case ID_ALREADY_CONNECTED:
				if (isVerbose)              
					printf("Already connected\n");

				break;

			case ID_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("We have been disconnected.\n");
				break;
			case ID_CONNECTION_LOST:
				if (isVerbose)
					printf("Connection lost.\n");

				break;
			default:

				break;
			}

			server->DeallocatePacket(packet);

			// Stay in the loop as long as there are more packets.
			packet = server->Receive();
		}

		for (int i=0;i<clientNum;i++)//Receive for all peers
		{

			packet=clientList[i]->Receive();

			if (isVerbose&&packet)
				printf("For peer %i\n",i);

			while(packet)
			{

				switch (packet->data[0])
				{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("Another client has disconnected.\n");

					break;
				case ID_REMOTE_CONNECTION_LOST:
					if (isVerbose)
						printf("Another client has lost the connection.\n");

					break;
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("Another client has connected.\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					if (isVerbose)              
						printf("Our connection request has been accepted.\n");

					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					if (isVerbose)
						printf("A connection has failed.\n");

					break;

				case ID_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("A connection is incoming.\n");

					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					if (isVerbose)              
						printf("The server is full.\n");

					break;

				case ID_ALREADY_CONNECTED:
					if (isVerbose)              
						printf("Already connected\n");

					break;

				case ID_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("We have been disconnected.\n");
					break;
				case ID_CONNECTION_LOST:
					if (isVerbose)
						printf("Connection lost.\n");

					break;
				default:

					break;
				}

				clientList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = clientList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}

	entryTime=RakNet::GetTime();

	while(RakNet::GetTime()-entryTime<2000)//Run for 2 Secoonds to process incoming disconnects
	{

		//Server receive

		packet=server->Receive();

		if (isVerbose&&packet)
			printf("For server\n");

		while(packet)
		{

			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("Another client has disconnected.\n");

				break;
			case ID_REMOTE_CONNECTION_LOST:
				if (isVerbose)
					printf("Another client has lost the connection.\n");

				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("Another client has connected.\n");
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				if (isVerbose)              
					printf("Our connection request has been accepted.\n");

				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				if (isVerbose)
					printf("A connection has failed.\n");

				break;

			case ID_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("A connection is incoming.\n");

				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				if (isVerbose)              
					printf("The server is full.\n");

				break;

			case ID_ALREADY_CONNECTED:
				if (isVerbose)              
					printf("Already connected\n");

				break;

			case ID_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("We have been disconnected.\n");
				break;
			case ID_CONNECTION_LOST:
				if (isVerbose)
					printf("Connection lost.\n");

				break;
			default:

				break;
			}

			server->DeallocatePacket(packet);

			// Stay in the loop as long as there are more packets.
			packet = server->Receive();
		}

		for (int i=0;i<clientNum;i++)//Receive for all peers
		{

			packet=clientList[i]->Receive();
			if (isVerbose&&packet)
				printf("For peer %i\n",i);

			while(packet)
			{

				switch (packet->data[0])
				{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("Another client has disconnected.\n");

					break;
				case ID_REMOTE_CONNECTION_LOST:
					if (isVerbose)
						printf("Another client has lost the connection.\n");

					break;
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("Another client has connected.\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					if (isVerbose)              
						printf("Our connection request has been accepted.\n");

					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					if (isVerbose)
						printf("A connection has failed.\n");

					break;

				case ID_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("A connection is incoming.\n");

					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					if (isVerbose)              
						printf("The server is full.\n");

					break;

				case ID_ALREADY_CONNECTED:
					if (isVerbose)              
						printf("Already connected\n");

					break;

				case ID_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("We have been disconnected.\n");
					break;
				case ID_CONNECTION_LOST:
					if (isVerbose)
						printf("Connection lost.\n");

					break;
				default:

					break;
				}

				clientList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = clientList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}

	//Connect

	for (int i=0;i<clientNum;i++)
	{

		currentSystem.SetBinaryAddress("127.0.0.1");
		currentSystem.port=60000;

		if(!clientList[i]->IsConnected (currentSystem,true,true) )//Are we connected or is there a pending operation ?
		{

			if (!clientList[i]->Connect("127.0.0.1", 60000, 0,0))
			{

				clientList[i]->GetSystemList(systemList,guidList);//Get connectionlist
				int len=systemList.Size();

				if (isVerbose)
					DebugTools::ShowError("Problem while calling connect. \n",!noPauses && isVerbose,__LINE__,__FILE__);

				return 1;//This fails the test, don't bother going on.

			}
		}

	}

	entryTime=RakNet::GetTime();

	while(RakNet::GetTime()-entryTime<5000)//Run for 5 Secoonds
	{

		//Server receive

		packet=server->Receive();
		if (isVerbose&&packet)
			printf("For server\n");

		while(packet)
		{

			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("Another client has disconnected.\n");

				break;
			case ID_REMOTE_CONNECTION_LOST:
				if (isVerbose)
					printf("Another client has lost the connection.\n");

				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("Another client has connected.\n");
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				if (isVerbose)              
					printf("Our connection request has been accepted.\n");

				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				if (isVerbose)
					printf("A connection has failed.\n");

				break;

			case ID_NEW_INCOMING_CONNECTION:
				if (isVerbose)              
					printf("A connection is incoming.\n");

				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				if (isVerbose)              
					printf("The server is full.\n");

				break;

			case ID_ALREADY_CONNECTED:
				if (isVerbose)              
					printf("Already connected\n");

				break;

			case ID_DISCONNECTION_NOTIFICATION:
				if (isVerbose)
					printf("We have been disconnected.\n");
				break;
			case ID_CONNECTION_LOST:
				if (isVerbose)
					printf("Connection lost.\n");

				break;
			default:

				break;
			}

			server->DeallocatePacket(packet);

			// Stay in the loop as long as there are more packets.
			packet = server->Receive();
		}

		for (int i=0;i<clientNum;i++)//Receive for all clients
		{

			packet=clientList[i]->Receive();
			if (isVerbose&&packet)
				printf("For peer %i\n",i);

			while(packet)
			{

				switch (packet->data[0])
				{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("Another client has disconnected.\n");

					break;
				case ID_REMOTE_CONNECTION_LOST:
					if (isVerbose)
						printf("Another client has lost the connection.\n");

					break;
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("Another client has connected.\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					if (isVerbose)              
						printf("Our connection request has been accepted.\n");

					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					if (isVerbose)
						printf("A connection has failed.\n");

					break;

				case ID_NEW_INCOMING_CONNECTION:
					if (isVerbose)              
						printf("A connection is incoming.\n");

					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					if (isVerbose)              
						printf("The server is full.\n");

					break;

				case ID_ALREADY_CONNECTED:
					if (isVerbose)              
						printf("Already connected\n");

					break;

				case ID_DISCONNECTION_NOTIFICATION:
					if (isVerbose)
						printf("We have been disconnected.\n");
					break;
				case ID_CONNECTION_LOST:
					if (isVerbose)
						printf("Connection lost.\n");

					break;
				default:

					break;
				}

				clientList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = clientList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}

	for (int i=0;i<clientNum;i++)
	{

		clientList[i]->GetSystemList(systemList,guidList);
		int connNum=guidList.Size();//Get the number of connections for the current peer
		if (connNum!=1)//Did we connect to all?
		{

			if (isVerbose)
				printf("Not all clients reconnected normally.\nFailed on clients number %i\n",i);

			if (isVerbose)
				DebugTools::ShowError("",!noPauses && isVerbose,__LINE__,__FILE__);

		
			

			return 2;

		}

	}

	

	if (isVerbose)
		printf("Pass\n");
	return 0;

}

RakNet::RakString ManyClientsOneServerNonBlockingTest::GetTestName()
{

	return "ManyClientsOneServerNonBlockingTest";

}

RakNet::RakString ManyClientsOneServerNonBlockingTest::ErrorCodeToString(int errorCode)
{

	switch (errorCode)
	{

	case 0:
		return "No error";
		break;

	case 1:
		return "The connect function failed.";
		break;

	case 2:
		return "Peers did not connect normally.";
		break;

	default:
		return "Undefined Error";
	}

}

ManyClientsOneServerNonBlockingTest::ManyClientsOneServerNonBlockingTest(void)
{
}

ManyClientsOneServerNonBlockingTest::~ManyClientsOneServerNonBlockingTest(void)
{
}

void ManyClientsOneServerNonBlockingTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}