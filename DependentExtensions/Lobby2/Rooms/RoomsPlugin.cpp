#include "RoomsPlugin.h"
#include "BitStream.h"
#include "RoomsErrorCodes.h"
#include "TableSerializer.h"
#include "RakAssert.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "ProfanityFilter.h"

using namespace RakNet;

int RoomsPlugin::RoomsPluginParticipantCompByRakString( const RakNet::RakString &key, RoomsPluginParticipant* const &data )
{
	return strcmp(key.C_String(), data->GetName().C_String());
}

void RoomsPluginFunc::PrintResult(void)
{
	printf("Result for user %s: %s\n", userName.C_String(), RoomsErrorCodeDescription::ToEnglish(resultCode));
}
void CreateRoom_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_CREATE_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	networkedRoomCreationParameters.Serialize(writeToBitstream, bitStream);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, gameIdentifier);
}
void CreateRoom_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	bitStream->Serialize( writeToBitstream, roomId );
}

void EnterRoom_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_ENTER_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	networkedRoomCreationParameters.Serialize(writeToBitstream, bitStream);
	bitStream->Serialize(writeToBitstream, roomMemberMode);
	query.Serialize(writeToBitstream, bitStream);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, gameIdentifier);
}
void EnterRoom_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	bitStream->Serialize( writeToBitstream, createdRoom );
	joinedRoomResult.Serialize(writeToBitstream, bitStream);
	bitStream->Serialize( writeToBitstream, roomId );
}

void JoinByFilter_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_JOIN_BY_FILTER;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, gameIdentifier);
	bitStream->Serialize(writeToBitstream, roomMemberMode);
	bitStream->Serialize(writeToBitstream, userName);
	query.Serialize(writeToBitstream, bitStream);
}
void JoinByFilter_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	joinedRoomResult.Serialize(writeToBitstream, bitStream);
}

void LeaveRoom_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_LEAVE_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream,userName);
}
void LeaveRoom_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	removeUserResult.Serialize(writeToBitstream, bitStream);
}

void GetInvitesToParticipant_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GET_INVITES_TO_PARTICIPANT;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
}
void GetInvitesToParticipant_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	unsigned int i;
	unsigned int listSize;
	listSize=invitedUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	for (i=0; i < listSize; i++)
	{
		if (writeToBitstream)
			invitedUsers[i].Serialize(true,bitStream);
		else
		{
			InvitedUser invitedUser;
			invitedUser.Serialize(false,bitStream);
			invitedUsers.Insert(invitedUser);
		}
	}
}

void SendInvite_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SEND_INVITE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, inviteeName);
	bitStream->Serialize(writeToBitstream, inviteToSpectatorSlot);
	bitStream->Serialize(writeToBitstream, subject);
	bitStream->Serialize(writeToBitstream, body);
}
void SendInvite_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void AcceptInvite_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_ACCEPT_INVITE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, inviteSender);
	bitStream->Serialize(writeToBitstream, roomId);
}
void AcceptInvite_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void StartSpectating_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_START_SPECTATING;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
}
void StartSpectating_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void StopSpectating_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_STOP_SPECTATING;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
}
void StopSpectating_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void GrantModerator_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GRANT_MODERATOR;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, newModerator);
}
void GrantModerator_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void ChangeSlotCounts_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_CHANGE_SLOT_COUNTS;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	slots.Serialize(writeToBitstream, bitStream);
}
void ChangeSlotCounts_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void SetCustomRoomProperties_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SET_CUSTOM_ROOM_PROPERTIES;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	if (writeToBitstream)
		TableSerializer::SerializeTable(&table, bitStream);
	else
		TableSerializer::DeserializeTable(bitStream, &table);
}
void SetCustomRoomProperties_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );

}
void GetRoomProperties_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GET_ROOM_PROPERTIES;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, roomName);
}
void GetRoomProperties_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	roomDescriptor.Serialize(writeToBitstream,bitStream);
}
void ChangeRoomName_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_CHANGE_ROOM_NAME;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, newRoomName);
}
void ChangeRoomName_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void SetHiddenFromSearches_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SET_HIDDEN_FROM_SEARCHES;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, hiddenFromSearches);
}
void SetHiddenFromSearches_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void SetDestroyOnModeratorLeave_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SET_DESTROY_ON_MODERATOR_LEAVE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, destroyOnModeratorLeave);
}
void SetDestroyOnModeratorLeave_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void SetReadyStatus_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SET_READY_STATUS;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, isReady);
}
void SetReadyStatus_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );

	unsigned int listSize;
	unsigned int i;
	RakNet::RakString name;
	listSize=readyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, readyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			readyUsers.Insert(name);
		}
	}

	listSize=unreadyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, unreadyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			unreadyUsers.Insert(name);
		}
	}
}

void GetReadyStatus_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GET_READY_STATUS;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	unsigned int listSize;
	RakNet::RakString name;
	unsigned int i;
	listSize=readyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, readyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			readyUsers.Insert(name);
		}
	}

	listSize=unreadyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, unreadyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			unreadyUsers.Insert(name);
		}
	}
}
void GetReadyStatus_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void SetRoomLockState_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SET_ROOM_LOCK_STATE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, roomLockState);
}
void SetRoomLockState_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void GetRoomLockState_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GET_ROOM_LOCK_STATE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
}
void GetRoomLockState_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	bitStream->Serialize(writeToBitstream, roomLockState);
}

void AreAllMembersReady_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_ARE_ALL_MEMBERS_READY;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, allReady);
}
void AreAllMembersReady_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void KickMember_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_KICK_MEMBER;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, kickedMember);
	bitStream->Serialize(writeToBitstream, reason);
}
void KickMember_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void UnbanMember_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_UNBAN_MEMBER;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, bannedMemberName);
}
void UnbanMember_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void GetBanReason_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_GET_BAN_REASON;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, reason);
}
void GetBanReason_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void AddUserToQuickJoin_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_ADD_USER_TO_QUICK_JOIN;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, gameIdentifier);
	networkedQuickJoinUser.Serialize(writeToBitstream, bitStream);
}
void AddUserToQuickJoin_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void RemoveUserFromQuickJoin_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_REMOVE_USER_FROM_QUICK_JOIN;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
}
void RemoveUserFromQuickJoin_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}

void IsInQuickJoin_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_IS_IN_QUICK_JOIN;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, isInQuickJoin);
}
void IsInQuickJoin_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}
SearchByFilter_Func::~SearchByFilter_Func()
{
	for (unsigned int i=0; i < roomsOutput.Size(); i++)
		RakNet::OP_DELETE(roomsOutput[i], __FILE__, __LINE__);
}
void SearchByFilter_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_SEARCH_BY_FILTER;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, gameIdentifier);
	bitStream->Serialize(writeToBitstream, onlyJoinable);
	roomQuery.Serialize(writeToBitstream, bitStream);
}
void SearchByFilter_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
	unsigned int i;
	unsigned int listSize;
	listSize=roomsOutput.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	for (i=0; i < listSize; i++)
	{
		if (writeToBitstream)
			roomsOutput[i]->Serialize(true,bitStream);
		else
		{
			RoomDescriptor *desc = RakNet::OP_NEW<RoomDescriptor>( __FILE__, __LINE__ );
			desc->Serialize(false,bitStream);
			roomsOutput.Insert(desc);
		}
	}
}

void ChangeHandle_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_CHANGE_HANDLE;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, newHandle);
}
void ChangeHandle_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}
void Chat_Func::SerializeIn(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPO_CHAT;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, chatMessage);
	bitStream->Serialize(writeToBitstream, privateMessageRecipient);
	bitStream->Serialize(writeToBitstream, chatDirectedToRoom);
}
void Chat_Func::SerializeOut(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	SerializeIn( writeToBitstream, bitStream );
	bitStream->Serialize( writeToBitstream, resultCode );
}
void QuickJoinExpired_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_QUICK_JOIN_EXPIRED;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	networkedQuickJoinUser.Serialize(writeToBitstream, bitStream);
}
void QuickJoinEnteredRoom_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_QUICK_JOIN_ENTERED_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	joinedRoomResult.Serialize(writeToBitstream, bitStream);
}
void RoomMemberStartedSpectating_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_STARTED_SPECTATING;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, roomId);
}
void RoomMemberStoppedSpectating_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_STARTED_SPECTATING;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, userName);
	bitStream->Serialize(writeToBitstream, roomId);
}
void ModeratorChanged_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_MODERATOR_CHANGED;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, newModerator);
	bitStream->Serialize(writeToBitstream, oldModerator);
	bitStream->Serialize(writeToBitstream, roomId);
}
void SlotCountsSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_SLOT_COUNTS_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	slots.Serialize(writeToBitstream, bitStream);
	bitStream->Serialize(writeToBitstream, roomId);
}
void CustomRoomPropertiesSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_CUSTOM_ROOM_PROPERTIES_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	if (writeToBitstream)
	{
		if (tablePtr==0)
			tablePtr=&table;
		TableSerializer::SerializeTable(tablePtr, bitStream);
	}
	else
	{
		TableSerializer::DeserializeTable(bitStream, &table);
		tablePtr=&table;
	}
}
void RoomNameSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_NAME_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, oldName);
	bitStream->Serialize(writeToBitstream, newName);
	bitStream->Serialize(writeToBitstream, roomId);
}
void HiddenFromSearchesSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_HIDDEN_FROM_SEARCHES_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, hiddenFromSearches);
}
void RoomMemberReadyStatusSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_READY_STATUS_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, isReady);
	bitStream->Serialize(writeToBitstream, roomMember);

	unsigned int listSize;
	unsigned int i;
	RakNet::RakString name;
	listSize=readyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, readyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			readyUsers.Insert(name);
		}
	}

	listSize=unreadyUsers.Size();
	bitStream->Serialize(writeToBitstream, listSize);
	if (writeToBitstream)
	{
		for (i=0; i < listSize; i++)
			bitStream->Serialize(writeToBitstream, unreadyUsers[i]);
	}
	else
	{
		for (i=0; i < listSize; i++)
		{
			bitStream->Serialize(writeToBitstream, name);
			unreadyUsers.Insert(name);
		}
	}
}
void RoomLockStateSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_LOCK_STATE_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, roomLockState);
}
void RoomMemberKicked_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_KICKED;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, kickedMember);
	bitStream->Serialize(writeToBitstream, moderator);
	bitStream->Serialize(writeToBitstream, reason);
}
void RoomMemberHandleSet_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_HANDLE_SET;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, oldName);
	bitStream->Serialize(writeToBitstream, newName);
	bitStream->Serialize(writeToBitstream, roomId);
}
void RoomMemberLeftRoom_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_LEFT_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomMember);
	bitStream->Serialize(writeToBitstream, roomId);
}
void RoomMemberJoinedRoom_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_MEMBER_JOINED_ROOM;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	if (joinedRoomResult==0 && writeToBitstream==false)
		joinedRoomResult = RakNet::OP_NEW<JoinedRoomResult>( __FILE__, __LINE__ );
	joinedRoomResult->Serialize(writeToBitstream, bitStream);
}
void RoomInvitationSent_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_INVITATION_SENT;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, invitorName);
	bitStream->Serialize(writeToBitstream, inviteeName);
	bitStream->Serialize(writeToBitstream, inviteToSpectatorSlot);
	bitStream->Serialize(writeToBitstream, subject);
	bitStream->Serialize(writeToBitstream, body);
	bitStream->Serialize(writeToBitstream, roomId);
}
void RoomInvitationWithdrawn_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_INVITATION_WITHDRAWN;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	invitedUser.Serialize(writeToBitstream, bitStream);
}
void RoomDestroyedOnModeratorLeft_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_ROOM_DESTROYED_ON_MODERATOR_LEFT;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, roomId);
	bitStream->Serialize(writeToBitstream, oldModerator);
	roomDescriptor.Serialize(writeToBitstream, bitStream);
}
void Chat_Notification::Serialize(bool writeToBitstream, RakNet::BitStream *bitStream)
{
	MessageID messageId = RPN_CHAT_NOTIFICATION;
	bitStream->Serialize(writeToBitstream, messageId);
	bitStream->Serialize(writeToBitstream, recipient);
	bitStream->Serialize(writeToBitstream, sender);
	bitStream->Serialize(writeToBitstream, privateMessageRecipient);
	bitStream->Serialize(writeToBitstream, chatMessage);
	bitStream->Serialize(writeToBitstream, filteredChatMessage);
}
RoomsPlugin::RoomsPlugin()
{
	lastUpdateTime=0;
	orderingChannel=0;
	profanityFilter=0;
	packetPriority=HIGH_PRIORITY;
	serverAddress=UNASSIGNED_SYSTEM_ADDRESS;
	roomsCallback=this;
}
RoomsPlugin::~RoomsPlugin()
{
	Clear();
}
void RoomsPlugin::SetOrderingChannel(char oc)
{
	orderingChannel=oc;
}
void RoomsPlugin::SetSendPriority(PacketPriority pp)
{
	packetPriority=pp;
}

void RoomsPlugin::SetRoomsCallback(RoomsCallback *_roomsCallback)
{
	roomsCallback=_roomsCallback;
}
void RoomsPlugin::ExecuteFunc(RoomsPluginFunc *func)
{
	ExecuteFunc(func,serverAddress);
}
void RoomsPlugin::ExecuteFunc(RoomsPluginFunc *func, SystemAddress remoteAddress)
{
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_ROOMS_EXECUTE_FUNC);
	if (IsServer())
		func->SerializeOut(true, &bs);
	else
		func->SerializeIn(true, &bs);
	SendUnified(&bs, packetPriority, RELIABLE_ORDERED, orderingChannel, remoteAddress, false);
}

void RoomsPlugin::ExecuteNotification(RoomsPluginNotification *func, RoomsPluginParticipant *recipient)
{
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_ROOMS_EXECUTE_FUNC);
	func->recipient=recipient->GetName();
	func->Serialize(true, &bs);
	SendUnified(&bs, packetPriority, RELIABLE_ORDERED, orderingChannel, recipient->GetSystemAddress(), false);
}
void RoomsPlugin::SetServerAddress( SystemAddress systemAddress )
{
	serverAddress=systemAddress;
}
bool RoomsPlugin::LoginRoomsParticipant(RakNet::RakString userName, SystemAddress roomsParticipantAddress, RakNetGUID guid, SystemAddress loginServerAddress)
{
	if (loginServerAddress!=UNASSIGNED_SYSTEM_ADDRESS && loginServers.GetIndexOf(loginServerAddress)==(unsigned int) -1)
		return false;
	bool objectExists;
	unsigned int index;
	index=roomsParticipants.GetIndexFromKey(userName, &objectExists);
	if (objectExists==false)
	{
		RoomsPluginParticipant *rpp = RakNet::OP_NEW<RoomsPluginParticipant>( __FILE__, __LINE__ );
		rpp->SetSystemAddress(roomsParticipantAddress);
		rpp->SetGUID(guid);
		rpp->SetName(userName);
		roomsParticipants.InsertAtIndex(rpp, index);
		return true;
	}
	return false;
}
bool RoomsPlugin::LogoffRoomsParticipant(RakNet::RakString userName, SystemAddress loginServerAddress)
{
	if (loginServerAddress!=UNASSIGNED_SYSTEM_ADDRESS && loginServers.GetIndexOf(loginServerAddress)==(unsigned int) -1)
		return false;
	bool objectExists;
	unsigned int index;
	index=roomsParticipants.GetIndexFromKey(userName, &objectExists);
	if (objectExists==true)
	{
		RemoveUserResult removeUserResult;
		roomsContainer.RemoveUser(roomsParticipants[index], &removeUserResult);
		ProcessRemoveUserResult(&removeUserResult);
		RakNet::OP_DELETE(roomsParticipants[index], __FILE__, __LINE__);
		roomsParticipants.RemoveAtIndex(index);
		return true;
	}
	return false;
}
void RoomsPlugin::ClearRoomMembers(void)
{
	unsigned int i;
	for (i=0; i < roomsParticipants.Size(); i++)
		RakNet::OP_DELETE(roomsParticipants[i], __FILE__, __LINE__);
	roomsParticipants.Clear();
}
void RoomsPlugin::SerializeLogin(RakNet::RakString userName, SystemAddress userAddress, RakNetGUID guid, RakNet::BitStream *bs)
{
	bs->Write((MessageID)ID_ROOMS_LOGON_STATUS);
	bs->Write(userName);
	bs->Write(true);
	bs->Write(userAddress);
	bs->Write(guid);
}
void RoomsPlugin::SerializeLogoff(RakNet::RakString userName, RakNet::BitStream *bs)
{
	bs->Write((MessageID)ID_ROOMS_LOGON_STATUS);
	bs->Write(userName);
	bs->Write(false);
}
void RoomsPlugin::SerializeChangeHandle(RakNet::RakString oldHandle, RakNet::RakString newHandle, RakNet::BitStream *bs)
{
	bs->Write((MessageID)ID_ROOMS_HANDLE_CHANGE);
	bs->Write(oldHandle);
	bs->Write(newHandle);
}
void RoomsPlugin::ChangeHandle(RakNet::RakString oldHandle, RakNet::RakString newHandle)
{
	RoomMemberHandleSet_Notification notification;
	notification.oldName=oldHandle;
	notification.newName=newHandle;
	roomsContainer.ChangeHandle( oldHandle, newHandle );
	RoomsPluginParticipant* roomsPluginParticipant = GetParticipantByHandle( newHandle, UNASSIGNED_SYSTEM_ADDRESS);
	if (roomsPluginParticipant && roomsPluginParticipant->GetRoom())
	{
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
}
void RoomsPlugin::AddLoginServerAddress(SystemAddress systemAddress)
{
	unsigned int index = loginServers.GetIndexOf(systemAddress);
	if (index==(unsigned int) -1)
		loginServers.Push(systemAddress);
}
void RoomsPlugin::RemoveLoginServerAddress(SystemAddress systemAddress)
{
	unsigned int index = loginServers.GetIndexOf(systemAddress);
	if (index!=(unsigned int) -1)
		loginServers.RemoveAtIndexFast(index);
}
void RoomsPlugin::ClearLoginServerAdddresses(void)
{
	loginServers.Clear();
}
void RoomsPlugin::SetProfanityFilter(ProfanityFilter *pf)
{
	profanityFilter=pf;
}
void RoomsPlugin::OnDetach(void)
{
	Clear();
}
void RoomsPlugin::OnShutdown(void)
{
	Clear();
}
void RoomsPlugin::Update(void)
{
	if (IsServer()==false)
		return;

	DataStructures::List<QuickJoinUser*> timeoutExpired;
	DataStructures::List<QuickJoinUser*> dereferencedPointers;
	DataStructures::List<JoinedRoomResult> joinedRoomMembers;

	RakNetTime curTime = RakNet::GetTime();
	if (lastUpdateTime!=0)
	{
		RakNetTime elapsedTime = curTime-lastUpdateTime;
		roomsContainer.ProcessQuickJoins( timeoutExpired, joinedRoomMembers, dereferencedPointers, elapsedTime );
		unsigned int i;
		for (i=0; i < timeoutExpired.Size(); i++)
		{
			QuickJoinExpired_Notification notification;
			notification.networkedQuickJoinUser=timeoutExpired[i]->networkedQuickJoinUser;
			ExecuteNotification(&notification, ((RoomsPluginParticipant*)timeoutExpired[i]->roomsParticipant));
		}
		for (i=0; i < joinedRoomMembers.Size(); i++)
		{
			((RoomsPluginParticipant*)joinedRoomMembers[i].joiningMember)->lastRoomJoined=joinedRoomMembers[i].roomOutput->GetID();

			QuickJoinEnteredRoom_Notification notificationToTarget;
			notificationToTarget.joinedRoomResult=joinedRoomMembers[i];
			notificationToTarget.joinedRoomResult.agrc=&roomsContainer;
			ExecuteNotification(&notificationToTarget, ((RoomsPluginParticipant*)joinedRoomMembers[i].joiningMember));
		}

		for (i=0; i < joinedRoomMembers.Size(); i++)
		{
			RoomMemberJoinedRoom_Notification notificationToRoom;
			notificationToRoom.joinedRoomResult=&joinedRoomMembers[i];
			notificationToRoom.joinedRoomResult->agrc=&roomsContainer;
			ExecuteNotificationToOtherRoomMembers(joinedRoomMembers[i].joiningMember->GetRoom(), (RoomsPluginParticipant*)joinedRoomMembers[i].joiningMember, &notificationToRoom);
			notificationToRoom.joinedRoomResult=0;
		}

		for (i=0; i < dereferencedPointers.Size(); i++)
			RakNet::OP_DELETE(dereferencedPointers[i], __FILE__, __LINE__);
	}

	lastUpdateTime=curTime;
}
PluginReceiveResult RoomsPlugin::OnReceive(Packet *packet)
{
	switch (packet->data[0]) 
	{
	case ID_ROOMS_EXECUTE_FUNC:
		OnRoomsExecuteFunc(packet);
		break;
	case ID_ROOMS_LOGON_STATUS:
		OnLoginStatus(packet);
		break;
	case ID_ROOMS_HANDLE_CHANGE:
		OnHandleChange(packet);
		break;
	}

	return RR_CONTINUE_PROCESSING;
}
void RoomsPlugin::OnLoginStatus(Packet *packet)
{
	unsigned int i;
	for (i=0; i < loginServers.Size(); i++)
	{
		if (loginServers[i]==packet->systemAddress)
		{
			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(1);
			RakNet::RakString name;
			bs.Read(name);
			bool loggedOn;
			bs.Read(loggedOn);
			SystemAddress userAddress;
			RakNetGUID guid;
			if (loggedOn)
			{
				bs.Read(userAddress);	
				bs.Read(guid);
				LoginRoomsParticipant(name, userAddress, guid, packet->systemAddress);
			}
			else
			{
				LogoffRoomsParticipant(name, packet->systemAddress);
			}
			break;
		}
	}
}
void RoomsPlugin::OnHandleChange(Packet *packet)
{
	unsigned int i;
	for (i=0; i < loginServers.Size(); i++)
	{
		if (loginServers[i]==packet->systemAddress)
		{
			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(1);
			RakNet::RakString oldHandle, newHandle;
			bs.Read(oldHandle);
			bs.Read(newHandle);
			ChangeHandle(oldHandle, newHandle);
			break;
		}
	}
}
void RoomsPlugin::OnRoomsExecuteFunc(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(1);
	if (packet->length<2)
		return;
	if (roomsCallback==0)
		return;
	switch (packet->data[1])
	{
	case RPO_CREATE_ROOM:
		{
			CreateRoom_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->CreateRoom_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_ENTER_ROOM:
		{
			EnterRoom_Func func;
			func.joinedRoomResult.agrc=&roomsContainer;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->EnterRoom_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_JOIN_BY_FILTER:
		{
			JoinByFilter_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->JoinByFilter_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_LEAVE_ROOM:
		{
			LeaveRoom_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->LeaveRoom_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GET_INVITES_TO_PARTICIPANT:
		{
			GetInvitesToParticipant_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GetInvitesToParticipant_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SEND_INVITE:
		{
			SendInvite_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SendInvite_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_ACCEPT_INVITE:
		{
			AcceptInvite_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->AcceptInvite_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_START_SPECTATING:
		{
			StartSpectating_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->StartSpectating_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_STOP_SPECTATING:
		{
			StopSpectating_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->StopSpectating_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GRANT_MODERATOR:
		{
			GrantModerator_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GrantModerator_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_CHANGE_SLOT_COUNTS:
		{
			ChangeSlotCounts_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->ChangeSlotCounts_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SET_CUSTOM_ROOM_PROPERTIES:
		{
			SetCustomRoomProperties_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SetCustomRoomProperties_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GET_ROOM_PROPERTIES:
		{
			GetRoomProperties_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GetRoomProperties_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_CHANGE_ROOM_NAME:
		{
			ChangeRoomName_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->ChangeRoomName_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SET_HIDDEN_FROM_SEARCHES:
		{
			SetHiddenFromSearches_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SetHiddenFromSearches_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SET_DESTROY_ON_MODERATOR_LEAVE:
		{
			SetDestroyOnModeratorLeave_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SetDestroyOnModeratorLeave_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SET_READY_STATUS:
		{
			SetReadyStatus_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SetReadyStatus_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GET_READY_STATUS:
		{
			GetReadyStatus_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GetReadyStatus_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SET_ROOM_LOCK_STATE:
		{
			SetRoomLockState_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SetRoomLockState_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GET_ROOM_LOCK_STATE:
		{
			GetRoomLockState_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GetRoomLockState_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_ARE_ALL_MEMBERS_READY:
		{
			AreAllMembersReady_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->AreAllMembersReady_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_KICK_MEMBER:
		{
			KickMember_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->KickMember_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_UNBAN_MEMBER:
		{
			UnbanMember_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->UnbanMember_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_GET_BAN_REASON:
		{
			GetBanReason_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->GetBanReason_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_ADD_USER_TO_QUICK_JOIN:
		{
			AddUserToQuickJoin_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->AddUserToQuickJoin_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_REMOVE_USER_FROM_QUICK_JOIN:
		{
			RemoveUserFromQuickJoin_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->RemoveUserFromQuickJoin_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_IS_IN_QUICK_JOIN:
		{
			IsInQuickJoin_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->IsInQuickJoin_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_SEARCH_BY_FILTER:
		{
			SearchByFilter_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->SearchByFilter_Callback(packet->systemAddress,&func);
		}
		break;

	case RPO_CHANGE_HANDLE:
		{
			ChangeHandle_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->ChangeHandle_Callback(packet->systemAddress,&func);
		}
		break;
	case RPO_CHAT:
		{
			Chat_Func func;
			if (IsServer()==false)
				func.SerializeOut(false,&bs);
			else
				func.SerializeIn(false,&bs);
			roomsCallback->Chat_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_QUICK_JOIN_EXPIRED:
		{
			QuickJoinExpired_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->QuickJoinExpired_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_QUICK_JOIN_ENTERED_ROOM:
		{
			QuickJoinEnteredRoom_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->QuickJoinEnteredRoom_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_STARTED_SPECTATING:
		{
			RoomMemberStartedSpectating_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberStartedSpectating_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_STOPPED_SPECTATING:
		{
			RoomMemberStoppedSpectating_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberStoppedSpectating_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_MODERATOR_CHANGED:
		{
			ModeratorChanged_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->ModeratorChanged_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_SLOT_COUNTS_SET:
		{
			SlotCountsSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->SlotCountsSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_CUSTOM_ROOM_PROPERTIES_SET:
		{
			CustomRoomPropertiesSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->CustomRoomPropertiesSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_NAME_SET:
		{
			RoomNameSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomNameSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_HIDDEN_FROM_SEARCHES_SET:
		{
			HiddenFromSearchesSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->HiddenFromSearchesSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_READY_STATUS_SET:
		{
			RoomMemberReadyStatusSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberReadyStatusSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_LOCK_STATE_SET:
		{
			RoomLockStateSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomLockStateSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_KICKED:
		{
			RoomMemberKicked_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberKicked_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_HANDLE_SET:
		{
			RoomMemberHandleSet_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberHandleSet_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_LEFT_ROOM:
		{
			RoomMemberLeftRoom_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberLeftRoom_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_MEMBER_JOINED_ROOM:
		{
			RoomMemberJoinedRoom_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomMemberJoinedRoom_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_INVITATION_SENT:
		{
			RoomInvitationSent_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomInvitationSent_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_INVITATION_WITHDRAWN:
		{
			RoomInvitationWithdrawn_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomInvitationWithdrawn_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_ROOM_DESTROYED_ON_MODERATOR_LEFT:
		{
			RoomDestroyedOnModeratorLeft_Notification func;
			func.Serialize(IsServer(),&bs);
			roomsCallback->RoomDestroyedOnModeratorLeft_Callback(packet->systemAddress,&func);
		}
		break;
	case RPN_CHAT_NOTIFICATION:
		{
			Chat_Notification func;
			func.Serialize(IsServer(),&bs);
			// When the filtered chat message is empty, that means the original chat message didn't have profanity anyway.
			if (func.filteredChatMessage.IsEmpty())
				func.filteredChatMessage=func.chatMessage;
			roomsCallback->Chat_Callback(packet->systemAddress,&func);
		}
		break;
	}
}
void RoomsPlugin::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	RemoveUserResult removeUserResult;
	unsigned i;
	i=0;
	while (i < roomsParticipants.Size())
	{
		if (roomsParticipants[i]->GetSystemAddress()==systemAddress)
		{
			roomsContainer.RemoveUser(roomsParticipants[i], &removeUserResult);
			ProcessRemoveUserResult(&removeUserResult);
			RakNet::OP_DELETE(roomsParticipants[i], __FILE__, __LINE__);
			roomsParticipants.RemoveAtIndex(i);
		}
		else
		{
			i++;
		}
	}

}
void RoomsPlugin::Clear(void)
{
	ClearRoomMembers();
	ClearLoginServerAdddresses();
}
bool RoomsPlugin::IsServer(void) const
{
	return roomsCallback==this;
}
RoomsPlugin::RoomsPluginParticipant* RoomsPlugin::GetParticipantByHandle(RakNet::RakString handle, SystemAddress senderAddress)
{
	if (roomsParticipants.HasData(handle))
	{
		RoomsPluginParticipant *rp = roomsParticipants.GetElementFromKey(handle);
		RakAssert(IsServer());
		if (senderAddress==UNASSIGNED_SYSTEM_ADDRESS || senderAddress==serverAddress)
			return rp;
		if (rp->GetSystemAddress()!=senderAddress)
			return 0;
		return rp;
	}
	return 0;
}
RoomsPlugin::RoomsPluginParticipant* RoomsPlugin::ValidateUserHandle(RoomsPluginFunc* func, SystemAddress systemAddress)
{
	RoomsPluginParticipant* roomsPluginParticipant = GetParticipantByHandle(func->userName, systemAddress);
	if (roomsPluginParticipant==0)
	{
		func->resultCode=REC_NOT_LOGGED_IN;
		ExecuteFunc(func, systemAddress);
	}
	return roomsPluginParticipant;
}
void RoomsPlugin::CreateRoom_Callback( SystemAddress senderAddress, CreateRoom_Func *callResult)
{
	RoomCreationParameters rcp;
	rcp.networkedRoomCreationParameters=callResult->networkedRoomCreationParameters;
	rcp.gameIdentifier=callResult->gameIdentifier;
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	rcp.firstUser=roomsPluginParticipant;
		
	callResult->resultCode=roomsContainer.CreateRoom(&rcp, profanityFilter);
	if (callResult->resultCode==REC_SUCCESS)
	{
		roomsPluginParticipant->lastRoomJoined=roomsPluginParticipant->GetRoom()->GetID();
		callResult->roomId=roomsPluginParticipant->lastRoomJoined;
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::EnterRoom_Callback( SystemAddress senderAddress, EnterRoom_Func *callResult)
{
	RoomCreationParameters rcp;
	rcp.networkedRoomCreationParameters=callResult->networkedRoomCreationParameters;
	rcp.gameIdentifier=callResult->gameIdentifier;
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	rcp.firstUser=roomsPluginParticipant;

	callResult->resultCode=roomsContainer.EnterRoom(&rcp, callResult->roomMemberMode, profanityFilter, &callResult->query, &callResult->joinedRoomResult);
	callResult->createdRoom=rcp.createdRoom;
	if (callResult->resultCode==REC_SUCCESS)
	{
		roomsPluginParticipant->lastRoomJoined=roomsPluginParticipant->GetRoom()->GetID();
		callResult->roomId=roomsPluginParticipant->lastRoomJoined;

		if (callResult->joinedRoomResult.roomOutput)
		{
			RoomMemberJoinedRoom_Notification notificationToRoom;
			notificationToRoom.joinedRoomResult=&callResult->joinedRoomResult;
			ExecuteNotificationToOtherRoomMembers(callResult->joinedRoomResult.roomOutput, roomsPluginParticipant, &notificationToRoom);
			notificationToRoom.joinedRoomResult=0;
		}
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::JoinByFilter_Callback( SystemAddress senderAddress, JoinByFilter_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.JoinByFilter(callResult->gameIdentifier, callResult->roomMemberMode, roomsPluginParticipant, roomsPluginParticipant->lastRoomJoined, &callResult->query, &callResult->joinedRoomResult );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RakAssert(callResult->joinedRoomResult.roomOutput->GetNumericProperty(DefaultRoomColumns::TC_USED_SLOTS)==callResult->joinedRoomResult.roomOutput->roomMemberList.Size()-1);

		roomsPluginParticipant->lastRoomJoined=roomsPluginParticipant->GetRoom()->GetID();

		RoomMemberJoinedRoom_Notification notificationToRoom;
		notificationToRoom.joinedRoomResult=&callResult->joinedRoomResult;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notificationToRoom);
		notificationToRoom.joinedRoomResult=0;
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::LeaveRoom_Callback( SystemAddress senderAddress, LeaveRoom_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.LeaveRoom( roomsPluginParticipant, &callResult->removeUserResult );
	ProcessRemoveUserResult(&callResult->removeUserResult);
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GetInvitesToParticipant_Callback( SystemAddress senderAddress, GetInvitesToParticipant_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	DataStructures::List<InvitedUser*> invitedUsers;
	callResult->resultCode=roomsContainer.GetInvitesToParticipant( roomsPluginParticipant, invitedUsers );
	unsigned int i;
	for (i=0; i < invitedUsers.Size(); i++)
		callResult->invitedUsers.Insert(* (invitedUsers[i]));
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SendInvite_Callback( SystemAddress senderAddress, SendInvite_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	RoomsPluginParticipant* inviteeId = GetParticipantByHandle( callResult->inviteeName, senderAddress );
	if (inviteeId==0)
	{
		callResult->resultCode=REC_SEND_INVITE_RECIPIENT_NOT_ONLINE;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	callResult->resultCode=roomsContainer.SendInvite( roomsPluginParticipant, inviteeId, callResult->inviteToSpectatorSlot, callResult->subject, callResult->body );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomInvitationSent_Notification notification;
		notification.invitorName=roomsPluginParticipant->GetName();
		notification.inviteeName=inviteeId->GetName();
		notification.inviteToSpectatorSlot=callResult->inviteToSpectatorSlot;
		notification.subject=callResult->subject;
		notification.body=callResult->body;
		ExecuteNotification(&notification, inviteeId);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::AcceptInvite_Callback( SystemAddress senderAddress, AcceptInvite_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	Room *room;
	callResult->resultCode=roomsContainer.AcceptInvite( callResult->roomId, &room, roomsPluginParticipant, callResult->inviteSender );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomMemberJoinedRoom_Notification notificationToRoom;
		notificationToRoom.joinedRoomResult=RakNet::OP_NEW<JoinedRoomResult>( __FILE__, __LINE__ );
		notificationToRoom.joinedRoomResult->acceptedInvitor=0;
		notificationToRoom.joinedRoomResult->acceptedInvitorName=callResult->inviteSender;
		notificationToRoom.joinedRoomResult->joiningMember=roomsPluginParticipant;
		notificationToRoom.joinedRoomResult->joiningMemberName=roomsPluginParticipant->GetName();
		notificationToRoom.joinedRoomResult->roomDescriptor.FromRoom(roomsPluginParticipant->GetRoom(), &roomsContainer);
		notificationToRoom.joinedRoomResult->roomOutput=roomsPluginParticipant->GetRoom();
		notificationToRoom.joinedRoomResult->agrc=&roomsContainer;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notificationToRoom);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::StartSpectating_Callback( SystemAddress senderAddress, StartSpectating_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.StartSpectating( roomsPluginParticipant );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomMemberStartedSpectating_Notification notification;	
		//notification.userName=roomsPluginParticipant->GetName();
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::StopSpectating_Callback( SystemAddress senderAddress, StopSpectating_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.StopSpectating( roomsPluginParticipant );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomMemberStoppedSpectating_Notification notification;
		//notification.userName=roomsPluginParticipant->GetName();
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GrantModerator_Callback( SystemAddress senderAddress, GrantModerator_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	RoomsPluginParticipant* newModerator = GetParticipantByHandle( callResult->newModerator, senderAddress );
	if (newModerator==0)
	{
		callResult->resultCode=REC_GRANT_MODERATOR_NEW_MODERATOR_NOT_ONLINE;
		ExecuteFunc(callResult, senderAddress);
		return;
	}

	DataStructures::List<InvitedUser> clearedInvites;
	callResult->resultCode=roomsContainer.GrantModerator( roomsPluginParticipant, newModerator, clearedInvites );

	if (callResult->resultCode==REC_SUCCESS)
	{
		ModeratorChanged_Notification notification;
		notification.oldModerator=roomsPluginParticipant->GetName();
		notification.newModerator=newModerator->GetName();
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);

		for (unsigned int i=0; i < clearedInvites.Size(); i++)
		{
//			Room *room = newModerator->GetRoom();
			RoomInvitationWithdrawn_Notification notification;
			notification.invitedUser=clearedInvites[i];
			ExecuteNotification(&notification,  GetParticipantByHandle(clearedInvites[i].target, UNASSIGNED_SYSTEM_ADDRESS));
		}
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::ChangeSlotCounts_Callback( SystemAddress senderAddress, ChangeSlotCounts_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.ChangeSlotCounts( roomsPluginParticipant, callResult->slots );

	if (callResult->resultCode==REC_SUCCESS)
	{
		SlotCountsSet_Notification notification;
		notification.slots=callResult->slots;
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}

	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SetCustomRoomProperties_Callback( SystemAddress senderAddress, SetCustomRoomProperties_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.SetCustomRoomProperties( roomsPluginParticipant, &callResult->table );
	if (callResult->resultCode==REC_SUCCESS)
	{
		CustomRoomPropertiesSet_Notification notification;
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.tablePtr=&callResult->table;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GetRoomProperties_Callback( SystemAddress senderAddress, GetRoomProperties_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	Room *room;
	if (callResult->roomName.IsEmpty())
	{
		callResult->resultCode=REC_GET_ROOM_PROPERTIES_EMPTY_ROOM_NAME;
	}
	else
	{
		room = roomsContainer.GetRoomByName(callResult->roomName);
		if (room==0)
		{
			callResult->resultCode=REC_GET_ROOM_PROPERTIES_UNKNOWN_ROOM_NAME;
		}
		else
		{
			callResult->roomDescriptor.FromRoom(room, &roomsContainer);
			callResult->resultCode=REC_SUCCESS;
		}
	}

	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::ChangeRoomName_Callback( SystemAddress senderAddress, ChangeRoomName_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	RoomNameSet_Notification notification;
	if (roomsPluginParticipant->GetRoom())
		notification.oldName=roomsPluginParticipant->GetRoom()->GetStringProperty(DefaultRoomColumns::TC_ROOM_NAME);
	callResult->resultCode=roomsContainer.ChangeRoomName( roomsPluginParticipant, callResult->newRoomName, profanityFilter );
	if (callResult->resultCode==REC_SUCCESS)
	{
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.newName=callResult->newRoomName;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SetHiddenFromSearches_Callback( SystemAddress senderAddress, SetHiddenFromSearches_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.SetHiddenFromSearches( roomsPluginParticipant, callResult->hiddenFromSearches );
	if (callResult->resultCode==REC_SUCCESS)
	{
		HiddenFromSearchesSet_Notification notification;
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.hiddenFromSearches=callResult->hiddenFromSearches;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SetDestroyOnModeratorLeave_Callback( SystemAddress senderAddress, SetDestroyOnModeratorLeave_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.SetDestroyOnModeratorLeave( roomsPluginParticipant, callResult->destroyOnModeratorLeave );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SetReadyStatus_Callback( SystemAddress senderAddress, SetReadyStatus_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.SetReadyStatus( roomsPluginParticipant, callResult->isReady );

	DataStructures::List<RoomsParticipant*> readyUsers;
	DataStructures::List<RoomsParticipant*> unreadyUsers;
	unsigned int i;
	roomsPluginParticipant->GetRoom()->GetReadyStatus(readyUsers, unreadyUsers);

	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomMemberReadyStatusSet_Notification notification;	
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.isReady=callResult->isReady;
		notification.roomMember=roomsPluginParticipant->GetName();

		for (i=0; i < readyUsers.Size(); i++)
			notification.readyUsers.Insert(readyUsers[i]->GetName());
		for (i=0; i < unreadyUsers.Size(); i++)
			notification.unreadyUsers.Insert(unreadyUsers[i]->GetName());

		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}

	for (i=0; i < readyUsers.Size(); i++)
		callResult->readyUsers.Insert(readyUsers[i]->GetName());
	for (i=0; i < unreadyUsers.Size(); i++)
		callResult->unreadyUsers.Insert(unreadyUsers[i]->GetName());

	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GetReadyStatus_Callback( SystemAddress senderAddress, GetReadyStatus_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	if (roomsPluginParticipant->GetRoom()==0)
	{
		callResult->resultCode=REC_GET_READY_STATUS_NOT_IN_ROOM;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	
	Room *room;
	DataStructures::List<RoomsParticipant*> readyUsers;
	DataStructures::List<RoomsParticipant*> unreadyUsers;
	callResult->resultCode=roomsContainer.GetReadyStatus( roomsPluginParticipant->GetRoom()->GetID(), &room, readyUsers, unreadyUsers );
	unsigned int i;
	for (i=0; i < readyUsers.Size(); i++)
		callResult->readyUsers.Insert(readyUsers[i]->GetName());
	for (i=0; i < unreadyUsers.Size(); i++)
		callResult->unreadyUsers.Insert(unreadyUsers[i]->GetName());
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SetRoomLockState_Callback( SystemAddress senderAddress, SetRoomLockState_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.SetRoomLockState( roomsPluginParticipant, callResult->roomLockState );
	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomLockStateSet_Notification notification;
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.roomLockState=callResult->roomLockState;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GetRoomLockState_Callback( SystemAddress senderAddress, GetRoomLockState_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	if (roomsPluginParticipant->GetRoom()==0)
	{
		callResult->resultCode=REC_GET_ROOM_LOCK_STATE_NOT_IN_ROOM;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	Room *room;
	callResult->resultCode=roomsContainer.GetRoomLockState( roomsPluginParticipant->GetRoom()->GetID(), &room, &callResult->roomLockState );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::AreAllMembersReady_Callback( SystemAddress senderAddress, AreAllMembersReady_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	if (roomsPluginParticipant->GetRoom()==0)
	{
		callResult->resultCode=REC_ARE_ALL_MEMBERS_READY_NOT_IN_ROOM;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	Room *room;
	callResult->resultCode=roomsContainer.AreAllMembersReady( roomsPluginParticipant->GetRoom()->GetID(), &room, &callResult->allReady );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::KickMember_Callback( SystemAddress senderAddress, KickMember_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	RoomsPluginParticipant* kickedMember = GetParticipantByHandle(callResult->kickedMember, UNASSIGNED_SYSTEM_ADDRESS);
	if (kickedMember==0)
	{
		callResult->resultCode=REC_KICK_MEMBER_TARGET_NOT_ONLINE;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	callResult->resultCode=roomsContainer.KickMember( roomsPluginParticipant, kickedMember, callResult->reason );

	if (callResult->resultCode==REC_SUCCESS)
	{
		RoomMemberKicked_Notification notification;
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.moderator=roomsPluginParticipant->GetName();
		notification.kickedMember=callResult->kickedMember;
		notification.reason=callResult->reason;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);

		// Also send notification to the guy kicked
		ExecuteNotification(&notification, kickedMember);
	}

	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::UnbanMember_Callback( SystemAddress senderAddress, UnbanMember_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=roomsContainer.UnbanMember( roomsPluginParticipant, callResult->bannedMemberName );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::GetBanReason_Callback( SystemAddress senderAddress, GetBanReason_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	Room *room;
	callResult->resultCode=roomsContainer.GetBanReason( callResult->roomId, &room, callResult->userName, &callResult->reason );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::AddUserToQuickJoin_Callback( SystemAddress senderAddress, AddUserToQuickJoin_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	QuickJoinUser *qju = RakNet::OP_NEW<QuickJoinUser>( __FILE__, __LINE__ );
	qju->networkedQuickJoinUser=callResult->networkedQuickJoinUser;
	qju->roomsParticipant=roomsPluginParticipant;
	callResult->resultCode=roomsContainer.AddUserToQuickJoin( callResult->gameIdentifier, qju );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::RemoveUserFromQuickJoin_Callback( SystemAddress senderAddress, RemoveUserFromQuickJoin_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	QuickJoinUser *qju;
	callResult->resultCode=roomsContainer.RemoveUserFromQuickJoin( roomsPluginParticipant, &qju );
	if (qju)
		RakNet::OP_DELETE(qju, __FILE__, __LINE__);
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::IsInQuickJoin_Callback( SystemAddress senderAddress, IsInQuickJoin_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	callResult->resultCode=REC_SUCCESS;
	callResult->isInQuickJoin=roomsContainer.IsInQuickJoin( roomsPluginParticipant );
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::SearchByFilter_Callback( SystemAddress senderAddress, SearchByFilter_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;

	DataStructures::OrderedList<Room*, Room*, AllGamesRoomsContainer::RoomsSortByName> roomsOutput;
	callResult->resultCode=roomsContainer.SearchByFilter( callResult->gameIdentifier, roomsPluginParticipant, &callResult->roomQuery, roomsOutput, callResult->onlyJoinable);
	unsigned i;
	RoomDescriptor *desc;
	for (i=0; i < roomsOutput.Size(); i++)
	{
		desc = RakNet::OP_NEW<RoomDescriptor>( __FILE__, __LINE__ );
		desc->FromRoom(roomsOutput[i], &roomsContainer);
		callResult->roomsOutput.Insert(desc);
	}
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::ChangeHandle_Callback( SystemAddress senderAddress, ChangeHandle_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	if (profanityFilter->HasProfanity(callResult->newHandle.C_String()))
	{
		callResult->resultCode=REC_CHANGE_HANDLE_CONTAINS_PROFANITY;
		ExecuteFunc(callResult, senderAddress);
		return;
	}

	RoomMemberHandleSet_Notification notification;
	if (roomsPluginParticipant)
		notification.oldName=roomsPluginParticipant->GetName();
	if (GetParticipantByHandle(callResult->newHandle, UNASSIGNED_SYSTEM_ADDRESS))
	{
		callResult->resultCode=REC_CHANGE_HANDLE_NEW_HANDLE_IN_USE;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	callResult->resultCode=REC_SUCCESS;
	roomsContainer.ChangeHandle( roomsPluginParticipant->GetName(), callResult->newHandle );
	if (roomsPluginParticipant->GetRoom())
	{
		notification.roomId=roomsPluginParticipant->GetRoom()->GetID();
		notification.newName=callResult->newHandle;
		ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	}
	ExecuteFunc(callResult, senderAddress);

}
void RoomsPlugin::Chat_Callback( SystemAddress senderAddress, Chat_Func *callResult)
{
	RoomsPluginParticipant* roomsPluginParticipant = ValidateUserHandle(callResult, senderAddress);
	if (roomsPluginParticipant==0)
		return;
	if (roomsPluginParticipant->GetRoom()==0 && callResult->chatDirectedToRoom)
	{
		callResult->resultCode=REC_CHAT_USER_NOT_IN_ROOM;
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	Chat_Notification notification;
	notification.sender=roomsPluginParticipant->GetName();
	notification.privateMessageRecipient=callResult->privateMessageRecipient;
	notification.chatMessage=callResult->chatMessage;
	notification.filteredChatMessage=callResult->chatMessage;
	profanityFilter->FilterProfanity(notification.filteredChatMessage.C_String(), notification.filteredChatMessage.C_StringUnsafe(), true);
	if (notification.filteredChatMessage==notification.chatMessage)
		notification.filteredChatMessage.Clear(); // Save bandwidth
	if (callResult->privateMessageRecipient.IsEmpty()==false)
	{
		RoomsPluginParticipant* recipient = GetParticipantByHandle(callResult->privateMessageRecipient, UNASSIGNED_SYSTEM_ADDRESS);
		if (recipient==0)
		{
			callResult->resultCode=REC_CHAT_RECIPIENT_NOT_ONLINE;
			ExecuteFunc(callResult, senderAddress);
			return;
		}

		if (callResult->chatDirectedToRoom)
		{
			if (recipient->GetRoom()==0)
			{
				callResult->resultCode=REC_CHAT_RECIPIENT_NOT_IN_ANY_ROOM;
				ExecuteFunc(callResult, senderAddress);
				return;
			}

			if (recipient->GetRoom()!=roomsPluginParticipant->GetRoom())
			{
				callResult->resultCode=REC_CHAT_RECIPIENT_NOT_IN_YOUR_ROOM;
				ExecuteFunc(callResult, senderAddress);
				return;
			}
		}

		callResult->resultCode=REC_SUCCESS;
		ExecuteNotification(&notification, recipient);
		ExecuteFunc(callResult, senderAddress);
		return;
	}
	else
	{
		if (callResult->chatDirectedToRoom==false)
		{
			// Chat not directed to room, and no recipients.
			callResult->resultCode=REC_CHAT_RECIPIENT_NOT_ONLINE;
			ExecuteFunc(callResult, senderAddress);
			return;
		}
	}

	callResult->resultCode=REC_SUCCESS;
	ExecuteNotificationToOtherRoomMembers(roomsPluginParticipant->GetRoom(), roomsPluginParticipant, &notification);
	ExecuteFunc(callResult, senderAddress);
}
void RoomsPlugin::ProcessRemoveUserResult(RemoveUserResult *removeUserResult)
{
	unsigned int j;
	for (j=0; j < removeUserResult->clearedInvitations.Size(); j++)
	{
		RoomsPluginParticipant* invitationRecipient = GetParticipantByHandle(removeUserResult->clearedInvitations[j].target, UNASSIGNED_SYSTEM_ADDRESS);
		if (invitationRecipient)
		{
			RoomInvitationWithdrawn_Notification notification;
			notification.invitedUser=removeUserResult->clearedInvitations[j];
			ExecuteNotification(&notification, invitationRecipient);
		}
	}
	if (removeUserResult->removedFromRoom)
	{
		if (removeUserResult->room)
		{
			if (removeUserResult->roomDestroyed==false)
			{
				if (removeUserResult->gotNewModerator)
				{
					ModeratorChanged_Notification notification;
					notification.oldModerator=removeUserResult->removedUserName;
					notification.newModerator=removeUserResult->room->GetModerator()->GetName();
					ExecuteNotificationToOtherRoomMembers(removeUserResult->room, 0, &notification);
				}

				RoomMemberLeftRoom_Notification notification;
				notification.roomId=removeUserResult->room->GetID();
				notification.roomMember=removeUserResult->removedUserName;
				ExecuteNotificationToOtherRoomMembers(removeUserResult->room, 0, &notification);
			}
			else
			{
				RoomDestroyedOnModeratorLeft_Notification notification;
				notification.oldModerator=removeUserResult->removedUserName;
				notification.roomId=removeUserResult->room->GetID();
				notification.roomDescriptor.FromRoom(removeUserResult->room, &roomsContainer);
				ExecuteNotificationToOtherRoomMembers(removeUserResult->room, 0, &notification);
			}
		}
	}
	if (removeUserResult->removedFromQuickJoin)
	{
		if (removeUserResult->qju)
			RakNet::OP_DELETE(removeUserResult->qju, __FILE__, __LINE__);
	}
	roomsContainer.DestroyRoomIfDead(removeUserResult->room);
}

void RoomsPlugin::ExecuteNotificationToOtherRoomMembers(Room *room, RoomsPluginParticipant* roomsPluginParticipant, RoomsPluginNotification *notification)
{
	unsigned roomMemberIndex;
	for (roomMemberIndex=0; roomMemberIndex < room->roomMemberList.Size(); roomMemberIndex++)
	{
		if (room->roomMemberList[roomMemberIndex]->roomsParticipant!=roomsPluginParticipant )
			ExecuteNotification(notification, ((RoomsPluginParticipant*)room->roomMemberList[roomMemberIndex]->roomsParticipant));
	}
}
