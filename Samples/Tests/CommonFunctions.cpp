#include "CommonFunctions.h"

CommonFunctions::CommonFunctions(void)
{
}

CommonFunctions::~CommonFunctions(void)
{
}

bool CommonFunctions::WaitAndConnect(RakPeerInterface *peer,char* ip,unsigned short int port,int millisecondsToWait)
{

	SystemAddress connectToAddress;

	connectToAddress.SetBinaryAddress(ip);
	connectToAddress.port=port;
	RakNetTime entryTime=RakNet::GetTime();

	while(!peer->IsConnected (connectToAddress,false,false)&&RakNet::GetTime()-entryTime<millisecondsToWait)
	{

		if(!peer->IsConnected (connectToAddress,true,true))
		{
			peer->Connect(ip,port,0,0);
		}

		RakSleep(100);

	}

	if (peer->IsConnected (connectToAddress,false,false))
	{
		return 1;
	}

	return 0;
}

void CommonFunctions::DisconnectAndWait(RakPeerInterface *peer,char* ip,unsigned short int port)
{
	SystemAddress targetAddress;

	targetAddress.SetBinaryAddress(ip);
	targetAddress.port=port;

	while(peer->IsConnected (targetAddress,true,true))//disconnect client
	{

		peer->CloseConnection (targetAddress,true,0,LOW_PRIORITY); 
	}

}

bool CommonFunctions::WaitForMessageWithID(RakPeerInterface *reciever,int id,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

			//printf("Packet %i\n",packet->data[0]);
			if (packet->data[0]==id)
			{
				reciever->DeallocatePacket(packet);
				return true;
			}

		}

	}

	return false;
}

Packet *CommonFunctions::WaitAndReturnMessageWithID(RakPeerInterface *reciever,int id,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

		//	printf("Packet %i\n",packet->data[0]);
			if (packet->data[0]==id)
			{
				return packet;
			}

		}

	}

	return 0;
}