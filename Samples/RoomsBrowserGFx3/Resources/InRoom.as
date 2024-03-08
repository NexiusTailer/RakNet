import gfx.io.GameDelegate;

function f2c_LeaveRoom()
{
	GameDelegate.call("f2c_LeaveRoom", [], this);
}

GameDelegate.addCallBack("c2f_LeaveRoom", this, "c2f_LeaveRoom");
function c2f_LeaveRoom(resultCode:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_LeaveRoom failure. Result= " + resultCode);
		
		/*
		REC_LEAVE_ROOM_UNKNOWN_ROOM_ID,
	REC_LEAVE_ROOM_CURRENTLY_IN_QUICK_JOIN,
	REC_LEAVE_ROOM_NOT_IN_ROOM,
	*/
	}
	
	gotoAndStop("CreateRoom");
}
inviteFriendButton.addEventListener("click", this, "f2c_SendInvite");
function f2c_SendInvite()
{
	GameDelegate.call("f2c_SendInvite", [playerNameTextInput.text,false], this);
}

GameDelegate.addCallBack("c2f_SendInvite", this, "c2f_SendInvite");
function c2f_SendInvite(resultCode:String, inviteeName:String, inviteToSpectatorSlot:Boolean ):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_SendInvite failure. Result= " + resultCode);
		
		/*
		REC_SEND_INVITE_UNKNOWN_ROOM_ID,
	REC_SEND_INVITE_INVITEE_ALREADY_INVITED,
	REC_SEND_INVITE_CANNOT_PERFORM_ON_SELF,
	REC_SEND_INVITE_INVITOR_ONLY_MODERATOR_CAN_INVITE, // INVITE_MODE_MODERATOR_ONLY
	REC_SEND_INVITE_INVITOR_LACK_INVITE_PERMISSIONS, // Any other INVITE_MODE
	REC_SEND_INVITE_INVITOR_NOT_IN_ROOM,
	REC_SEND_INVITE_NO_SLOTS,
	REC_SEND_INVITE_INVITEE_ALREADY_IN_THIS_ROOM,
	REC_SEND_INVITE_INVITEE_BANNED,
	REC_SEND_INVITE_RECIPIENT_NOT_ONLINE,
	REC_SEND_INVITE_ROOM_LOCKED,
	*/
	}
}
startSpectatingButton.addEventListener("click", this, "f2c_StartSpectating");
function f2c_StartSpectating()
{
	GameDelegate.call("f2c_StartSpectating", [], this);
}

GameDelegate.addCallBack("c2f_StartSpectating", this, "c2f_StartSpectating");
function c2f_StartSpectating(resultCode:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_StartSpectating failure. Result= " + resultCode);
		
		/*
		REC_START_SPECTATING_UNKNOWN_ROOM_ID,
	REC_START_SPECTATING_ALREADY_SPECTATING,
	REC_START_SPECTATING_NO_SPECTATOR_SLOTS_AVAILABLE,
	REC_START_SPECTATING_NOT_IN_ROOM,
	REC_START_SPECTATING_REASSIGN_MODERATOR_BEFORE_SPECTATE,
	REC_START_SPECTATING_ROOM_LOCKED,
	*/
	}
}
stopSpectatingButton.addEventListener("click", this, "f2c_StopSpectating");
function f2c_StopSpectating()
{
	GameDelegate.call("f2c_StopSpectating", [], this);
}

GameDelegate.addCallBack("c2f_StopSpectating", this, "c2f_StopSpectating");
function c2f_StopSpectating(resultCode:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_StopSpectating failure. Result= " + resultCode);
		
		/*
		REC_STOP_SPECTATING_UNKNOWN_ROOM_ID,
	REC_STOP_SPECTATING_NOT_IN_ROOM,
	REC_STOP_SPECTATING_NOT_CURRENTLY_SPECTATING,
	REC_STOP_SPECTATING_NO_SLOTS,
	REC_STOP_SPECTATING_ROOM_LOCKED,
	*/
	}
}
makeModeratorButton.addEventListener("click", this, "f2c_GrantModerator");
function f2c_GrantModerator()
{
	GameDelegate.call("f2c_GrantModerator", [playerNameTextInput.text], this);
}

GameDelegate.addCallBack("c2f_GrantModerator", this, "c2f_GrantModerator");
function c2f_GrantModerator(resultCode:String, newModerator:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_GrantModerator failure. Result= " + resultCode);
		
		/*
		REC_GRANT_MODERATOR_UNKNOWN_ROOM_ID,
	REC_GRANT_MODERATOR_NEW_MODERATOR_NOT_ONLINE,
	REC_GRANT_MODERATOR_NOT_IN_ROOM,
	REC_GRANT_MODERATOR_NEW_MODERATOR_NOT_IN_ROOM,
	REC_GRANT_MODERATOR_CANNOT_PERFORM_ON_SELF,
	REC_GRANT_MODERATOR_MUST_BE_MODERATOR_TO_GRANT_MODERATOR,
	REC_GRANT_MODERATOR_NEW_MODERATOR_NOT_IN_PLAYABLE_SLOT,
	*/
	}
}

readyButton.addEventListener("click", this, "f2c_SetReadyStatus_true");
unreadyButton.addEventListener("click", this, "f2c_SetReadyStatus_false");

function f2c_SetReadyStatus_true()
{
	GameDelegate.call("f2c_SetReadyStatus", [true], this);
}
function f2c_SetReadyStatus_false()
{
	GameDelegate.call("f2c_SetReadyStatus", [false], this);
}

GameDelegate.addCallBack("c2f_SetReadyStatus", this, "c2f_SetReadyStatus");
function c2f_SetReadyStatus():Void
{
	var resultCode:String = arguments[0];
	
	if (resultCode=="REC_SUCCESS")
	{
		var isReady:Boolean = arguments[1];
		var readyUsersListSize:Number = arguments[2];
		var argumentIndex=3;
		for (var i:Number = 0; i < roomMemberListSize; i++)
		{
			var userName:String = arguments[argumentIndex++];
		}	
		
		var unreadyUsersListSize:Number = arguments[argumentIndex++];
		for (var i:Number = 0; i < roomMemberListSize; i++)
		{
			var userName:String = arguments[argumentIndex++];
		}
	}
	else
	{
		trace("c2f_SetReadyStatus failure. Result= " + resultCode);
		
		/*
		REC_SET_READY_STATUS_UNKNOWN_ROOM_ID,
	REC_SET_READY_STATUS_NOT_IN_ROOM,
	REC_SET_READY_STATUS_NOT_IN_PLAYABLE_SLOT,
	REC_SET_READY_STATUS_AUTO_LOCK_ALL_PLAYERS_READY,
	*/
	}
}

function f2c_GetReadyStatus()
{
	GameDelegate.call("f2c_GetReadyStatus", [], this);
}

GameDelegate.addCallBack("c2f_GetReadyStatus", this, "c2f_GetReadyStatus");
function c2f_GetReadyStatus(resultCode:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
		var readyUsersListSize:Number = arguments[1];
		var argumentIndex=2;
		for (var i:Number = 0; i < roomMemberListSize; i++)
		{
			var userName:String = arguments[argumentIndex++];
		}	
		
		var unreadyUsersListSize:Number = arguments[argumentIndex++];
		for (var i:Number = 0; i < roomMemberListSize; i++)
		{
			var userName:String = arguments[argumentIndex++];
		}
	}
	else
	{
		trace("c2f_GetReadyStatus failure. Result= " + resultCode);
		
		/*
		REC_GET_READY_STATUS_NOT_IN_ROOM,
	REC_GET_READY_STATUS_UNKNOWN_ROOM_ID,
	*/
	}
}

lockRoomButton.addEventListener("click", this, "f2c_SetRoomLockState_Locked");
unlockRoomButton.addEventListener("click", this, "f2c_SetRoomLockState_Unlocked");
function f2c_SetRoomLockState_Locked()
{
// NOT_LOCKED
// PLAYERS_LOCKED
// ALL_LOCKED
	GameDelegate.call("f2c_SetRoomLockState", ["ALL_LOCKED"], this);
}
function f2c_SetRoomLockState_Unlocked()
{
// NOT_LOCKED
// PLAYERS_LOCKED
// ALL_LOCKED
	GameDelegate.call("f2c_SetRoomLockState", ["NOT_LOCKED"], this);
}

GameDelegate.addCallBack("c2f_SetRoomLockState", this, "c2f_SetRoomLockState");
function c2f_SetRoomLockState(resultCode:String, roomLockState:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
		if ( roomLockState=="NOT_LOCKED")
		{
			teamsAreLockedLabel.text="Not Locked";
		}
		else
		{
			teamsAreLockedLabel.text="Locked";
		}
	}
	else
	{
		trace("c2f_SetRoomLockState failure. Result= " + resultCode);
		
		/*
		REC_SET_ROOM_LOCK_STATE_UNKNOWN_ROOM_ID,
	REC_SET_ROOM_LOCK_STATE_NOT_IN_ROOM,
	REC_SET_ROOM_LOCK_STATE_MUST_BE_MODERATOR,
	REC_SET_ROOM_LOCK_STATE_BAD_ENUMERATION_VALUE,
	*/
	}
}

function f2c_GetRoomLockState()
{
	GameDelegate.call("f2c_GetRoomLockState", [], this);
}

GameDelegate.addCallBack("c2f_GetRoomLockState", this, "c2f_GetRoomLockState");
function c2f_GetRoomLockState(resultCode:String, roomLockState:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
		
		// NOT_LOCKED
		// PLAYERS_LOCKED
		// ALL_LOCKED
	
		if ( roomLockState=="NOT_LOCKED")
		{
			teamsAreLockedLabel.text="Not Locked";
		}
		else
		{
			teamsAreLockedLabel.text="Locked";
		}
	
	}
	else
	{
		trace("c2f_GetRoomLockState failure. Result= " + resultCode);
		
		/*
		REC_GET_ROOM_LOCK_STATE_UNKNOWN_ROOM_ID,
	REC_GET_ROOM_LOCK_STATE_NOT_IN_ROOM,
	*/
	}
}

function f2c_AreAllMembersReady()
{
	GameDelegate.call("f2c_AreAllMembersReady", [], this);
}

GameDelegate.addCallBack("c2f_AreAllMembersReady", this, "c2f_AreAllMembersReady");
function c2f_AreAllMembersReady(resultCode:String, allReady:Boolean):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_AreAllMembersReady failure. Result= " + resultCode);
		
		/*
		REC_ARE_ALL_MEMBERS_READY_UNKNOWN_ROOM_ID,
	REC_ARE_ALL_MEMBERS_READY_NOT_IN_ROOM,
	*/
	}
}

kickSelectedPlayerButton.addEventListener("click", this, "f2c_KickMember");
function f2c_KickMember()
{
	var memberName:String = roomMembersScrollingList.dataProvider[roomMembersScrollingList.selectedIndex];
	GameDelegate.call("f2c_KickMember", [memberName,"Reason goes here"], this);
}

GameDelegate.addCallBack("c2f_KickMember", this, "c2f_KickMember");
function c2f_KickMember(resultCode:String, kickedMember:String, reason:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
	}
	else
	{
		trace("c2f_KickMember failure. Result= " + resultCode);
		
		/*
		REC_KICK_MEMBER_UNKNOWN_ROOM_ID,
	REC_KICK_MEMBER_NOT_IN_ROOM,
	REC_KICK_MEMBER_TARGET_NOT_ONLINE,
	REC_KICK_MEMBER_TARGET_NOT_IN_YOUR_ROOM,
	REC_KICK_MEMBER_MUST_BE_MODERATOR,
	REC_KICK_MEMBER_CANNOT_PERFORM_ON_SELF,
	*/
	}
}
sendChatMessageButton.addEventListener("click", this, "f2c_Room_Chat_Func");
function f2c_Room_Chat_Func()
{
	GameDelegate.call("f2c_Room_Chat_Func", [chatTextInput.text], this);
}
GameDelegate.addCallBack("c2f_Chat_Callback", this, "c2f_Chat_Callback");
function c2f_Chat_Callback(resultCode:String, chatRecipient:String, chatTextInput:String):Void
{
	if (resultCode=="REC_SUCCESS")
	{
		trace(chatRecipient + " >> " + chatTextInput + "\n");
		chatTextArea.text+=chatRecipient + " >> " + chatTextInput + "\n";
	}
	else
	{
		trace("c2f_Chat_Callback failure. Result= " + resultCode + " when sent to " + chatRecipient);

			/*
			REC_CHAT_USER_NOT_IN_ROOM,
			REC_CHAT_RECIPIENT_NOT_ONLINE,
			REC_CHAT_RECIPIENT_NOT_IN_ANY_ROOM,
			REC_CHAT_RECIPIENT_NOT_IN_YOUR_ROOM,
			*/

	}
}

GameDelegate.addCallBack("c2f_Chat_Notification", this, "c2f_Chat_Notification");
function c2f_Chat_Notification(sender:String, chatRecipient:String, chatTextInput:String, profanityFilteredTextInput:String ):Void
{
	chatTextArea.text+=sender + " << " + chatTextInput + "\n";
}
startGameButton.addEventListener("click", this, "f2c_StartGame");
function f2c_StartGame()
{
	GameDelegate.call("f2c_StartGame", [], this);
}
GameDelegate.addCallBack("c2f_StartGame", this, "c2f_StartGame");
function c2f_StartGame( resultCode:String ):Void
{
	// Result of asking C++ to start the game
	
	if (resultCode=="START_GAME_SUCCESS")
	{
	}
	else
	{
		trace("c2f_StartGame failure. Result= " + resultCode);

			/*
			User defined codes here
			*/

	}
}
GameDelegate.addCallBack("c2f_StartGame_Notification", this, "c2f_StartGame_Notification");
function c2f_StartGame_Notification( ):Void
{
	// Tell actionscript that C++ start the game
}

GameDelegate.addCallBack("c2f_RoomMemberStartedSpectating_Callback", this, "c2f_RoomMemberStartedSpectating_Callback")
function c2f_RoomMemberStartedSpectating_Callback( userName:String ):Void
{
}
GameDelegate.addCallBack("c2f_RoomMemberStoppedSpectating_Callback", this, "c2f_RoomMemberStoppedSpectating_Callback")
function c2f_RoomMemberStoppedSpectating_Callback( userName:String ):Void
{
}

GameDelegate.addCallBack("c2f_ModeratorChanged_Callback", this, "c2f_ModeratorChanged_Callback")
function c2f_ModeratorChanged_Callback( newModerator:String, oldModerator:String ):Void
{
}
GameDelegate.addCallBack("c2f_RoomMemberReadyStatusSet_Callback", this, "c2f_RoomMemberReadyStatusSet_Callback")
function c2f_RoomMemberReadyStatusSet_Callback( ):Void
{
	var isReady:Boolean = arguments[0];
	var roomMemberName:String = arguments[1];
	var num:Number = arguments[2];
	var argumentIndex=3;
	
	// users in the ready state
	for (var i:Number = 0; i < num; i++)
	{
		var roomMemberName = arguments[argumentIndex++];
	}
	
	num = arguments[argumentIndex++];
	
	// Users in the unready state
	for (var i:Number = 0; i < num; i++)
	{
		var roomMemberName = arguments[argumentIndex++];
	}	
}
GameDelegate.addCallBack("c2f_RoomLockStateSet_Callback", this, "c2f_RoomLockStateSet_Callback")
function c2f_RoomLockStateSet_Callback(  roomLockState:String ):Void
{
	// NOT_LOCKED
	// PLAYERS_LOCKED
	// ALL_LOCKED
	if ( roomLockState=="NOT_LOCKED")
	{
		teamsAreLockedLabel.text="Not Locked";
	}
	else
	{
		teamsAreLockedLabel.text="Locked";
	}
}
function RemoveFromRoomMembersList(roomMember:String)
{
	for (var i:Number = 0; i < roomMembersScrollingList.dataProvider.length; i++)
	{
		var memberName:String = roomMembersScrollingList.dataProvider[i];
		if (memberName==roomMember)
		{
			roomMembersScrollingList.splice(i,1);
			break;
		}
	}
}
function AddToRoomMembersList(roomMember:String)
{
	for (var i:Number = 0; i < roomMembersScrollingList.dataProvider.length; i++)
	{
		var memberName:String = roomMembersScrollingList.dataProvider[i];
		if (memberName==roomMember)
		{
			return;
		}
	}
	
	roomMembersScrollingList.dataProvider.push(roomMember);
	roomMembersScrollingList.dataProvider.invalidate();
}
GameDelegate.addCallBack("c2f_RoomMemberKicked_Callback", this, "c2f_RoomMemberKicked_Callback")
function c2f_RoomMemberKicked_Callback( roomMember:String, moderator:String, reason:String ):Void
{
	RemoveFromRoomMembersList(roomMember);
	
}
GameDelegate.addCallBack("c2f_RoomMemberLeftRoom_Callback", this, "c2f_RoomMemberLeftRoom_Callback")
function c2f_RoomMemberLeftRoom_Callback( roomMember:String ):Void
{
	RemoveFromRoomMembersList(roomMember);
}
GameDelegate.addCallBack("c2f_RoomMemberJoinedRoom_Callback", this, "c2f_RoomMemberJoinedRoom_Callback")
function c2f_RoomMemberJoinedRoom_Callback( acceptedInvitorName:String, acceptedInvitorAddress:String, joiningMemberName:String, joiningMemberAddress:String ):Void
{
	AddToRoomMembersList(joiningMemberName);
}
GameDelegate.addCallBack("c2f_RoomDestroyedOnModeratorLeft_Callback", this, "c2f_RoomDestroyedOnModeratorLeft_Callback")
function c2f_RoomDestroyedOnModeratorLeft_Callback( oldModerator:String ):Void
{
	trace("The room was destroyed.");
	gotoAndStop("CreateRoom");
}

function f2c_GetRoomProperties()
{
	GameDelegate.call("f2c_GetRoomProperties", [], this);
}
GameDelegate.addCallBack("c2f_GetRoomProperties", this, "c2f_GetRoomProperties")
function c2f_GetRoomProperties( ):Void
{
	var resultCode:String = arguments[0];
	if (resultCode=="REC_SUCCESS")
	{
		roomMembersScrollingList.dataProvider=[];
		
		
		var roomName:String = arguments[1];
		var roomMemberListSize:Number = arguments[2];
		var argumentIndex=3;
		for (var i:Number = 0; i < roomMemberListSize; i++)
		{
			var roomMemberName:String = arguments[argumentIndex++];
			// RMM_MODERATOR
			// RMM_PUBLIC
			// RMM_RESERVED
			// RMM_SPECTATOR_PUBLIC
			// RMM_SPECTATOR_RESERVED
			// RMM_ANY_PLAYABLE
			// RMM_ANY_SPECTATOR
			var roomMemberMode:String = arguments[argumentIndex++];
			var roomMemberIsReady:Boolean = arguments[argumentIndex++];
			var roomMemberAddress:String = arguments[argumentIndex++];
			var roomMemberGuid:String = arguments[argumentIndex++];
			
			
			roomMembersScrollingList.dataProvider.push(roomMemberName);
		}
		roomMembersScrollingList.dataProvider.invalidate();
		
		var banListSize:Number = arguments[argumentIndex++];
		for (var i:Number = 0; i < banListSize; i++)
		{
			var roomMemberName:String = arguments[argumentIndex++];		
			var reason:String = arguments[argumentIndex++];
		}	
		
		// NOT_LOCKED
		// PLAYERS_LOCKED
		// ALL_LOCKED
		var roomLockState:String = arguments[argumentIndex++];
		var roomId:Number = arguments[argumentIndex++];
		var automaticallyLockRoomWhenAllUsersAreReady:Boolean = arguments[argumentIndex++];
		var roomIsHiddenFromSearches:Boolean = arguments[argumentIndex++];
		
		// For both inviteToRoomPermission and inviteToSpectatorSlotPermission
		// INVITE_MODE_ANYONE_CAN_INVITE,
		// INVITE_MODE_MODERATOR_CAN_INVITE,
		// INVITE_MODE_PUBLIC_SLOTS_CAN_INVITE,
		// INVITE_MODE_RESERVED_SLOTS_CAN_INVITE,
		// INVITE_MODE_SPECTATOR_SLOTS_CAN_INVITE,
		// INVITE_MODE_MODERATOR_OR_PUBLIC_SLOTS_CAN_INVITE,
		// INVITE_MODE_MODERATOR_OR_PUBLIC_OR_RESERVED_SLOTS_CAN_INVITE,
		var inviteToRoomPermission:String = arguments[argumentIndex++];
		var inviteToSpectatorSlotPermission:String = arguments[argumentIndex++];
				
		roomNameLabel.text=roomName;
		if ( roomLockState=="NOT_LOCKED")
		{
			teamsAreLockedLabel.text="Not Locked";
		}
		else
		{
			teamsAreLockedLabel.text="Locked";
		}
	}
	else
	{
		trace("c2f_GetRoomProperties failure. Result= " + resultCode);
		
		/*
		REC_GET_ROOM_PROPERTIES_EMPTY_ROOM_NAME_AND_NOT_IN_A_ROOM,
		REC_GET_ROOM_PROPERTIES_UNKNOWN_ROOM_NAME,
		*/
	}
}

f2c_GetRoomProperties();
