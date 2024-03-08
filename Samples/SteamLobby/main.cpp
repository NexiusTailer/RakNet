#include <stdio.h>
#include <stdlib.h>
#include "Lobby2Client_Steam.h" // If Lobby2Client_Steam.h is included before SocketLayer.h, then it will use the steam send functions
#include "Lobby2Message_Steam.h"
#include "RakNetTime.h"
#include "RakSleep.h"
#include "RakNetTypes.h"
#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include <windows.h>
#include <Kbhit.h>

using namespace RakNet;

Lobby2MessageFactory_Steam *messageFactory;
Lobby2Client_Steam *lobby2Client;
RakPeerInterface *rakPeer;
CSteamID lastRoom;

void PrintCommands(void)
{
	printf("a. GetRoomList\n");
	printf("b. LeaveRoom\n");
	printf("c. CreateRoom\n");
	printf("d. JoinRoom\n");
	printf("e. RefreshRoom\n");
	printf("f. SendRoomChatMessage\n");
	printf("g. ListRoomMembers\n");
	printf("?. Help\n");
	printf("(Escape). Quit\n");
}

struct SteamResults : public RakNet::Lobby2Callbacks
{
	virtual void MessageResult(RakNet::Notification_Console_RoomMemberConnectivityUpdate *message)
	{
		RakNet::Notification_Console_RoomMemberConnectivityUpdate_Steam *msgSteam = (RakNet::Notification_Console_RoomMemberConnectivityUpdate_Steam *) message;
		RakNet::RakString msg;
		msgSteam->DebugMsg(msg);
		printf("%s\n", msg.C_String());
		rakPeer->Connect(msgSteam->remoteSystem.ToString(false), msgSteam->remoteSystem.port, 0, 0);
	}

	virtual void MessageResult(RakNet::Console_SearchRooms *message)
	{
		RakNet::Console_SearchRooms_Steam *msgSteam = (RakNet::Console_SearchRooms_Steam *) message;
		RakNet::RakString msg;
		msgSteam->DebugMsg(msg);
		printf("%s\n", msg.C_String());
		if (msgSteam->roomIds.GetSize()>0)
		{
			lastRoom=msgSteam->roomIds[0];
		}
	}

	virtual void ExecuteDefaultResult(RakNet::Lobby2Message *message)
	{
		RakNet::RakString out;
		message->DebugMsg(out);
		printf("%s\n", out.C_String());
	}
};

int main(int argc, char **argv)
{
	if (argc>1)
	{
		printf("Command arguments:\n");
		for (int i=1; i < argc; i++)
		{
			printf("%i. %s\n", i, argv[i]);
		}
	}

	SteamResults steamResults;
	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	messageFactory = new Lobby2MessageFactory_Steam;
	lobby2Client = new Lobby2Client_Steam("1.0");
	lobby2Client->AddCallbackInterface(&steamResults);
	lobby2Client->SetMessageFactory(messageFactory);
	rakPeer->AttachPlugin(lobby2Client);
	rakPeer->Startup(32,0,&SocketDescriptor(1234,0),1);
	rakPeer->SetMaximumIncomingConnections(32);
	RakNet::Lobby2Message* msg = messageFactory->Alloc(RakNet::L2MID_Client_Login);
	lobby2Client->SendMsg(msg);
	if (msg->resultCode!=L2RC_PROCESSING && msg->resultCode!=L2RC_SUCCESS)
	{
		printf("Steam must be running to play this game (SteamAPI_Init() failed).\n");
		printf("If this fails, steam_appid.txt was probably not in the working directory.\n");
		messageFactory->Dealloc(msg);
		return -1;
	}
	messageFactory->Dealloc(msg);
	
	PrintCommands();

	bool quit=false;
	char ch;
	while(!quit)
	{
		Packet *p;
		for (p=rakPeer->Receive(); p; rakPeer->DeallocatePacket(p), p=rakPeer->Receive())
		{
			switch (p->data[0])
			{
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_ALREADY_CONNECTED:
				// Connection lost normally
				printf("ID_ALREADY_CONNECTED\n");
				break;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_CONNECTION_LOST\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
				break;
			case ID_CONNECTION_BANNED: // Banned from this server
				printf("We are banned from this server.\n");
				break;			
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("Connection attempt failed\n");
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// Sorry, the server is full.  I don't do anything here but
				// A real app should tell the user
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
				break;
			case ID_MODIFIED_PACKET:
				// Cheater!
				printf("ID_MODIFIED_PACKET\n");
				break;

			case ID_INVALID_PASSWORD:
				printf("ID_INVALID_PASSWORD\n");
				break;

			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				printf("ID_CONNECTION_LOST\n");
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				// This tells the client they have connected
				printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(), p->guid.ToString());
				break;

			case ID_NEW_INCOMING_CONNECTION:
				printf("ID_NEW_INCOMING_CONNECTION");
				break;

			default:
				// It's a client, so just show the message
				printf("Unknown Message ID %i\n", p->data[0]);
				break;
			}

		}
		if (kbhit())
		{
			ch=getch();

			switch (ch)
			{
			case 'a':
				{

					RakNet::Lobby2Message* logoffMsg = messageFactory->Alloc(RakNet::L2MID_Console_SearchRooms);
					lobby2Client->SendMsg(logoffMsg);
					messageFactory->Dealloc(logoffMsg);
				}
				break;

			case 'b':
				{
					if (lobby2Client->GetRoomID()==0)
					{
						printf("Not in a room\n");
						break;
					}
					RakNet::Console_LeaveRoom_Steam* msg = (RakNet::Console_LeaveRoom_Steam*) messageFactory->Alloc(RakNet::L2MID_Console_LeaveRoom);
					msg->roomId=lobby2Client->GetRoomID();
					lobby2Client->SendMsg(msg);
					messageFactory->Dealloc(msg);
				}
				break;

			case 'c':
				{
					if (lobby2Client->GetRoomID()!=0)
					{
						printf("Already in a room\n");
						break;
					}

					RakNet::Console_CreateRoom_Steam* msg = (RakNet::Console_CreateRoom_Steam*) messageFactory->Alloc(RakNet::L2MID_Console_CreateRoom);
					// set the name of the lobby if it's ours
					char rgchLobbyName[256];
					msg->roomIsPublic=true;
					_snprintf( rgchLobbyName, sizeof( rgchLobbyName ), "%s's lobby", SteamFriends()->GetPersonaName() );
					msg->roomName=rgchLobbyName;
					lobby2Client->SendMsg(msg);
					messageFactory->Dealloc(msg);

				}
				break;

			case 'd':
				{
					if (lobby2Client->GetRoomID()!=0)
					{
						printf("Already in a room\n");
						break;
					}

					RakNet::Console_JoinRoom_Steam* msg = (RakNet::Console_JoinRoom_Steam*) messageFactory->Alloc(RakNet::L2MID_Console_JoinRoom);
					printf("Enter room id, or enter for %"PRINTF_64_BIT_MODIFIER"u: ", lastRoom.ConvertToUint64());
					char str[256];
					gets(str);
					if (str[0]==0)
					{
						msg->roomId=lastRoom.ConvertToUint64();
					}
					else
					{
						msg->roomId=_atoi64(str);
					}
					lobby2Client->SendMsg(msg);
					messageFactory->Dealloc(msg);
				}
				break;

			case 'e':
				{
					if (lobby2Client->GetRoomID()==0)
					{
						printf("Not in a room\n");
						break;
					}

					RakNet::Console_GetRoomDetails_Steam* msg = (RakNet::Console_GetRoomDetails_Steam*) messageFactory->Alloc(RakNet::L2MID_Console_GetRoomDetails);
					msg->roomId=lobby2Client->GetRoomID();
					lobby2Client->SendMsg(msg);
					messageFactory->Dealloc(msg);
				}
				break;

			case 'f':
				{
					if (lobby2Client->GetRoomID()==0)
					{
						printf("Not in a room\n");
						break;

					}
					RakNet::Console_SendRoomChatMessage_Steam* msg = (RakNet::Console_SendRoomChatMessage_Steam*) messageFactory->Alloc(RakNet::L2MID_Console_SendRoomChatMessage);
					msg->message="Test chat message.";
					msg->roomId=lobby2Client->GetRoomID();
					lobby2Client->SendMsg(msg);
					messageFactory->Dealloc(msg);

				}
				break;

			case 'g':
				{
					DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> roomMembers;
					lobby2Client->GetRoomMembers(roomMembers);
					for (DataStructures::DefaultIndexType i=0; i < roomMembers.GetSize(); i++)
					{
						printf("%i. %s ID=%"PRINTF_64_BIT_MODIFIER"u\n", i+1, lobby2Client->GetRoomMemberName(roomMembers[i]), roomMembers[i]);
					}
				}
				break;


			case '?':
				{
					PrintCommands();

				}
				break;

			case 27:
				{
					quit=true;
				}
				break;
			}
		}

		RakSleep(30);
	}
	
	RakNet::Lobby2Message* logoffMsg = messageFactory->Alloc(RakNet::L2MID_Client_Logoff);
	lobby2Client->SendMsg(logoffMsg);
	messageFactory->Dealloc(logoffMsg);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);

	return 1;
}
