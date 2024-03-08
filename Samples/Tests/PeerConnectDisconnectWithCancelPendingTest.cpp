#include "PeerConnectDisconnectWithCancelPendingTest.h"



/*
What is being done here is having 8 peers all connect to eachother, disconnect, connect again.

Do this for about 10 seconds. Then allow them all to connect for one last time.

This test also tests the cancelpendingconnections.

Also tests nonblocking connects, the simpler one PeerConnectDisconnect tests withoput it


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
int PeerConnectDisconnectWithCancelPendingTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{

	const int peerNum= 8;
	const int maxConnections=peerNum*3;//Max allowed connections for test set to times 3 to eliminate problem variables
	RakPeerInterface *peerList[peerNum];//A list of 8 peers
	int connectionAmount[peerNum];//Counter for me to keep track of connection requests and accepts


	char str[512];
	char pauseStr[512];
	SystemAddress currentSystem;

	Packet *packet;





	//Initializations of the arrays
	for (int i=0;i<peerNum;i++)
	{
		peerList[i]=RakNetworkFactory::GetRakPeerInterface();
		connectionAmount[i]=0;




		peerList[i]->Startup(maxConnections, 30, &SocketDescriptor(60000+i,0), 1);
		peerList[i]->SetMaximumIncomingConnections(maxConnections);

	}




	//Connect all the peers together

	strcpy(str, "127.0.0.1");

	for (int i=0;i<peerNum;i++)
	{

		for (int j=i+1;j<peerNum;j++)//Start at i+1 so don't connect two of the same together.
		{

			if (!peerList[i]->Connect(str, 60000+j, 0,0))
			{

				if (isVerbose)
					printf("Problem while calling connect. Enter to continue. \n");

				if (!noPauses && isVerbose)
					gets(str);
				return 1;//This fails the test, don't bother going on.

			}

		}	

	}



	RakNetTime entryTime=RakNet::GetTime();//Loop entry time

	DataStructures::List< SystemAddress  > systemList;
	DataStructures::List< RakNetGUID > guidList;



	printf("Entering disconnect loop \n");


	while(RakNet::GetTime()-entryTime<10000)//Run for 10 Secoonds
	{


		//Disconnect all peers IF connected to any
		for (int i=0;i<peerNum;i++)
		{

			peerList[i]->GetSystemList(systemList,guidList);//Get connectionlist
			int len=systemList.Size();

			for (int j=0;j<len;j++)//Disconnect them all
			{


				peerList[i]->CloseConnection (systemList[j],true,0,LOW_PRIORITY); 	
			}


		}


		RakSleep(100);
		//Clear pending if not finished
		strcpy(str, "127.0.0.1");

		int before[peerNum],after[peerNum];
		for (int i=0;i<peerNum;i++)
		{



			for (int j=i+1;j<peerNum;j++)//Start at i+1 so don't connect two of the same together.
			{

				currentSystem.SetBinaryAddress(str);
				currentSystem.port=60000+j;


				before[i]=peerList[i]->GetIndexFromSystemAddress(currentSystem);



				peerList[i]->CancelConnectionAttempt(currentSystem);  	//Make sure a connection is not pending before trying to connect.


				after[i] =peerList[i]->GetIndexFromSystemAddress(currentSystem);


				peerList[i]->GetSystemList(systemList,guidList);//Get connectionlist for use in breakpoint

				int len=systemList.Size();




			}

		}
	


		//Connect

		for (int i=0;i<peerNum;i++)
		{

			for (int j=i+1;j<peerNum;j++)//Start at i+1 so don't connect two of the same together.
			{



				if (!peerList[i]->Connect(str, 60000+j, 0,0))
				{


					currentSystem.SetBinaryAddress(str);
					currentSystem.port=60000+j;
					/*

					The reason for this code originally was to breakpoint right after we get a list of
					connections. The list was zero so it did not return false because it
					was already connected.

					However I mad it into a condition to see if it failed when there are no connections.

					This will be changed to see if it fails but is not connected to target peer.

					*/

					peerList[i]->GetSystemList(systemList,guidList);//Get connectionlist

					int len=systemList.Size();




					if(after[i]!=-1&&before[i]==after[i]&&!peerList[i]->IsConnected (currentSystem,false,true) )//Did we drop a pending connection? 
					{
						if (isVerbose)
							printf("Did not cancel the pending request \n");

						//if (!noPauses && isVerbose)
						//gets(str);
					}

					if (len==0)//No connections, should not fail.
					{
						if (isVerbose)
							printf("Problem while calling connect. \n");

						if (!noPauses && isVerbose)
							gets(str);
						return 1;//This fails the test, don't bother going on.
					}

				}

			}	

		}




		for (int i=0;i<peerNum;i++)//Receive for all peers
		{
			if (isVerbose)
				printf("For peer %i\n",i);

			packet=peerList[i]->Receive();


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

				peerList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = peerList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}

	while(RakNet::GetTime()-entryTime<2000)//Run for 2 Secoonds to process incoming disconnects
	{






		for (int i=0;i<peerNum;i++)//Receive for all peers
		{
			if (isVerbose)
				printf("For peer %i\n",i);

			packet=peerList[i]->Receive();


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

				peerList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = peerList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}

	//Connect
	strcpy(str, "127.0.0.1");
	for (int i=0;i<peerNum;i++)
	{

		for (int j=i+1;j<peerNum;j++)//Start at i+1 so don't connect two of the same together.
		{


			currentSystem.SetBinaryAddress(str);
			currentSystem.port=60000+j;

			peerList[i]->CancelConnectionAttempt(currentSystem);  	//Make sure a connection is not pending before trying to connect.


			if (!peerList[i]->Connect(str, 60000+j, 0,0))
			{


				peerList[i]->GetSystemList(systemList,guidList);//Get connectionlist
				int len=systemList.Size();




				if (len==0)//No connections, should not fail.
				{

					if (isVerbose)
						printf("Problem while calling connect. \n");

					if (!noPauses && isVerbose)
						gets(str);

					return 1;//This fails the test, don't bother going on.
				}

			}

		}	

	}

	entryTime=RakNet::GetTime();

	while(RakNet::GetTime()-entryTime<5000)//Run for 5 Secoonds
	{






		for (int i=0;i<peerNum;i++)//Receive for all peers
		{
			if (isVerbose)
				printf("For peer %i\n",i);

			packet=peerList[i]->Receive();


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

				peerList[i]->DeallocatePacket(packet);

				// Stay in the loop as long as there are more packets.
				packet = peerList[i]->Receive();
			}
		}
		RakSleep(0);//If needed for testing
	}



	for (int i=0;i<peerNum;i++)
	{

		peerList[i]->GetSystemList(systemList,guidList);
		int connNum=guidList.Size();//Get the number of connections for the current peer
		if (connNum!=peerNum-1)//Did we connect to all?
		{


			if (isVerbose)
				printf("Not all peers reconnected normally.\n");

			if (!noPauses && isVerbose)
			{
				printf("Press enter to continue \n");
				gets(pauseStr);
			}

			for (int i=0;i<peerNum;i++)//Clean
			{
				RakNetworkFactory::DestroyRakPeerInterface(peerList[i]);
			}




			return 2;


		}


	}


	for (int i=0;i<peerNum;i++)//Clean
	{
		RakNetworkFactory::DestroyRakPeerInterface(peerList[i]);
	}

	if (isVerbose)
		printf("Pass\n");
	return 0;

}



RakNet::RakString PeerConnectDisconnectWithCancelPendingTest::GetTestName()
{

	return "PeerConnectDisconnectWithCancelPendingTest";

}

RakNet::RakString PeerConnectDisconnectWithCancelPendingTest::ErrorCodeToString(int errorCode)
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


PeerConnectDisconnectWithCancelPendingTest::PeerConnectDisconnectWithCancelPendingTest(void)
{
}

PeerConnectDisconnectWithCancelPendingTest::~PeerConnectDisconnectWithCancelPendingTest(void)
{
}
