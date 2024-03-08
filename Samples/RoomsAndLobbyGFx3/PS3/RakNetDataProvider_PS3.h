
#ifndef INC_RakNetDataProvider_PS3_H
#define INC_RakNetDataProvider_PS3_H

// RakNet information
#include "RakNetTypes.h"
#include "RakString.h"
#include "Lobby2Message.h"
#include "RakNetTime.h"
#include "RakString.h"
#include "DS_OrderedList.h"
#include "LobbyDataProvider.h"
#include "RoomsPlugin.h"

#ifndef LOBBY_NO_ONLINE

#include "GHash.h"
#include "GMsgFormat.h"
#include "GFxString.h"

// Utility macros to convert between GFx types and char
#ifdef GSI_UNICODE
#define GSC_TYPE wchar_t
#define GSC_CAST_IN(x) (const unsigned short*)L ## x
#define GSC_CAST_OUT(x) (const wchar_t*) ## x
#else
#define GSC_TYPE char
#define GSC_CAST_IN(x) (const char*)(x)
#define GSC_CAST_OUT(x) (const char*) ## x
#endif


namespace RakNet
{
	class Lobby2Client;
	struct Lobby2MessageFactory;
}

struct PlayerBrowserData
{
	RakNet::RakString name; // Player's name
	unsigned char team; // Player's team (not sure if the LobbyDataProvider class supports more than 0 and 1)
	int score;
	unsigned int deaths;
	unsigned int ping; // Ping to the game host? Average ping?

	void Serialize(RakNet::BitStream *bitStream, bool writeToBitstream);
};

/// This is data that should be set with RakPeerInterface::SetOfflinePingResponse()
/// It should also be able to write this data to a LobbyServer instance
struct ServerAndRoomBrowserData
{
	RakNet::RakString gameName;
	unsigned short maxPlayers;
	RakNet::RakString mapName;
	RakNet::RakString gameMode;
	bool serverHasAPassword;
	RakNet::RakString autoBalanced; // Write the string you want to show up in the table. This might be restricted to "ON" or "OFF" ?
	RakNet::RakString gameVersion; // Write the string you want to show up in the table
	RakNet::RakString mapSize; // Write the string you want to show up in the table
	RakNet::RakString friendlyFire; // Write the string you want to show up in the table. This might be restricted to "ON" or "OFF" ?
	DataStructures::List<PlayerBrowserData> players; // Players in the game.

	/// For the LAN browser, it expects the offline ping response to be packed using the format from this structure
	/// Therefore, to advertise that your server is available or updated, fill out the parameters in this structure, then call SetAsOfflinePingResponse()
	/// When your server is no longer available, set the offline ping response to NULL
	void SetAsOfflinePingResponse(RakPeerInterface *rakPeer);

	/// Write to a RakNet table data structure, useful when creating the room or setting the room properties
	/// Writes everything EXCEPT the list of players
	void WriteToTable(DataStructures::Table *table);

	/// \internal
	void Serialize(RakNet::BitStream *bitStream, bool writeToBitstream);
	/// \internal
	void WriteToLobbyServer(GPtr<LobbyServer> lobbyServer);

	// TODO - write to rooms server
};

/// This is information the system needs to work. For this demo, the define AUTOMANAGE_RAKNET_INSTANCES will setup
/// everything with defaults.
struct RakNetDataProviderExternalInterfaces
{
	/// \param[in] serverAddress Address of the server. If we are not connected by the time the user tries to login, a connection with no password will attempted
	SystemAddress serverAddress;
	/// \param[in] externallyAllocatedLobby2Client Lobby2Client plugin, used to login and get friends information
	RakNet::Lobby2Client *externallyAllocatedLobby2Client;
	/// \param[in] externallyAllocatedLobby2MessageFactory Message factory for the Lobby2Client
	RakNet::Lobby2MessageFactory *externallyAllocatedLobby2MessageFactory;
	/// \param[in] externallyAllocatedRoomsPlugin RoomsPlugin module, used to work with rooms on the server
	RakNet::RoomsPlugin *externallyAllocatedRoomsPlugin;
	/// \param[in] titleName Optional - If the login is for a particular game, rather than a generic lobby, you can enter it here. See System_CreateTitle
	RakNet::RakString titleName;
	/// \param[in] titleSecretKey Optional - If the login is for a particular game, and the game has a secret key, enter it here
	RakNet::RakString titleSecretKey;
	/// \param[in] When searching the LAN, a ping will be sent to this port. Any system that returns IP_PONG will be listed.
	unsigned short wanServerPort;
};

class RakNetDataProvider_PS3 : public LobbyDataProvider, RakNet::Lobby2Callbacks, RakNet::RoomsCallback
{
public:
    RakNetDataProvider_PS3();
    ~RakNetDataProvider_PS3();

	/// Set externally allocated, required information
	/// The pointers should remain valid until Shutdown() is called.
	/// \param[in] externalInterfaces Information to be provided by the user about how to connect to the server
	void SetExternalInterfaces(const RakNetDataProviderExternalInterfaces &externalInterfaces);

	/// Start the system. Run() calls this automatically. If you don't use Run(), call it yourself before calling Tick() the first time, or after shutting down
	void Startup(void);

	/// Update system by one tick (the Run() thread does this automatically, but for single threaded, just call Tick() yourself)
	void Tick(void);

	/// Shutdown the system. Run() calls this automatically. If you don't use Run(), call it yourself after finishing with the system.
	void Shutdown(void);

	/// Utility function - write usable properties of NetworkedRoomCreationParameters to a LobbyServer for display in the games / rooms list
	void WriteRoomCreationParametersToLobbyServer( RakNet::CreateRoom_Func *callResult, GPtr<LobbyServer> lobbyServer);

	/// Utility function - write usable properties of NetworkedRoomCreationParameters to a LobbyServer for display in the games / rooms list
	void WriteRoomDescriptorToLobbyServer( RakNet::RoomDescriptor *roomDescriptor, GPtr<LobbyServer> lobbyServer);

	virtual UPInt       GetTotalServerCount(LobbyServer::NetworkType net);
	virtual void		OnBuddyPanelStateChange(bool buddyPanelIsAvailable);

	enum ConnectionState
	{
		UNKNOWN,
		CONNECTING_THEN_LOGIN,
		CONNECTING_THEN_CREATE_ACCOUNT,
		LOGGING_IN,
		LOGGED_IN,
		NOT_LOGGED_IN
	} connectionState;
	bool disconnectFromServerOnLogoff;

protected:

	// Entry point for threaded mode
	virtual SInt Run();

	// Process a specific command. This virtual method allows data provider 
	// implementations to handle commands differently.
	virtual void  ProcessCommand(LDPCommand* cmd);

	void RN_ConnectUser(LDPCommand* cmd);
	void RN_ConnectNewUser(LDPCommand* cmd);
	void RN_Logout();
	void RN_GetBuddyInfo(LDPCommand* cmd);
	void RN_SendMessage(LDPCommand* cmd);
	void RN_AddBuddy(LDPCommand* cmd);
	void RN_RemoveBuddy(LDPCommand* cmd);
	void RN_AcceptBuddyRequest(LDPCommand* cmd);
	void RN_DenyBuddyRequest(LDPCommand* cmd);
	void RN_PopulateServerList(LDPCommand* cmd);
	void RN_RefreshServer(LDPCommand* cmd);
	void RN_StopUpdatingServerList(LDPCommand* cmd);

	// Allocated and set by the user through calling SetExternalInterfaces()
	SystemAddress serverAddress;
	RakNet::Lobby2Client *lobby2Client;
	RakNet::Lobby2MessageFactory *msgFactory;
	RakNet::RoomsPlugin *roomsPlugin;
	RakPeerInterface *rakPeer;
	RakNet::RakString titleName;
	RakNet::RakString titleSecretKey;
	unsigned short lanServerPort;

	// When this time passes, signal complete to the UI
	// If 0, do not use
	RakNetTimeMS timeWhenWANUpdateFinished;

	void WriteTableToLobbyServer( DataStructures::Table *table, GPtr<LobbyServer> lobbyServer);

	// Allocated servers, need to keep track of them for WAN
	void AddServer(SystemAddress systemAddress, GPtr<LobbyServer> server);
	GPtr<LobbyServer> GetServer(SystemAddress systemAddress);
	GPtr<LobbyServer> AllocOrGetServer(SystemAddress systemAddress, LDPResult **r, LobbyServer::NetworkType network);
	void ClearServers(void);
	GHash< SystemAddress, GPtr<LobbyServer> >  ServerMap;

	void AddBuddy(RakNet::RakString buddyName, GPtr<LobbyBuddy> buddy);
	GPtr<LobbyBuddy> GetBuddy(RakNet::RakString buddyName);
	void ClearBuddies(void);
	static int BuddyComp( RakNet::RakString const &key, GPtr<LobbyBuddy> const &data );
	// GHash doesn't support strings as keys. I could use RakString::ToInteger(), which works, but there's no guarantee of non-collision
	DataStructures::OrderedList<RakNet::RakString, GPtr<LobbyBuddy>, BuddyComp> buddyList;

	virtual void RemoveBuddy(const LobbyBuddy* pbuddy);

#ifdef AUTOMANAGE_RAKNET_INSTANCES
	bool instancesWereAutocreated;
#endif

	// Uses loginUsername and loginPassword
	void SendLoginMessage(void);
	RakNet::RakString loginUsername;
	RakNet::RakString loginPassword;
	RakNet::RakString createAccountEmail;
	void ConnectToServer(ConnectionState newConnectionState);
	void SendRegisterAccountMessage(void);
	void OnFailedConnectionAttempt(MessageID messageId, SystemAddress systemAddress);
	const char* MessageIdToString(MessageID messageId) const;
	void OnNewConnection(MessageID messageId, SystemAddress systemAddress);
	void OnOfflinePong(Packet *packet);
	void OnDroppedConnection(Packet *packet);
	void DeleteBuddy(RakNet::RakString buddyName);
	void PushBuddyOnlineStatus(RakNet::RakString buddyName, bool isOnline, const RakNet::Lobby2Presence &presence);
	void SetBuddyPresence(LobbyBuddy* buddy, const RakNet::Lobby2Presence &presence);

	// Lobby2 callbacks
	virtual void MessageResult(RakNet::Client_Login *message);
	virtual void MessageResult(RakNet::Client_Logoff *message);
	virtual void MessageResult(RakNet::Client_RegisterAccount *message);
	virtual void MessageResult(RakNet::Friends_GetFriends *message);
	virtual void MessageResult(RakNet::Friends_GetInvites *message);
	virtual void MessageResult(RakNet::Notification_Friends_StatusChange *message);
	virtual void MessageResult(RakNet::Friends_SendInvite *message);
	virtual void MessageResult(RakNet::Friends_AcceptInvite *message);
	virtual void MessageResult(RakNet::Friends_RejectInvite *message);
	virtual void MessageResult(RakNet::Friends_Remove *message);
	virtual void MessageResult(RakNet::Notification_Friends_PresenceUpdate *message);
};

#endif  // LOBBY_NO_ONLINE

#endif  // INC_GameSpyDataProvider_H
