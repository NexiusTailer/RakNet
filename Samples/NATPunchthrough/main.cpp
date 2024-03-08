/// \file
/// \brief Tests the NAT-punchthrough plugin
///
/// This file is part of RakNet Copyright 2003 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation; either version 2 of the License, or (at your
/// option) any later version.

#include "RakPeerInterface.h"
#include "RakSleep.h"
#include <stdio.h>
#include <stdlib.h>
#include "RakNetworkFactory.h"
#include <string.h>
#include "Kbhit.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "ConnectionGraph.h"
#include "PacketLogger.h"
#include "StringCompressor.h"
#include "PacketFileLogger.h"
#include "GetTime.h"
#include "NatPunchthroughClient.h"
#include "NatPunchthroughServer.h"
#include "SocketLayer.h"
#include "RakString.h"

#define NAT_PUNCHTHROUGH_FACILITATOR_PORT 60481
#define NAT_PUNCHTHROUGH_FACILITATOR_PASSWORD ""
#define DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP "216.224.123.180"

struct NatPunchthroughDebugInterface_PacketFileLogger : public NatPunchthroughDebugInterface
{
	// Set to non-zero to write to the packetlogger!
	PacketFileLogger *pl;

	NatPunchthroughDebugInterface_PacketFileLogger() {pl=0;}
	~NatPunchthroughDebugInterface_PacketFileLogger() {}
	virtual void OnClientMessage(const char *msg)
	{
		if (pl)
		{
			pl->WriteMiscellaneous("Nat", msg);
		}
	}
	
};


void VerboseTest()
{
	// Another program to test this is http://midcom-p2p.sourceforge.net/

	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	NatPunchthroughClient natPunchthroughClient;
	NatPunchthroughServer natPunchthroughServer;
	ConnectionGraph connectionGraph;
	NatPunchthroughDebugInterface_Printf debugInterface;
	natPunchthroughClient.SetDebugInterface(&debugInterface);
	char mode[64], facilitatorIP[64];
	rakPeer->AttachPlugin(&connectionGraph);

	printf("Tests the NAT Punchthrough plugin\n");
	printf("Difficulty: Intermediate\n\n");

	int i;
	DataStructures::List<RakNet::RakString> internalIds;
	char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ];
	SocketLayer::Instance()->GetMyIP( ipList );

	for (i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
	{
		RakNet::RakString rs;
		if (ipList[i][0])
		{
			rs = ipList[i];

			if (internalIds.GetIndexOf(rs)==(unsigned int)-1)
			{	
				internalIds.Push(rs);
			}
		}
	}

	SocketDescriptor socketDescriptor;
	char str[128];
	if (internalIds.Size()>1)
	{
		printf("Which IP to bind to (Enter = any)?\n");
		for (i=0; i < (int) internalIds.Size(); i++)
		{
			printf("%i. %s\n", i+1, internalIds[i].C_String());
		}
		gets(str);
		if (str[0])
		{
			int index = atoi(str);
			strcpy(socketDescriptor.hostAddress, internalIds[i-1].C_String());
		}
	}
	if (socketDescriptor.hostAddress[0]==0)
		printf("My internal IP is %s.\n", internalIds[0].C_String());

	printf("Act as (S)ender, (R)ecipient, or (F)acilitator?\n");
	gets(mode);
	if (mode[0]=='s' || mode[0]=='S' || mode[0]=='r' || mode[0]=='R')
	{
		PunchthroughConfiguration *pc = natPunchthroughClient.GetPunchthroughConfiguration();
		printf("Modify TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL?\n");
		gets(str);
		if (str[0]!=0)
			pc->TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL=atoi(str);

		printf("Modify TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL?\n");
		gets(str);
		if (str[0]!=0)
			pc->TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL=atoi(str);

		printf("Modify UDP_SENDS_PER_PORT?\n");
		gets(str);
		if (str[0]!=0)
			pc->UDP_SENDS_PER_PORT=atoi(str);

		printf("Modify INTERNAL_IP_WAIT_AFTER_ATTEMPTS?\n");
		gets(str);
		if (str[0]!=0)
			pc->INTERNAL_IP_WAIT_AFTER_ATTEMPTS=atoi(str);

		printf("Modify MAX_PREDICTIVE_PORT_RANGE?\n");
		gets(str);
		if (str[0]!=0)
			pc->MAX_PREDICTIVE_PORT_RANGE=atoi(str);

		printf("Modify EXTERNAL_IP_WAIT_BETWEEN_PORTS?\n");
		gets(str);
		if (str[0]!=0)
			pc->EXTERNAL_IP_WAIT_BETWEEN_PORTS=atoi(str);

		printf("Modify EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS?\n");
		gets(str);
		if (str[0]!=0)
			pc->EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS=atoi(str);

		rakPeer->Startup(1024,0,&socketDescriptor, 1);
		rakPeer->SetMaximumIncomingConnections(32);
		printf("Enter facilitator IP: ");
		gets(facilitatorIP);
		if (facilitatorIP[0]==0)
			strcpy(facilitatorIP, DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP);

		rakPeer->AttachPlugin(&natPunchthroughClient);
		printf("Connecting to facilitator.\n");
		rakPeer->Connect(facilitatorIP, NAT_PUNCHTHROUGH_FACILITATOR_PORT, NAT_PUNCHTHROUGH_FACILITATOR_PASSWORD, (int) strlen(NAT_PUNCHTHROUGH_FACILITATOR_PASSWORD));
	}
	else
	{
		socketDescriptor.port=NAT_PUNCHTHROUGH_FACILITATOR_PORT;
		rakPeer->Startup(1024,0,&socketDescriptor, 1);
		rakPeer->SetMaximumIncomingConnections(32);
		rakPeer->AttachPlugin(&natPunchthroughServer);

		printf("Ready.\n");
	}

	printf("'c'lear the screen.\n'q'uit.\n");
	Packet *p;
	while (1)
	{
		p=rakPeer->Receive();
		while (p)
		{
			if (p->data[0]==ID_REMOTE_NEW_INCOMING_CONNECTION && (mode[0]=='s' || mode[0]=='S'))
			{
				RakNet::BitStream bsIn(p->data, p->length, false);
				ConnectionGraphGroupID group1, group2;
				SystemAddress node1, node2, remoteAddress, serverAddress;
				RakNetGUID guid1, guid2, remoteGuid, serverGuid;
				bsIn.IgnoreBytes(1);
				bsIn.Read(node1);
				bsIn.Read(group1);
				bsIn.Read(guid1);
				bsIn.Read(node2);
				bsIn.Read(group2);
				bsIn.Read(guid2);
				if (rakPeer->GetSystemAddressFromGuid(guid1)==UNASSIGNED_SYSTEM_ADDRESS)
				{
					remoteAddress=node1;
					remoteGuid=guid1;
					serverAddress=node2;
					serverGuid=guid2;
				}
				else
				{
					remoteAddress=node2;
					remoteGuid=guid2;
					serverAddress=node1;
					serverGuid=guid1;
				}

				printf("Remote address is %s\n", remoteAddress.ToString(true));
				printf("Remote guid is %s\n", remoteGuid.ToString());
				printf("Server address is %s\n", serverAddress.ToString(true));
				printf("Server guid is %s\n", serverGuid.ToString());

				printf("Connecting to remote system.\n");
				natPunchthroughClient.OpenNAT(remoteGuid, rakPeer->GetSystemAddressFromIndex(0));
			}
			if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
				printf("ID_DISCONNECTION_NOTIFICATION\n");
			else if (p->data[0]==ID_CONNECTION_LOST)
			{
				printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString());
			}
			else if (p->data[0]==ID_NO_FREE_INCOMING_CONNECTIONS)
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
			else if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
			{
					printf("ID_NEW_INCOMING_CONNECTION from %s with guid %s\n", p->systemAddress.ToString(), p->guid.ToString());
			}
			else if (p->data[0]==ID_REMOTE_NEW_INCOMING_CONNECTION)
			{
				printf("Got ID_REMOTE_NEW_INCOMING_CONNECTION from %s\n", p->systemAddress.ToString(true));
			}
			else if (p->data[0]==ID_CONNECTION_GRAPH_REPLY)
			{
				printf("Got ID_CONNECTION_GRAPH_REPLY from %s\n", p->systemAddress.ToString(true));
			}
			else if (p->data[0]==ID_REMOTE_DISCONNECTION_NOTIFICATION)
			{
				printf("Got ID_REMOTE_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));
			}
			else if (p->data[0]==ID_REMOTE_CONNECTION_LOST)
			{
				printf("Got ID_REMOTE_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));
			}
			else if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				if (mode[0]=='s' || mode[0]=='S')
				{
					printf("ID_CONNECTION_REQUEST_ACCEPTED from %s.\nMy external IP is %s\n", p->systemAddress.ToString(), rakPeer->GetExternalID(p->systemAddress).ToString());
				}
				else
					printf("ID_CONNECTION_REQUEST_ACCEPTED from %s.\nMy external IP is %s\n", p->systemAddress.ToString(), rakPeer->GetExternalID(p->systemAddress). ToString());
			}
			else if (p->data[0]==ID_CONNECTION_ATTEMPT_FAILED)
			{
				if (p->systemAddress.port==NAT_PUNCHTHROUGH_FACILITATOR_PORT)
					printf("ID_CONNECTION_ATTEMPT_FAILED (facilitator)\n");
				else
					printf("ID_CONNECTION_ATTEMPT_FAILED (recipient)\n");
			}
			else if (p->data[0]==ID_NAT_TARGET_NOT_CONNECTED)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_TARGET_NOT_CONNECTED for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_TARGET_UNRESPONSIVE)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_TARGET_UNRESPONSIVE for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_CONNECTION_TO_TARGET_LOST for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_ALREADY_IN_PROGRESS)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_ALREADY_IN_PROGRESS for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_PUNCHTHROUGH_FAILED)
			{
				printf("ID_NAT_PUNCHTHROUGH_FAILED for %s\n", p->guid.ToString());
			}
			else if (p->data[0]==ID_NAT_PUNCHTHROUGH_SUCCEEDED)
			{
				printf("ID_NAT_PUNCHTHROUGH_SUCCEEDED for %s\n", p->systemAddress.ToString());
				if (mode[0]=='s' || mode[0]=='S')
				{
					rakPeer->Connect(p->systemAddress.ToString(), p->systemAddress.port, 0, 0);
				}
			}
			else if (p->data[0]==ID_PONG)
			{
				RakNetTime time;
				memcpy((char*)&time, p->data+1, sizeof(RakNetTime));
				printf("Got pong from %s with time %i\n", p->systemAddress.ToString(), RakNet::GetTime() - time);
			}
			else if (p->data[0]==ID_INVALID_PASSWORD)
			{
				printf("ID_INVALID_PASSWORD\n");
			}
			else if (p->data[0]==ID_NAT_ALREADY_IN_PROGRESS)
			{
				SystemAddress systemAddress;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(systemAddress);
				printf("ID_NAT_ALREADY_IN_PROGRESS to %s\n", systemAddress.ToString());
			}
			else
			{
				printf("Unknown packet ID %i from %s\n", p->data[0], p->systemAddress.ToString(true));
			}

			rakPeer->DeallocatePacket(p);
			p=rakPeer->Receive();
		}

		if (kbhit())
		{
			char ch = getch();
			if (ch=='q' || ch=='Q')
				break;
			if (ch=='c' || ch=='C')
			{
#ifdef _WIN32
				system("cls");
#else
				printf("Unsupported on this OS\n");
#endif
			}
		}

		RakSleep(30);
	}

	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
}
void SimpleClientTest()
{
	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	NatPunchthroughClient natPunchthroughClient;
	ConnectionGraph connectionGraph;
	rakPeer->AttachPlugin(&connectionGraph);
	rakPeer->Startup(1024,0,&SocketDescriptor(), 1);
	rakPeer->SetMaximumIncomingConnections(32);
	rakPeer->AttachPlugin(&natPunchthroughClient);
	printf("Connecting to facilitator.\n");
	rakPeer->Connect(DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP, NAT_PUNCHTHROUGH_FACILITATOR_PORT, NAT_PUNCHTHROUGH_FACILITATOR_PASSWORD, (int) strlen(NAT_PUNCHTHROUGH_FACILITATOR_PASSWORD));
	PacketFileLogger packetFileLogger;
	rakPeer->AttachPlugin(&packetFileLogger);
	NatPunchthroughDebugInterface_PacketFileLogger pfl;
	natPunchthroughClient.SetDebugInterface(&pfl);
	pfl.pl=&packetFileLogger;
	packetFileLogger.StartLog("NatPunchTest");

	printf("'c'lear the screen.\n'q'uit.\n");
	Packet *p;
	while (1)
	{
		p=rakPeer->Receive();
		while (p)
		{
			if (p->data[0]==ID_REMOTE_NEW_INCOMING_CONNECTION)
			{
				RakNet::BitStream bsIn(p->data, p->length, false);
				ConnectionGraphGroupID group1, group2;
				SystemAddress node1, node2, remoteAddress, serverAddress;
				RakNetGUID guid1, guid2, remoteGuid, serverGuid;
				bsIn.IgnoreBytes(1);
				bsIn.Read(node1);
				bsIn.Read(group1);
				bsIn.Read(guid1);
				bsIn.Read(node2);
				bsIn.Read(group2);
				bsIn.Read(guid2);
				if (rakPeer->GetSystemAddressFromGuid(guid1)==UNASSIGNED_SYSTEM_ADDRESS)
				{
					remoteAddress=node1;
					remoteGuid=guid1;
					serverAddress=node2;
					serverGuid=guid2;
				}
				else
				{
					remoteAddress=node2;
					remoteGuid=guid2;
					serverAddress=node1;
					serverGuid=guid1;
				}

//				printf("Remote address is %s\n", remoteAddress.ToString(true));
//				printf("Remote guid is %s\n", remoteGuid.ToString());
//				printf("Server address is %s\n", serverAddress.ToString(true));
//				printf("Server guid is %s\n", serverGuid.ToString());

				printf("Connecting to %s.\n", remoteAddress.ToString(true));
				natPunchthroughClient.OpenNAT(remoteGuid, rakPeer->GetSystemAddressFromIndex(0));
			}
			if (p->data[0]==ID_DISCONNECTION_NOTIFICATION)
				printf("%s disconnected\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_CONNECTION_LOST)
				printf("Lost connection to %s\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_NO_FREE_INCOMING_CONNECTIONS)
				printf("%s is has no free connections.\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_NEW_INCOMING_CONNECTION)
				printf("%s connected to us\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				if (strcmp(p->systemAddress.ToString(false),DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP)==0)
					printf("Connected to the facilitator.\n");
				else
					printf("Connected to %s\n", p->systemAddress.ToString(true));
			}
			else if (p->data[0]==ID_CONNECTION_ATTEMPT_FAILED)
				printf("Failed to connect to %s\n", p->systemAddress.ToString(true));
			else if (p->data[0]==ID_NAT_TARGET_NOT_CONNECTED)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_TARGET_NOT_CONNECTED for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_TARGET_UNRESPONSIVE)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_TARGET_UNRESPONSIVE for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
				printf("ID_NAT_CONNECTION_TO_TARGET_LOST for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_ALREADY_IN_PROGRESS)
			{
				RakNetGUID g;
				RakNet::BitStream b(p->data, p->length, false);
				b.IgnoreBits(8); // Ignore the ID_...
				b.Read(g);
			//	printf("ID_NAT_ALREADY_IN_PROGRESS for %s\n", g.ToString());
			}
			else if (p->data[0]==ID_NAT_PUNCHTHROUGH_FAILED)
				printf("Failed NAT punch to %s\n", p->guid.ToString());
			else if (p->data[0]==ID_NAT_PUNCHTHROUGH_SUCCEEDED)
				printf("Succeeded NAT punch to %s\n", p->systemAddress.ToString());

			rakPeer->DeallocatePacket(p);
			p=rakPeer->Receive();
		}

		if (kbhit())
		{
			char ch = getch();
			if (ch=='q' || ch=='Q')
				break;
			if (ch=='c' || ch=='C')
			{
#ifdef _WIN32
				system("cls");
#else
				printf("Unsupported on this OS\n");
#endif
			}
		}

		RakSleep(30);
	}

	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
}


// This sample starts one of three systems: A facilitator, a recipient, and a sender.  The sender wants to connect to the recipient but
// cannot because both the sender and recipient are behind NATs.  So they both connect to the facilitator first.
// The facilitator will synchronize a countdown timer so that the sender will try to connect to the recipient at the same time
// the recipient will send an offline message to the sender.  This will, in theory, punch through the NAT.
//
// IMPORTANT: The recipient and sender need to know each other's external IP addresses to be able to try to connect to each other.
// The facilitator should transmit this on connection, such as with a lobby server, which I do here using the ConnectionGraph plugin.
// That plugin will cause the lobby server to send ID_REMOTE_* system notifications to its connected systems.
int main(void)
{
#ifdef _DEBUG
	// Use to start the facilitator
	VerboseTest();
#else
	SimpleClientTest();
#endif
	return 1;
}