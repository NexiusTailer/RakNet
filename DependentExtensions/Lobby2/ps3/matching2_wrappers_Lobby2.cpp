#ifdef SN_TARGET_PS3

/*   SCE CONFIDENTIAL                                       */
/*   PLAYSTATION(R)3 Programmer Tool Runtime Library 250.001 */
/*   Copyright (C) 2007 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

//****************************************************
//
// Title:       Matching2 sample program
// Description: Network initialization, Matching2
//              functionality demonstration.
// Date:        7/2007 
// File:        matching2_wrappers.cpp
//
//****************************************************

#include <stdio.h>
#include <string.h>
#include "matching2_wrappers_Lobby2.h"
//#include "../np_conf.h"



//----------------------------------------------------
// Functions Implementation
//----------------------------------------------------

//***********************************************
// Function: sample_join_lobby()
// Description: Call this to join a lobby.
//***********************************************
int sample_join_lobby(SceNpMatching2ContextId ctxId,
		     SceNpMatching2RequestId* assignedReqId_ptr,
		     SceNpMatching2LobbyId lobbyId,
		     SceNpMatching2RequestCallback callback,
		     void* cb_args)
{
	int ret = 0;

	SceNpMatching2JoinLobbyRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	
	memset(&reqParam, 0, sizeof(reqParam));	
	
	reqParam.lobbyId = lobbyId;

	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default timeout

	ret = sceNpMatching2JoinLobby(ctxId,
				     &reqParam,
				     &optParam,
				     assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2JoinLobby() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2JoinLobby() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif	

	return ret;
}

//***********************************************
// Function: sample_leave_lobby()
// Description: Call this to leave a lobby.
//***********************************************
int sample_leave_lobby(SceNpMatching2ContextId ctxId,
		      SceNpMatching2RequestId* assignedReqId_ptr,
		      SceNpMatching2LobbyId lobbyId,
		      SceNpMatching2RequestCallback callback,
		      void* cb_args)
{
	int ret = 0;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2LeaveLobbyRequest reqParam;

	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.lobbyId = lobbyId;
	// option data
	memset(&reqParam.optData, 0, sizeof(SceNpMatching2PresenceOptionData));
	reqParam.optData.len = 0;

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2LeaveLobby(ctxId,
				      &reqParam,
				      &optParam,
				      assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2LeaveLobby() failed. ret = 0x%x\n", ret);
	}
#ifdef DEBUG
	printf("sceNpMatching2LeaveLobby() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}

//*************************************************
// Function: sample_register_lobby_event_callback()
// Description: Register a callback function for
//              lobby events.
//*************************************************
int sample_register_lobby_event_callback(SceNpMatching2ContextId ctxId, 
					SceNpMatching2LobbyEventCallback callback, 
					void* cb_args)
{
	int ret = 0;

	ret = sceNpMatching2RegisterLobbyEventCallback(ctxId, callback, cb_args);
	if (ret < 0) {
		printf("sceNpMatching2RegisterLobbyEventCallback() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2RegisterLobbyEventCallback() succeeded.\n");
#endif

	return ret;
}

//*************************************************
// Function: sample_register_lobby_message_callback()
// Description: Register a callback function for
//              lobby message events.
//*************************************************
int sample_register_lobby_message_callback(SceNpMatching2ContextId ctxId, 
					  SceNpMatching2LobbyMessageCallback callback,
					  void* cb_args)
{
	int ret = 0;

	ret = sceNpMatching2RegisterLobbyMessageCallback(ctxId, callback, cb_args);
	if (ret < 0) {
		printf("sceNpMatching2RegisterLobbyMessageCallback() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2RegisterLobbyMessageCallback() succeeded.\n");
#endif

	return ret;
}

//*************************************************
// Function: sample_get_lobby_member_data_internal()
// Description: Make request for LobbyMemberDataInternal.
//*************************************************
int sample_get_lobby_member_data_internal(SceNpMatching2ContextId ctxId,
					  SceNpMatching2RequestId* assignedReqId_ptr,
					  SceNpMatching2LobbyId lobbyId,
					  SceNpMatching2LobbyMemberId lobbyMemberId,
					  SceNpMatching2RequestCallback callback,
					  void* cb_args)
{
	int ret = 0;

	SceNpMatching2GetLobbyMemberDataInternalRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	
	memset(&reqParam, 0, sizeof(reqParam));	
	reqParam.lobbyId = lobbyId;
	reqParam.memberId = lobbyMemberId;

	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default timeout

	ret = sceNpMatching2GetLobbyMemberDataInternal(ctxId,
				      		       &reqParam,
						       &optParam,
						       assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2GetLobbyMemberDataInternal() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2GetLobbyDataInternal() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif	

	return ret;
}

//***********************************************
// Function: sample_send_lobby_invitation()
// Description: Call this to send a lobby invitation.
//***********************************************
int sample_send_lobby_invitation(SceNpMatching2ContextId ctxId,
				 SceNpMatching2RequestId* assignedReqId_ptr,
				 SceNpMatching2LobbyId lobbyId,
				 SceNpMatching2RoomId roomId,
				 SceNpMatching2LobbyMemberId lobbyMemberId,
				 SceNpMatching2RequestCallback callback,
				 void* cb_args)
{
	SceNpMatching2SendLobbyInvitationRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.lobbyId = lobbyId;
	reqParam.castType = SCE_NP_MATCHING2_CASTTYPE_UNICAST;
	reqParam.dst.unicastTarget = lobbyMemberId;
	SceNpMatching2JoinedSessionInfo joinedSessionInfo;
	memset(&joinedSessionInfo, 0, sizeof(joinedSessionInfo));
	joinedSessionInfo.sessionType = SCE_NP_MATCHING2_SESSION_TYPE_ROOM;
	joinedSessionInfo.roomId = roomId;
	reqParam.invitationData.targetSession = &joinedSessionInfo;
	reqParam.invitationData.targetSessionNum = 1;
	reqParam.invitationData.optData = NULL;
	reqParam.invitationData.optDataLen = 0;
	reqParam.option = SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_NPID;
	
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default timeout
	
	int ret = sceNpMatching2SendLobbyInvitation(ctxId, &reqParam, &optParam, assignedReqId_ptr);
	
	if (ret < 0) {
		printf("sceNpMatching2SendLobbyInvitation() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2SendLobbyInvitation() succeeded.\n");
	printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}

//***********************************************
// Function: sample_send_lobby_chat_message()
// Description: Call this to send a lobby message.
//***********************************************
int sample_send_lobby_chat_message(SceNpMatching2ContextId ctxId,
				  SceNpMatching2RequestId* assignedReqId_ptr,
				  SceNpMatching2LobbyId lobbyId,
				  const char* message,
				  int message_len,
				  SceNpMatching2RequestCallback callback,
				  void* cb_args)
{
	int ret=0;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2SendLobbyChatMessageRequest reqParam;

	//J リクエストパラメータ
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.lobbyId = lobbyId;
	reqParam.castType = SCE_NP_MATCHING2_CASTTYPE_BROADCAST;
	reqParam.msg = message;
	reqParam.msgLen = message_len;
	reqParam.option = SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_NPID;

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2SendLobbyChatMessage(ctxId, &reqParam, &optParam, assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2SendLobbyChatMessage() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2SendLobbyChatMessage() succeeded.\n");
	printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}


//***********************************************
// Function: sample_create_join_room()
// Description: Create a room and join it.
//***********************************************
int sample_createjoin_room(SceNpMatching2ContextId ctxId,
			   SceNpMatching2RequestId* assignedReqId_ptr,
			   SceNpMatching2WorldId worldId,
			   SceNpMatching2LobbyId lobbyId,
			   int total_slots,
			   SceNpMatching2RequestCallback callback,
			   void* cb_args,
			   bool hidden,
			   int numGroups,
			   const char *password,
			   unsigned int passwordSlotMaskBits)
{
	int ret = 0;

	SceNpMatching2CreateJoinRoomRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2RoomPasswordSlotMask slotMask;
	SceNpMatching2SessionPassword passwordStruct;
	if (password)
	{
		memset(&passwordStruct, 0, sizeof(passwordStruct));
		strncpy((char*) passwordStruct.data, password, SCE_NP_MATCHING2_SESSION_PASSWORD_SIZE);
		reqParam.roomPassword = &passwordStruct;
	}

	memset(&slotMask, 0, sizeof(slotMask));
	memset(&reqParam, 0, sizeof(reqParam));
	memset(&optParam, 0, sizeof(optParam));

	// Flag Attribute
	SceNpMatching2FlagAttr flagAttr;
	flagAttr = SCE_NP_MATCHING2_ROOM_FLAG_ATTR_OWNER_AUTO_GRANT | SCE_NP_MATCHING2_ROOM_FLAG_ATTR_NAT_TYPE_RESTRICTION;
	if (hidden)
		flagAttr |= SCE_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN;
	
	
	/*
	 * CreateJoinRoom parameter
	 */
#ifdef DEBUG
	printf("CreateJoinRoom Lobby ID: 0x%016llx\n", lobbyId);
#endif
	reqParam.worldId = worldId;
	reqParam.lobbyId = lobbyId;
	reqParam.maxSlot = total_slots;
	reqParam.roomPassword = NULL;
	reqParam.flagAttr = flagAttr;

	SceNpMatching2RoomGroupConfig groupConfig[32];
	SceNpMatching2GroupLabel gl;
	if (numGroups>1)
	{
		for (int i=0; i < numGroups; i++)
		{
			memset(&groupConfig[i], 0, sizeof(SceNpMatching2RoomGroupConfig));
			groupConfig[i].slotNum = total_slots/numGroups;
			groupConfig[i].withLabel = true;
			sprintf((char*) groupConfig[i].label.data,"%i", i);
		}

		reqParam.groupConfigNum=numGroups;
		reqParam.groupConfig=(SceNpMatching2RoomGroupConfig*) &groupConfig;

		reqParam.joinRoomGroupLabel=&gl;
		memcpy(reqParam.joinRoomGroupLabel->data, groupConfig[0].label.data, SCE_NP_MATCHING2_GROUP_LABEL_SIZE);
	}
	else if (passwordSlotMaskBits && password)
	{
		for (int i=0; i < 32; i++)
		{
			if (passwordSlotMaskBits & (1<<i))
				SCE_NP_MATCHING2_ADD_SLOTNUM_TO_ROOM_PASSWORD_SLOT_MASK(slotMask, i);
		}

		reqParam.passwordSlotMask = &slotMask;
	}
	
	//J オプションパラメータ
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default timeout

	SceNpMatching2SignalingOptParam sigOptParam;
	sigOptParam.type = SCE_NP_MATCHING2_SIGNALING_TYPE_MESH;
	reqParam.sigOptParam = &sigOptParam;

	//Issue request to create a room///////
	ret = sceNpMatching2CreateJoinRoom(ctxId,
					   &reqParam,
					   &optParam,
					   assignedReqId_ptr);
	if (ret < 0)
	{
		printf("sceNpMatching2CreateJoinRoom() failed. ret = 0x%x\n", ret);
		return ret;
	}
	
#ifdef DEBUG
	printf("sceNpMatching2CreateJoinRoom() succeeded.\n");
	printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}

//***********************************************
// Function: sample_leave_room()
// Description: Call this to leave a room.
//***********************************************
int sample_leave_room(SceNpMatching2ContextId ctxId,
		      SceNpMatching2RequestId* assignedReqId_ptr,
		      SceNpMatching2RoomId roomId,
		      SceNpMatching2RequestCallback callback,
		      void* cb_args)
{
	int ret = 0;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2LeaveRoomRequest reqParam;

	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.roomId = roomId;
	// option data
	memset(&reqParam.optData, 0, sizeof(SceNpMatching2PresenceOptionData));
	reqParam.optData.len = 0;

	//J オプションパラメータ
      	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2LeaveRoom(ctxId,
				      &reqParam,
				      &optParam,
				      assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2LeaveRoom() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2LeaveRoom() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}

//***********************************************
// Function: sample_search_room()
// Description: Call this to search for rooms.
//***********************************************
int sample_search_room(SceNpMatching2ContextId ctxId,
		       SceNpMatching2RequestId* assignedReqId_ptr,
			   // Either lobbyId or worldId should be 0
			   SceNpMatching2LobbyId lobbyId,
		       SceNpMatching2WorldId worldId,
		       unsigned int startIndex,
		       unsigned int max,
			   bool returnRandomResults,
		       SceNpMatching2RequestCallback callback,
			   SceNpMatching2IntSearchFilter *_intFilter,
				unsigned int _intFilterNum,
				SceNpMatching2BinSearchFilter *_binFilter,
				unsigned int _binFilterNum,
		       void* cb_args)
{
	int ret = 0;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2SearchRoomRequest reqParam;

	memset(&reqParam, 0, sizeof(reqParam));

#ifdef DEBUG
	printf("Search Room Lobby ID: 0x%016llx\n", lobbyId);
#endif
	reqParam.lobbyId = lobbyId;
	reqParam.worldId = worldId;
	reqParam.rangeFilter.startIndex = startIndex;
	reqParam.rangeFilter.max = max;
	reqParam.flagFilter = 0;
	reqParam.flagAttr = 0;
	reqParam.intFilter = _intFilter;
	reqParam.intFilterNum = _intFilterNum;
	reqParam.binFilter = _binFilter;
	reqParam.binFilterNum = _binFilterNum;
	reqParam.attrId = NULL;
	reqParam.attrIdNum = 0;
	reqParam.option = SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_NPID |
	                  SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_ONLINENAME | 
					  SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_NAT_TYPE_FILTER; 
	if (returnRandomResults)
		reqParam.option |= SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_RANDOM;
	// SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_NAT_TYPE_FILTER ?
                             //0x0 = No options
	                     //0x1 = with NP ID
	                     //0x2 = with Online Name
	                     //0x3 = with NP ID & Online Name
	                     //0x4 = with avatar URL
	                     //0x5 = with NP ID & avatar URL
                             //0x6 = with Online Name & avatar URL
                             //0x7 = with NP ID & Online Name & avatar URL
	                     //SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_NPID 0x01
	                     //SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_ONLINENAME 0x02
	                     //SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_AVATARURL 0x04


	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2SearchRoom(ctxId,
				       &reqParam,
				       &optParam,
				       assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2SearchRoom() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2SearchRoom() succeeded.\n");
	printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
#endif

	return ret;
}

//***********************************************
// Function: sample_get_room_data_external_list()
// Description: Get room data external info.
//***********************************************
int sample_get_room_data_external_list(SceNpMatching2ContextId ctxId,
				       SceNpMatching2RequestId* assignedReqId_ptr,
				       SceNpMatching2RoomId* roomId,
				       unsigned int roomIdNum,
				       SceNpMatching2RequestCallback callback,
				       void* cb_args)
{
	int ret = 0;
	
	SceNpMatching2GetRoomDataExternalListRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.roomId = roomId;
	reqParam.roomIdNum = roomIdNum;
	
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0;
	
	ret = sceNpMatching2GetRoomDataExternalList(ctxId,
						    &reqParam,
						    &optParam,
						    assignedReqId_ptr);
	
	if(ret < 0)
	{
		printf("sceNpMatching2GetRoomDataExternalList() failed. ret = 0x%x\n", ret);
		return ret;
	}
	
#ifdef DEBUG
	printf("sceNpMatching2GetRoomDataExternalList() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif

	return ret;
}

//***********************************************
// Function: sample_join_room()
// Description: Call this to join a room.
//***********************************************
int sample_join_room(SceNpMatching2ContextId ctxId,
		     SceNpMatching2RequestId* assignedReqId_ptr,
		     SceNpMatching2RoomId roomId,
		     SceNpMatching2RequestCallback callback,
		     void* cb_args,
			 int joinGroupId)
{
	int ret = 0;

	SceNpMatching2JoinRoomRequest reqParam;
	SceNpMatching2RequestOptParam optParam;
	
	//J リクエストパラメータ
	memset(&reqParam, 0, sizeof(reqParam));

	reqParam.roomPassword = NULL;
	reqParam.roomId = roomId;
	
#ifdef DEBUG
	printf("Join Room roomId: 0x%016llx\n", roomId);
#endif

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0;

	SceNpMatching2GroupLabel groupLabel;

	if (joinGroupId>=0)
	{
		memset(&groupLabel, 0, sizeof(groupLabel));
		sprintf((char*) groupLabel.data, "%i", joinGroupId);
		reqParam.joinRoomGroupLabel = &groupLabel;
	}

	ret = sceNpMatching2JoinRoom(ctxId,
				     &reqParam,
				     &optParam,
				     assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2JoinRoom() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2JoinRoom() succeeded.\n");
	printf("assignedReqId = 0x%x\n", *assignedReqId_ptr);
#endif	

	return ret;
}

//***********************************************
// Function: sample_send_room_chat_message()
// Description: Call this to send a room message.
//***********************************************
int sample_send_room_chat_message(SceNpMatching2ContextId ctxId,
				  SceNpMatching2RequestId* assignedReqId_ptr,
				  SceNpMatching2RoomId roomId,
				  const char* message,
				  int message_len,
				  SceNpMatching2RequestCallback callback,
				  void* cb_args)
{
	int ret=0;
	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2SendRoomChatMessageRequest reqParam;

	//J リクエストパラメータ
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.roomId = roomId;
	reqParam.castType = SCE_NP_MATCHING2_CASTTYPE_BROADCAST;
	reqParam.msg = message;
	reqParam.msgLen = message_len;
	reqParam.option = SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_NPID;

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2SendRoomChatMessage(
		ctxId,
		&reqParam,
		&optParam,
		assignedReqId_ptr);
	if (ret < 0) {
		printf("sceNpMatching2SendRoomChatMessage() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2SendRoomChatMessage() succeeded.\n");
	printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
#endif
	
	return ret;
}

//***************************************************
// Function: sample_kickout_room_member()
// Description: Call this to someone out of the room.
//***************************************************
int sample_kickout_room_member(SceNpMatching2ContextId ctxId,
			       SceNpMatching2RequestId* assignedReqId_ptr,
			       SceNpMatching2RoomId roomId,
			       SceNpMatching2RoomMemberId targetmemberId,
			       SceNpMatching2RequestCallback callback,
			       void* cb_args)
{
	int ret = 0;

	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2KickoutRoomMemberRequest reqParam;

	//J リクエストパラメータ
	memset(&reqParam, 0, sizeof(reqParam));

	reqParam.roomId = roomId;
	reqParam.target = targetmemberId;

	// option data
	memset(&reqParam.optData, 0, sizeof(SceNpMatching2PresenceOptionData));
	memcpy(&reqParam.optData.data, "Option Data", sizeof(reqParam.optData.data));
	reqParam.optData.len = strlen("Option Data");

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

		ret = sceNpMatching2KickoutRoomMember(ctxId,
							&reqParam,
							&optParam,
							assignedReqId_ptr);
		if (ret < 0) {
			printf("sceNpMatching2KickoutRoomMember() failed. ret = 0x%x\n", ret);
			return ret;
		}
#ifdef DEBUG
		printf("sceNpMatching2KickoutRoomMember() succeeded.\n");
		printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
		printf("KickoutRoomMember done\n");
#endif

	return ret;
}

//***************************************************
// Function: sample_grant_room_owner()
// Description: Call this to grant ownership of your
//		room to someone else.
//***************************************************
int sample_grant_room_owner(SceNpMatching2ContextId ctxId,
			    SceNpMatching2RequestId* assignedReqId_ptr,
			    SceNpMatching2RoomId roomId,
			    SceNpMatching2RoomMemberId newownerId,
			    SceNpMatching2RequestCallback callback,
			    void* cb_args)
{
	int ret = 0;

	SceNpMatching2RequestOptParam optParam;
	SceNpMatching2GrantRoomOwnerRequest reqParam;
	
	//J リクエストパラメータ
	memset(&reqParam, 0, sizeof(reqParam));

	reqParam.roomId = roomId;
	reqParam.newOwner = newownerId;

	// option data. Use it for whatever you want
	memset(&reqParam.optData, 0, sizeof(SceNpMatching2PresenceOptionData));
	memcpy(&reqParam.optData.data, "GRANT_OWNER", sizeof(reqParam.optData.data));
	reqParam.optData.len = strlen("GRANT_OWNER");

	//J オプションパラメータ
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = callback;
	optParam.cbFuncArg = cb_args;
	optParam.timeout = 0; // default

	ret = sceNpMatching2GrantRoomOwner(ctxId,
					&reqParam,
					&optParam,
					assignedReqId_ptr);
		if (ret < 0) {
			printf("sceNpMatching2GrantRoomOwner() failed. ret = 0x%x\n", ret);
			return ret;
		}
#ifdef DEBUG
		printf("sceNpMatching2GrantRoomOwner() succeeded.\n");
		printf("assignedReqId = 0x%08x\n", *assignedReqId_ptr);
		printf("GrantRoomOwner done.\n");
#endif

	return ret;
}

static bool _matching2_shutted_down = true;
//***********************************************
// Function: sample_matching2_init()
// Description: Initialization Matching2 & NP2.
//***********************************************
int sample_matching2_init(SceNpMatching2ContextId* ctxId_ptr,
			  SceNpId* npId_ptr,
			  SceNpMatching2ContextCallback context_cb,
			  void* cb_arg,
			  const SceNpCommunicationId* npCommId,
			  const SceNpCommunicationPassphrase* npCommPassphrase)
{
	if(_matching2_shutted_down == false)
		return 0;
	_matching2_shutted_down = false;

	int ret = 0;
	ret = doMatching2Init(npId_ptr);
	if(ret < 0)
	{
		printf("Matching2 Init failed\n");
		return ret;
	}
	ret = doMatching2CreateContext(ctxId_ptr, npId_ptr, npCommId, npCommPassphrase);
	if(ret < 0)
	{
		printf("Matching2 Create Context failed\n");
		return ret;
	}
	ret = doMatching2RegisterContextCallback((*ctxId_ptr), context_cb, cb_arg);
	if(ret < 0)
	{
		printf("Matching2 Register Context Callback failed\n");
		return ret;
	}
	ret = doMatching2ContextStart(*ctxId_ptr);
	if(ret < 0)
	{
		printf("Matching2 Start Context failed\n");
		return ret;
	}
	
	return ret;
}

//***********************************************
// Function: sample_matching2_shutdown()
// Description: Shutdown Matching2 & NP2.
//***********************************************
int sample_matching2_shutdown(const SceNpMatching2ContextId& ctxId)
{
	if(_matching2_shutted_down)
		return 0;
	_matching2_shutted_down = true;

	int ret = 0;
	ret = doMatching2ContextStop(ctxId);
	if(ret < 0)
	{
		printf("Matching2 Stopt Context failed\n");
	}
	
	ret = doMatching2DestroyContext(ctxId);
	if(ret < 0)
	{
		printf("Matching2 Destroy Context failed\n");
	}
	
	ret = doMatching2Term();
	if(ret < 0)
	{
		printf("Matching2 Term failed\n");
	}
	
	return ret;
}

//*************************************************
// Function: sample_register_room_event_callback()
// Description: Register a callback function for
//              room events.
//*************************************************
int sample_register_room_event_callback(SceNpMatching2ContextId ctxId, 
					SceNpMatching2RoomEventCallback callback, 
					void* cb_args)
{
	int ret = 0;

	ret = sceNpMatching2RegisterRoomEventCallback(ctxId,
			callback,
			cb_args);
	if (ret < 0) {
		printf("sceNpMatching2RegisterRoomEventCallback() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2RegisterRoomEventCallback() succeeded.\n");
#endif

	return ret;
}

//*************************************************
// Function: sample_register_room_message_callback()
// Description: Register a callback function for
//              room message events.
//*************************************************
int sample_register_room_message_callback(SceNpMatching2ContextId ctxId, 
					  SceNpMatching2RoomMessageCallback callback,
					  void* cb_args)
{
	int ret = 0;

	ret = sceNpMatching2RegisterRoomMessageCallback(ctxId,	
			callback,
			cb_args);
	if (ret < 0) {
		printf("sceNpMatching2RegisterRoomMessageCallback() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2RegisterRoomMessageCallback() succeeded.\n");
#endif

	return ret;
}




//-------------------------------------------------------------------------
// Helper functions
// Note: These functions are being called by the above functions
//-------------------------------------------------------------------------

//***********************************************
// Function: doMatching2Init()
// Description: Matching2 initialization.
//***********************************************
int doMatching2Init(SceNpId* npId_ptr)
{
        printf("Matching2 init\n"); //test

	int ret = 0;
	int status;
	ret = sceNpManagerGetStatus(&status);
	if (ret < 0) {
		printf("sceNpGetStatus() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpGetStatus - status = %d\n", status);
#endif
	
	
	size_t stackSize = 0; // default
	int priority = 0; // default
	ret = sceNpMatching2Init2(stackSize, priority, NULL);
	if (0 > ret) {
		printf("sceNpMatching2Init() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2Init() succeeded.\n");
#endif

	//Get User NP ID///////////////////
	ret = sceNpManagerGetNpId(npId_ptr);
	if (ret < 0) {
		printf("sceNpManagerGetNpId() failed. ret = 0x%x\n", ret);
		return ret;
	}

	return ret;
}

//***********************************************
// Function: doMatching2Term()
// Description: Matching2 shutdown.
//***********************************************
int doMatching2Term()
{
#ifdef DEBUG
        printf("matching2 shutting down\n");
#endif

	//Terminate Matching2//////////////
	int ret = 0;

	ret = sceNpMatching2Term2();
	if (0 > ret) {
		printf("sceNpMatching2Term() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2Term() succeeded.\n");
#endif
	
	return ret;
}


//***********************************************
// Function: doMatching2CreateContext()
// Description: Create Matching2 context.
//***********************************************
int doMatching2CreateContext(SceNpMatching2ContextId* ctxId_ptr, const SceNpId* npId_ptr,
							 const SceNpCommunicationId* npCommId,
							 const SceNpCommunicationPassphrase* npCommPassphrase)
{
	int ret = 0;
	int option;

	// Option
	option = (SCE_NP_MATCHING2_CONTEXT_OPTION_USE_ONLINENAME |
			   SCE_NP_MATCHING2_CONTEXT_OPTION_USE_AVATARURL);
#ifdef DEBUG
	printf("set context option 0x%x\n", option);
#endif

	ret = sceNpMatching2CreateContext(npId_ptr,
					  npCommId,
					  npCommPassphrase,
					  ctxId_ptr,
					  option);
	if (ret < 0) {
		printf("sceNpMatching2CreateContext() failed. ret = 0x%x\n", ret);
#ifdef DEBUG
		printf("ctxId: 0x%x\n", (*ctxId_ptr));
#endif
		(*ctxId_ptr) = 0;
		return ret;
	}
#ifdef DEBUG
	printf("Create Context succeeded.\nContext ID: 0x%x\n", (*ctxId_ptr));
#endif
	
	return ret;
}

//***********************************************
// Function: doMatching2RegisterContextCallback()
// Description: Register Matching2 context callback.
//***********************************************
int doMatching2RegisterContextCallback(SceNpMatching2ContextId ctxId,
				       SceNpMatching2ContextCallback context_cb,
				       void* cb_arg)
{
	int ret = 0;
	ret = sceNpMatching2RegisterContextCallback(ctxId, context_cb, cb_arg);
	if(ret < 0)
	{
		printf("sceNpMatching2RegisterContextCallback() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2RegisterContextCallback() succeeded.\n");
#endif
	
	return ret;
}

//***********************************************
// Function: doMatching2CreateContext()
// Description: Create Matching2 context.
//***********************************************
int doMatching2DestroyContext(const SceNpMatching2ContextId& ctxId)
{
	int ret = 0;

	ret = sceNpMatching2DestroyContext(ctxId);
	if (0 > ret) {
		printf("sceNpMatching2DestroyContext() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2DestroyContext() succeeded.\n");
#endif

	return ret;
}

//***********************************************
// Function: doMatching2ContextStart()
// Description: Start Matching2 context.
//***********************************************
int doMatching2ContextStart(const SceNpMatching2ContextId& ctxId)
{
	int ret = 0;

	ret = sceNpMatching2ContextStart(ctxId);
	if (0 > ret) {
		printf("sceNpMatching2ContextStart() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2ContextStart() succeeded.\n");
#endif
	
	return ret;
}

//***********************************************
// Function: doMatching2StopContext()
// Description: Stop Matching2 context.
//***********************************************
int doMatching2ContextStop(const SceNpMatching2ContextId& ctxId)
{
	int ret = 0;

	ret = sceNpMatching2ContextStop(ctxId);
	if (0 > ret) {
		printf("sceNpMatching2ContextStop() failed. ret = 0x%x\n", ret);
		return ret;
	}
#ifdef DEBUG
	printf("sceNpMatching2ContextStop() succeeded.\n");
#endif

	return ret;
}

//*************************************************
// Function: doMatching2SetDefaultRequestOptParam()
// Description: Set default option parameters.
//*************************************************
bool doMatching2SetDefaultRequestOptParam(SceNpMatching2ContextId ctxId, SceNpMatching2RequestCallback matching2_request_cb)
{
	int ret = 0;
	SceNpMatching2RequestOptParam optParam;

	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = matching2_request_cb;
	optParam.cbFuncArg = NULL;
	optParam.timeout = (7 * 1000 * 1000);

	ret = sceNpMatching2SetDefaultRequestOptParam(ctxId, &optParam);
	if (ret < 0) {
		printf("sceNpMatching2SetDefaultRequestOptParam() failed. ret = 0x%x\n", ret);
		return false;
	}
#ifdef DEBUG
	printf("sceNpMatching2SetDefaultRequestOptParam() succeeded.\n");
#endif

	return true;
}

//*************************************************
// Function: sample_get_server_id_list()
// Description: Get server ID list
//*************************************************
int sample_get_server_id_list(SceNpMatching2ContextId ctxId, SceNpMatching2ServerId* server_id_array, int server_id_num)
{
	int ret;
	//Get server IDs
	ret = sceNpMatching2GetServerIdListLocal(ctxId, server_id_array, server_id_num);
	if (ret < 0) {
		printf("sceNpMatching2GetServerIdListLocal() failed. ret = 0x%x\n", ret);
	}
	
#ifdef DEBUG
	printf("num servers %d \n", ret);
#endif
	
	return ret;
}

#endif // #ifdef SN_TARGET_PS3