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
// File:        matching2_wrappers.h
//
//****************************************************

#ifndef _MATCHING2_WRAPPERS_H_
#define _MATCHING2_WRAPPERS_H_

#include <np.h>
#include <np/matching2.h>


//-----------------------------------------
// Public Function Declarations
//-----------------------------------------
int sample_matching2_init(SceNpMatching2ContextId* ctxId_ptr,
			  SceNpId* npId_ptr,
			  SceNpMatching2ContextCallback context_cb,
			  void* cb_arg,
			  const SceNpCommunicationId* npCommId,
			  const SceNpCommunicationPassphrase* npCommPassphrase);

int sample_matching2_shutdown(const SceNpMatching2ContextId& ctxId);

int sample_get_server_id_list(SceNpMatching2ContextId ctxId, 
			      SceNpMatching2ServerId* server_id_array, 
			      int server_id_num);

int sample_join_lobby(SceNpMatching2ContextId ctxId,
		     SceNpMatching2RequestId* assignedReqId_ptr,
		     SceNpMatching2LobbyId lobbyId,
		     SceNpMatching2RequestCallback callback,
		     void* cb_args);

int sample_leave_lobby(SceNpMatching2ContextId ctxId,
		       SceNpMatching2RequestId* assignedReqId_ptr,
		       SceNpMatching2LobbyId lobbyId,
		       SceNpMatching2RequestCallback callback,
		       void* cb_args);

int sample_send_lobby_invitation(SceNpMatching2ContextId ctxId,
				 SceNpMatching2RequestId* assignedReqId_ptr,
				 SceNpMatching2LobbyId lobbyId,
				 SceNpMatching2RoomId roomId,
				 SceNpMatching2LobbyMemberId lobbyMemberId,
				 SceNpMatching2RequestCallback callback,
				 void* cb_args);

int sample_send_lobby_chat_message(SceNpMatching2ContextId ctxId,
				  SceNpMatching2RequestId* assignedReqId_ptr,
				  SceNpMatching2LobbyId lobbyId,
				  const char* message,
				  int message_len,
				  SceNpMatching2RequestCallback callback,
				  void* cb_args);

int sample_register_lobby_event_callback(SceNpMatching2ContextId ctxId, 
				       SceNpMatching2LobbyEventCallback callback, 
				       void* cb_args);
					
int sample_register_lobby_message_callback(SceNpMatching2ContextId ctxId,
				       SceNpMatching2LobbyMessageCallback callback,
				       void* cb_args);

int sample_get_lobby_member_data_internal(SceNpMatching2ContextId ctxId,
					  SceNpMatching2RequestId* assignedReqId_ptr,
					  SceNpMatching2LobbyId lobbyId,
					  SceNpMatching2LobbyMemberId lobbyMemberId,
					  SceNpMatching2RequestCallback callback,
					  void* cb_args);

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
			   unsigned int passwordSlotMask);

int sample_leave_room(SceNpMatching2ContextId ctxId,
		      SceNpMatching2RequestId* assignedReqId_ptr,
		      SceNpMatching2RoomId roomId,
		      SceNpMatching2RequestCallback callback,
		      void* cb_args);

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
		       void* cb_args);

int sample_get_room_data_external_list(SceNpMatching2ContextId ctxId,
				       SceNpMatching2RequestId* assignedReqId_ptr,
				       SceNpMatching2RoomId* roomId,
				       unsigned int roomIdNum,
				       SceNpMatching2RequestCallback callback,
				       void* cb_args);

int sample_join_room(SceNpMatching2ContextId ctxId,
		     SceNpMatching2RequestId* assignedReqId_ptr,
		     SceNpMatching2RoomId roomId,
		     SceNpMatching2RequestCallback callback,
		     void* cb_args,
			 int joinGroupId);

int sample_send_room_chat_message(SceNpMatching2ContextId ctxId,
				  SceNpMatching2RequestId* assignedReqId_ptr,
				  SceNpMatching2RoomId roomId,
				  const char* message,
				  int message_len,
				  SceNpMatching2RequestCallback callback,
				  void* cb_args);

int sample_kickout_room_member( SceNpMatching2ContextId ctxId,
				SceNpMatching2RequestId* assignedReqId_ptr,
				SceNpMatching2RoomId roomId,
				SceNpMatching2RoomMemberId newownerId,
				SceNpMatching2RequestCallback callback,
				void* cb_args);

int sample_grant_room_owner(	SceNpMatching2ContextId ctxId,
				SceNpMatching2RequestId* assignedReqId_ptr,
				SceNpMatching2RoomId roomId,
				SceNpMatching2RoomMemberId memberId,
			    	SceNpMatching2RequestCallback callback,
			    	void* cb_args);

int sample_register_room_event_callback(SceNpMatching2ContextId ctxId, 
					SceNpMatching2RoomEventCallback callback, 
					void* cb_args);
					
int sample_register_room_message_callback(SceNpMatching2ContextId ctxId,
					  SceNpMatching2RoomMessageCallback callback,
					  void* cb_args);



//--------------------------------------------------------
// Helper functions
// Note: These functions are called by the above functions
//--------------------------------------------------------
int doMatching2Init(SceNpId* npId_ptr);
int doMatching2CreateContext(SceNpMatching2ContextId* ctxId_ptr, const SceNpId* npId_ptr,
							 const SceNpCommunicationId* npCommId,
							 const SceNpCommunicationPassphrase* npCommPassphrase);
int doMatching2RegisterContextCallback(SceNpMatching2ContextId ctxId, SceNpMatching2ContextCallback context_cb, void* cb_arg);
int doMatching2ContextStart(const SceNpMatching2ContextId& ctxId);
int doMatching2ContextStop(const SceNpMatching2ContextId& ctxId);
int doMatching2DestroyContext(const SceNpMatching2ContextId& ctxId);
int doMatching2Term();


bool doMatching2SetDefaultRequestOptParam(SceNpMatching2ContextId ctxId, SceNpMatching2RequestCallback matching2_request_cb);

#endif //_MATCHING2_WRAPPERS_H_

#endif // #ifdef SN_TARGET_PS3