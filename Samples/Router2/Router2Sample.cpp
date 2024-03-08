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




bool SendGUID(RakNetGUID GUID,SystemAddress routerAddress,char * stringRouterIp,RakPeerInterface *peer,unsigned char messageId)
{
	if(!peer->IsConnected (routerAddress,true,true) )//Are we connected or is there a pending operation ?
	{

		peer->Connect(stringRouterIp,routerAddress.port,0,0);

	}


	RakNet::BitStream bitStream;
	bitStream.Reset();
	bitStream.Write(messageId);
	bitStream.Write(GUID);



	RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 1000;
	while (peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, routerAddress, false)==false)
	{
		if (RakNet::GetTimeMS()>stopWaiting)
		{
			return false;
		}

	}


	return true;
}


RakNetGUID QueryRouterForSourceGUID(SystemAddress routerAddress,char * stringRouterIp,RakPeerInterface *peer)
{



	if(!peer->IsConnected (routerAddress,true,true) )//Are we connected or is there a pending operation ?
	{

		peer->Connect(stringRouterIp,routerAddress.port,0,0);

	}

	RakNetGUID returnedGUID;

	Packet *packet;
	RakNet::BitStream bitStream;
	bitStream.Reset();
	bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+4));



	RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 1000;
	while (peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, routerAddress, false)==false)
	{
		if (RakNet::GetTimeMS()>stopWaiting)
		{
			return UNASSIGNED_RAKNET_GUID;
		}

	}


	stopWaiting = RakNet::GetTimeMS() + 1000;
	while (RakNet::GetTimeMS()<stopWaiting)
	{


		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{

			//	printf("Talker recieved packet id %i\n",packet->data[0]);

			if (packet->data[0]==ID_USER_PACKET_ENUM+5)
			{

				bitStream.Reset();
				bitStream.Write((char*)packet->data, packet->length);
				bitStream.IgnoreBits(8); 
				bitStream.Read(returnedGUID);
				return returnedGUID;
			}
		}

	}



	return UNASSIGNED_RAKNET_GUID;


}



RakNetGUID QueryRouterForEndPointGUID(SystemAddress routerAddress,char * stringRouterIp,RakPeerInterface *peer)
{



	if(!peer->IsConnected (routerAddress,true,true) )//Are we connected or is there a pending operation ?
	{

		peer->Connect(stringRouterIp,routerAddress.port,0,0);

	}

	RakNetGUID returnedGUID;

	Packet *packet;
	RakNet::BitStream bitStream;
	bitStream.Reset();
	bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+6));



	RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 1000;
	while (peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, routerAddress, false)==false)
	{
		if (RakNet::GetTimeMS()>stopWaiting)
		{
			return UNASSIGNED_RAKNET_GUID;
		}

	}


	stopWaiting = RakNet::GetTimeMS() + 1000;
	while (RakNet::GetTimeMS()<stopWaiting)
	{


		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{

			//printf("Talker recieved packet id %i\n",packet->data[0]);

			if (packet->data[0]==ID_USER_PACKET_ENUM+7)
			{

				bitStream.Reset();
				bitStream.Write((char*)packet->data, packet->length);
				bitStream.IgnoreBits(8); 
				bitStream.Read(returnedGUID);
				return returnedGUID;
			}
		}

	}



	return UNASSIGNED_RAKNET_GUID;


}



int RouterMode()
{

	RakNet::BitStream bitStream;

	RakPeerInterface  *router;
	RakNet::Router2 routerR2;



	router = RakNetworkFactory::GetRakPeerInterface();



	router->AttachPlugin(&routerR2);
	routerR2.SetMaximumForwardingRequests(8);



	SocketDescriptor sdRouter(1235,0);



	router->Startup(8,0,&sdRouter,1);
	router->SetMaximumIncomingConnections(8);





	RakSleep(100);

	SystemAddress routerAddress;




	RakNetGUID endpointGuid=UNASSIGNED_RAKNET_GUID;

	RakNetGUID sourceGuid=UNASSIGNED_RAKNET_GUID;



	char loopNumber=0;
	int lastSequence;
	int dupsRecieved;
	SystemAddress receivedAddress;
	RakNetGUID returnId;



	printf("Waiting for connections and data");

	do
	{

		Packet *packet;
		bool testPassed=false;

		RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 5000;
		while (RakNet::GetTimeMS()<stopWaiting)
		{






			for (packet=router->Receive(); packet; router->DeallocatePacket(packet), packet=router->Receive())
			{
				printf("Router  Recieved packet id %i\n",packet->data[0]);

				if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
				{
					printf("Router Recieved packet id %i, incoming connection, Ip:%s\n",packet->data[0],packet->systemAddress.ToString());

				}


				if (packet->data[0]==ID_USER_PACKET_ENUM+2)
				{


					bitStream.Reset();
					bitStream.Write((char*)packet->data, packet->length);
					bitStream.IgnoreBits(8); 
					bitStream.Read(receivedAddress);
					returnId=router->GetGuidFromSystemAddress(receivedAddress);


					bitStream.Reset();
					bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+3));
					bitStream.Write(returnId);


					router->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, packet->systemAddress, false);

				}


				if (packet->data[0]==ID_USER_PACKET_ENUM+8)
				{


					bitStream.Reset();
					bitStream.Write((char*)packet->data, packet->length);
					bitStream.IgnoreBits(8); 
					bitStream.Read(sourceGuid);


				}


				if (packet->data[0]==ID_USER_PACKET_ENUM+9)
				{


					bitStream.Reset();
					bitStream.Write((char*)packet->data, packet->length);
					bitStream.IgnoreBits(8); 
					bitStream.Read(endpointGuid);


				}

				if (packet->data[0]==ID_USER_PACKET_ENUM+4)
				{





					bitStream.Reset();
					bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+5));
					bitStream.Write(sourceGuid);


					router->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, packet->systemAddress, false);

				}



				if (packet->data[0]==ID_USER_PACKET_ENUM+6)
				{





					bitStream.Reset();
					bitStream.Write((unsigned char) (ID_USER_PACKET_ENUM+7));
					bitStream.Write(endpointGuid);


					router->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED ,0, packet->systemAddress, false);

				}

			}

















			RakSleep(0);
		}





		RakSleep(0);


	}
	while(1);




	RakNetworkFactory::DestroyRakPeerInterface(router);

	return 0;
}




int EndPointMode(char * routerIp,char * backupRouterIp,bool isBackupRouter)
{
	RakPeerInterface *endpoint,*routerTalker;
	RakNet::Router2 endpointR2;


	routerTalker= RakNetworkFactory::GetRakPeerInterface();




	endpoint = RakNetworkFactory::GetRakPeerInterface();



	endpoint->AttachPlugin(&endpointR2);



	SocketDescriptor sdEndpoint(1236,0);


	endpoint->Startup(8,0,&sdEndpoint,1);
	endpoint->SetMaximumIncomingConnections(8);


	routerTalker->Startup(8,0,&SocketDescriptor(1233,0),1);




	endpoint->Connect(routerIp,1235,0,0);

	if (isBackupRouter)
		endpoint->Connect(backupRouterIp,1235,0,0);


	routerTalker->Connect(routerIp,1235,0,0);


	RakSleep(100);

	SystemAddress routerAddress;



	routerAddress.SetBinaryAddress(routerIp);
	routerAddress.port=1235;



	printf("Sending endpoint GUID\n");
	while(!SendGUID(endpoint->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS),routerAddress,routerIp,routerTalker,ID_USER_PACKET_ENUM+9))
	{
		RakSleep(1000);
	}



	RakNetGUID sourceGuid=UNASSIGNED_RAKNET_GUID;


	printf("Waiting for source GUID\n");
	while(sourceGuid==UNASSIGNED_RAKNET_GUID)
	{
		sourceGuid=QueryRouterForSourceGUID(routerAddress,routerIp,routerTalker);
		RakSleep(1000);
	}


	printf("sourceGuid=%s\n",sourceGuid.ToString());




	char loopNumber=0;
	int lastSequence;
	int dupsRecieved;
	SystemAddress receivedAddress;
	RakNetGUID returnId;




	do
	{

		Packet *packet;
		bool testPassed=false;

		RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 5000;
		while (RakNet::GetTimeMS()<stopWaiting)
		{







			lastSequence=-1;
			dupsRecieved=0;
			for (packet=endpoint->Receive(); packet; endpoint->DeallocatePacket(packet), packet=endpoint->Receive())
			{


				printf("Endpoint recieved packet id %i\n",packet->data[0]);
				if (packet->data[0]==ID_USER_PACKET_ENUM+1  && packet->guid==sourceGuid)
				{

					printf("Recieved packet from loop %i\n",packet->data[9]);
					testPassed=true;
					stopWaiting=0;
					if (packet->data[9]==lastSequence)
					{
						dupsRecieved++;
						printf("This is duplicate number %i\n",dupsRecieved);

					}
					else
					{
						dupsRecieved=0;

					}
					lastSequence=packet->data[9];
					//break;
				}



			}










			RakSleep(0);
		}





		RakSleep(0);


	}
	while(1);




	RakNetworkFactory::DestroyRakPeerInterface(endpoint);

	return 0;
}



int SourceMode(char * routerIp,char * backupRouterIp,bool isBackupRouter)
{
	RakPeerInterface *source,*routerTalker;
	RakNet::Router2 sourceR2;


	routerTalker= RakNetworkFactory::GetRakPeerInterface();



	source = RakNetworkFactory::GetRakPeerInterface();




	source->AttachPlugin(&sourceR2);




	SocketDescriptor sdSource(1234,0);



	source->Startup(8,0,&sdSource,1);


	routerTalker->Startup(8,0,&SocketDescriptor(1232,0),1);





	source->Connect(routerIp,1235,0,0);

	if (isBackupRouter)
		source->Connect(backupRouterIp,1235,0,0);


	routerTalker->Connect(routerIp,1235,0,0);


	RakSleep(100);

	SystemAddress routerAddress;


	routerAddress.SetBinaryAddress(routerIp);
	routerAddress.port=1235;


	printf("Sending source GUID\n");
	while(!SendGUID(source->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS),routerAddress,routerIp,routerTalker,ID_USER_PACKET_ENUM+8))
	{
		RakSleep(1000);}


	RakNetGUID endpointGuid=UNASSIGNED_RAKNET_GUID;


	printf("Getting endpoint GUID\n");
	while(endpointGuid==UNASSIGNED_RAKNET_GUID)
	{
		endpointGuid=  QueryRouterForEndPointGUID(routerAddress,routerIp,routerTalker);
		RakSleep(1000);
	}



	printf("endpointGuid=%s\n",endpointGuid.ToString());





	sourceR2.Connect(endpointGuid);

	char loopNumber=0;
	int lastSequence;
	int dupsRecieved;
	SystemAddress receivedAddress;
	RakNetGUID returnId;



	do
	{

		Packet *packet;
		bool testPassed=false;

		RakNetTimeMS stopWaiting = RakNet::GetTimeMS() + 5000;
		while (RakNet::GetTimeMS()<stopWaiting)
		{



			for (packet=source->Receive(); packet; source->DeallocatePacket(packet), packet=source->Receive())
			{

				printf("Source recieved packet id %i\n",packet->data[0]);
				if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED  && packet->guid==endpointGuid)
				{
					testPassed=true;
					stopWaiting=0;
					//break;
				}


			}













			RakSleep(0);
		}




		RakSleep(50);
		char str[]="AAAAAAAAAA";
		str[0]=ID_USER_PACKET_ENUM+1;
		str[9]=loopNumber;
		printf("Sending packet from loop %i\n",loopNumber);
		source->Send(str, (int) strlen(str)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		loopNumber++;





	}
	while(1);


	RakNetworkFactory::DestroyRakPeerInterface(source);



	return 0;
}



int main(void)
{
	printf("Demonstration of the router2 plugin.\n");
	printf("The router2 plugin allows you to connect to a system, routing messages through a third system\n");
	printf("This is useful if you can connect to the second system, but not the third, due to NAT issues.\n");
	printf("Difficulty: Advanced\n\n");

	char routerType;
	char isBackupRouterChar;
	bool isBackupRouter;
	char routerIp[15];
	char backupRouterIp[15];


	bool passedDisconnect=false;
	bool passedRerouting=false;
	bool passedRecieve=false;






	printf("Source(S),Endpoint(E),router(R) :");
	fflush(stdin);
	routerType=getchar();


	if (routerType>97)
	{
		routerType-=32;
	}

	switch (routerType)
	{
	case 'S':

	case 'E':
	case 'R':

		break;
	default:
		return 1;


	}

	if (routerType=='E'||routerType=='S')
	{
		fflush(stdin);

		printf("RouterIp : ");
		fgets(routerIp, sizeof(routerIp), stdin);

		if (routerIp[strlen(routerIp)-1]=='\n')
		{
			routerIp[strlen(routerIp)-1]=0;

		}


		fflush(stdin);

		printf("Have backup router? (Y)(N) :");
		isBackupRouterChar=getchar();

		if (isBackupRouterChar>97)
		{
			isBackupRouterChar-=32;
		}


		if (isBackupRouterChar=='Y')
		{
			isBackupRouter=true;
		}
		else
		{
			isBackupRouter=false;
		}


		if (isBackupRouter)
		{
			fflush(stdin);

			printf("BackupRouterIp : ");
			fgets(backupRouterIp, sizeof(backupRouterIp), stdin);

			if (backupRouterIp[strlen(backupRouterIp)-1]=='\n')
			{
				backupRouterIp[strlen(backupRouterIp)-1]=0;

			}
		}

	}


	switch (routerType)
	{
	case 'S':
		SourceMode(routerIp, backupRouterIp, isBackupRouter);
		break;

	case 'E':
		EndPointMode(routerIp, backupRouterIp, isBackupRouter);
		break;
	case 'R':
		RouterMode();
		break;

	default:
		return 1;


	}





	return 0;
}




