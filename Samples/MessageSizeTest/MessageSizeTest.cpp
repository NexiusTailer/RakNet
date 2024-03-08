#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include <cstdio>
#include <memory.h>
#include <cstring>
#include <stdlib.h>
#include "Rand.h"
#include "RakNetStatistics.h"
#include "RakSleep.h"
#include "RakMemoryOverride.h"

int main(int argc, char **argv)
{
	RakPeerInterface *sender, *receiver;

	printf("This project tests sending messages of various sizes.\n");
	sender = RakNetworkFactory::GetRakPeerInterface();
	receiver = RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor sd1(1234,0),sd2(1235,0);
	receiver->Startup(32, 30, &sd1, 1);
	receiver->SetMaximumIncomingConnections(32);
	sender->Startup(1, 30, &sd2, 1);
	sender->Connect("127.0.0.1", 1234, 0, 0);
	RakSleep(100);

	char data[4000];
	data[0]=ID_USER_PACKET_ENUM;
	int stride, sum;
	int sendCount, receiveCount;
	for (stride=1; stride < 2000; stride++)
	{
		sendCount=0;
		receiveCount=0;
		for (sum=0; sum < 4000; sum+=stride)
		{
			sender->Send(data,stride,HIGH_PRIORITY,RELIABLE_ORDERED,0,UNASSIGNED_SYSTEM_ADDRESS,true);
			sendCount++;
		}

		RakSleep(100);

		Packet *p;
		for (p=sender->Receive(); p; sender->DeallocatePacket(p), p=sender->Receive())
			;
		for (p=receiver->Receive(); p; receiver->DeallocatePacket(p), p=receiver->Receive())
		{
			if (p->data[0]==ID_USER_PACKET_ENUM)
				receiveCount++;
		}

		if (sendCount==receiveCount)
			printf("Stride=%i Sends=%i Receives=%i\n", stride, sendCount, receiveCount);
		else
			printf("ERROR! Stride=%i Sends=%i Receives=%i\n", stride, sendCount, receiveCount);
	}

	if (sender)
		RakNetworkFactory::DestroyRakPeerInterface(sender);
	if (receiver)
		RakNetworkFactory::DestroyRakPeerInterface(receiver);

	return 1;
}