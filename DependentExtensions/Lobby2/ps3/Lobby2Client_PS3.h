#ifdef SN_TARGET_PS3

#ifndef __LOBBY_2_CLIENT_PS3_H
#define __LOBBY_2_CLIENT_PS3_H

#include "Lobby2Plugin.h"
#include "DS_OrderedList.h"
#include <sys/process.h>
#include <cell/sysmodule.h>
#include <netex/net.h>
#include <netex/libnetctl.h>
#include <sysutil_common.h>
#include <sysutil_sysparam.h>
#include <cell/dbgfont.h>
#include <cell/pad.h>
#include <np.h>
#include <sysutil/sysutil_avc2.h>
#include "SimpleMutex.h"

namespace RakNet
{
struct Lobby2Message;

struct RoomMemberInfo
{
	SceNpMatching2RoomMemberId roomMemberId;
	SceNpId npId;
	bool isRoomOwner;
	bool isAvcStreamingTarget;
	SystemAddress systemAddress;
	RakNet::RakString groupLabel;
};

struct Room
{
	Room();
	~Room();
	SceNpMatching2RoomId roomId;
	DataStructures::List<RoomMemberInfo*> roomMembers;
	void AddRoomMember(SceNpMatching2RoomMemberId _memberId, SceNpId *_npId, bool _isRoomOwner, RakNet::RakString groupLabel);
	void RemoveRoomMember(const SceNpMatching2RoomMemberId &roomMemberId);
};

enum FriendStatusContext
{
	FRIEND_STATUS_OFFLINE,
	FRIEND_STATUS_PRESENCE,
	FRIEND_STATUS_OUT_OF_CONTEXT,
};

struct FriendStatus
{
	SceNpId userId;
	FriendStatusContext friendStatus;
	RakNet::BitStream presenceInfo;
};

class RAK_DLL_EXPORT Lobby2Client_PS3 : public RakNet::Lobby2Plugin
{
public:	
	Lobby2Client_PS3();
	virtual ~Lobby2Client_PS3();

	/// Send a command to the server
	/// \param[in] msg The message that represents the command
	virtual void SendMessage(Lobby2Message *msg);

	/// Same as SendMessage()
	/// Also calls Dealloc on the message factory
	virtual void SendMsgAndDealloc(Lobby2Message *msg);

	// Base class implementation
	virtual void Update(void);

	// Returns if titleUseStorage is initialized
	bool WasTUSInitialized(void) const;

	/// \internal
	static void sysutil_callback( uint64_t status, uint64_t param, void * userdata );
	
	static int Lobby2MessageComp( const unsigned int &key, Lobby2Message * const &data );

	int GetServerIDs(SceNpMatching2ServerId *server_id, int arrayLength);

	void ShutdownMatching2(void);
	// Return 0 on match
	int CompareNpIds(const SceNpId *npid1, const SceNpId *npid2) const;
	const SceNpId* GetNpId(void) const {return &m_npId;}
	Room* GetRoom(SceNpMatching2RoomId roomId) const;

	// Internally tracks room state for your convenience
	bool IsInRoom(void) const;
	bool AreWeTheRoomOwner(const SceNpMatching2RoomId *roomid) const;
	unsigned int GetNumRoomMembers(const SceNpMatching2RoomId *roomid) const;
	RoomMemberInfo* GetRoomMemberAtIndex(unsigned int idx, const SceNpMatching2RoomId *roomid) const;
	RoomMemberInfo* GetRoomMemberById(SceNpMatching2RoomMemberId id, const SceNpMatching2RoomId *roomid) const;
	RoomMemberInfo* GetRoomMemberByNpId(SceNpId *id, const SceNpMatching2RoomId *roomid) const;
	RoomMemberInfo* GetRoomMemberByHandle(const char *handle, const SceNpMatching2RoomId *roomid) const;
	bool IsUserInRoom(const SceNpMatching2RoomId *roomid, const SceNpId *id);
	const SceNpMatching2RoomId* GetFirstRoomID(void) const;
	void SetStreamAVC2ToChatTarget(RoomMemberInfo *roomMemberInfo, bool stream);

	unsigned int GetNumFriends(void) const;
	FriendStatus* GetFriendAtIndex(unsigned int idx) const;
	FriendStatus* GetFriendByName(const char *name) const;
	bool IsUserFriend(const SceNpId *id) const;

	// Returns all members in the current lobby, and my own lobby member id
	// Return value is number of members returned
	// Negative values are error conditions
	int GetLobbyMemberID(SceNpMatching2LobbyId lobbyId, SceNpMatching2LobbyMemberId *lobbyMembers, int lobbyMembersLen, SceNpMatching2LobbyMemberId *me);
	SceNpMatching2LobbyMemberId GetMyLobbyMemberID(SceNpMatching2LobbyId lobbyId);

	SceNpMatching2ContextId		m_ctxId;

	static void
		matching2_signaling_cbfunc(
		SceNpMatching2ContextId        ctxId,	
		SceNpMatching2RoomId           roomId,
		SceNpMatching2RoomMemberId     peerMemberId,
		SceNpMatching2Event            event,
		int     errorCode,
		void   *arg
		);

	static void getServerInfoCb(SceNpMatching2ContextId	ctxId,
		SceNpMatching2RequestId	reqId,
		SceNpMatching2Event	event,
		SceNpMatching2EventKey	eventKey,
		int	errorCode,
		size_t	dataSize,
		void*	arg);

	static void getWorldInfoListCb(SceNpMatching2ContextId	ctxId,
		SceNpMatching2RequestId	reqId,
		SceNpMatching2Event		event,
		SceNpMatching2EventKey	eventKey,
		int		errorCode,
		size_t	dataSize,
		void*	arg);

	static void getLobbyInfoListCb(SceNpMatching2ContextId	ctxId,
		SceNpMatching2RequestId	reqId,
		SceNpMatching2Event		event,
		SceNpMatching2EventKey	eventKey,
		int		errorCode,
		size_t	dataSize,
		void*		arg);

	// --------- Lobby -------

	static void joinLobbyCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void leaveLobbyCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void sendLobbyChatMsgCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void searchRoomCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void getRoomDataExternalListCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Callback for GetLobbyMemberDataInternal/////////////////////////////////
	static void getLobbyMemberDataInternalCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);


	//--------------------------------------------------------------
	// Lobby callbacks
	//--------------------------------------------------------------
	static void matching2_lobby_event_cbfunc(SceNpMatching2ContextId         ctxId,
		SceNpMatching2LobbyId            lobbyId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void memberJoinedCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2LobbyId            lobbyId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void memberLeftCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2LobbyId            lobbyId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void lobbyDestroyedCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2LobbyId            lobbyId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	static void updatedLobbyMemberDataInternalCb(SceNpMatching2ContextId         ctxId,	
		SceNpMatching2LobbyId            lobbyId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//---------------------------------------------------------------------
	// Lobby message callbacks
	//---------------------------------------------------------------------
	static void matching2_lobby_message_cbfunc(SceNpMatching2ContextId        ctxId,
		SceNpMatching2LobbyId           lobbyId,
		SceNpMatching2LobbyMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void lobbyChatMessageCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2LobbyId           lobbyId,
		SceNpMatching2LobbyMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void lobbyInvitationCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2LobbyId           lobbyId,
		SceNpMatching2LobbyMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void matching2_room_message_cbfunc(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RoomId           roomId,
		SceNpMatching2RoomMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void matching2_room_event_cbfunc(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	// -------------------------------- ROOMS --------------------------------------

	static void sendRoomChatMsgCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Createjoin room callback////////////////////////////////////////
	static void createJoinRoomCb(	SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Join room callback//////////////////////////////////////////////
	static void joinRoomCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Leave room callback//////////////////////////////////////////////
	static void leaveRoomCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RequestId        reqId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//--------------------------------------------------------------
	// Room callbacks
	//--------------------------------------------------------------

	//Member joined room event callback//////////////////////////////////////
	static void memberJoinedRoomCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	//Member left room event callback//////////////////////////////////////
	static void memberLeftRoomCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Member(you) got kicked out event callback/////////////////////////////
	static void kickedoutCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Room destroyed event callback/////////////////////////////////////////
	static void roomDestroyedCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//Room owner changed event callback///////////////////////////////////////
	static void ownerChangedCb(SceNpMatching2ContextId         ctxId,
		SceNpMatching2RoomId            roomId,
		SceNpMatching2Event             event,
		SceNpMatching2EventKey          eventKey,
		int     errorCode,
		size_t  dataSize,
		void   *arg);

	//---------------------------------------------------------------------
	// Room message callbacks
	//---------------------------------------------------------------------

	//Room chat message callback/////////////////////////////////////////
	static void roomChatMessageCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RoomId           roomId,
		SceNpMatching2RoomMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	//Room message callback//////////////////////////////////////////////
	static void roomMessageCb(SceNpMatching2ContextId        ctxId,
		SceNpMatching2RoomId           roomId,
		SceNpMatching2RoomMemberId     srcMemberId,
		SceNpMatching2Event            event,
		SceNpMatching2EventKey         eventKey,
		int errorCode,
		size_t  dataSize,
		void   *arg);

	static void chat_eventcb( CellSysutilAvc2EventId event_id,
		CellSysutilAvc2EventParam event_param,
		void *userdata );

	static int basicCb(int event, int retCode, uint32_t reqId, void *arg);

protected:
	void PushCompletedCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc);
	void CallCompletedCallbacks(void);
	void ClearCompletedCallbacks(void);
	void PushDeferredCallback(Lobby2Message *msg);
	Lobby2Message *GetDeferredCallback(unsigned int requestId, bool peek);
	Lobby2Message *GetDeferredCallbackByMessageId(Lobby2MessageID messageId, bool peek);
	DataStructures::OrderedList<const unsigned int, Lobby2Message *, Lobby2MessageComp> deferredCallbacks;
	SimpleMutex completedCallbacksMutex;
	// Previously I was just calling the Lobby2Message callbacks directly
	// However, due to a design flaw and incorrect documentation for the PS3, some callbacks are called from their thread rather than when you call cellSysutilCheckCallback
	// This can cause crashes for obviously reasons. In particular, GFx 3.0 cannot handle calls from threads
	DataStructures::List<Lobby2Message *> completedCallbacks;

	static void contextCb(SceNpMatching2ContextId	ctxId,
		SceNpMatching2Event		event,
		SceNpMatching2EventCause	eventCause,
		int		errorCode,
		void*	arg);

	// Plugin interface functions
	void OnAttach(void);

	void OnEnteredRoom(SceNpMatching2RoomId room_id);
	void OnLeftRoom(SceNpMatching2RoomId roomId);

	int cellNetCtlNetStartDialogUnloadAsyncResultCode;
	
	bool titleUserStorageInitialized;
	int titleCtxId;
	void InitTUS(const SceNpCommunicationId *communicationId, const SceNpCommunicationPassphrase *passphrase);
	void ShutdownTUS();

	bool isAvc2JoinSucceeded;
	bool isAvc2Streaming;

	//NP ID of the user
	SceNpId 			m_npId;
	bool avc2Init;

	// People in the room
	// Room owner
	// Are we in a room?
	DataStructures::List<Room*> rooms;

	int GetRoomIndex(SceNpMatching2RoomId roomId) const;
	void ClearRooms(void);
	Room* AddRoom(SceNpMatching2RoomId roomId);

	void UpdateAVC2StreamTargets(void);


	DataStructures::List<FriendStatus*> friends;
	void ClearFriends(void);
	FriendStatus* AddFriend(SceNpId *_userId, FriendStatusContext status);
	void RemoveFriend(SceNpId *_userId);

	void GetBasicEvents(void);
};
};

#endif

#endif // #ifdef SN_TARGET_PS3