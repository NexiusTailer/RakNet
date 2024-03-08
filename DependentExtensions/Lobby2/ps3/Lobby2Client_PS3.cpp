#ifdef SN_TARGET_PS3

#include "Lobby2Client_PS3.h"
#include "Lobby2Message_PS3.h"
#include <stdlib.h>

#include "matching2_wrappers_Lobby2.h"

using namespace RakNet;

// This is bad, but no way around it.
// The notifications matching2_lobby_message_cbfunc always have 0 for the arg parameter, so there's no non-global way to get the pointer to Lobby2Client_PS3
static RakNet::Lobby2Client_PS3 *lobby2Client;

int Lobby2Client_PS3::Lobby2MessageComp( const unsigned int &key, Lobby2Message * const &data )
{
	if (key < data->requestId)
		return -1;
	if (key > data->requestId)
		return 1;
	return 0;
}

void Lobby2Client_PS3::GetBasicEvents(void)
{
	int eventCode;

	uint8_t buffer[SCE_NP_BASIC_MAX_MESSAGE_SIZE]; //Buffer to store incoming data
	size_t size;
	SceNpUserInfo from;
	int eventResult;
	FriendStatus *fs;

	do 
	{
		eventCode=SCE_NP_BASIC_EVENT_UNKNOWN;
		size = sizeof(buffer);
		eventResult = sceNpBasicGetEvent(
			&eventCode,
			&from,
			buffer,
			&size);
		// In ProDG, asserts crash the system and are not debuggable. Stupid
		if (!(eventResult==SCE_NP_BASIC_ERROR_NO_EVENT || eventResult==0))
		{
			printf("Error: eventResult==%X\n", eventResult);

		}

		// PS3 bug: If called in thread
		// Returned:
		// SCE_NP_BASIC_ERROR_NO_EVENT 0x8002a66a There is no event available
		// 	There are no events in the incoming event data queue. 
		// However, output parameters got changed correctly

		if (eventResult==0)
		{
			Notification_Friends_StatusChange_PS3 *notificationFriends = (Notification_Friends_StatusChange_PS3 *)
				lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Friends_StatusChange);
			switch (eventCode)
			{
			case SCE_NP_BASIC_EVENT_OFFLINE:

				AddFriend(&from.userId,FRIEND_STATUS_OFFLINE);

				notificationFriends->op=Notification_Friends_StatusChange::FRIEND_LOGGED_OFF;
				memcpy(&notificationFriends->from, &from, sizeof(from));
				notificationFriends->otherHandle=from.name.data;
				PushCompletedCBWithResultCode(notificationFriends, L2RC_SUCCESS);

				break;
			case SCE_NP_BASIC_EVENT_PRESENCE:

				fs = lobby2Client->AddFriend(&from.userId,FRIEND_STATUS_PRESENCE);
				fs->presenceInfo.Reset();
				fs->presenceInfo.WriteAlignedBytes(buffer,size);
				RakAssert(size>0);

				notificationFriends->op=Notification_Friends_StatusChange::FRIEND_LOGGED_IN;
				memcpy(&notificationFriends->from, &from, sizeof(from));
				notificationFriends->otherHandle=from.name.data;
				PushCompletedCBWithResultCode(notificationFriends, L2RC_SUCCESS);
				break;
			case SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT:

				AddFriend(&from.userId,FRIEND_STATUS_OUT_OF_CONTEXT);

				notificationFriends->op=Notification_Friends_StatusChange::FRIEND_LOGGED_IN_DIFFERENT_CONTEXT;
				memcpy(&notificationFriends->from, &from, sizeof(from));
				notificationFriends->otherHandle=from.name.data;
				PushCompletedCBWithResultCode(notificationFriends, L2RC_SUCCESS);

				break;
			case SCE_NP_BASIC_EVENT_FRIEND_REMOVED:

				RemoveFriend(&from.userId);

				notificationFriends->op=Notification_Friends_StatusChange::YOU_WERE_REMOVED_AS_A_FRIEND;
				memcpy(&notificationFriends->from, &from, sizeof(from));
				notificationFriends->otherHandle=from.name.data;
				PushCompletedCBWithResultCode(notificationFriends, L2RC_SUCCESS);

				break;
			case SCE_NP_BASIC_EVENT_MESSAGE:
				{
					Notification_ReceivedDataMessageFromUser_PS3 *notificationData = (Notification_ReceivedDataMessageFromUser_PS3 *)
						lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_ReceivedDataMessageFromUser);
					memcpy(&notificationData->sender, &from.userId, sizeof(from.userId));
					notificationData->bitStream.WriteAlignedBytes(buffer,size);
					PushCompletedCBWithResultCode(notificationData, L2RC_SUCCESS);
				}

				break;
			}

			msgFactory->Dealloc(notificationFriends);

		}
	} while (eventResult==0);

}

int Lobby2Client_PS3::basicCb(int event, int retCode, uint32_t reqId, void *arg)
{
	(void)reqId;
	(void)arg;

	lobby2Client->GetBasicEvents();

	return 0;
}

void Lobby2Client_PS3::chat_eventcb( CellSysutilAvc2EventId event_id,
				  CellSysutilAvc2EventParam event_param,
				  void *userdata )
{
	switch (event_id)
	{
	case CELL_AVC2_EVENT_LOAD_SUCCEEDED:
		break;
	case CELL_AVC2_EVENT_LOAD_FAILED:
		break;                     
	case CELL_AVC2_EVENT_JOIN_SUCCEEDED:
		lobby2Client->isAvc2JoinSucceeded=true;
//		cellSysutilAvc2StartStreaming();
		break;
	case CELL_AVC2_EVENT_JOIN_FAILED:
		break;
	case CELL_AVC2_EVENT_LEAVE_SUCCEEDED:
		break;
	case CELL_AVC2_EVENT_LEAVE_FAILED:
		break;
	case CELL_AVC2_EVENT_UNLOAD_SUCCEEDED:
		break;
	case CELL_AVC2_EVENT_UNLOAD_FAILED:
		break;
	case CELL_AVC2_EVENT_SYSTEM_NEW_MEMBER_JOINED:
		break;
	case CELL_AVC2_EVENT_SYSTEM_MEMBER_LEFT:
		break;
	case CELL_AVC2_EVENT_SYSTEM_SESSION_ESTABLISHED:
		break;
	case CELL_AVC2_EVENT_SYSTEM_SESSION_CANNOT_ESTABLISHED:
		break;
	case CELL_AVC2_EVENT_SYSTEM_SESSION_DISCONNECTED:
		break;
	case CELL_AVC2_EVENT_SYSTEM_VOICE_DETECTED:
		break;
	}

	Notification_Console_ChatEvent_PS3 *notification=
		(Notification_Console_ChatEvent_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_ChatEvent);
	notification->extendedResultCode=event_id;
	lobby2Client->PushCompletedCBWithResultCode(notification, L2RC_SUCCESS);
	lobby2Client->msgFactory->Dealloc(notification);

	lobby2Client->UpdateAVC2StreamTargets();
}

void
Lobby2Client_PS3::matching2_signaling_cbfunc(
						   SceNpMatching2ContextId        ctxId,	
						   SceNpMatching2RoomId           roomId,
						   SceNpMatching2RoomMemberId     peerMemberId,
						   SceNpMatching2Event            event,
						   int     errorCode,
						   void   *arg
						   )
{
	/*
	PRINTF("[ %s ]\n", __FUNCTION__);

	PRINTF("ctxId = %d\n", ctxId);	
	PRINTF("roomId = 0x%016llx\n", roomId);
	PRINTF("peerMemberId = 0x%x\n", peerMemberId);
	PRINTF("event = %s\n", sce_np_matching2_debug_get_event_name(event));
	PRINTF("errorCode = 0x%x\n", errorCode);
	PRINTF("arg = %p\n", arg);

	int ret = 0;
	int connState = 0;
	struct in_addr peerAddr;
	in_port_t peerPort = 0;
	struct matching2_info *m2Info = (struct matching2_info *)arg;
	SceNpMatching2RoomMemberDataInternal *member;

	if (event == SCE_NP_MATCHING2_SIGNALING_EVENT_Dead) {
		for (member = m2Info->joinedRoom->memberList.members;
			member != NULL;
			member = member->next) {
				if (member->memberId == peerMemberId) {
					matching2_draw_signaling_information(event, errorCode,
						member->userInfo.npId.handle.data);
					break;
				}
		}
	}
	if (event == SCE_NP_MATCHING2_SIGNALING_EVENT_Established) {
		memset(&peerAddr, 0, sizeof(peerAddr));
		ret = sceNpMatching2SignalingGetConnectionStatus(
			ctxId, roomId, peerMemberId,
			&connState, &peerAddr, &peerPort);
		if (ret < 0) {
			PRINTF("sceNpMatching2SignalingGetConnectionStatus() failed . ret = 0x%x\n", ret);
			return;
		}
		PRINTF("connState = %d\n", connState);
		PRINTF("peerPort : %d\n", ntohs(peerPort));
		PRINTF("peerAddr : %s\n", inet_ntoa(peerAddr));
	}
	*/

	Notification_Console_RoomMemberConnectivityUpdate_PS3 *notification=
		(Notification_Console_RoomMemberConnectivityUpdate_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_RoomMemberConnectivityUpdate);
	notification->extendedResultCode=errorCode;
	notification->isNowEstablished = (event == SCE_NP_MATCHING2_SIGNALING_EVENT_Established);
	notification->roomId=roomId;
	notification->peerMemberId=peerMemberId;


	int ret;
	SceNpSignalingConnectionInfo connInfo;

	memset (&connInfo, 0, sizeof (connInfo));
	ret = sceNpMatching2SignalingGetConnectionInfo(
		ctxId, roomId, peerMemberId, 
		SCE_NP_SIGNALING_CONN_INFO_PEER_ADDRESS,
		&connInfo);
	if (ret < 0) {
		//E Error handling
		lobby2Client->msgFactory->Dealloc(notification);
		return;
	}

	notification->systemAddress.port=connInfo.address.port;
	notification->systemAddress.binaryAddress=connInfo.address.addr.s_addr;
	memcpy(&notification->npId, &connInfo.npId, sizeof(connInfo.npId));

	RoomMemberInfo *rmi = lobby2Client->GetRoomMemberById(peerMemberId,&roomId);
	if (rmi)
		rmi->systemAddress=notification->systemAddress;

	lobby2Client->PushCompletedCBWithResultCode(notification, L2RC_SUCCESS);
	lobby2Client->msgFactory->Dealloc(notification);

	return;
}

void Lobby2Client_PS3::getServerInfoCb(SceNpMatching2ContextId	ctxId,
											  SceNpMatching2RequestId	reqId,
											  SceNpMatching2Event	event,
											  SceNpMatching2EventKey	eventKey,
											  int	errorCode,
											  size_t	dataSize,
											  void*	arg)
{
	(void)event;
	(void)reqId;
	Lobby2Client_PS3* instance_ptr = lobby2Client;

	Console_GetServerStatus_PS3 *lobby2Msg;
	lobby2Msg = (Console_GetServerStatus_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;

	if (errorCode < 0 || dataSize==0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Response data buffer/////////////////////
	SceNpMatching2GetServerInfoResponse respData;
	memset(&respData, 0, sizeof(respData));

	//Get response data///////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, &respData, dataSize);
	if(lobby2Msg->extendedResultCode < 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Save server info
	memcpy(&lobby2Msg->serverInfo, &respData.server, sizeof(respData.server));

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	instance_ptr->msgFactory->Dealloc(lobby2Msg);
}

void Lobby2Client_PS3::getWorldInfoListCb(SceNpMatching2ContextId	ctxId,
												 SceNpMatching2RequestId	reqId,
												 SceNpMatching2Event		event,
												 SceNpMatching2EventKey	eventKey,
												 int		errorCode,
												 size_t	dataSize,
												 void*	arg)
{
	(void)event;
	(void)reqId;
	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_GetWorldListFromServer_PS3 *lobby2Msg;
	lobby2Msg = (Console_GetWorldListFromServer_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		if (errorCode==0x80022B1E)
			instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_PASSWORD_IS_WRONG);
		else
			instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Response data buffer//////////////////////////////
	SceNpMatching2GetWorldInfoListResponse* respData = NULL;
	respData = (SceNpMatching2GetWorldInfoListResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	//Get response data/////////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if(lobby2Msg->extendedResultCode < 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		rakFree(respData);
		return;
	}

	// Copy out the world data to my own buffer
	lobby2Msg->response.worldNum=respData->worldNum;
	if (respData->worldNum>0)
	{
		lobby2Msg->response.world=RakNet::OP_NEW_ARRAY<SceNpMatching2World>(respData->worldNum, __FILE__, __LINE__);
		memcpy(lobby2Msg->response.world, respData->world, sizeof(SceNpMatching2World) * respData->worldNum);
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	//Free response data buffer
	rakFree(respData);

	instance_ptr->msgFactory->Dealloc(lobby2Msg);

	return;
}

void Lobby2Client_PS3::getLobbyInfoListCb(SceNpMatching2ContextId	ctxId,
												 SceNpMatching2RequestId	reqId,
												 SceNpMatching2Event		event,
												 SceNpMatching2EventKey	eventKey,
												 int		errorCode,
												 size_t	dataSize,
												 void*		arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_GetLobbyListFromWorld_PS3 *lobby2Msg;
	lobby2Msg = (Console_GetLobbyListFromWorld_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	SceNpMatching2GetLobbyInfoListResponse* respData = NULL;
	respData = (SceNpMatching2GetLobbyInfoListResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if(lobby2Msg->extendedResultCode < 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		rakFree(respData);
		return;
	}

	SceNpMatching2LobbyDataExternal* lobbyDataExternal;
	lobby2Msg->startIndex=respData->range.startIndex;
	lobby2Msg->total=respData->range.total;
	lobby2Msg->size=respData->range.size;
	lobbyDataExternal=respData->lobbyDataExternal;
	for(unsigned int i=0; i<respData->range.size; ++i)
	{
		lobby2Msg->lobbies.Insert(lobbyDataExternal->lobbyId, __FILE__, __LINE__);
		lobbyDataExternal = lobbyDataExternal->next;
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);

	rakFree(respData);
	return;
}

void Lobby2Client_PS3::contextCb(SceNpMatching2ContextId	ctxId,
								   SceNpMatching2Event		event,
								   SceNpMatching2EventCause	eventCause,
								   int		errorCode,
								   void*	arg)
{
	(void)ctxId;
	Lobby2Client_PS3* instance_ptr = lobby2Client;

	switch(event)
	{
	case SCE_NP_MATCHING2_CONTEXT_EVENT_StartOver:
		{
			// Disconnected, should logoff
			Notification_Console_ContextError *notification=
				(Notification_Console_ContextError *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_ContextError);
			notification->extendedResultCode=errorCode;
			if(eventCause == SCE_NP_MATCHING2_EVENT_CAUSE_NP_SIGNED_OUT)
			{
				instance_ptr->PushCompletedCBWithResultCode(notification, L2RC_Notification_ContextError_SignedOut);
				lobby2Client->ClearRooms();
				lobby2Client->ClearFriends();
			}
			else if(eventCause == SCE_NP_MATCHING2_EVENT_CAUSE_SYSTEM_ERROR)
			{
				instance_ptr->PushCompletedCBWithResultCode(notification, L2RC_Notification_ContextError_SystemError);
				lobby2Client->ClearRooms();
				lobby2Client->ClearFriends();
			}
			else
			{
				instance_ptr->PushCompletedCBWithResultCode(notification, L2RC_GENERAL_ERROR);
			}

			lobby2Client->msgFactory->Dealloc(notification);
		}
		break;

	default:
		break;
	}
}

void Lobby2Client_PS3::sysutil_callback( uint64_t status, uint64_t param, void * userdata )
{
	(void)param;
	Lobby2Client_PS3 *instance_ptr = (Lobby2Client_PS3*)userdata;

	switch(status)
	{
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		//	printf( "CELL_SYSUTIL_REQUEST_EXITGAME\n" );
		break;
	case CELL_SYSUTIL_NET_CTL_NETSTART_LOADED:
		break;
	case CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED:
		{
			// Finished can be finished logging in, or can come from unplugging the cable.
			int err;
			struct CellNetCtlNetStartDialogResult netstart_result;
			memset(&netstart_result, 0, sizeof(netstart_result));
			netstart_result.size = sizeof(netstart_result);
			err = cellNetCtlNetStartDialogUnloadAsync(&netstart_result);
			Lobby2Message *loginMsg;
			loginMsg = instance_ptr->GetDeferredCallbackByMessageId(L2MID_Client_Login, true);
			instance_ptr->cellNetCtlNetStartDialogUnloadAsyncResultCode=netstart_result.result;
			if (loginMsg)
				loginMsg->extendedResultCode=netstart_result.result;

			if (err!=0)
			{
				if (loginMsg)
				{
					instance_ptr->PushCompletedCBWithResultCode(loginMsg, L2RC_GENERAL_ERROR);
					instance_ptr->msgFactory->Dealloc(loginMsg);
				}
				else
				{
					Platform_Shutdown *notification=
						(Platform_Shutdown *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Platform_Shutdown);
					instance_ptr->PushCompletedCBWithResultCode(notification, L2RC_GENERAL_ERROR);
					lobby2Client->msgFactory->Dealloc(notification);
				}
				
				return;
			}

			// Proceeds later to CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED
		}
		break;
	case CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED:
		{
			Lobby2Message *msg = instance_ptr->GetDeferredCallbackByMessageId(L2MID_Client_Login, false);
			if (msg)
			{
				// Success, return result of Login()
				if (msg->extendedResultCode == CELL_NET_CTL_ERROR_DIALOG_CANCELED)
				{
					instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_Client_Login_CANCELLED);
				}
				else if (msg->extendedResultCode == CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED)
				{
					Notification_Console_CableDisconnected *notification=
						(Notification_Console_CableDisconnected *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_CableDisconnected);
					instance_ptr->PushCompletedCBWithResultCode(notification, L2RC_Notification_Console_CableDisconnected);
					instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_Client_Login_CABLE_NOT_CONNECTED);
					lobby2Client->msgFactory->Dealloc(notification);
				}
				else if (msg->extendedResultCode == CELL_NET_CTL_ERROR_NET_NOT_CONNECTED)
				{
					instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_Client_Login_NET_NOT_CONNECTED);
				}
				else if (msg->extendedResultCode != 0)
				{
					// Other error
					instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
				}
				else
				{
					// Success
					msg->extendedResultCode = sample_matching2_init(&instance_ptr->m_ctxId, &instance_ptr->m_npId, Lobby2Client_PS3::contextCb, instance_ptr, ((Client_Login_PS3*)msg)->npCommId, ((Client_Login_PS3*)msg)->npCommPassphrase);
					if(msg->extendedResultCode != CELL_OK)
					{
						instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_Client_Login_CONTEXT_CREATION_FAILURE);
					}
					else
					{
						// Register callbacks
						msg->extendedResultCode = sample_register_lobby_event_callback(instance_ptr->m_ctxId,
							Lobby2Client_PS3::matching2_lobby_event_cbfunc,
							NULL);
						if(msg->extendedResultCode != CELL_OK)
						{
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						msg->extendedResultCode = sample_register_lobby_message_callback(instance_ptr->m_ctxId,
							Lobby2Client_PS3::matching2_lobby_message_cbfunc,
							NULL);

						if(msg->extendedResultCode != CELL_OK)
						{
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						msg->extendedResultCode = sample_register_room_event_callback(instance_ptr->m_ctxId,
							Lobby2Client_PS3::matching2_room_event_cbfunc,
							NULL);
						if(msg->extendedResultCode != CELL_OK)
						{
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						//Register room message callback/////////////////////////////////////
						//Do this before joining or creating the room////////////////////////
						msg->extendedResultCode = sample_register_room_message_callback(instance_ptr->m_ctxId,
							Lobby2Client_PS3::matching2_room_message_cbfunc,
							NULL);
						if(msg->extendedResultCode != CELL_OK)
						{
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						msg->extendedResultCode = sceNpBasicRegisterContextSensitiveHandler(((Client_Login_PS3*)msg)->npCommId,
							basicCb, NULL);
						if(msg->extendedResultCode != CELL_OK)
						{
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						//J Signaling Callbackを設定
						msg->extendedResultCode = sceNpMatching2RegisterSignalingCallback(
							instance_ptr->m_ctxId, matching2_signaling_cbfunc, NULL);


						/* Must use cellSysutilAvc2InitParam() for clearing CellSysutilAvc2InitParam struct*/
						CellSysutilAvc2InitParam chat_avc2param;
						msg->extendedResultCode = cellSysutilAvc2InitParam(CELL_SYSUTIL_AVC2_INIT_PARAM_VERSION, &chat_avc2param);
						if( msg->extendedResultCode != CELL_OK )
						{
						//	ERR( "cellSysutilAvc2InitParam failed (0x%x)\n", ret );
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						/* Setting application specific parameters */
						chat_avc2param.media_type = CELL_SYSUTIL_AVC2_VOICE_CHAT;
						chat_avc2param.max_players = 8;
						chat_avc2param.voice_param.voice_quality = CELL_SYSUTIL_AVC2_VOICE_QUALITY_NORMAL;
						chat_avc2param.voice_param.max_speakers = 4;
						chat_avc2param.spu_load_average  = 100;

						msg->extendedResultCode = cellSysutilAvc2Load(
							instance_ptr->m_ctxId,
							SYS_MEMORY_CONTAINER_ID_INVALID,
							chat_eventcb,
							0,
							&chat_avc2param
							);

						if( msg->extendedResultCode != CELL_OK )
						{
							//	ERR( "cellSysutilAvc2InitParam failed (0x%x)\n", ret );
							instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_GENERAL_ERROR);
							instance_ptr->msgFactory->Dealloc(msg);
							return;
						}

						instance_ptr->avc2Init=true;

						if (((Client_Login_PS3*)msg)->supportTitleUserStorage)
							instance_ptr->InitTUS(((Client_Login_PS3*)msg)->npCommId, ((Client_Login_PS3*)msg)->npCommPassphrase);

						// User needs to know npId and ctxId
						instance_ptr->PushCompletedCBWithResultCode(msg, L2RC_SUCCESS);
					}
				}

				// Always dealloc after finishing with a deferred callback
				instance_ptr->msgFactory->Dealloc(msg);
			}

			lobby2Client->ClearRooms();
			lobby2Client->ClearFriends();
			
		}
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------------------------------------------------------------

//**************************************************************
// Function: sendLobbyChatMsgCb
// Description: This callback function is called when you send a
//		message.
//**************************************************************
void Lobby2Client_PS3::sendLobbyChatMsgCb(SceNpMatching2ContextId        ctxId,
										  SceNpMatching2RequestId        reqId,
										  SceNpMatching2Event            event,
										  SceNpMatching2EventKey         eventKey,
										  int     errorCode,
										  size_t  dataSize,
										  void   *arg)
{		
	(void)event;
	(void)arg;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_SendLobbyChatMessage_PS3 *lobby2Msg;
	lobby2Msg = (Console_SendLobbyChatMessage_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	SceNpMatching2SendLobbyChatMessageResponse *respData = NULL;
	respData = (SceNpMatching2SendLobbyChatMessageResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(respData);
		return;
	}

	// Copy out the meaningful data
	lobby2Msg->wasFiltered=respData->filtered;

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);
	rakFree(respData);


	return;
}


//**************************************************************
// Function: joinLobbyCb
// Description: This callback function is called when the user 
//              joins a lobby. The error code will indicate 
//              whether it was successful or not.
//**************************************************************
void Lobby2Client_PS3::joinLobbyCb(SceNpMatching2ContextId        ctxId,
								   SceNpMatching2RequestId        reqId,
								   SceNpMatching2Event            event,
								   SceNpMatching2EventKey         eventKey,
								   int     errorCode,
								   size_t  dataSize,
								   void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_JoinLobby_PS3 *lobby2Msg;
	lobby2Msg = (Console_JoinLobby_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;

		if (errorCode < 0)
		{
			if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_LOBBY_FULL)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinLobby_LOBBY_FULL);
			}
			else if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY_INSTANCE ||
				(uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinLobby_NO_SUCH_LOBBY);
			}
			else
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
			}
		}
		else
			instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);

		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Response data buffer//////////////////////////////////
	SceNpMatching2JoinLobbyResponse *respData = NULL;
	respData = (SceNpMatching2JoinLobbyResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	//Get response data///////////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(respData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->m_lobby_me = respData->lobbyDataInternal->memberIdList.me;
	for(size_t i=0; i<respData->lobbyDataInternal->memberIdList.memberIdNum; ++i)
	{
		lobby2Msg->memberIds.Insert(respData->lobbyDataInternal->memberIdList.memberId[i], __FILE__, __LINE__);
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);
	//Free response data
	rakFree(respData);

	return;
}

//**************************************************************
// Function: leaveLobbyCb
// Description: This callback function is called when the user 
//              leaves the lobby. The error code will indicate 
//              whether it was successful or not.
//**************************************************************
void Lobby2Client_PS3::leaveLobbyCb(SceNpMatching2ContextId        ctxId,
									SceNpMatching2RequestId        reqId,
									SceNpMatching2Event            event,
									SceNpMatching2EventKey         eventKey,
									int     errorCode,
									size_t  dataSize,
									void   *arg)
{
	(void)ctxId;
	(void)reqId;
	(void)event;
	(void)eventKey;
	(void)dataSize;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_LeaveLobby_PS3 *lobby2Msg;
	lobby2Msg = (Console_LeaveLobby_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);
	return;
}

//**************************************************************
// Function: searchRoomCb
// Description: This callback function is called when the user 
//              searches for rooms. The error code will indicate 
//              whether it was successful or not.
//**************************************************************
void Lobby2Client_PS3::searchRoomCb(SceNpMatching2ContextId        ctxId,
									SceNpMatching2RequestId        reqId,
									SceNpMatching2Event            event,
									SceNpMatching2EventKey         eventKey,
									int     errorCode,
									size_t  dataSize,
									void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_SearchRooms_PS3 *lobby2Msg;
	lobby2Msg = (Console_SearchRooms_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Get Response Data///////////////////////////////
	SceNpMatching2SearchRoomResponse *respData = NULL;
	respData = (SceNpMatching2SearchRoomResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->startIndex=respData->range.startIndex;
	lobby2Msg->total=respData->range.total;
	lobby2Msg->size=respData->range.size;
	SceNpMatching2RoomDataExternal* roomDataExternal;
	roomDataExternal = respData->roomDataExternal;
	for(size_t i=0; i<respData->range.size; ++i)
	{
		RakNet::RakString s;
		s = roomDataExternal->owner->onlineName->data;
		lobby2Msg->roomNames.Insert(s, __FILE__, __LINE__);
		lobby2Msg->roomIds.Insert(roomDataExternal->roomId, __FILE__, __LINE__);
		roomDataExternal = roomDataExternal->next;
	}


	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);
	//Free response buffer
	rakFree(respData);
	return;
}

//**************************************************************
// Function: GetRoomDataExternalListCb
// Description: This callback function is called when get_room_details()
//		is called.
//**************************************************************
void Lobby2Client_PS3::getRoomDataExternalListCb(SceNpMatching2ContextId        ctxId,
												 SceNpMatching2RequestId        reqId,
												 SceNpMatching2Event            event,
												 SceNpMatching2EventKey         eventKey,
												 int     errorCode,
												 size_t  dataSize,
												 void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_GetRoomDetails_PS3 *lobby2Msg;
	lobby2Msg = (Console_GetRoomDetails_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Get Response Data///////////////////////////////
	SceNpMatching2GetRoomDataExternalListResponse *respData = NULL;
	respData = (SceNpMatching2GetRoomDataExternalListResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(respData);
		lobby2Msg->extendedResultCode=lobby2Msg->extendedResultCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	if(respData->roomDataExternalNum == 0)
	{
		rakFree(respData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_GetRoomDetails_NO_ROOMS_FOUND);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}


	lobby2Msg->roomName=respData->roomDataExternal->owner->onlineName->data;
	lobby2Msg->roomId=respData->roomDataExternal->roomId;
	lobby2Msg->curMemberNum=respData->roomDataExternal->curMemberNum;
	lobby2Msg->maxSlot=respData->roomDataExternal->maxSlot;
	lobby2Msg->roomSearchableIntAttrExternalNum=respData->roomDataExternal->roomSearchableIntAttrExternalNum;
	lobby2Msg->roomSearchableBinAttrExternalNum=respData->roomDataExternal->roomSearchableBinAttrExternalNum;
	lobby2Msg->roomBinAttrExternalNum=respData->roomDataExternal->roomBinAttrExternalNum;


	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);
	//Free response buffer
	rakFree(respData);
	return;
}

//------------------------------------------------------------------------------------
// Lobby event callbacks
//------------------------------------------------------------------------------------


//**************************************************************
// Function: matching2_lobby_event_cbfunc
// Description: This callback function is called when there is
//              a lobby event. It then calls the appropriate
//		handler for each event.
//**************************************************************
void Lobby2Client_PS3::matching2_lobby_event_cbfunc(SceNpMatching2ContextId         ctxId,
													SceNpMatching2LobbyId            lobbyId,
													SceNpMatching2Event             event,
													SceNpMatching2EventKey          eventKey,
													int errorCode,
													size_t  dataSize,
													void   *arg)
{
	if (event == SCE_NP_MATCHING2_LOBBY_EVENT_MemberJoined)
		memberJoinedCb(ctxId, lobbyId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_LOBBY_EVENT_MemberLeft)
		memberLeftCb(ctxId, lobbyId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_LOBBY_EVENT_LobbyDestroyed)
		lobbyDestroyedCb(ctxId, lobbyId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_LOBBY_EVENT_UpdatedLobbyMemberDataInternal)
		updatedLobbyMemberDataInternalCb(ctxId, lobbyId, event, eventKey, errorCode, dataSize, arg);

	return;
}


//**************************************************************
// Function: memberJoinedCb
// Description: This callback function is called when someone else 
//              joins the lobby.
//**************************************************************
void Lobby2Client_PS3::memberJoinedCb(SceNpMatching2ContextId         ctxId,
									  SceNpMatching2LobbyId            lobbyId,
									  SceNpMatching2Event             event,
									  SceNpMatching2EventKey          eventKey,
									  int errorCode,
									  size_t  dataSize,
									  void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_MemberJoinedLobby_PS3 *lobby2Msg=
		(Notification_Console_MemberJoinedLobby_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_MemberJoinedLobby);

	lobby2Msg->lobbyId=lobbyId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer/////////////////////
	SceNpMatching2LobbyMemberUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2LobbyMemberUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data///////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(cbData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->memberId=cbData->lobbyMemberDataInternal->memberId;
	lobby2Msg->targetHandle=cbData->lobbyMemberDataInternal->userInfo.onlineName->data;

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}

//**************************************************************
// Function: memberLeftCb
// Description: This callback function is called when someone else 
//              leaves the lobby.
//**************************************************************
void Lobby2Client_PS3::memberLeftCb(SceNpMatching2ContextId         ctxId,
									SceNpMatching2LobbyId            lobbyId,
									SceNpMatching2Event             event,
									SceNpMatching2EventKey          eventKey,
									int     errorCode,
									size_t  dataSize,
									void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_MemberLeftLobby_PS3 *lobby2Msg=
		(Notification_Console_MemberLeftLobby_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_MemberLeftLobby);

	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->lobbyId=lobbyId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer///////////////////////////////////////
	SceNpMatching2LobbyMemberUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2LobbyMemberUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data//////////////////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(cbData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->memberId=cbData->lobbyMemberDataInternal->memberId;
	lobby2Msg->targetHandle=cbData->lobbyMemberDataInternal->userInfo.onlineName->data;

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);


	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}

//**************************************************************
// Function: lobbyDestroyedCb
// Description: This callback function is called when the lobby
//		gets destroyed.
//**************************************************************
void Lobby2Client_PS3::lobbyDestroyedCb(SceNpMatching2ContextId         ctxId,
										SceNpMatching2LobbyId            lobbyId,
										SceNpMatching2Event             event,
										SceNpMatching2EventKey          eventKey,
										int     errorCode,
										size_t  dataSize,
										void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_LobbyDestroyed_PS3 *lobby2Msg=
		(Notification_Console_LobbyDestroyed_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_LobbyDestroyed);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->lobbyId=lobbyId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer////////////////////////
	SceNpMatching2LobbyUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2LobbyUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data///////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(cbData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Free callback data
	rakFree(cbData);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}

//--------------------------------------------------------------------------
// Room Message Callbacks
//--------------------------------------------------------------------------

//**************************************************************
// Function: matching2_room_message_cbfunc
// Description: This callback function is called when there is a
//              message event in the room.
//		It calls the appropriate handler for each
//		message event.
//**************************************************************
void Lobby2Client_PS3::matching2_room_message_cbfunc(SceNpMatching2ContextId        ctxId,
							  SceNpMatching2RoomId           roomId,
							  SceNpMatching2RoomMemberId     srcMemberId,
							  SceNpMatching2Event            event,
							  SceNpMatching2EventKey         eventKey,
							  int errorCode,
							  size_t  dataSize,
							  void   *arg)
{
	if (event == SCE_NP_MATCHING2_ROOM_MSG_EVENT_ChatMessage)
		roomChatMessageCb(ctxId, roomId, srcMemberId, event, eventKey, errorCode, dataSize, arg);
	if (event == SCE_NP_MATCHING2_ROOM_MSG_EVENT_Message)
		roomMessageCb(ctxId, roomId, srcMemberId, event, eventKey, errorCode, dataSize, arg);

	return;
}

void Lobby2Client_PS3::updatedLobbyMemberDataInternalCb(SceNpMatching2ContextId         ctxId,	
														SceNpMatching2LobbyId            lobbyId,
														SceNpMatching2Event             event,
														SceNpMatching2EventKey          eventKey,
														int     errorCode,
														size_t  dataSize,
														void   *arg)
{
	(void)event;
	(void)lobbyId;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_LobbyMemberDataUpdated_PS3 *lobby2Msg=
		(Notification_Console_LobbyMemberDataUpdated_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_LobbyMemberDataUpdated);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->lobbyId=lobbyId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer/////////////////////////
	SceNpMatching2LobbyMemberDataInternalUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2LobbyMemberDataInternalUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data////////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(cbData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}

//--------------------------------------------------------------------------
// Lobby Message Callbacks
//--------------------------------------------------------------------------

//**************************************************************
// Function: matching2_lobby_message_cbfunc
// Description: This callback function is called when there is a
//              message event in the lobby.
//		It calls the appropriate handler for each
//		message event.
//**************************************************************
void Lobby2Client_PS3::matching2_lobby_message_cbfunc(SceNpMatching2ContextId        ctxId,
													  SceNpMatching2LobbyId           lobbyId,
													  SceNpMatching2LobbyMemberId     srcMemberId,
													  SceNpMatching2Event            event,
													  SceNpMatching2EventKey         eventKey,
													  int errorCode,
													  size_t  dataSize,
													  void   *arg)
{
	if (event == SCE_NP_MATCHING2_LOBBY_MSG_EVENT_ChatMessage)
		lobbyChatMessageCb(ctxId, lobbyId, srcMemberId, event, eventKey, errorCode, dataSize, arg);
	if (event == SCE_NP_MATCHING2_LOBBY_MSG_EVENT_Invitation)
		lobbyInvitationCb(ctxId, lobbyId, srcMemberId, event, eventKey, errorCode, dataSize, arg);

	return;
}

//**************************************************************
// Function: lobbyChatMessageCb
// Description: This callback function is called when there is a
//              chat message sent to the lobby.
//**************************************************************
void Lobby2Client_PS3::lobbyChatMessageCb(SceNpMatching2ContextId        ctxId,
										  SceNpMatching2LobbyId           lobbyId,
										  SceNpMatching2LobbyMemberId     srcMemberId,
										  SceNpMatching2Event            event,
										  SceNpMatching2EventKey         eventKey,
										  int errorCode,
										  size_t  dataSize,
										  void   *arg)
{
	(void)srcMemberId;
	(void)errorCode;
	(void)event;
	(void)arg;

	// arg is always 0, this is a notification!
	Lobby2Client_PS3 *instance_ptr = lobby2Client;

	Notification_Console_LobbyGotChatMessage_PS3 *lobby2Msg=
		(Notification_Console_LobbyGotChatMessage_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_LobbyGotChatMessage);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->lobbyId=lobbyId;
	lobby2Msg->srcMemberId=srcMemberId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Message info buffer////////////////////////////
	SceNpMatching2LobbyMessageInfo *msgInfo = NULL;
	msgInfo = (SceNpMatching2LobbyMessageInfo *)rakMalloc(dataSize);
	if(msgInfo == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(msgInfo, 0, dataSize);

	//Get message info data////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, msgInfo, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(msgInfo);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->sender=msgInfo->srcMember->npId.handle.data;
	lobby2Msg->message=(char*) msgInfo->msg;

	//Free message info buffer
	rakFree(msgInfo);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}

//**************************************************************
// Function: lobbyInvitationCb
// Description: This callback function is called when there you
//              receive an invitation sent in the lobby.
//**************************************************************
void Lobby2Client_PS3::lobbyInvitationCb(SceNpMatching2ContextId        ctxId,
										 SceNpMatching2LobbyId           lobbyId,
										 SceNpMatching2LobbyMemberId     srcMemberId,
										 SceNpMatching2Event            event,
										 SceNpMatching2EventKey         eventKey,
										 int errorCode,
										 size_t  dataSize,
										 void   *arg)
{

	(void)srcMemberId;
	(void)errorCode;
	(void)event;
	(void)arg;

	// arg is always 0 on notifications
	Lobby2Client_PS3 *instance_ptr = lobby2Client;
	Notification_Console_LobbyGotRoomInvitation_PS3 *lobby2Msg=
		(Notification_Console_LobbyGotRoomInvitation_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_LobbyGotRoomInvitation);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->lobbyId=lobbyId;
	lobby2Msg->srcMemberId=srcMemberId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	
	//Invitation info buffer///////////////////////////////
	SceNpMatching2LobbyInvitationInfo *invitationInfo = NULL;
	invitationInfo = (SceNpMatching2LobbyInvitationInfo *)rakMalloc(dataSize);
	if(invitationInfo == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(invitationInfo, 0, dataSize);

	//Get invitation info data//////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, invitationInfo, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(invitationInfo);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->sender=invitationInfo->srcMember->npId.handle.data;
	lobby2Msg->roomId=invitationInfo->invitationData.targetSession->roomId;

	//Free invitation info buffer
	rakFree(invitationInfo);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}

//**************************************************************
// Function: getLobbyMemberDataInternalCb
// Description: This callback function is called when we make a
//		request for lobbyMemberDataInternal of a lobby
//		member.
//**************************************************************
void Lobby2Client_PS3::getLobbyMemberDataInternalCb(SceNpMatching2ContextId        ctxId,
													SceNpMatching2RequestId        reqId,
													SceNpMatching2Event            event,
													SceNpMatching2EventKey         eventKey,
													int     errorCode,
													size_t  dataSize,
													void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_GetLobbyMemberData_PS3 *lobby2Msg;
	lobby2Msg = (Console_GetLobbyMemberData_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Response data buffer//////////////////////////
	SceNpMatching2GetLobbyMemberDataInternalResponse *respData = NULL;
	respData = (SceNpMatching2GetLobbyMemberDataInternalResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	//Get the event data///////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		rakFree(respData);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		return;
	}

	lobby2Msg->targetHandle=respData->lobbyMemberDataInternal->userInfo.onlineName->data;

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	instance_ptr->msgFactory->Dealloc(lobby2Msg);

	rakFree(respData);
	return;
}


// ----------------------------------------------ROOMS---------------------------------------------------------------

//---------------------------------------------------------------------------
// Matching callbacks
//---------------------------------------------------------------------------

//**************************************************************
// Function: sendRoomChatMsgCb
// Description: This callback function is called when you send a
//		message.
//**************************************************************
void Lobby2Client_PS3::sendRoomChatMsgCb(SceNpMatching2ContextId        ctxId,
										 SceNpMatching2RequestId        reqId,
										 SceNpMatching2Event            event,
										 SceNpMatching2EventKey         eventKey,
										 int     errorCode,
										 size_t  dataSize,
										 void   *arg)
{		
	(void)event;
	(void)arg;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_SendRoomChatMessage_PS3 *lobby2Msg;
	lobby2Msg = (Console_SendRoomChatMessage_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	SceNpMatching2SendRoomChatMessageResponse *respData = NULL;
	respData = (SceNpMatching2SendRoomChatMessageResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);

	rakFree(respData);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	return;
}

void Lobby2Client_PS3::createJoinRoomCb(	SceNpMatching2ContextId        ctxId,
										SceNpMatching2RequestId        reqId,
										SceNpMatching2Event            event,
										SceNpMatching2EventKey         eventKey,
										int     errorCode,
										size_t  dataSize,
										void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_CreateRoom_PS3 *lobby2Msg;
	lobby2Msg = (Console_CreateRoom_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Response data buffer//////////////////////////
	SceNpMatching2CreateJoinRoomResponse *respData = NULL;
	respData = (SceNpMatching2CreateJoinRoomResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	//Get the event data///////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		rakFree(respData);
		return;
	}

	lobby2Msg->roomId=respData->roomDataInternal->roomId;
	lobby2Msg->m_room_me = respData->roomDataInternal->memberList.me->memberId;
	lobby2Msg->m_room_owner = respData->roomDataInternal->memberList.owner->memberId;

	// Internal tracking of room members
	Room *room = instance_ptr->AddRoom(lobby2Msg->roomId);

	//Save room member list////////////////////////////////////////////
	SceNpMatching2RoomMemberDataInternal* roomMemberDataInternal;
	roomMemberDataInternal = respData->roomDataInternal->memberList.members;
	unsigned int num_members = respData->roomDataInternal->memberList.membersNum;
	RakNet::RakString groupLabel;
	if (roomMemberDataInternal->roomGroup)
		groupLabel=roomMemberDataInternal->roomGroup->label.data;
	for(size_t i=0; i<num_members; ++i)
	{
		RakNet::RakString roomMemberName;
		roomMemberName=roomMemberDataInternal->userInfo.onlineName->data;

		lobby2Msg->roomMemberNames.Insert(roomMemberName, __FILE__, __LINE__);
		//lobby2Msg->roomMemberIds.Insert(respData->roomDataInternal->memberList.owner->memberId);
		lobby2Msg->roomMemberIds.Insert(roomMemberDataInternal->memberId, __FILE__, __LINE__);

		bool isOwner=roomMemberDataInternal->memberId==respData->roomDataInternal->memberList.owner->memberId;
		room->AddRoomMember(roomMemberDataInternal->memberId,&roomMemberDataInternal->userInfo.npId,isOwner, groupLabel);

		roomMemberDataInternal = roomMemberDataInternal->next;
	}

	//Free response buffer
	rakFree(respData);

	// Entered a room
	instance_ptr->OnEnteredRoom(lobby2Msg->roomId);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	return;

}

//**************************************************************
// Function: joinRoomCb
// Description: This callback function is called when the user 
//              joins a room. The error code will indicate 
//              whether it was successful or not.
//**************************************************************
void Lobby2Client_PS3::joinRoomCb(SceNpMatching2ContextId        ctxId,
								  SceNpMatching2RequestId        reqId,
								  SceNpMatching2Event            event,
								  SceNpMatching2EventKey         eventKey,
								  int     errorCode,
								  size_t  dataSize,
								  void   *arg)
{
	(void)event;
	(void)reqId;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_JoinRoom_PS3 *lobby2Msg;
	lobby2Msg = (Console_JoinRoom_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0 || dataSize == 0)
	{
		//Error handling////////////////////////////////////
		if (errorCode < 0) {
			if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinRoom_ROOM_FULL);
			}
			else if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinRoom_WRONG_PASSWORD);
			}
			else if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM_INSTANCE ||
				(uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinRoom_NO_SUCH_ROOM);
			}
			else if((uint32_t)errorCode == SCE_NP_MATCHING2_SERVER_ERROR_BLOCKED)
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinRoom_SERVER_ERROR_BLOCKED);
			}
			else
			{
				instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_Console_JoinRoom_ROOM_FULL);
			}

			instance_ptr->msgFactory->Dealloc(lobby2Msg);
			return;
		}

		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}


	//Response data buffer//////////////////////////////////
	SceNpMatching2JoinRoomResponse *respData = NULL;
	respData = (SceNpMatching2JoinRoomResponse*)rakMalloc(dataSize);
	if(respData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(respData, 0, dataSize);

	//Get response data///////////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, respData, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		rakFree(respData);
		return;
	}

	lobby2Msg->roomId=respData->roomDataInternal->roomId;
	lobby2Msg->m_room_me=respData->roomDataInternal->memberList.me->memberId;
	lobby2Msg->m_room_owner=respData->roomDataInternal->memberList.owner->memberId;


	Room *room = instance_ptr->AddRoom(lobby2Msg->roomId);

	//Make room member list/////////////////////////////////////////////
	SceNpMatching2RoomMemberDataInternal* roomMemberDataInternal;
	roomMemberDataInternal = respData->roomDataInternal->memberList.members;
	unsigned int num_members = respData->roomDataInternal->memberList.membersNum;
	RakNet::RakString groupLabel;
	if (roomMemberDataInternal->roomGroup)
		groupLabel=roomMemberDataInternal->roomGroup->label.data;
	for(size_t i=0; i<num_members; ++i)
	{
		RakNet::RakString roomMemberName = roomMemberDataInternal->userInfo.onlineName->data;
		lobby2Msg->roomMemberIds.Insert(roomMemberDataInternal->memberId, __FILE__, __LINE__);
		lobby2Msg->roomMemberNames.Insert(roomMemberName, __FILE__, __LINE__);

		bool isOwner=roomMemberDataInternal->memberId==respData->roomDataInternal->memberList.owner->memberId;
		room->AddRoomMember(roomMemberDataInternal->memberId,&roomMemberDataInternal->userInfo.npId,isOwner,groupLabel);

		roomMemberDataInternal = roomMemberDataInternal->next;
	}

	//Free response data
	rakFree(respData);

	// Entered a room
	instance_ptr->OnEnteredRoom(lobby2Msg->roomId);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	return;
}
//**************************************************************
// Function: leaveRoomCb
// Description: This callback function is called when the user 
//              leaves the room. The error code will indicate 
//              whether it was successful or not.
//**************************************************************
void Lobby2Client_PS3::leaveRoomCb(SceNpMatching2ContextId        ctxId,
								   SceNpMatching2RequestId        reqId,
								   SceNpMatching2Event            event,
								   SceNpMatching2EventKey         eventKey,
								   int     errorCode,
								   size_t  dataSize,
								   void   *arg)
{
	(void)ctxId;
	(void)reqId;
	(void)event;
	(void)eventKey;
	(void)dataSize;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Console_LeaveRoom_PS3 *lobby2Msg;
	lobby2Msg = (Console_LeaveRoom_PS3 *) instance_ptr->GetDeferredCallback(reqId, false);
	lobby2Msg->extendedResultCode=errorCode;
	if (lobby2Msg==0)
		return;
	if (errorCode < 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		instance_ptr->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	// Leave room if in one
	instance_ptr->OnLeftRoom(lobby2Msg->roomId);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	return;
}


//------------------------------------------------------------------------------------
// Room event callbacks
//------------------------------------------------------------------------------------


//**************************************************************
// Function: matching2_room_event_cbfunc
// Description: This callback function is called when there is
//              a room event. It then calls the appropriate
//		handler for each event.
//**************************************************************
void Lobby2Client_PS3::matching2_room_event_cbfunc(SceNpMatching2ContextId         ctxId,
												   SceNpMatching2RoomId            roomId,
												   SceNpMatching2Event             event,
												   SceNpMatching2EventKey          eventKey,
												   int errorCode,
												   size_t  dataSize,
												   void   *arg)
{
	if (event == SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined)
		memberJoinedRoomCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft)
		memberLeftRoomCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_Kickedout)
		kickedoutCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_RoomOwnerChanged)
		ownerChangedCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);
	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed)
		roomDestroyedCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);

	//	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal)
	//	updateRoomDataInternalCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);
	//	else if (event == SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal)
	//	updateRoomMemberDataInternalCb(ctxId, roomId, event, eventKey, errorCode, dataSize, arg);

//	else
//		DEBUG_PRINTF("Unknown event: 0x%x\n", event);

	return;
}

void Lobby2Client_PS3::memberJoinedRoomCb(SceNpMatching2ContextId         ctxId,
				   SceNpMatching2RoomId            roomId,
				   SceNpMatching2Event             event,
				   SceNpMatching2EventKey          eventKey,
				   int errorCode,
				   size_t  dataSize,
				   void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_MemberJoinedRoom_PS3 *lobby2Msg=
		(Notification_Console_MemberJoinedRoom_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_MemberJoinedRoom);

	lobby2Msg->roomId=roomId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer/////////////////////
	SceNpMatching2RoomMemberUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2RoomMemberUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data///////////////////////////////
	int ret = 0;
	ret = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (ret < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(cbData);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->memberName=cbData->roomMemberDataInternal->userInfo.onlineName->data;
	lobby2Msg->srcMemberId=cbData->roomMemberDataInternal->memberId;

	RakNet::RakString groupLabel;
	if (cbData->roomMemberDataInternal->roomGroup)
		groupLabel=cbData->roomMemberDataInternal->roomGroup->label.data;
	Room *room = lobby2Client->GetRoom(roomId);
	room->AddRoomMember(lobby2Msg->srcMemberId,&cbData->roomMemberDataInternal->userInfo.npId,false,groupLabel);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}

//**************************************************************
// Function: memberLeftRoomCb
// Description: This callback function is called when someone else 
//              leaves the room.
//**************************************************************
void Lobby2Client_PS3::memberLeftRoomCb(SceNpMatching2ContextId         ctxId,
										SceNpMatching2RoomId            roomId,
										SceNpMatching2Event             event,
										SceNpMatching2EventKey          eventKey,
										int     errorCode,
										size_t  dataSize,
										void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_MemberLeftRoom_PS3 *lobby2Msg=
		(Notification_Console_MemberLeftRoom_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_MemberLeftRoom);
	lobby2Msg->roomId=roomId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer///////////////////////////////////////
	SceNpMatching2RoomMemberUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2RoomMemberUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data//////////////////////////////////////////
	int ret = 0;
	ret = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (ret < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		rakFree(cbData);
		return;
	}

	lobby2Msg->memberName=cbData->roomMemberDataInternal->userInfo.onlineName->data;
	lobby2Msg->srcMemberId=cbData->roomMemberDataInternal->memberId;


	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	Room *room = instance_ptr->GetRoom(roomId);
	room->RemoveRoomMember(lobby2Msg->srcMemberId);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}

//**************************************************************
// Function: kickedoutCb
// Description: This callback function is called when you get
//		kicked out of a room.
//**************************************************************
void Lobby2Client_PS3::kickedoutCb(SceNpMatching2ContextId         ctxId,
								   SceNpMatching2RoomId            roomId,
								   SceNpMatching2Event             event,
								   SceNpMatching2EventKey          eventKey,
								   int     errorCode,
								   size_t  dataSize,
								   void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_KickedOutOfRoom_PS3 *lobby2Msg=
		(Notification_Console_KickedOutOfRoom_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_KickedOutOfRoom);

	lobby2Msg->roomId=roomId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer////////////////////////////////
	SceNpMatching2RoomUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2RoomUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data///////////////////////////////////
	int ret = 0;
	ret = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (ret < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(cbData);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	// Leave room if in one
	instance_ptr->OnLeftRoom(lobby2Msg->roomId);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}

//**************************************************************
// Function: roomDestroyedCb
// Description: This callback function is called when the room
//		gets destroyed.
//**************************************************************
void Lobby2Client_PS3::roomDestroyedCb(SceNpMatching2ContextId         ctxId,
									   SceNpMatching2RoomId            roomId,
									   SceNpMatching2Event             event,
									   SceNpMatching2EventKey          eventKey,
									   int     errorCode,
									   size_t  dataSize,
									   void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_RoomWasDestroyed_PS3 *lobby2Msg=
		(Notification_Console_RoomWasDestroyed_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_RoomWasDestroyed);
	lobby2Msg->roomId=roomId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer////////////////////////
	SceNpMatching2RoomUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2RoomUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data///////////////////////////
	int ret = 0;
	ret = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (ret < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(cbData);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Free callback data
	rakFree(cbData);

	// Leave room if in one
	instance_ptr->OnLeftRoom(lobby2Msg->roomId);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);
}


//**************************************************************
// Function: ownerChangedCb
// Description: This callback function is called when the owner
//		has changed.
//**************************************************************
void Lobby2Client_PS3::ownerChangedCb(SceNpMatching2ContextId         ctxId,
									  SceNpMatching2RoomId            roomId,
									  SceNpMatching2Event             event,
									  SceNpMatching2EventKey          eventKey,
									  int     errorCode,
									  size_t  dataSize,
									  void   *arg)
{
	(void)event;
	(void)arg;

	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_RoomOwnerChanged_PS3 *lobby2Msg=
		(Notification_Console_RoomOwnerChanged_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_RoomOwnerChanged);
	lobby2Msg->roomId=roomId;
	lobby2Msg->extendedResultCode=errorCode;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Callback data buffer/////////////////////////
	SceNpMatching2RoomOwnerUpdateInfo *cbData = NULL;
	cbData = (SceNpMatching2RoomOwnerUpdateInfo *)rakMalloc(dataSize);
	if(cbData == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(cbData, 0, dataSize);

	//Get callback data////////////////////////////////
	int ret = 0;
	ret = sceNpMatching2GetEventData(ctxId, eventKey, cbData, dataSize);
	if (ret < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(cbData);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->prevOwner=cbData->prevOwner;
	lobby2Msg->newOwner=cbData->newOwner;

	Room *room = instance_ptr->GetRoom(roomId);
	
	for (unsigned int i=0; i < room->roomMembers.Size(); i++)
	{
		if (room->roomMembers[i]->roomMemberId==lobby2Msg->newOwner)
			room->roomMembers[i]->isRoomOwner=true;
		else
			room->roomMembers[i]->isRoomOwner=false;
	}

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);

	//Free callback data buffer
	rakFree(cbData);

	lobby2Client->msgFactory->Dealloc(lobby2Msg);

	return;
}


//**************************************************************
// Function: roomChatMessageCb
// Description: This callback function is called when there is a
//              chat message sent to the room.
//**************************************************************
void Lobby2Client_PS3::roomChatMessageCb(SceNpMatching2ContextId        ctxId,
										 SceNpMatching2RoomId           roomId,
										 SceNpMatching2RoomMemberId     srcMemberId,
										 SceNpMatching2Event            event,
										 SceNpMatching2EventKey         eventKey,
										 int errorCode,
										 size_t  dataSize,
										 void   *arg)
{
	(void)event;
	(void)arg;
	(void)errorCode;
	(void)srcMemberId;

	//Error handling////////////////////////
	//Lobby2Client_PS3* instance_ptr = lobby2Client;
	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_RoomChatMessage_PS3 *lobby2Msg=
		(Notification_Console_RoomChatMessage_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_RoomChatMessage);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->roomId=roomId;
	lobby2Msg->srcMemberId=srcMemberId;
	if (errorCode < 0 || dataSize == 0)
	{
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Message info buffer////////////////////////////
	SceNpMatching2RoomMessageInfo *msgInfo = NULL;
	msgInfo = (SceNpMatching2RoomMessageInfo *)rakMalloc(dataSize);
	if(msgInfo == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(msgInfo, 0, dataSize);

	//Get message info data////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, msgInfo, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(msgInfo);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->sender=msgInfo->srcMember->npId.handle.data;
	lobby2Msg->message=(char*) msgInfo->msg;


	//Free message info buffer
	rakFree(msgInfo);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}

//**************************************************************
// Function: roomMessageCb
// Description: This callback function is called when there is a
//              message sent in the room.
//**************************************************************
void Lobby2Client_PS3::roomMessageCb(SceNpMatching2ContextId        ctxId,
									 SceNpMatching2RoomId           roomId,
									 SceNpMatching2RoomMemberId     srcMemberId,
									 SceNpMatching2Event            event,
									 SceNpMatching2EventKey         eventKey,
									 int errorCode,
									 size_t  dataSize,
									 void   *arg)
{
	(void)event;
	(void)arg;
	(void)errorCode;
	(void)roomId;
	(void)srcMemberId;


	//Error handling////////////////////////
	//Lobby2Client_PS3* instance_ptr = lobby2Client;
	Lobby2Client_PS3* instance_ptr = lobby2Client;
	Notification_Console_RoomMessage_PS3 *lobby2Msg=
		(Notification_Console_RoomMessage_PS3 *)lobby2Client->msgFactory->Alloc(RakNet::L2MID_Notification_Console_RoomMessage);
	lobby2Msg->extendedResultCode=errorCode;
	lobby2Msg->roomId=roomId;
	lobby2Msg->srcMemberId=srcMemberId;
	if (errorCode < 0 || dataSize == 0)
	{
		lobby2Msg->extendedResultCode=errorCode;
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	//Message info buffer///////////////////////////////
	SceNpMatching2RoomMessageInfo *msgInfo = NULL;
	msgInfo = (SceNpMatching2RoomMessageInfo *)rakMalloc(dataSize);
	if(msgInfo == NULL)
	{
		sceNpMatching2ClearEventData(ctxId, eventKey);
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_OUT_OF_MEMORY);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}
	memset(msgInfo, 0, dataSize);

	//Get message info data//////////////////////////////
	lobby2Msg->extendedResultCode = sceNpMatching2GetEventData(ctxId, eventKey, msgInfo, dataSize);
	if (lobby2Msg->extendedResultCode < 0) {
		instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_GENERAL_ERROR);
		rakFree(msgInfo);
		lobby2Client->msgFactory->Dealloc(lobby2Msg);
		return;
	}

	lobby2Msg->sender=msgInfo->srcMember->npId.handle.data;
	lobby2Msg->message=(char*)msgInfo->msg;

	//Free message info buffer
	rakFree(msgInfo);

	instance_ptr->PushCompletedCBWithResultCode(lobby2Msg, L2RC_SUCCESS);
	lobby2Client->msgFactory->Dealloc(lobby2Msg);
	return;
}


// ------------------------------------------------------------------------------------------------------------------

Room::Room()
{

}
Room::~Room()
{
	unsigned int i;
	for (i=0; i < roomMembers.Size(); i++)
	{
		RakNet::OP_DELETE(roomMembers[i], __FILE__,__LINE__);
	}
}
void Room::AddRoomMember(SceNpMatching2RoomMemberId _memberId, SceNpId *_npId, bool _isRoomOwner, RakNet::RakString groupLabel)
{
	unsigned int i;
	for (i=0; i < roomMembers.Size(); i++)
	{
		if (roomMembers[i]->roomMemberId==_memberId)
		{
			return;
		}
	}
	RoomMemberInfo *rmi = RakNet::OP_NEW<RoomMemberInfo>(__FILE__,__LINE__);
	rmi->roomMemberId=_memberId;
	rmi->isRoomOwner=_isRoomOwner;
	rmi->isAvcStreamingTarget=_isRoomOwner;
	rmi->groupLabel=groupLabel;
	memcpy(&rmi->npId, _npId, sizeof(SceNpId));

	roomMembers.Push(rmi, __FILE__, __LINE__);

}
void Room::RemoveRoomMember(const SceNpMatching2RoomMemberId &roomMemberId)
{
	unsigned int i;
	for (i=0; i < roomMembers.Size(); i++)
	{
		if (roomMembers[i]->roomMemberId==roomMemberId)
		{
			RakNet::OP_DELETE(roomMembers[i],__FILE__,__LINE__);
			roomMembers.RemoveAtIndex(i);
			return;
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------

Lobby2Client_PS3::Lobby2Client_PS3()
{
	m_ctxId=0;
	lobby2Client=this;
	avc2Init=false;
	titleUserStorageInitialized=false;
	isAvc2JoinSucceeded=isAvc2Streaming=false;
}

Lobby2Client_PS3::~Lobby2Client_PS3()
{
	unsigned int i;
	// To rakFree memory, which should be freed in the callback
	for (i=0; i < deferredCallbacks.Size(); i++)
	{
		msgFactory->Dealloc(deferredCallbacks[i]);
	}
	deferredCallbacks.Clear(true, __FILE__, __LINE__);

	ClearFriends();
	ClearRooms();
	ClearCompletedCallbacks();
}

void Lobby2Client_PS3::SendMessage(Lobby2Message *msg)
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
		msg->refCount++;
		PushDeferredCallback(msg);
	}
}
void Lobby2Client_PS3::SendMsgAndDealloc(Lobby2Message *msg)
{
	SendMessage(msg);
	msgFactory->Dealloc(msg);

}
void Lobby2Client_PS3::Update(void)
{
	cellSysutilCheckCallback();

	// cellSysutilCheckCallback does not consistently run in the user thread despite what the documentation says
	// So completed calls are pushed to completedCallbacks and called here
	CallCompletedCallbacks();
	unsigned int i;
	i=0;
	while (i < deferredCallbacks.Size())
	{
		// Some messages require polling, rather than do callbacks
		// PS3 is inconsistent this way
		if (deferredCallbacks[i]->WasCompleted())
		{
			if (deferredCallbacks[i]->resultCode==L2RC_PROCESSING)
				PushCompletedCBWithResultCode(deferredCallbacks[i], L2RC_SUCCESS);
			else
				PushCompletedCBWithResultCode(deferredCallbacks[i], deferredCallbacks[i]->resultCode);
			msgFactory->Dealloc(deferredCallbacks[i]);
			deferredCallbacks.RemoveAtIndex(i);
		}
		else
		{
			i++;
		}
	}
}
bool Lobby2Client_PS3::WasTUSInitialized(void) const
{
	return titleUserStorageInitialized;
}
void Lobby2Client_PS3::PushDeferredCallback(Lobby2Message *msg)
{
	deferredCallbacks.Insert(msg->requestId, msg, true, __FILE__, __LINE__);
}
Lobby2Message *Lobby2Client_PS3::GetDeferredCallback(unsigned int requestId, bool peek)
{
	unsigned int index;
	bool objectExists;
	index = deferredCallbacks.GetIndexFromKey(requestId, &objectExists);
	if (objectExists)
	{
		Lobby2Message *msg = deferredCallbacks[index];
		if (peek==false)
			deferredCallbacks.RemoveAtIndex(index);
		return msg;
	}
	else
		return 0;
}
Lobby2Message *Lobby2Client_PS3::GetDeferredCallbackByMessageId(Lobby2MessageID messageId, bool peek)
{
	unsigned int index;
	for (index=0; index < deferredCallbacks.Size(); index++)
	{
		Lobby2MessageID id = deferredCallbacks[index]->GetID();
		if (id==messageId)
		{
			Lobby2Message *msg = deferredCallbacks[index];
			if (peek==false)
				deferredCallbacks.RemoveAtIndex(index);
			return msg;
		}
	}
	return 0;
}
void Lobby2Client_PS3::PushCompletedCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc)
{
	if (msg)
	{
		msg->resultCode=rc;
		msg->refCount++;
		completedCallbacksMutex.Lock();
		completedCallbacks.Push(msg, __FILE__, __LINE__);
		completedCallbacksMutex.Unlock();
	}	
}
void Lobby2Client_PS3::CallCompletedCallbacks(void)
{
	if (completedCallbacks.Size()==0)
		return;

	Lobby2Message *msg;
	DataStructures::List<Lobby2Message *> completedCallbacksCopy;
	unsigned long h;
	completedCallbacksMutex.Lock();
	for (h=0; h < completedCallbacks.Size(); h++)
		completedCallbacksCopy.Push(completedCallbacks[h], __FILE__, __LINE__);
	completedCallbacks.Clear(false, __FILE__, __LINE__);
	completedCallbacksMutex.Unlock();

	for (h=0; h < completedCallbacksCopy.Size(); h++)
	{
		msg=completedCallbacksCopy[h];
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
		msgFactory->Dealloc(msg);
	}
}
void Lobby2Client_PS3::ClearCompletedCallbacks(void)
{
	Lobby2Message *msg;
	completedCallbacksMutex.Lock();
	for (unsigned long h=0; h < completedCallbacks.Size(); h++)
	{
		msg=completedCallbacks[h];
		msgFactory->Dealloc(msg);
	}
	completedCallbacks.Clear(false, __FILE__, __LINE__);
	completedCallbacksMutex.Unlock();
}
void Lobby2Client_PS3::ShutdownMatching2(void)
{
	ShutdownTUS();

	if (avc2Init)
		cellSysutilAvc2Unload();
	avc2Init=false;
	isAvc2JoinSucceeded=isAvc2Streaming=false;

	sample_matching2_shutdown(m_ctxId);
	m_ctxId=0;

	lobby2Client->ClearRooms();
	lobby2Client->ClearFriends();
	ClearCompletedCallbacks();
}
bool Lobby2Client_PS3::IsInRoom(void) const
{
	return rooms.Size()>0;
}
int Lobby2Client_PS3::GetRoomIndex(SceNpMatching2RoomId roomId) const
{
	unsigned int i;
	for (i=0; i < rooms.Size(); i++)
	{
		if (rooms[i]->roomId==roomId)
			return i;
	}
	return -1;
}
void Lobby2Client_PS3::ClearRooms(void)
{
	unsigned int i;
	for (i=0; i < rooms.Size(); i++)
		RakNet::OP_DELETE(rooms[i],__FILE__,__LINE__);
	rooms.Clear(false, __FILE__, __LINE__);
}
void Lobby2Client_PS3::ClearFriends(void)
{
	unsigned int i;
	for (i=0; i < friends.Size(); i++)
		RakNet::OP_DELETE(friends[i],__FILE__,__LINE__);
	friends.Clear(false, __FILE__, __LINE__);
}
FriendStatus* Lobby2Client_PS3::AddFriend(SceNpId *_userId, FriendStatusContext status)
{
	for (int i=0; i < friends.Size(); i++)
	{
		if (memcmp(&friends[i]->userId,_userId,sizeof(SceNpId))==0)
		{
			friends[i]->friendStatus=status;
			return friends[i];
		}
	}

	FriendStatus *fs = RakNet::OP_NEW<FriendStatus>(__FILE__,__LINE__);
	memcpy(&fs->userId,_userId,sizeof(SceNpId));
	fs->friendStatus=status;
	friends.Push(fs, __FILE__, __LINE__);
	return fs;
}
void Lobby2Client_PS3::RemoveFriend(SceNpId *_userId)
{
	for (int i=0; i < friends.Size(); i++)
	{
		if (memcmp(&friends[i]->userId,_userId,sizeof(SceNpId))==0)
		{
			RakNet::OP_DELETE(friends[i],__FILE__,__LINE__);
			friends.RemoveAtIndex(i);
			return;
		}
	}
}
Room* Lobby2Client_PS3::AddRoom(SceNpMatching2RoomId roomId)
{
	Room* room;
	room = GetRoom(roomId);
	if (room)
		return room;

	room = RakNet::OP_NEW<Room>(__FILE__,__LINE__);
	rooms.Push(room, __FILE__, __LINE__);
	room->roomId=roomId;
	return room;	
}
int Lobby2Client_PS3::CompareNpIds(const SceNpId *npid1, const SceNpId *npid2) const
{
	int order;
	if (sceNpUtilCmpNpIdInOrder(npid1,npid2,&order)==0)
		return order;
	else
		return memcmp(npid1,npid2,sizeof(SceNpId));
}
Room*  Lobby2Client_PS3::GetRoom(SceNpMatching2RoomId roomId) const
{
	unsigned int i;
	for (i=0; i < rooms.Size(); i++)
	{
		if (rooms[i]->roomId==roomId)
			return rooms[i];
	}
	return 0;
}
bool Lobby2Client_PS3::AreWeTheRoomOwner(const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	unsigned int i;
	for (i=0; i < room->roomMembers.Size(); i++)
	{
		if (CompareNpIds(&room->roomMembers[i]->npId,&m_npId)==0)
		{
			return room->roomMembers[i]->isRoomOwner;
		}
	}
	return false;
}
unsigned int Lobby2Client_PS3::GetNumRoomMembers(const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	return room->roomMembers.Size();
}
RoomMemberInfo* Lobby2Client_PS3::GetRoomMemberAtIndex(unsigned int idx, const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	return room->roomMembers[idx];
}
RoomMemberInfo* Lobby2Client_PS3::GetRoomMemberById(SceNpMatching2RoomMemberId id, const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	unsigned int i;
	for (i=0; i < room->roomMembers.Size(); i++)
	{
		if (room->roomMembers[i]->roomMemberId==id)
			return room->roomMembers[i];
	}
	return 0;
}
RoomMemberInfo* Lobby2Client_PS3::GetRoomMemberByNpId(SceNpId *id, const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	unsigned int i;
	for (i=0; i < room->roomMembers.Size(); i++)
	{
		if (CompareNpIds(&room->roomMembers[i]->npId,id)==0)
			return room->roomMembers[i];
	}
	return 0;
}
RoomMemberInfo* Lobby2Client_PS3::GetRoomMemberByHandle(const char *handle, const SceNpMatching2RoomId *roomid) const
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return 0;
	unsigned int i;
	for (i=0; i < room->roomMembers.Size(); i++)
	{
		if (strcmp(handle, room->roomMembers[i]->npId.handle.data)==0)
			return room->roomMembers[i];
	}
	return 0;
}
bool Lobby2Client_PS3::IsUserInRoom(const SceNpMatching2RoomId *roomid, const SceNpId *id)
{
	Room *room = GetRoom(*roomid);
	if (room==0)
		return false;
	for (int i=0; i < room->roomMembers.Size(); i++)
	{
		if (CompareNpIds(&room->roomMembers[i]->npId, id)==0)
		{
			return true;
		}
	}
	return false;
}
const SceNpMatching2RoomId* Lobby2Client_PS3::GetFirstRoomID(void) const
{
	if (rooms.Size()==0)
		return 0;
	return &rooms[0]->roomId;
}
unsigned int Lobby2Client_PS3::GetNumFriends(void) const
{
	return friends.Size();
}
FriendStatus* Lobby2Client_PS3::GetFriendAtIndex(unsigned int idx) const
{
	if (idx >= friends.Size())
		return NULL;

	return friends[idx];
}
FriendStatus* Lobby2Client_PS3::GetFriendByName(const char *name) const
{
	for (unsigned int i=0; i < friends.Size(); i++)
	{
		if (strcmp(friends[i]->userId.handle.data,name)==0)
			return friends[i];
	}
	return 0;
}
bool Lobby2Client_PS3::IsUserFriend(const SceNpId *id) const
{
	unsigned int i;
	for (i=0; i < friends.Size(); i++)
	{
		if (CompareNpIds(&friends[i]->userId, id)==0)
			return true;
	}
	return false;
}
int Lobby2Client_PS3::GetLobbyMemberID(SceNpMatching2LobbyId lobbyId, SceNpMatching2LobbyMemberId *lobbyMembers, int lobbyMembersLen, SceNpMatching2LobbyMemberId *me)
{
	SceNpMatching2LobbyMemberId *lobbyMembersPtr;
	SceNpMatching2LobbyMemberId *mePtr;
	SceNpMatching2LobbyMemberId meStack;
	SceNpMatching2LobbyMemberId lobbyMembersStack[1];

	if (lobbyMembersLen>SCE_NP_MATCHING2_LOBBY_MAX_SLOT)
		lobbyMembersLen=SCE_NP_MATCHING2_LOBBY_MAX_SLOT;
	if (lobbyMembersLen==0)
		lobbyMembersPtr=lobbyMembersStack;
	else
		lobbyMembersPtr=lobbyMembers;
	if (me==0)
		mePtr=&meStack;
	else
		mePtr=me;

	int ret;

	//E Get lobby member ID list
	ret = sceNpMatching2GetLobbyMemberIdListLocal(
		m_ctxId, lobbyId, lobbyMembersPtr, 
		lobbyMembersLen, mePtr);

	return ret;

}
SceNpMatching2LobbyMemberId Lobby2Client_PS3::GetMyLobbyMemberID(SceNpMatching2LobbyId lobbyId)
{
	SceNpMatching2LobbyMemberId out;
	GetLobbyMemberID(lobbyId, 0,0,&out);
	return out;
}
void Lobby2Client_PS3::OnAttach(void)
{
	// Global pointer to use if callback does not have context data
	lobby2Client=this;
}
int Lobby2Client_PS3::GetServerIDs(SceNpMatching2ServerId *server_id, int arrayLength)
{
	return sample_get_server_id_list(m_ctxId, server_id, arrayLength);
}


void Lobby2Client_PS3::OnEnteredRoom(SceNpMatching2RoomId room_id)
{
//	CellSysutilAvc2EventId event_id;
//	CellSysutilAvc2EventParam event_param;

	// Assuming that the NP matching 2 utility was used to join a matching room
	// and the room ID was obtained
	cellSysutilAvc2JoinChatRequest(&room_id);
}
void Lobby2Client_PS3::OnLeftRoom(SceNpMatching2RoomId roomId)
{
	CellSysutilAvc2EventId event_id;
	CellSysutilAvc2EventParam event_param;
	cellSysutilAvc2StopStreaming();
	int ret = cellSysutilAvc2LeaveChat(&event_id, &event_param);
	if( ret != CELL_OK )
	{
		// Error handling
	}

	int idx = GetRoomIndex(roomId);
	if (idx!=-1)
	{
		RakNet::OP_DELETE(rooms[idx],__FILE__,__LINE__);
		rooms.RemoveAtIndex(idx);
	}
}
void Lobby2Client_PS3::InitTUS(const SceNpCommunicationId *communicationId, const SceNpCommunicationPassphrase *passphrase)
{
	if (titleUserStorageInitialized==true)
		return;
	int ret = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP_TUS);
	if(ret <0) {
		return;
	}

	 ret = sceNpTusInit(0);
	if (ret < 0) {
		RakAssert("InitTus Failed" && 0);
		return;
	}
	int titleCtxId;
	ret = sceNpTusCreateTitleCtx(communicationId, passphrase, &m_npId);
	if (ret < 0) {
		RakAssert("InitTus Failed" && 0);
		return;
	}
	titleCtxId = ret;
	titleUserStorageInitialized=true;
}
void Lobby2Client_PS3::ShutdownTUS()
{
	if (titleUserStorageInitialized==false)
		return;
	titleUserStorageInitialized=false;
	sceNpTusDestroyTitleCtx(titleCtxId);
	sceNpTusTerm();

}
void Lobby2Client_PS3::SetStreamAVC2ToChatTarget(RoomMemberInfo *roomMemberInfo, bool stream)
{
	if (roomMemberInfo->isAvcStreamingTarget!=stream)
	{
		roomMemberInfo->isAvcStreamingTarget=stream;
		UpdateAVC2StreamTargets();
	}
}
void Lobby2Client_PS3::UpdateAVC2StreamTargets(void)
{
	if (isAvc2JoinSucceeded==true)
	{
		bool anyTalking=false;
		for (unsigned int i=0; i < rooms.Size() && anyTalking==false; i++)
		{
			Room * room = rooms[i];
			for (unsigned int j=0; j < room->roomMembers.Size(); j++)
			{
				if (room->roomMembers[j]->isAvcStreamingTarget==true)
				{
					anyTalking=true;
					break;
				}
			}
		}

		if (anyTalking==true && isAvc2Streaming==false)
		{
			// Start streaming
			cellSysutilAvc2StartStreaming();
			isAvc2Streaming=true;
		}
		else if (anyTalking==false && isAvc2Streaming==true)
		{
			// Stop streaming
			cellSysutilAvc2StopStreaming();
			isAvc2Streaming=false;
		}

		if (anyTalking==false)
			return;

		SceNpMatching2RoomMemberId memberIds[8];
		CellSysutilAvc2RoomMemberList memberList;
		memberList.member_num = 0;
		memberList.member_id  = memberIds;
		for (unsigned int i=0; i < rooms.Size() && memberList.member_num<8; i++)
		{
			Room * room = rooms[i];
			for (unsigned int j=0; j < room->roomMembers.Size() && memberList.member_num<8; j++)
			{
				if (room->roomMembers[j]->isAvcStreamingTarget==true)
				{
					memberIds[memberList.member_num]=room->roomMembers[j]->roomMemberId;
					memberList.member_num++;
				}
			}
		}

		CellSysutilAvc2StreamingTarget target;
		memset( &target, 0x00, sizeof(target) );
		target.target_mode      = CELL_SYSUTIL_AVC2_CHAT_TARGET_MODE_PRIVATE;
		target.room_member_list = memberList;
		int ret = cellSysutilAvc2SetStreamingTarget(target);
		if( ret != CELL_OK )
		{
			// Error handling
		}

	}
}
#endif // #ifdef SN_TARGET_PS3