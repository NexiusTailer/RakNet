#ifdef SN_TARGET_PS3

#include "Lobby2Message.h"

#ifndef __LOBBY_2_MESSAGE_PS3_H
#define __LOBBY_2_MESSAGE_PS3_H

#include <sys/process.h>
#include <cell/sysmodule.h>
#include <netex/net.h>
#include <netex/libnetctl.h>
#include <sysutil_common.h>
#include <sysutil_sysparam.h>
#include <np.h>
#include <sysutil/sysutil_avc2.h>


#include "DS_Table.h"

// Sample usage:
/*
Here's some instructions and code demonstrating how to use Lobby2 with the PS3. Generally speaking, each message that you allocate and send to lobby2Client will trigger a callback in the callback class you register with AddCallbackInterface(). While not every operation is exposed, enough operations are exposed to get into a room and start a game.

Example sequence of commands from the user:

Login
GetWorldListFromServer
GetLobbyListFromWorld
SearchRooms
CreateRoom
JoinRoom

You also get callbacks for notifications. Some of the ones relevant to this example:

Notification_Console_MemberJoinedLobby
Notification_Console_MemberLeftLobby
Notification_Console_RoomMemberConnectivityUpdate (signaling completed)
Notification_Console_ChatEvent (voice chat, not room text chat)

Here's a code sample on setting up the systems, and logging in:

class CBInterface : public RakNet::Lobby2Callbacks
{
virtual void MessageResult(RakNet::Notification_Console_RoomMemberConnectivityUpdate *message);
} cbInterface;

RakNet::Lobby2Client_PS3 *lobby2Client;
RakNet::Lobby2MessageFactory_PS3 *messageFactory;
lobby2Client = new RakNet::Lobby2Client_PS3;
messageFactory = new RakNet::Lobby2MessageFactory_PS3;
lobby2Client->AddCallbackInterface(cbInterface);

messageFactory->Alloc(RakNet::L2MID_Client_Login);
((RakNet::Client_Login_PS3*) msg)->cellSysutilRegisterCallback_slot=3;
((RakNet::Client_Login_PS3*) msg)->npCommId=gGameEngine->GetCommunicationID();
((RakNet::Client_Login_PS3*) msg)->npCommPassphrase=gGameEngine->GetCommunicationPassphrase();
lobby2Client->SendMessage(msg);
messageFactory->Dealloc(msg);


Note that a lot of the code that actually does the work originally comes from the sample np_gui_matching2 from usr\local\cell\samples\sdk\network\np

*/

int NP_init();
int NP_shutdown();

namespace RakNet
{

#define __L2_MSG_DB_HEADER(__NAME__,__DB__) \
	struct __NAME__##_##__DB__ : public __NAME__

	/*
	__L2_MSG_DB_HEADER(Platform_Startup, PS3)
	{
		Platform_Startup_PS3();
		virtual bool ClientImpl( Lobby2Client *client);
	};

	__L2_MSG_DB_HEADER(Platform_Shutdown, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);
	};
	*/

	/// Note: Start RakNet BEFORE calling this. After calling this, RakNet's thread can fail to start for unknown reasons. Seems to be related to calling pthread_attr_setstacksize
	__L2_MSG_DB_HEADER(Client_Login, PS3)
	{
		Client_Login_PS3();
		virtual bool ClientImpl( Lobby2Client *client);

		// cellSysutilRegisterCallback will be called so this system knows when
		// CELL_SYSUTIL_NET_CTL_NETSTART_LOADED,
		// CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED,
		// NP_GUI_NETSTART_STATE_UNLOADING
		// NP_GUI_NETSTART_STATE_UNLOADED
		// Has arrived. The first parameter to this function should be set here, and should not be used elsewhere.
		// currently valid from 0-3
		int cellSysutilRegisterCallback_slot;
		bool supportTitleUserStorage;

		// These should be held statically somewhere, and changed to point to the value
		const SceNpCommunicationId* npCommId;
		const SceNpCommunicationPassphrase* npCommPassphrase;
	};

	__L2_MSG_DB_HEADER(Client_Logoff, PS3)
	{
		Client_Logoff_PS3();
		virtual bool ClientImpl( Lobby2Client *client);

		int cellSysutilRegisterCallback_slot;
	};

	__L2_MSG_DB_HEADER(Client_PerTitleIntegerStorage, PS3)
	{
		Client_PerTitleIntegerStorage_PS3();
		~Client_PerTitleIntegerStorage_PS3();
		virtual bool ClientImpl( Lobby2Client *client);
		bool WasCompleted(void);

		/// \internal
		bool wasStarted;
		SceNpTusVariable resultVariable;
	};

	__L2_MSG_DB_HEADER(Client_PerTitleBinaryStorage, PS3)
	{
		Client_PerTitleBinaryStorage_PS3();
		~Client_PerTitleBinaryStorage_PS3();
		virtual bool ClientImpl( Lobby2Client *client);
		bool WasCompleted(void);

		/// \internal
		bool wasStarted;
		SceNpTusDataStatus dataStatus;
		char buffOut[262144];
	};

	__L2_MSG_DB_HEADER(Console_GetServerStatus, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_GetServerStatus::DebugMsg(out);
				return;
			}

			if (serverInfo.status==SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE)
				out.Set("%s: Server %i status = Available", GetName(), serverInfo.serverId);
			else if (serverInfo.status==SCE_NP_MATCHING2_SERVER_STATUS_UNAVAILABLE)
				out.Set("%s: Server %i status = Unavailable", GetName(), serverInfo.serverId);
			else if (serverInfo.status==SCE_NP_MATCHING2_SERVER_STATUS_BUSY)
				out.Set("%s: Server %i status = Busy", GetName(), serverInfo.serverId);
			else if (serverInfo.status==SCE_NP_MATCHING2_SERVER_STATUS_MAINTENANCE)
				out.Set("%s: Server %i status = Maintenance", GetName(), serverInfo.serverId);
			else
				out.Set("%s: Server %i status = <INTERNAL ERROR IN LOBBY2MESSAGE_PS3.h>", GetName(), serverInfo.serverId);
		}

		// input
		SceNpMatching2ServerId serverId;

		// Output
		SceNpMatching2Server serverInfo;
	};

	__L2_MSG_DB_HEADER(Console_GetWorldListFromServer, PS3)
	{
		Console_GetWorldListFromServer_PS3();
		virtual ~Console_GetWorldListFromServer_PS3();
		virtual bool ClientImpl( Lobby2Client *client);
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_GetWorldListFromServer::DebugMsg(out);
				return;
			}
			out.Set("%s: %i worlds", GetName(), response.worldNum);
			for (unsigned int i=0; i < response.worldNum; i++)
			{
				out += RakNet::RakString("\n%i. worldId=%i", i+1, response.world->worldId);
			}

		}

		// Input
		SceNpMatching2ServerId serverId;

		// Output
		SceNpMatching2GetWorldInfoListResponse response;
	};

	__L2_MSG_DB_HEADER(Console_GetLobbyListFromWorld, PS3)
	{
		Console_GetLobbyListFromWorld_PS3();
		virtual ~Console_GetLobbyListFromWorld_PS3();

		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_GetLobbyListFromWorld::DebugMsg(out);
				return;
			}
			out.Set("%s: %i lobbies", GetName(), lobbies.Size());
			for (unsigned int i=0; i < lobbies.Size(); i++)
			{
#if defined(_WIN32)
				out += RakNet::RakString("\n%i. lobbyId=%I64u", i+1, lobbies[i]);
#else
				out += RakNet::RakString("\n%i. lobbyId=%llu", i+1, lobbies[i]);
#endif
			}
		}

		// Input
		SceNpMatching2WorldId m_worldId;

		// Output
		DataStructures::List<SceNpMatching2LobbyId> lobbies;
		unsigned int startIndex, total, size;
	};

	__L2_MSG_DB_HEADER(Console_JoinLobby, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_JoinLobby::DebugMsg(out);
				return;
			}
			out.Set("%s: Joined lobby with %i members.\n", GetName(), memberIds.Size());
			out += RakNet::RakString("My ID = %i.\n", m_lobby_me);
			for (unsigned int i=0; i < memberIds.Size(); i++)
			{
				out += RakNet::RakString("\n%i. memberId=%i", i+1, memberIds[i]);
			}
		}


		// Input
		SceNpMatching2LobbyId m_lobbyId;

		// Output
		SceNpMatching2LobbyMemberId m_lobby_me;
		DataStructures::List<SceNpMatching2LobbyMemberId> memberIds;
	};

	__L2_MSG_DB_HEADER(Console_LeaveLobby, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpMatching2LobbyId m_lobbyId;
	};

	__L2_MSG_DB_HEADER(Console_SendLobbyChatMessage, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_SendLobbyChatMessage::DebugMsg(out);
				return;
			}
			if (wasFiltered)
				out = "Message was filtered for profanity.\n";
			else
				out = "Message sent";
		}

		// Input
		SceNpMatching2LobbyId m_lobbyId;

		// Output
		bool wasFiltered;
	};

	__L2_MSG_DB_HEADER(Console_SearchRooms, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_SearchRooms::DebugMsg(out);
				return;
			}
			out.Set("%s: startIndex=%i. total=%i. size=%i.\n", GetName(), startIndex, total, size);
			for (unsigned int i=0; i < roomNames.Size(); i++)
			{
#if defined(_WIN32)
				out += RakNet::RakString("\n%i. roomId=%I64u", i+1, roomIds[i]);
#else
				out += RakNet::RakString("\n%i. roomId=%llu", i+1, roomIds[i]);
#endif
				out += RakNet::RakString(". roomName=%s\n", roomNames[i].C_String());
			}
		}

		// Lobby to search. 0 for unused (no lobbies). Don't have 0 for both world and lobby.
		SceNpMatching2LobbyId m_lobbyId;
		// World to search. 0 for unused (no worlds). Don't have 0 for both world and lobby.
		SceNpMatching2WorldId m_worldId;
		// Passed to SceNpMatching2RangeFilter startIndex. If the total number of results is > max, can read at an offset
		// Use SCE_NP_MATCHING2_RANGE_FILTER_START_INDEX_MIN if you don't care
		unsigned int m_search_room_startIndex;
		// Passed to SceNpMatching2RangeFilter max. Maximum number of elements to obtain
		// Use SCE_NP_MATCHING2_RANGE_FILTER_MAX if you don't care
		unsigned int rangeFilterMax;
		// If true, uses SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_RANDOM
		bool returnRandomResults;
		// Applied as filters to the query. Pointers should point to data held elsewhere. Scope does not need to exist past call to Lobby2Client_PS3::SendMessage
		// You can only have one binary field, and 8 integer fields
		// Binary field must have byteLength < SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_MAX_SIZE
		DataStructures::List<DataStructures::Table::FilterQuery *> filterQueries;

		// Output
		unsigned int startIndex, total, size;
		DataStructures::List<RakNet::RakString> roomNames;
		DataStructures::List<uint64_t> roomIds;
	};

	__L2_MSG_DB_HEADER(Console_GetRoomDetails, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);


		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_GetRoomDetails::DebugMsg(out);
				return;
			}
			out.Set("%s: roomName=%s.\nroomDataExternalNum=%i. size=%i.\n", GetName(), roomName.C_String(),roomDataExternalNum);
			out += RakNet::RakString("curMemberNum=%i. maxSlot=%i. roomSearchableIntAttrExternalNum=%i\n", curMemberNum, maxSlot, roomSearchableIntAttrExternalNum);
			out += RakNet::RakString("roomSearchableBinAttrExternalNum=%i. roomBinAttrExternalNum=%i.\n", roomSearchableBinAttrExternalNum, roomBinAttrExternalNum);
		}

		// Input
		SceNpMatching2RoomId roomId;

		// Output
		RakNet::RakString roomName;
		int roomDataExternalNum;
		unsigned int curMemberNum, maxSlot, roomSearchableIntAttrExternalNum, roomSearchableBinAttrExternalNum, roomBinAttrExternalNum;
	};

	__L2_MSG_DB_HEADER(Console_GetLobbyMemberData, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_GetLobbyMemberData::DebugMsg(out);
				return;
			}
			out.Set("%s: targetHandle=%s.\n", GetName(), targetHandle.C_String());
		}

		// Input
		SceNpMatching2LobbyId m_lobbyId;
		SceNpMatching2LobbyMemberId lobbyMemberId;

		// Output
		RakNet::RakString targetHandle;
	};

	__L2_MSG_DB_HEADER(Console_CreateRoom, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_CreateRoom::DebugMsg(out);
				return;
			}
#if defined(_WIN32)
			out.Set("%s: roomId=%I64u\n", GetName(), roomId);
#else
			out.Set("%s: roomId=%llu\n", GetName(), roomId);
#endif

			out+= RakNet::RakString("m_room_me=%i. m_room_owner=%i.\n", m_room_me, m_room_owner);

			out += RakNet::RakString("%i room members.\n", roomMemberNames.Size());
			for (unsigned int i=0; i < roomMemberNames.Size(); i++)
			{
				out += RakNet::RakString("\n%i. roomMemberId=%i. Name=%s", i+1, roomMemberIds[i], roomMemberNames[i].C_String());
			}
		}

		// Input
		SceNpMatching2LobbyId m_lobbyId;
		SceNpMatching2WorldId m_worldId;
		int room_total_slots;
		bool hidden;
		// Room groups. Use 0 for none. 2 for 2. // Group labels are 0,1,2,etc.
		int numGroups;
		// Each bit indictes if the password applies to this slow. Only used when numGroups==0 and there is a password
		unsigned int passwordSlotMaskBits;
		RakNet::RakString password;
	
		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId m_room_me;
		SceNpMatching2RoomMemberId m_room_owner;
		DataStructures::List<RakNet::RakString> roomMemberNames;
		DataStructures::List<SceNpMatching2RoomMemberId> roomMemberIds;
	};

	__L2_MSG_DB_HEADER(Console_SetRoomSearchProperties, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_SetRoomSearchProperties::DebugMsg(out);
				return;
			}
		}

		// Input
		DataStructures::List<DataStructures::Table::Cell *> searchProperties;
		SceNpMatching2RoomId roomId;

	};

	__L2_MSG_DB_HEADER(Console_UpdateRoomParameters, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_UpdateRoomParameters::DebugMsg(out);
				return;
			}
		}

		// Input
		bool isHidden;
		bool isClosed;
		SceNpMatching2RoomId roomId;
		int passwordSlotMaskBits;
	};

	__L2_MSG_DB_HEADER(Console_JoinRoom, PS3)
	{
		Console_JoinRoom_PS3() {joinGroupId=-1;}
		virtual bool ClientImpl( Lobby2Client *client);


		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Console_JoinRoom::DebugMsg(out);
				return;
			}


			out+= RakNet::RakString("m_room_me=%i. m_room_owner=%i.\n", m_room_me, m_room_owner);

			out += RakNet::RakString("%i room members.\n", roomMemberNames.Size());
			for (unsigned i=0; i < roomMemberNames.Size(); i++)
			{
				out += RakNet::RakString("\n%i. roomMemberId=%i. Name=%s", i+1, roomMemberIds[i], roomMemberNames[i].C_String());
			}
		}

		// Input
		SceNpMatching2RoomId roomId;
		// -1 for unset
		int joinGroupId;

		// Output
		SceNpMatching2RoomMemberId m_room_me;
		SceNpMatching2RoomMemberId m_room_owner;
		DataStructures::List<RakNet::RakString> roomMemberNames;
		DataStructures::List<SceNpMatching2RoomMemberId> roomMemberIds;
	};

	__L2_MSG_DB_HEADER(Console_LeaveRoom, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpMatching2RoomId roomId;

		// Output
	};

	__L2_MSG_DB_HEADER(Console_SendLobbyInvitationToRoom, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpMatching2LobbyId		m_target_lobbyId;
		SceNpMatching2LobbyMemberId targetMemberId;
		SceNpMatching2RoomId roomId;

		// Output
	};

	__L2_MSG_DB_HEADER(Console_SendGUIInvitationToRoom, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpId *recipient;
		RakNet::RakString defaultSubject;
		RakNet::RakString defaultBody;

		// Output
	};

	__L2_MSG_DB_HEADER(Console_SendDataMessageToUser, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpId recipient;
		RakNet::BitStream bitStream;

		// sceNpBasicSendMessage

		// Output
	};

	__L2_MSG_DB_HEADER(Console_SendRoomChatMessage, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

		// Input
		SceNpMatching2RoomId roomId;

		// Output
	};

	__L2_MSG_DB_HEADER(Console_SetPresence, PS3)
	{
		virtual bool ClientImpl( Lobby2Client *client);

	};

	__L2_MSG_DB_HEADER(Friends_GetFriends, PS3)
	{
		Friends_GetFriends_PS3();
		virtual ~Friends_GetFriends_PS3();
		virtual bool ClientImpl( Lobby2Client *client);

		Lobby2ResultCode PopulateFriends(void);
		bool WasCompleted(void);

		// Output
		// Pointers are cleared in destructor. Use friends.Clear() to free to list so it isn't deallocated
		DataStructures::List<SceNpId*> friendIds;
	};


	__L2_MSG_DB_HEADER(Notification_Console_MemberJoinedLobby, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_MemberJoinedLobby::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("memberId=%I64u lobbyId=%I64u\n", memberId, lobbyId);
#else
			out += RakNet::RakString("memberId=%llu lobbyId=%llu\n", memberId, lobbyId);
#endif
		}

		// Output
		uint64_t memberId;
		SceNpMatching2LobbyId lobbyId;
	};

	__L2_MSG_DB_HEADER(Notification_Friends_StatusChange, PS3)
	{
		// Output
		SceNpUserInfo from;
	};

	__L2_MSG_DB_HEADER(Notification_Console_MemberLeftLobby, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_MemberLeftLobby::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("memberId=%I64u lobbyId=%I64u\n", memberId, lobbyId);
#else
			out += RakNet::RakString("memberId=%llu lobbyId=%llu\n", memberId, lobbyId);
#endif
		}

		// Output
		uint64_t memberId;
		SceNpMatching2LobbyId lobbyId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_LobbyDestroyed, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_LobbyDestroyed::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("lobbyId=%I64u\n", lobbyId);
#else
			out += RakNet::RakString("lobbyId=%llu\n", lobbyId);
#endif
		}

		// Output
		SceNpMatching2LobbyId lobbyId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_LobbyMemberDataUpdated, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_LobbyMemberDataUpdated::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("lobbyId=%I64u\n", lobbyId);
#else
			out += RakNet::RakString("lobbyId=%llu\n", lobbyId);
#endif
		}

		// Output
		SceNpMatching2LobbyId lobbyId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_LobbyGotChatMessage, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_LobbyGotChatMessage::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("lobbyId=%I64u ", lobbyId);
#else
			out += RakNet::RakString("lobbyId=%llu ", lobbyId);
#endif
			out += RakNet::RakString("srcMemberId=%i\n", srcMemberId);
		}

		// Output
		SceNpMatching2LobbyId lobbyId;
		SceNpMatching2LobbyMemberId     srcMemberId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_LobbyGotRoomInvitation, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_LobbyGotRoomInvitation::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
#if defined(_WIN32)
			out += RakNet::RakString("lobbyId=%I64u ", lobbyId);
#else
			out += RakNet::RakString("lobbyId=%llu ", lobbyId);
#endif
			out += RakNet::RakString("roomId=%i ", roomId);
			out += RakNet::RakString("srcMemberId=%i\n", srcMemberId);
		}

		// Output
		SceNpMatching2LobbyId lobbyId;
		SceNpMatching2LobbyMemberId     srcMemberId;
		SceNpMatching2RoomId roomId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_MemberJoinedRoom, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_MemberJoinedRoom::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i ", roomId);
			out += RakNet::RakString("srcMemberId=%i\n", srcMemberId);
		}


		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId     srcMemberId;

	};

	__L2_MSG_DB_HEADER(Notification_Console_MemberLeftRoom, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_MemberLeftRoom::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i ", roomId);
			out += RakNet::RakString("srcMemberId=%i\n", srcMemberId);
		}


		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId     srcMemberId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_KickedOutOfRoom, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_KickedOutOfRoom::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i\n", roomId);
		}


		// Output
		SceNpMatching2RoomId roomId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_RoomWasDestroyed, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_RoomWasDestroyed::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i\n", roomId);
		}

		// Output
		SceNpMatching2RoomId roomId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_RoomOwnerChanged, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_RoomOwnerChanged::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("prevOwner=%i newOwner=%i\n", prevOwner, newOwner);
		}

		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId prevOwner;
		SceNpMatching2RoomMemberId newOwner;
	};

	__L2_MSG_DB_HEADER(Notification_Console_RoomChatMessage, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_RoomChatMessage::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i srcMemberId=%i\n", roomId,srcMemberId);
		}


		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId     srcMemberId;

	};

	__L2_MSG_DB_HEADER(Notification_Console_RoomMessage, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_RoomMessage::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i srcMemberId=%i\n", roomId,srcMemberId);
		}

		// Output
		SceNpMatching2RoomId roomId;
		SceNpMatching2RoomMemberId     srcMemberId;
	};

	__L2_MSG_DB_HEADER(Notification_Console_RoomMemberConnectivityUpdate, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_RoomMemberConnectivityUpdate::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("roomId=%i\npeerMemberId=%i wasConnected=%i\n", roomId,peerMemberId,isNowEstablished);
			out += systemAddress.ToString(true);
			out += "\n";
		}

		// Output
		SceNpMatching2RoomId           roomId;
		SceNpMatching2RoomMemberId     peerMemberId;
		SceNpId npId;
		bool isNowEstablished;
	};

	__L2_MSG_DB_HEADER(Notification_Console_ChatEvent, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_Console_ChatEvent::DebugMsg(out);
				return;
			}

			out+= RakNet::RakString("%s: ", GetName());
			switch (extendedResultCode)
			{
			case CELL_AVC2_EVENT_LOAD_SUCCEEDED:
				out+="LOAD_SUCCEEDED";
				break;
			case CELL_AVC2_EVENT_LOAD_FAILED:
				out+="LOAD_FAILED";
				break;                     
			case CELL_AVC2_EVENT_JOIN_SUCCEEDED:
				out+="JOIN_SUCCEEDED";
				break;
			case CELL_AVC2_EVENT_JOIN_FAILED:
				out+="JOIN_FAILED";
				break;
			case CELL_AVC2_EVENT_LEAVE_SUCCEEDED:
				out+="LEAVE_SUCCEEDED";
				break;
			case CELL_AVC2_EVENT_LEAVE_FAILED:
				out+="LEAVE_FAILED";
				break;
			case CELL_AVC2_EVENT_UNLOAD_SUCCEEDED:
				out+="UNLOAD_SUCCEEDED";
				break;
			case CELL_AVC2_EVENT_UNLOAD_FAILED:
				out+="UNLOAD_FAILED";
				break;
			case CELL_AVC2_EVENT_SYSTEM_NEW_MEMBER_JOINED:
				out+="SYSTEM_NEW_MEMBER_JOINED";
				break;
			case CELL_AVC2_EVENT_SYSTEM_MEMBER_LEFT:
				out+="SYSTEM_MEMBER_LEFT";
				break;
			case CELL_AVC2_EVENT_SYSTEM_SESSION_ESTABLISHED:
				out+="SYSTEM_SESSION_ESTABLISHED";
				break;
			case CELL_AVC2_EVENT_SYSTEM_SESSION_CANNOT_ESTABLISHED:
				out+="SYSTEM_SESSION_CANNOT_ESTABLISHED";
				break;
			case CELL_AVC2_EVENT_SYSTEM_SESSION_DISCONNECTED:
				out+="SYSTEM_SESSION_DISCONNECTED";
				break;
			case CELL_AVC2_EVENT_SYSTEM_VOICE_DETECTED:
				out+="SYSTEM_VOICE_DETECTED";
				break;
			}
		}
	};

	__L2_MSG_DB_HEADER(Notification_ReceivedDataMessageFromUser, PS3)
	{
		virtual void DebugMsg(RakNet::RakString &out) const
		{
			if (resultCode!=L2RC_SUCCESS)
			{
				Notification_ReceivedDataMessageFromUser::DebugMsg(out);
				return;
			}

			out += RakNet::RakString("%s: ", GetName());
			out += RakNet::RakString("%s send us %i bytes of binary data\n", sender.handle.data, bitStream.GetNumberOfBytesUsed());
		}

		// Output
		SceNpId sender;
		RakNet::BitStream bitStream;
	};


	// --------------------------------------------- Database specific factory class for all messages --------------------------------------------

#define __L2_MSG_FACTORY_IMPL(__NAME__,__DB__) {case L2MID_##__NAME__ : Lobby2Message *m = RakNet::OP_NEW< __NAME__##_##__DB__ >(__FILE__, __LINE__) ; return m;}

	struct Lobby2MessageFactory_PS3 : public Lobby2MessageFactory
	{
		Lobby2MessageFactory_PS3() {}
		virtual ~Lobby2MessageFactory_PS3() {}
		virtual Lobby2Message *Alloc(Lobby2MessageID id)
		{
			switch (id)
			{
				__L2_MSG_FACTORY_IMPL(Client_Login, PS3);
				__L2_MSG_FACTORY_IMPL(Client_Logoff, PS3);
				__L2_MSG_FACTORY_IMPL(Client_PerTitleIntegerStorage, PS3);
				__L2_MSG_FACTORY_IMPL(Client_PerTitleBinaryStorage, PS3);
				__L2_MSG_FACTORY_IMPL(Friends_GetFriends, PS3);
				__L2_MSG_FACTORY_IMPL(Console_GetServerStatus, PS3);
				__L2_MSG_FACTORY_IMPL(Console_GetWorldListFromServer, PS3);
				__L2_MSG_FACTORY_IMPL(Console_GetLobbyListFromWorld, PS3);
				__L2_MSG_FACTORY_IMPL(Console_JoinLobby, PS3);
				__L2_MSG_FACTORY_IMPL(Console_LeaveLobby, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SendLobbyChatMessage, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SearchRooms, PS3);
				__L2_MSG_FACTORY_IMPL(Console_GetRoomDetails, PS3);
				__L2_MSG_FACTORY_IMPL(Console_GetLobbyMemberData, PS3);
				__L2_MSG_FACTORY_IMPL(Console_CreateRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SetRoomSearchProperties, PS3);
				__L2_MSG_FACTORY_IMPL(Console_UpdateRoomParameters, PS3);
				__L2_MSG_FACTORY_IMPL(Console_JoinRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Console_LeaveRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SendLobbyInvitationToRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SendGUIInvitationToRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SendDataMessageToUser, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SendRoomChatMessage, PS3);
				__L2_MSG_FACTORY_IMPL(Console_SetPresence, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Friends_StatusChange, PS3);
				__L2_MSG_FACTORY_BASE(Notification_Console_CableDisconnected);
				__L2_MSG_FACTORY_BASE(Notification_Console_ContextError);
				__L2_MSG_FACTORY_IMPL(Notification_Console_MemberJoinedLobby, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_MemberLeftLobby, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_LobbyDestroyed, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_LobbyMemberDataUpdated, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_LobbyGotChatMessage, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_LobbyGotRoomInvitation, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_MemberJoinedRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_MemberLeftRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_KickedOutOfRoom, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_RoomWasDestroyed, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_RoomOwnerChanged, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_RoomChatMessage, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_RoomMessage, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_RoomMemberConnectivityUpdate, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_Console_ChatEvent, PS3);
				__L2_MSG_FACTORY_IMPL(Notification_ReceivedDataMessageFromUser, PS3);

			default:
				return Lobby2MessageFactory::Alloc(id);
			};
		};
	};
}; // namespace RakNet

#endif

#endif // #ifdef SN_TARGET_PS3