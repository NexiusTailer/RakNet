#include "Lobby2Client_Steam.h"
#include "Lobby2Message_Steam.h"
#include <stdlib.h>
#include "NativeTypes.h"
#include "MTUSize.h"
#include <windows.h>

using namespace RakNet;

SNetSocket_t Lobby2Client_Steam::m_hSocketServer;

DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(Lobby2Message,uint64_t,requestId);

Lobby2Client_Steam::Lobby2Client_Steam() :
m_CallbackLobbyDataUpdated( this, &Lobby2Client_Steam::OnLobbyDataUpdatedCallback ),
m_CallbackPersonaStateChange( this, &Lobby2Client_Steam::OnPersonaStateChange ),
m_CallbackLobbyDataUpdate( this, &Lobby2Client_Steam::OnLobbyDataUpdate ),
m_CallbackChatDataUpdate( this, &Lobby2Client_Steam::OnLobbyChatUpdate ),
m_CallbackChatMessageUpdate( this, &Lobby2Client_Steam::OnLobbyChatMessage ),
m_SocketStatusCallback( this, &Lobby2Client_Steam::OnSocketStatusCallback )
{

}

Lobby2Client_Steam::Lobby2Client_Steam(const char *gameVersion) :
m_CallbackLobbyDataUpdated( this, &Lobby2Client_Steam::OnLobbyDataUpdatedCallback ),
m_CallbackPersonaStateChange( this, &Lobby2Client_Steam::OnPersonaStateChange ),
m_CallbackLobbyDataUpdate( this, &Lobby2Client_Steam::OnLobbyDataUpdate ),
m_CallbackChatDataUpdate( this, &Lobby2Client_Steam::OnLobbyChatUpdate ),
m_CallbackChatMessageUpdate( this, &Lobby2Client_Steam::OnLobbyChatMessage ),
m_SocketStatusCallback( this, &Lobby2Client_Steam::OnSocketStatusCallback )
{
	roomId=0;
	m_hSocketServer=0;
	versionString=gameVersion;

}
Lobby2Client_Steam::~Lobby2Client_Steam()
{

}
void Lobby2Client_Steam::SendMsg(Lobby2Message *msg)
{
	if (msg->ClientImpl((Lobby2Client*) this))
	{
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}
	else
	{
		// Won't be deleted by the user's call to Deref.
		msg->resultCode=L2RC_PROCESSING;
		msg->AddRef();
		PushDeferredCallback(msg);
	}
}
void Lobby2Client_Steam::Update(void)
{
	SteamAPI_RunCallbacks();

	/*
	// sending data
	// must be a handle to a connected socket
	// data is all sent via UDP, and thus send sizes are limited to 1200 bytes; after this, many routers will start dropping packets
	// use the reliable flag with caution; although the resend rate is pretty aggressive,
	// it can still cause stalls in receiving data (like TCP)
	virtual bool SendDataOnSocket( SNetSocket_t hSocket, void *pubData, uint32 cubData, bool bReliable ) = 0;

	// receiving data
	// returns false if there is no data remaining
	// fills out *pcubMsgSize with the size of the next message, in bytes
	virtual bool IsDataAvailableOnSocket( SNetSocket_t hSocket, uint32 *pcubMsgSize ) = 0; 

	// fills in pubDest with the contents of the message
	// messages are always complete, of the same size as was sent (i.e. packetized, not streaming)
	// if *pcubMsgSize < cubDest, only partial data is written
	// returns false if no data is available
	virtual bool RetrieveDataFromSocket( SNetSocket_t hSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize ) = 0; 
	*/
}

void Lobby2Client_Steam::PushDeferredCallback(Lobby2Message *msg)
{
	deferredCallbacks.Push(msg, msg->requestId, __FILE__, __LINE__ );
}
void Lobby2Client_Steam::CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc)
{
	if (msg)
	{
		msg->resultCode=rc;
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}	
}
void Lobby2Client_Steam::OnLobbyMatchListCallback( LobbyMatchList_t *pCallback, bool bIOFailure )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		// Get any instance of Console_SearchRooms
		if (deferredCallbacks[i]->GetID()==L2MID_Console_SearchRooms)
		{
			Console_SearchRooms_Steam *callbackResult = (Console_SearchRooms_Steam *) deferredCallbacks[i];
			//			iterate the returned lobbies with GetLobbyByIndex(), from values 0 to m_nLobbiesMatching-1
			// lobbies are returned in order of closeness to the user, so add them to the list in that order
			for ( uint32 iLobby = 0; iLobby < pCallback->m_nLobbiesMatching; iLobby++ )
			{
				CSteamID steamId = SteamMatchmaking()->GetLobbyByIndex( iLobby );
				callbackResult->roomIds.Push(steamId, __FILE__, __LINE__ );
				RakNet::RakString s = SteamMatchmaking()->GetLobbyData( steamId, "name" );
				callbackResult->roomNames.Push(s, __FILE__, __LINE__ );
			}

			CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);
			break;
		}
	}
}
void Lobby2Client_Steam::OnLobbyDataUpdatedCallback( LobbyDataUpdate_t *pCallback )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_GetRoomDetails)
		{
			Console_GetRoomDetails_Steam *callbackResult = (Console_GetRoomDetails_Steam *) deferredCallbacks[i];
			if (callbackResult->roomId==pCallback->m_ulSteamIDLobby)
			{
				const char *pchLobbyName = SteamMatchmaking()->GetLobbyData( pCallback->m_ulSteamIDLobby, "name" );
				if ( pchLobbyName[0] )
				{
					callbackResult->roomName=pchLobbyName;
				}
				CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
				msgFactory->Dealloc(callbackResult);
				deferredCallbacks.RemoveAtIndex(i);
				break;
			}
		}
	}

	Console_GetRoomDetails_Steam notification;
	const char *pchLobbyName = SteamMatchmaking()->GetLobbyData( pCallback->m_ulSteamIDLobby, "name" );
	if ( pchLobbyName[0] )
	{
		notification.roomName=pchLobbyName;
	}
	notification.roomId=pCallback->m_ulSteamIDLobby;
	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_CreateRoom)
		{
			Console_CreateRoom_Steam *callbackResult = (Console_CreateRoom_Steam *) deferredCallbacks[i];
			callbackResult->roomId=pCallback->m_ulSteamIDLobby;
			SteamMatchmaking()->SetLobbyData( callbackResult->roomId, "name", callbackResult->roomName.C_String() );
			roomId=pCallback->m_ulSteamIDLobby;

			CreatePunchListenSocket();

			printf("\nNumber of Steam Lobby Members:%i in Lobby Name:%s\n", SteamMatchmaking()->GetNumLobbyMembers(roomId), callbackResult->roomName.C_String());
			roomMembers.Push(SteamMatchmaking()->GetLobbyOwner(roomId).ConvertToUint64());// GetLobbyMemberByIndex( roomId, 0 ).ConvertToUint64());
			
			CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);

			// Commented out: Do not send the notification for yourself
			// CallRoomCallbacks();
			break;
		}
	}
}
void Lobby2Client_Steam::OnLobbyJoined( LobbyEnter_t *pCallback, bool bIOFailure )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_JoinRoom)
		{
			Console_JoinRoom_Steam *callbackResult = (Console_JoinRoom_Steam *) deferredCallbacks[i];

			if (pCallback->m_EChatRoomEnterResponse==k_EChatRoomEnterResponseSuccess)
			{
				roomId=pCallback->m_ulSteamIDLobby;
				CreatePunchListenSocket();

				CallCBWithResultCode(callbackResult, L2RC_SUCCESS);

				// First push to prevent being notified of ourselves
				roomMembers.Push(SteamUser()->GetSteamID().ConvertToUint64());

				CallRoomCallbacks();

				// In case the asynch lobby update didn't get it fast enough
				if (roomMembers.GetIndexOf(SteamUser()->GetSteamID().ConvertToUint64())==-1)
					roomMembers.Push(SteamUser()->GetSteamID().ConvertToUint64());

				DataStructures::DefaultIndexType j;
				for (j=0; j < roomMembers.GetSize(); j++)
				{
					if (roomMembers[j]==SteamUser()->GetSteamID().ConvertToUint64())
						continue;
				}
			}
			else
			{
				CallCBWithResultCode(callbackResult, L2RC_Console_JoinRoom_NO_SUCH_ROOM);
			}

			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);
			break;
		}
	}
}
bool Lobby2Client_Steam::IsCommandRunning( Lobby2MessageID msgId )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==msgId)
		{
			return true;
		}
	}
	return false;
}

void Lobby2Client_Steam::OnPersonaStateChange( PersonaStateChange_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every friend who changes state
	// so make sure the user is in the lobby before acting
	if ( !SteamFriends()->IsUserInSource( pCallback->m_ulSteamID, roomId ) )
		return;

	if ((pCallback->m_nChangeFlags & k_EPersonaChangeNameFirstSet) ||
		(pCallback->m_nChangeFlags & k_EPersonaChangeName))
	{
		Notification_Friends_StatusChange_Steam notification;
		notification.friendId=pCallback->m_ulSteamID;
		const char *pchName = SteamFriends()->GetFriendPersonaName( notification.friendId );
		notification.friendNewName=pchName;
		CallCBWithResultCode(&notification, L2RC_SUCCESS);
	}
}
void Lobby2Client_Steam::OnLobbyDataUpdate( LobbyDataUpdate_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	if ( roomId != pCallback->m_ulSteamIDLobby )
		return;

	Notification_Console_UpdateRoomParameters_Steam notification;
	notification.roomId=roomId;
	notification.roomNewName=SteamMatchmaking()->GetLobbyData( roomId, "name" );
	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::OnLobbyChatUpdate( LobbyChatUpdate_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	if ( roomId != pCallback->m_ulSteamIDLobby )
		return;

	// Purpose: Handles users in the lobby joining or leaving ??????
	CallRoomCallbacks();	
}
void Lobby2Client_Steam::OnLobbyChatMessage( LobbyChatMsg_t *pCallback )
{
	CSteamID speaker;
	EChatEntryType entryType;
	char data[2048];
	int cubData=sizeof(data);
	int len = SteamMatchmaking()->GetLobbyChatEntry( roomId, pCallback->m_iChatID, &speaker, data, cubData, &entryType);
	if (entryType==k_EChatEntryTypeChatMsg)
	{
		Notification_Console_RoomChatMessage_Steam notification;
		notification.message=data;
		CallCBWithResultCode(&notification, L2RC_SUCCESS);
	}

}
void Lobby2Client_Steam::GetRoomMembers(DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> &_roomMembers)
{
	_roomMembers.Clear();
	int cLobbyMembers = SteamMatchmaking()->GetNumLobbyMembers( roomId );
	for ( int i = 0; i < cLobbyMembers; i++ )
	{
		CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex( roomId, i ) ;
		uint64_t memberid=steamIDLobbyMember.ConvertToUint64();
		_roomMembers.Push(memberid);
	}
}

const char * Lobby2Client_Steam::GetRoomMemberName(uint64_t memberId)
{
	return SteamFriends()->GetFriendPersonaName( memberId );
}

bool Lobby2Client_Steam::IsRoomOwner(CSteamID roomid)
{
	if(SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(roomid))
		return true;

	return false;
}

bool Lobby2Client_Steam::IsInRoom(void) const
{
	return roomMembers.GetSize() > 0;
}

void Lobby2Client_Steam::CallRoomCallbacks()
{
	DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> currentMembers;
	GetRoomMembers(currentMembers);

	DataStructures::DefaultIndexType currentMemberIndex=0, oldMemberIndex=0;
	while (currentMemberIndex < currentMembers.GetSize() && oldMemberIndex < roomMembers.GetSize())
	{
		if (currentMembers[currentMemberIndex]<roomMembers[oldMemberIndex])
		{
			// new member
			NotifyNewMember(currentMembers[currentMemberIndex]);

			currentMemberIndex++;

			
		}
		else if (currentMembers[currentMemberIndex]>roomMembers[oldMemberIndex])
		{
			// dropped member
			NotifyDroppedMember(roomMembers[oldMemberIndex]);
			oldMemberIndex++;

			
		}
		else
		{
			currentMemberIndex++;
			oldMemberIndex++;
		}
	}

	while (oldMemberIndex < roomMembers.GetSize())
	{
		// dropped member
		NotifyDroppedMember(roomMembers[oldMemberIndex]);

		oldMemberIndex++;
	}
	while (currentMemberIndex < currentMembers.GetSize())
	{
		// new member
		NotifyNewMember(currentMembers[currentMemberIndex]);

		currentMemberIndex++;
	}

	roomMembers=currentMembers;
}
void Lobby2Client_Steam::NotifyNewMember(uint64_t memberId)
{
	const char *pchName = SteamFriends()->GetFriendPersonaName( memberId );

	Notification_Console_MemberJoinedRoom_Steam notification;
	notification.roomId=roomId;
	notification.srcMemberId=memberId;
	notification.memberName=SteamFriends()->GetFriendPersonaName( memberId );

	CallCBWithResultCode(&notification, L2RC_SUCCESS);

	// Start punch to the new member
	PunchTarget(memberId);
}
void Lobby2Client_Steam::NotifyDroppedMember(uint64_t memberId)
{
	const char *pchName = SteamFriends()->GetFriendPersonaName( memberId );

	Notification_Console_MemberLeftRoom_Steam notification;
	notification.roomId=roomId;
	notification.srcMemberId=memberId;
	notification.memberName=SteamFriends()->GetFriendPersonaName( memberId );
	CallCBWithResultCode(&notification, L2RC_SUCCESS);

	CloseWriteSocket(memberId);
}
void Lobby2Client_Steam::ClearRoom(void)
{
	roomId=0; roomMembers.Clear();
	if (m_hSocketServer!=0)
	{
		if (SteamNetworking())
		{
			SteamNetworking()->DestroyListenSocket( m_hSocketServer, false );
			m_hSocketServer=0;
		}
	}
	CloseAllWriteSockets();
}
void Lobby2Client_Steam::CreatePunchListenSocket(void)
{
	if (m_hSocketServer==0)
	{
		m_hSocketServer = SteamNetworking()->CreateListenSocket( 0, 0, 0, true );
	}
}
void Lobby2Client_Steam::PunchTarget(uint64_t roomMemberId)
{
	if (roomMemberId < SteamUser()->GetSteamID().ConvertToUint64())
//	SNetSocket_t newSock =
		SteamNetworking()->CreateP2PConnectionSocket( roomMemberId, 0, 10, true );
//	printf("Adding %i into queue with size %i at line %i\n", newSock, punchInProgress.GetSize(), __LINE__);
//	punchInProgress.Push(newSock);
}
void Lobby2Client_Steam::OnSocketStatusCallback( SocketStatusCallback_t *pCallback )
{
	printf("Callback with m_hSocket=%i && m_hListenSocket=%i. m_hSocketServer=%i. State=%i\n", pCallback->m_hSocket, pCallback->m_hListenSocket, m_hSocketServer, pCallback->m_eSNetSocketState);

//	if ( m_hSocketServer && m_hSocketServer != pCallback->m_hListenSocket )
//		return;

//	DataStructures::DefaultIndexType i;
//	for (i=0; i < punchInProgress.GetSize(); i++)
//	{
//		if (punchInProgress[i]==pCallback->m_hSocket)
//		{
			switch (pCallback->m_eSNetSocketState)
			{
				case k_ESNetSocketStateConnected:
				{
//					punchInProgress.RemoveAtIndex(i);
					SocketLayer::Instance()->SetSocketLayerOverride(this);

					CSteamID pSteamIDRemote;
					int peSocketStatus;
					uint32 punIPRemote;
					uint16 punPortRemote;
					SteamNetworking()->GetSocketInfo(pCallback->m_hSocket, &pSteamIDRemote, &peSocketStatus, &punIPRemote, &punPortRemote );
					Notification_Console_RoomMemberConnectivityUpdate_Steam notification;
					notification.succeeded=true;
					notification.remoteSystem.binaryAddress=ntohl(punIPRemote);
					notification.remoteSystem.port=ntohs(punPortRemote);
					AddWriteSocket(pCallback->m_hSocket, notification.remoteSystem, pCallback->m_steamIDRemote);

					CallCBWithResultCode(&notification, L2RC_SUCCESS);
				}


				break;
					

			case k_ESNetSocketStateDisconnecting:
			case k_ESNetSocketStateLocalDisconnect:
			case k_ESNetSocketStateTimeoutDuringConnect:
			case k_ESNetSocketStateRemoteEndDisconnected:
			case k_ESNetSocketStateConnectionBroken:
				{
					switch (pCallback->m_eSNetSocketState)
					{
					case k_ESNetSocketStateDisconnecting:
						printf("k_ESNetSocketStateDisconnecting");
						break;
					case k_ESNetSocketStateLocalDisconnect:
						printf("k_ESNetSocketStateLocalDisconnect");
						break;
					case k_ESNetSocketStateTimeoutDuringConnect:
						printf("k_ESNetSocketStateTimeoutDuringConnect");
						break;
					case k_ESNetSocketStateRemoteEndDisconnected:
						printf("k_ESNetSocketStateRemoteEndDisconnected");
						break;
					case k_ESNetSocketStateConnectionBroken:
						printf("k_ESNetSocketStateConnectionBroken");
						break;
					}
					printf("\n");

//					printf("Removing %i from queue with size %i at index %i line %i\n", punchInProgress[i], punchInProgress.GetSize(), i, __LINE__);
//					unsigned int oldSize=punchInProgress.GetSize();
//					punchInProgress.RemoveAtIndex(i);
//					RakAssert(oldSize==punchInProgress.GetSize()-1);
					Notification_Console_RoomMemberConnectivityUpdate_Steam notification;
					notification.succeeded=false;
					CallCBWithResultCode(&notification, L2RC_SUCCESS);
					SteamNetworking()->DestroySocket( pCallback->m_hSocket, false );
					break;
				}

			case k_ESNetSocketStateInitiated:
				{
					printf("k_ESNetSocketStateInitiated\n");
				}
				break;

			case k_ESNetSocketStateLocalCandidatesFound:
				{
					printf("k_ESNetSocketStateLocalCandidatesFound\n");
				}
				break;
				
			}

//			break;
//		}
//	}
}
void Lobby2Client_Steam::NotifyLeaveRoom(void)
{
	ClearRoom();
}

int Lobby2Client_Steam::RakNetSendTo( SOCKET s, const char *data, int length, SystemAddress systemAddress )
{
	return RakNet::Lobby2Client_Steam::SendTo(data,length,systemAddress);
}

int Lobby2Client_Steam::RakNetRecvFrom( const SOCKET sIn, RakPeer *rakPeerIn, char dataOut[ MAXIMUM_MTU_SIZE ], SystemAddress *senderOut, bool calledFromMainThread)
{
	int dataLengthOut;
	bool success = RakNet::Lobby2Client_Steam::RecvFrom(dataOut, &dataLengthOut, senderOut);
	if (success)
		return dataLengthOut;
	return 0;
}

DataStructures::Multilist<ML_ORDERED_LIST, Lobby2Client_Steam::OutputSocket*, SystemAddress> Lobby2Client_Steam::outputSockets;
DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(Lobby2Client_Steam::OutputSocket, SystemAddress,systemAddress)

bool Lobby2Client_Steam::RecvFrom(char *dataOut, int *dataLengthOut, SystemAddress *sender)
{
	DataStructures::DefaultIndexType i;
	uint32_t size;
	*sender=UNASSIGNED_SYSTEM_ADDRESS;

	for (i=0; i < outputSockets.GetSize(); i++)
	{
		if (SteamNetworking() && SteamNetworking()->IsDataAvailableOnSocket(outputSockets[i]->socket, &size))
		{
			if (SteamNetworking()->RetrieveDataFromSocket(outputSockets[i]->socket, dataOut, MAXIMUM_MTU_SIZE, &size))
			{
				*dataLengthOut=(unsigned int) size;
				*sender=outputSockets[i]->systemAddress;
				return true;
			}
		}
	}
	
	return false;
}
int Lobby2Client_Steam::SendTo(const char *dataIn, unsigned int dataLengthIn, SystemAddress recipient)
{
	DataStructures::DefaultIndexType idx =  GetOutputSocketIndex(recipient);
	if (idx==-1)
		return -1;

	bool success = SteamNetworking()->SendDataOnSocket(outputSockets[idx]->socket, (void*) dataIn, dataLengthIn, false);
	RakAssert(success);
	return 1;
}
void Lobby2Client_Steam::AddWriteSocket(SNetSocket_t socket, SystemAddress systemAddress, CSteamID steamIDRemote)
{
	RakAssert(GetOutputSocketIndex(systemAddress)==-1);
	OutputSocket *os = RakNet::OP_NEW<OutputSocket>(__FILE__,__LINE__);
	os->socket=socket;
	os->systemAddress=systemAddress;
	os->steamIDRemote=steamIDRemote;
	outputSockets.Push(os,systemAddress,__FILE__,__LINE__);
}
void Lobby2Client_Steam::CloseWriteSocket(CSteamID steamIDRemote)
{
	DataStructures::DefaultIndexType idx = GetOutputSocketIndex(steamIDRemote);
	if (idx==-1)
		return;
	SteamNetworking()->DestroySocket(outputSockets[idx]->socket, false);
	RakNet::OP_DELETE(outputSockets[idx],__FILE__,__LINE__);
	outputSockets.RemoveAtIndex(idx,__FILE__,__LINE__);
}
void Lobby2Client_Steam::CloseAllWriteSockets(void)
{
	DataStructures::DefaultIndexType idx;
	for (idx=0; idx < outputSockets.GetSize(); idx++)
	{
		SteamNetworking()->DestroySocket(outputSockets[idx]->socket, false);
		RakNet::OP_DELETE(outputSockets[idx],__FILE__,__LINE__);
	}
	outputSockets.Clear(false, __FILE__,__LINE__);
}
DataStructures::DefaultIndexType Lobby2Client_Steam::GetOutputSocketIndex(SystemAddress sa)
{
	return outputSockets.GetIndexOf(sa);
}
DataStructures::DefaultIndexType Lobby2Client_Steam::GetOutputSocketIndex(CSteamID id)
{
	DataStructures::DefaultIndexType idx;
	for (idx=0; idx < outputSockets.GetSize(); idx++)
	{
		if (outputSockets[idx]->steamIDRemote==id)
			return idx;
	}
	return -1;
}
