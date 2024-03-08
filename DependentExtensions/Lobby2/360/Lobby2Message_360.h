#include "Lobby2Message.h"

#ifndef __LOBBY_2_MESSAGE_360_H
#define __LOBBY_2_MESSAGE_360_H

namespace RakNet
{


#define __L2_MSG_DB_HEADER(__NAME__,__DB__) \
	struct __NAME__##_##__DB__ : public __NAME__

	__L2_MSG_DB_HEADER(Client_Login, 360)
	{
		Client_Login_360();
		virtual bool ClientImpl( Lobby2Client *client);
	};

	__L2_MSG_DB_HEADER(Client_Logoff, 360)
	{
		Client_Logoff_360();
		virtual bool ClientImpl( Lobby2Client *client);
	};

	__L2_MSG_DB_HEADER(Console_CreateRoom, 360)
	{
		Console_CreateRoom_360();
		virtual bool ClientImpl( Lobby2Client *client);

		// In
		DWORD sessionFlagsToXSessionCreate;
		DWORD localConsoleUserIndexCreatingRoom;

			// Out
		ULONGLONG hostSessionNonce;
		HANDLE sessionIdentificationHandle;
	//	XOVERLAPPED        *m_Overlapped;
		XSESSION_INFO      m_SessionInfo;
	};

	__L2_MSG_DB_HEADER(Console_JoinRoom, 360)
	{
		Console_JoinRoom_360();
		virtual bool ClientImpl( Lobby2Client *client);

		// In
		DWORD sessionFlagsToXSessionCreate;
		DWORD localConsoleUserIndexJoiningRoom;
		XSESSION_INFO      *m_SessionInfo;
		ULONGLONG *hostSessionNonce;
		int publicSlots;
		int privateSlots;

		// Out
		HANDLE *sessionIdentificationHandle;
	//	XOVERLAPPED        *m_Overlapped;
	};

	__L2_MSG_DB_HEADER(Console_SearchRoomsInLobby, 360)
	{
		Console_SearchRoomsInLobby_360();
		~Console_SearchRoomsInLobby_360();
		virtual bool ClientImpl( Lobby2Client *client);

		// In
		// Data must remain valid until callback returns
		DataStructures::List<XUSER_PROPERTY> searchProperties;
		DataStructures::List<XUSER_CONTEXT> searchContexts;
		DWORD sessionOwner;
		int dwProcedureIndex; // The index of the matchmaking query, which is defined in XLAST. It is available in the spa.h file. 
		int maxResultsToReturn;

		// Out
		// On callback success, m_pSearchResults is allocated and is up to the user to deallocate when no longer needed
		// Use FreeSearchResults to do so
		PXSESSION_SEARCHRESULT_HEADER m_pSearchResults;
		static void FreeSearchResults(PXSESSION_SEARCHRESULT_HEADER header);

	};

	__L2_MSG_DB_HEADER(Notification_Console_LobbyGotRoomInvitation, 360)
	{
		DWORD owner;
		XINVITE_INFO inviteInfo;
	};

	__L2_MSG_DB_HEADER(Notification_Console_MuteListChanged, 360)
	{
	};

	__L2_MSG_DB_HEADER(Notification_Console_Local_Users_Changed, 360)
	{
		// How many users are signed in? From 0 to XUSER_MAX_COUNT
		int numberOfSignedInPlayers;

		// For each user, what is their new controller index, XUID, and handle?
		unsigned int usedControllerIndex[XUSER_MAX_COUNT];
		XUID userXUID[XUSER_MAX_COUNT];
		WCHAR userHandle[ XUSER_MAX_COUNT ][ XUSER_NAME_SIZE ];
	};


	// --------------------------------------------- Database specific factory class for all messages --------------------------------------------

#define __L2_MSG_FACTORY_IMPL(__NAME__,__DB__) {case L2MID_##__NAME__ : Lobby2Message *m = RakNet::OP_NEW< __NAME__##_##__DB__ >() ; return m;}

	struct Lobby2MessageFactory_360 : public Lobby2MessageFactory
	{
		Lobby2MessageFactory_360() {}
		virtual ~Lobby2MessageFactory_360() {}
		virtual Lobby2Message *Alloc(Lobby2MessageID id)
		{
			switch (id)
			{
				__L2_MSG_FACTORY_IMPL(Client_Login, 360);
				__L2_MSG_FACTORY_IMPL(Client_Logoff, 360);
				__L2_MSG_FACTORY_IMPL(Console_CreateRoom, 360);
				__L2_MSG_FACTORY_IMPL(Console_JoinRoom, 360);
				__L2_MSG_FACTORY_IMPL(Console_SearchRoomsInLobby, 360);

				__L2_MSG_FACTORY_IMPL(Notification_Console_LobbyGotRoomInvitation, 360);
				__L2_MSG_FACTORY_IMPL(Notification_Console_MuteListChanged, 360);
				__L2_MSG_FACTORY_IMPL(Notification_Console_Local_Users_Changed, 360);



			default:
				return Lobby2MessageFactory::Alloc(id);
			};
		};
	};
}; // namespace RakNet

#endif
