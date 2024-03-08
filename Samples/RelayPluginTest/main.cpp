#include "RakPeerInterface.h"
#include "RakSleep.h"
#include "RelayPlugin.h"
#include "Gets.h"
#include "Kbhit.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"

using namespace RakNet;

int main(void)
{
	printf("Tests the RelayPlugin as a server.\n");
	printf("Difficulty: Beginner\n\n");

	char str[64], str2[64];

	RakNet::RakPeerInterface *peer=RakNet::RakPeerInterface::GetInstance();
	RelayPlugin *relayPlugin = RelayPlugin::GetInstance();
	peer->AttachPlugin(relayPlugin);

	// Get our input
	char ip[64], serverPort[30], listenPort[30];
	puts("Enter the port to listen on");
	Gets(listenPort,sizeof(listenPort));
	if (listenPort[0]==0)
		strcpy(listenPort, "1234");

	// Connecting the client is very simple.  0 means we don't care about
	// a connectionValidationInteger, and false for low priority threads
	RakNet::SocketDescriptor socketDescriptor(atoi(listenPort),0);
	socketDescriptor.socketFamily=AF_INET;
	peer->Startup(8,&socketDescriptor, 1);
	peer->SetMaximumIncomingConnections(8);
	peer->SetOccasionalPing(true);

	puts("Enter IP to connect to, or enter for none");
	Gets(ip, sizeof(ip));
	peer->AllowConnectionResponseIPMigration(false);
	if (ip[0])
	{
		puts("Enter the port to connect to");
		Gets(serverPort,sizeof(serverPort));
		if (serverPort[0]==0)
			strcpy(serverPort, "1234");

		RakNet::ConnectionAttemptResult car = peer->Connect(ip, atoi(serverPort), 0, 0);
		RakAssert(car==RakNet::CONNECTION_ATTEMPT_STARTED);
	}

	peer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString(str);
	printf("My GUID is %s\n", str);

	printf("(A)dd participant\n");
	printf("(S)end to participant\n");
	printf("(Q)uit\n");


	while (1)
	{
		if (kbhit())
		{
			char ch = getch();
			if (ch=='a' || ch=='A')
			{
				char name[128];
				printf("Enter name of participant: ");
				Gets(name, sizeof(name));
				if (name[0])
				{
					printf("Enter GUID of participant: ");
					char guid[128];
					Gets(guid, sizeof(guid));
					if (guid[0])
					{
						RakNetGUID g;
						g.FromString(guid);
						relayPlugin->AddParticipantOnServer(name, g);
						printf("Done\n");
					}
					else
					{
						printf("Operation aborted\n");
					}
				}
				else
				{
					printf("Operation aborted\n");
				}

			}
			else if (ch=='s' || ch=='S')
			{
				char name[128];
				printf("Enter name of participant: ");
				Gets(name, sizeof(name));
				if (name[0])
				{
					printf("Enter message to send: ");
					char msg[256];
					Gets(msg, sizeof(msg));
					RakString msgRs = msg;
					BitStream msgBs;
					msgBs.Write(msgRs);
					relayPlugin->SendToParticipant(peer->GetGUIDFromIndex(0), name, &msgBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0 );
				}
				else
				{
					printf("Operation aborted\n");
				}
			}
			else if (ch=='q')
			{
				break;
			}
		}

		Packet *packet;
		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			packet->guid.ToString(str);
			packet->systemAddress.ToString(true,str2);
			if (packet->data[0]==ID_NEW_INCOMING_CONNECTION)
			{
				printf("ID_NEW_INCOMING_CONNECTION from %s on %s\n", str, str2);
			}
			else if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				printf("ID_CONNECTION_REQUEST_ACCEPTED from %s on %s\n", str, str2);
			}
			else if (packet->data[0]==ID_RELAY_PLUGIN_FROM_RELAY)
			{
				BitStream msgRs;
				RakString senderRs;
				BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(MessageID));
				bsIn.Read(senderRs);
				bsIn.Read(&msgRs);
				RakString message;
				msgRs.Read(message);
				printf("Got relayed message: %s\n", message.C_String());
			}
		}

		RakSleep(30);
	}
	

	return 1;
}

