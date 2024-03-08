#ifndef __LOBBY_2_CLIENT_STEAM_H
#define __LOBBY_2_CLIENT_STEAM_H

#include "Lobby2Plugin.h"
#include "DS_Multilist.h"
#include "SocketLayer.h"
#include "steam_api.h"

namespace RakNet
{
struct Lobby2Message;

class RAK_DLL_EXPORT Lobby2Client_Steam : public RakNet::Lobby2Plugin, public SocketLayerOverride
{
public:	
	Lobby2Client_Steam(const char *gameVersion);
	virtual ~Lobby2Client_Steam();

	/// Send a command to the server
	/// \param[in] msg The message that represents the command
	virtual void SendMsg(Lobby2Message *msg);

	// Base class implementation
	virtual void Update(void);

	void OnLobbyMatchListCallback( LobbyMatchList_t *pCallback, bool bIOFailure );
	void OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure );
	void OnLobbyJoined( LobbyEnter_t *pCallback, bool bIOFailure );
	bool IsCommandRunning( Lobby2MessageID msgId );
	void GetRoomMembers(DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> &_roomMembers);
	const char * GetRoomMemberName(uint64_t memberId);
	bool IsRoomOwner(const CSteamID roomid);
	bool IsInRoom(void) const;
	uint64_t GetNumRoomMembers(const uint64_t roomid){return SteamMatchmaking()->GetNumLobbyMembers(roomid);}
	uint64 GetMyUserID(void){return SteamUser()->GetSteamID().ConvertToUint64();}
	const char* GetMyUserPersonalName(void) {return SteamFriends()->GetPersonaName();}
	
	void PunchTarget(uint64_t roomMemberId);
	void NotifyLeaveRoom(void);

	// Returns 0 if none
	uint64_t GetRoomID(void) const {return roomId;}

	/// Called when SendTo would otherwise occur.
	virtual int RakNetSendTo( SOCKET s, const char *data, int length, SystemAddress systemAddress );

	/// Called when RecvFrom would otherwise occur. Return number of bytes read. Write data into dataOut
	virtual int RakNetRecvFrom( const SOCKET sIn, RakPeer *rakPeerIn, char dataOut[ MAXIMUM_MTU_SIZE ], SystemAddress *senderOut, bool calledFromMainThread);


	// This is junk, but necessary because you have to use their system to send
	struct OutputSocket
	{
		SNetSocket_t socket;
		SystemAddress systemAddress;
		CSteamID steamIDRemote;
	};
	static bool RecvFrom(char *dataOut, int *dataLengthOut, SystemAddress *sender);
	static int SendTo(const char *dataIn, unsigned int dataLengthIn, SystemAddress recipient);
	static void AddWriteSocket(SNetSocket_t socket, SystemAddress systemAddress, CSteamID steamIDRemote);
	static void CloseWriteSocket(CSteamID steamIDRemote);
	static void CloseAllWriteSockets(void);
	static DataStructures::Multilist<ML_ORDERED_LIST, OutputSocket*, SystemAddress> outputSockets;
	static DataStructures::DefaultIndexType GetOutputSocketIndex(SystemAddress sa);
	static DataStructures::DefaultIndexType GetOutputSocketIndex(CSteamID id);
	
protected:
		Lobby2Client_Steam();
	void CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc);
	void PushDeferredCallback(Lobby2Message *msg);
	void CallRoomCallbacks(void);
	void NotifyNewMember(uint64_t memberId);
	void NotifyDroppedMember(uint64_t memberId);
	void CallCompletedCallbacks(void);

	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyDataUpdatedCallback, LobbyDataUpdate_t, m_CallbackLobbyDataUpdated );
	STEAM_CALLBACK( Lobby2Client_Steam, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackChatDataUpdate );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyChatMessage, LobbyChatMsg_t, m_CallbackChatMessageUpdate );
	STEAM_CALLBACK( Lobby2Client_Steam, OnSocketStatusCallback, SocketStatusCallback_t, m_SocketStatusCallback );

	DataStructures::Multilist<ML_UNORDERED_LIST, Lobby2Message *, uint64_t > deferredCallbacks;

	uint64_t roomId;
	DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> roomMembers;
	DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> rooms;

	// DataStructures::Multilist<ML_UNORDERED_LIST, SNetSocket_t> punchInProgress;


	void CreatePunchListenSocket(void);
	void ClearRoom(void);
	static SNetSocket_t m_hSocketServer;

	RakNet::RakString versionString;
};
};

#endif
