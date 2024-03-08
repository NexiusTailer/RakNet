#include "TestHelpers.h"

TestHelpers::TestHelpers(void)
{
}

TestHelpers::~TestHelpers(void)
{
}

void TestHelpers::StandardServerPrep(RakPeerInterface *&server)
{

	server=RakNetworkFactory::GetRakPeerInterface();
	server->Startup(1,30,&SocketDescriptor(60000,0),1);
	server->SetMaximumIncomingConnections(1);

}

void TestHelpers::StandardClientPrep(RakPeerInterface *&client)
{

	client=RakNetworkFactory::GetRakPeerInterface();

	client->Startup(1,30,&SocketDescriptor(),1);

}

void TestHelpers::StandardServerPrep(RakPeerInterface *&server,DataStructures::List <RakPeerInterface *> &destroyList)
{

	StandardServerPrep(server);
	destroyList.Push(server,__FILE__,__LINE__);

}

void TestHelpers::StandardClientPrep(RakPeerInterface *&client,DataStructures::List <RakPeerInterface *> &destroyList)
{

	StandardClientPrep(client);
	destroyList.Push(client,__FILE__,__LINE__);

}

//returns false if not connected
bool TestHelpers::WaitAndConnectTwoPeersLocally(RakPeerInterface *connector,RakPeerInterface *connectee,int millisecondsToWait)
{

	SystemAddress connecteeAdd=connectee->GetInternalID();
	return CommonFunctions::WaitAndConnect(connector,"127.0.0.1",connecteeAdd.port,millisecondsToWait);

}

//returns false if connect fails
bool TestHelpers::ConnectTwoPeersLocally(RakPeerInterface *connector,RakPeerInterface *connectee)
{
	SystemAddress connecteeAdd=connectee->GetInternalID();
	return connector->Connect("127.0.0.1",connecteeAdd.port,0,0);
}

bool TestHelpers::BroadCastTestPacket(RakPeerInterface *sender,PacketReliability rel,PacketPriority pr,int typeNum)//returns send return value
{

	char str2[]="AAAAAAAAAA";
	str2[0]=typeNum;
	return sender->Send(str2,(int) strlen(str2)+1, pr, rel  ,0, UNASSIGNED_SYSTEM_ADDRESS, true)>0;
}

bool TestHelpers::SendTestPacketDirected(RakPeerInterface *sender,char * ip,int port,PacketReliability rel,PacketPriority pr,int typeNum)//returns send return value
{

	SystemAddress recAddress;

	recAddress.SetBinaryAddress(ip);
	recAddress.port=port;

	char str2[]="AAAAAAAAAA";
	str2[0]=typeNum;
	return sender->Send(str2,(int) strlen(str2)+1, pr, rel  ,0, recAddress, false)>0;
}

bool TestHelpers::WaitForTestPacket(RakPeerInterface *reciever,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

			if (packet->data[0]==ID_USER_PACKET_ENUM+1)
			{
				reciever->DeallocatePacket(packet);
				return true;
			}

		}

	}

	return false;

}

void RecieveForXTime(RakPeerInterface *reciever,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

		}

	}

}