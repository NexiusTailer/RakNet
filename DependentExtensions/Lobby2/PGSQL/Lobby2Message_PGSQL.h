#include "Lobby2Message.h"
#include "Lobby2Server.h"
// libpq-fe.h is part of PostgreSQL which must be installed on this computer to use the PostgreRepository
#include "libpq-fe.h"
#include "PostgreSQLInterface.h"
#include "EpochTimeToString.h"

#ifndef __LOBBY_2_MESSAGE_PGSQL_H
#define __LOBBY_2_MESSAGE_PGSQL_H

namespace RakNet
{

// --------------------------------------------- Database specific message implementations for the server --------------------------------------------

#define __L2_MSG_DB_HEADER(__NAME__,__DB__) \
struct __NAME__##_##__DB__ : public __NAME__

struct ClanMemberDescriptor
{
	unsigned int userId;
	RakNet::RakString name;
	bool isSubleader;
	ClanMemberState memberState;
	RakNet::RakString banReason;
};


// Helper functions
unsigned int GetUserRowFromHandle(RakNet::RakString userName, PostgreSQLInterface *pgsql);
unsigned int GetClanIdFromHandle(RakNet::RakString clanName, PostgreSQLInterface *pgsql);
bool IsClanLeader(RakNet::RakString clanName, unsigned int userId, PostgreSQLInterface *pgsql);
unsigned int GetClanLeaderId(unsigned int clanId, PostgreSQLInterface *pgsql);
bool IsClanLeader(unsigned int clanId, unsigned int userId, PostgreSQLInterface *pgsql);
ClanMemberState GetClanMemberState(unsigned int clanId, unsigned int userId, bool *isSubleader, PostgreSQLInterface *pgsql);
void GetClanMembers(unsigned int clanId, DataStructures::List<ClanMemberDescriptor> &clanMembers, PostgreSQLInterface *pgsql);
bool IsTitleInUse(RakNet::RakString titleName, PostgreSQLInterface *pgsql);
bool StringContainsProfanity(RakNet::RakString string, PostgreSQLInterface *pgsql);
bool IsValidCountry(RakNet::RakString string, bool *countryHasStates, PostgreSQLInterface *pgsql);
bool IsValidState(RakNet::RakString string, PostgreSQLInterface *pgsql);
bool IsValidRace(RakNet::RakString string, PostgreSQLInterface *pgsql);
void GetFriendIDs(unsigned int callerUserId, bool excludeIfIgnored, PostgreSQLInterface *pgsql, DataStructures::List<unsigned int> &output);
void GetClanMateIDs(unsigned int callerUserId, bool excludeIfIgnored, PostgreSQLInterface *pgsql, DataStructures::List<unsigned int> &output);
bool IsIgnoredByTarget(unsigned int callerUserId, unsigned int targetUserId, PostgreSQLInterface *pgsql);
void OutputFriendsNotification(RakNet::Notification_Friends_StatusChange::Status notificationType, Lobby2ServerCommand *command, PostgreSQLInterface *pgsql);
void GetFriendHandlesByStatus(unsigned int callerUserId, RakNet::RakString status, PostgreSQLInterface *pgsql, DataStructures::List<RakNet::RakString> &output, bool callerIsUserOne);
void SendEmail(DataStructures::List<RakNet::RakString> &recipientNames, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql);
void SendEmail(DataStructures::List<unsigned int> &targetUserIds, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql);
void SendEmail(unsigned int targetUserId, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql);

__L2_MSG_DB_HEADER(Platform_Startup, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface ) {return false;}};
__L2_MSG_DB_HEADER(Platform_Shutdown, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface ) {return false;}};
__L2_MSG_DB_HEADER(System_CreateDatabase, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_DestroyDatabase, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_CreateTitle, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_DestroyTitle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_GetTitleRequiredAge, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_GetTitleBinaryData, PGSQL) {virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_RegisterProfanity, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_BanUser, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_UnbanUser, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(CDKey_Add, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(CDKey_GetStatus, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(CDKey_Use, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(CDKey_FlagStolen, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_Login, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_Logoff, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_RegisterAccount, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_SetEmailAddressValidated, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_ValidateHandle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_DeleteAccount, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_PruneAccounts, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_GetEmailAddress, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_GetPasswordRecoveryQuestionByHandle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_GetPasswordByPasswordRecoveryAnswer, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_ChangeHandle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_UpdateAccount, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_StartIgnore, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_StopIgnore, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Client_GetIgnoreList, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_SendInvite, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_AcceptInvite, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_RejectInvite, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_GetInvites, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_GetFriends, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Friends_Remove, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(BookmarkedUsers_Add, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(BookmarkedUsers_Remove, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(BookmarkedUsers_Get, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Emails_Send, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Emails_Get, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Emails_Delete, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Emails_SetStatus, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_SubmitMatch, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_GetMatches, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_GetMatchBinaryData, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_GetTotalScore, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_WipeScoresForPlayer, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_WipeMatches, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_PruneMatches, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_UpdateRating, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_WipeRatings, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Ranking_GetRating, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_Create, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetProperties, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetProperties, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetMyMemberProperties, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GrantLeader, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetSubleaderStatus, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetMemberRank, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetMemberProperties, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_ChangeHandle, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_Leave, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_Get, PGSQL){	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};

__L2_MSG_DB_HEADER(Clans_SendJoinInvitation, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;

		unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
		if (clanId==0)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_UNKNOWN_CLAN;
			return true;
		}

		bool isSubleader;
		RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
		if (clanMemberState!=CMD_ACTIVE)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_NOT_IN_CLAN;
			return true;
		}

		unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);

		if (isSubleader==false && clanLeaderId!=command->callerUserId)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_MUST_BE_LEADER_OR_SUBLEADER;
			return true;
		}

		// Does target already have an entry?
		unsigned int targetId = RakNet::GetUserRowFromHandle(targetHandle, pgsql);
		if (targetId==0)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_UNKNOWN_TARGET_HANDLE;
			return true;
		}

		if (targetId==command->callerUserId)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_CANNOT_PERFORM_ON_SELF;
			return true;
		}

		bool isTargetSubleader;
		RakNet::ClanMemberState targetClanMemberState = GetClanMemberState(clanId, targetId, &isTargetSubleader, pgsql);
		if (targetClanMemberState==CMD_ACTIVE)
		{
			// active member
			resultCode=L2RC_Clans_SendJoinInvitation_TARGET_ALREADY_IN_CLAN;
			return true;
		}

		if (targetClanMemberState==CMD_BANNED)
		{
			// banned
			resultCode=L2RC_Clans_SendJoinInvitation_TARGET_IS_BANNED;
			return true;
		}

		if (targetClanMemberState==CMD_JOIN_INVITED)
		{
			// already invited
			resultCode=L2RC_Clans_SendJoinInvitation_REQUEST_ALREADY_PENDING;
			return true;
		}

		if (targetClanMemberState==CMD_JOIN_REQUESTED)
		{
			resultCode=L2RC_Clans_SendJoinInvitation_TARGET_ALREADY_REQUESTED;
			// already requested
			return true;
		}

		// Add row to lobby2.clanMembers
		result = pgsql->QueryVaridic(
			"INSERT INTO lobby2.clanMembers (userId_fk, clanId_fk, isSubleader, memberState_fk) VALUES "
			"(%i, %i, false, (SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_JoinInvited') );"
			,targetId, clanId);
		PQclear(result);

		// Send email to target
		SendEmail(targetId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_SendJoinInvitation", pgsql);

		// Send notification to target, leader, subleaders about this invite
		Notification_Clans_PendingJoinStatus *notification;
		notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
		notification->clanHandle=clanHandle;
		notification->targetHandle=targetHandle;
		notification->sourceHandle=command->callingUserName;		
		notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_INVITATION;
		notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_SENT;
		command->server->AddOutputFromThread(notification, targetId, ""); // target

		DataStructures::List<ClanMemberDescriptor> clanMembers;
		GetClanMembers(clanId, clanMembers, pgsql);
		for (unsigned int i=0; i < clanMembers.Size(); i++)
		{
			if (clanMembers[i].memberState!=CMD_ACTIVE)
				continue;
			if (clanMembers[i].isSubleader==false && clanMembers[i].userId!=clanLeaderId)
				continue;

			notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
			notification->clanHandle=clanHandle;
			notification->targetHandle=targetHandle;
			notification->sourceHandle=command->callingUserName;		
			notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_INVITATION;
			notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_SENT;
			command->server->AddOutputFromThread(notification, clanMembers[i].userId, ""); // subleader

		}

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_WithdrawJoinInvitation, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;

		unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
		if (clanId==0)
		{
			resultCode=L2RC_Clans_WithdrawJoinInvitation_UNKNOWN_CLAN;
			return true;
		}

		// Does target already have an entry?
		unsigned int targetId = RakNet::GetUserRowFromHandle(targetHandle, pgsql);
		if (targetId==0)
		{
			resultCode=L2RC_Clans_WithdrawJoinInvitation_UNKNOWN_TARGET_HANDLE;
			return true;
		}

		if (targetId==command->callerUserId)
		{
			resultCode=L2RC_Clans_WithdrawJoinInvitation_CANNOT_PERFORM_ON_SELF;
			return true;
		}

		bool isSubleader;
		RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, targetId, &isSubleader, pgsql);
		if (clanMemberState!=CMD_JOIN_INVITED)
		{
			resultCode=L2RC_Clans_WithdrawJoinInvitation_NO_SUCH_INVITATION_EXISTS;
			return true;
		}

		unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);
		if (isSubleader==false && clanLeaderId!=command->callerUserId)
		{
			resultCode=L2RC_Clans_WithdrawJoinInvitation_MUST_BE_LEADER_OR_SUBLEADER;
			return true;
		}

		result = pgsql->QueryVaridic("DELETE FROM lobby2.clanMembers WHERE userId_fk=%i AND clanId_fk=%i", targetId, clanId);
		PQclear(result);

		// Send email to target
		SendEmail(targetId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_WithdrawJoinInvitation", pgsql);

		// Send notification to target, leader, subleaders
		Notification_Clans_PendingJoinStatus *notification;
		notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
		notification->clanHandle=clanHandle;
		notification->targetHandle=targetHandle;
		notification->sourceHandle=command->callingUserName;		
		notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_INVITATION;
		notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_WITHDRAWN;
		command->server->AddOutputFromThread(notification, targetId, ""); // target

		
		DataStructures::List<ClanMemberDescriptor> clanMembers;
		GetClanMembers(clanId, clanMembers, pgsql);
		for (unsigned int i=0; i < clanMembers.Size(); i++)
		{
			if (clanMembers[i].memberState!=CMD_ACTIVE)
				continue;
			if (clanMembers[i].isSubleader==false && clanMembers[i].userId!=clanLeaderId)
				continue;
			if (command->callerUserId==clanMembers[i].userId)
				continue;

			notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
			notification->clanHandle=clanHandle;
			notification->targetHandle=targetHandle;
			notification->sourceHandle=command->callingUserName;		
			notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_INVITATION;
			notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_WITHDRAWN;
			command->server->AddOutputFromThread(notification, clanMembers[i].userId, ""); // subleader

		}

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_AcceptJoinInvitation, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;

		if (failIfAlreadyInClan)
		{
			result = pgsql->QueryVaridic("SELECT clanMemberId_pk FROM lobby2.clanMembers WHERE userId_fk=%i AND memberState_fk=(SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_Active')", command->callerUserId);
			int numRowsReturned = PQntuples(result);
			PQclear(result);
			if (numRowsReturned>0)
			{
				resultCode=L2RC_Clans_AcceptJoinInvitation_ALREADY_IN_CLAN;
				return true;
			}
		}

		unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
		if (clanId==0)
		{
			resultCode=L2RC_Clans_AcceptJoinInvitation_UNKNOWN_CLAN;
			return true;
		}

		bool isSubleader;
		RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
		if (clanMemberState!=CMD_JOIN_INVITED)
		{
			resultCode=L2RC_Clans_AcceptJoinInvitation_NO_SUCH_INVITATION_EXISTS;
			return true;
		}

		// Change status from invited to clan member
		result = pgsql->QueryVaridic("UPDATE lobby2.clanMembers SET memberState_fk=(SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_Active') WHERE userId_fk=%i AND clanId_fk=%i", command->callerUserId, clanId);
		PQclear(result);

		// Notify all members about this new member
		DataStructures::List<ClanMemberDescriptor> clanMembers;
		GetClanMembers(clanId, clanMembers, pgsql);
		for (unsigned int i=0; i < clanMembers.Size(); i++)
		{
			if (clanMembers[i].memberState!=CMD_ACTIVE)
				continue;
			if (clanMembers[i].userId==command->callerUserId )
				continue;

			Notification_Clans_NewClanMember *notification = (Notification_Clans_NewClanMember *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_NewClanMember);
			notification->clanHandle=clanHandle;
			notification->targetHandle=command->callingUserName;	
			command->server->AddOutputFromThread(notification, clanMembers[i].userId, ""); // subleader

		}

		unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);

		// Send email to leader
		SendEmail(clanLeaderId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_AcceptJoinInvitation", pgsql);

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_RejectJoinInvitation, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;

		unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
		if (clanId==0)
		{
			resultCode=L2RC_Clans_RejectJoinInvitation_UNKNOWN_CLAN;
			return true;
		}

		bool isSubleader;
		RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
		if (clanMemberState!=CMD_JOIN_INVITED)
		{
			resultCode=L2RC_Clans_RejectJoinInvitation_NO_SUCH_INVITATION_EXISTS;
			return true;
		}

		result = pgsql->QueryVaridic("DELETE FROM lobby2.clanMembers WHERE userId_fk=%i AND clanId_fk=%i", command->callerUserId, clanId);
		PQclear(result);

		unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);

		// Send email to leader
		SendEmail(clanLeaderId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_RejectJoinInvitation", pgsql);

		// Subleader and leader notification
		DataStructures::List<ClanMemberDescriptor> clanMembers;
		GetClanMembers(clanId, clanMembers, pgsql);
		for (unsigned int i=0; i < clanMembers.Size(); i++)
		{
			if (clanMembers[i].memberState!=CMD_ACTIVE)
				continue;
			if (clanMembers[i].isSubleader==false && clanMembers[i].userId!=clanLeaderId)
				continue;

			Notification_Clans_PendingJoinStatus *notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
			notification->clanHandle=clanHandle;
			notification->sourceHandle=command->callingUserName;		
			notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_INVITATION;
			notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_WITHDRAWN;
			command->server->AddOutputFromThread(notification, clanMembers[i].userId, ""); // subleader

		}

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_DownloadInvitationList, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic(
			" SELECT clanHandle FROM lobby2.clans INNER JOIN "
			" (SELECT clanId_fk FROM lobby2.clanMembers WHERE userId_fk=%i AND memberState_fk = "
			" (SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_JoinInvited') ) as tbl1 "
			" ON tbl1.clanId_fk = lobby2.clans.clanId_pk;", command->callerUserId);

		if (result==0)
		{
			PQclear(result);
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
			return true;
		}
		int numRowsReturned = PQntuples(result);
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			OpenInvite oi;
			PostgreSQLInterface::PQGetValueFromBinary(&oi.clanHandle, result, i, "clanHandle");
			invitations.Insert(oi);
		}

		PQclear(result);
		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_SendJoinRequest, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;

		clanJoined=false;
		unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
		if (clanId==0)
		{
			resultCode=L2RC_Clans_SendJoinRequest_UNKNOWN_CLAN;
			return true;
		}

		bool isSubleader;
		RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
		if (clanMemberState==CMD_ACTIVE)
		{
			resultCode=L2RC_Clans_SendJoinRequest_ALREADY_IN_CLAN;
			return true;
		}
		if (clanMemberState==CMD_BANNED)
		{
			resultCode=L2RC_Clans_SendJoinRequest_BANNED;
			return true;
		}
		if (clanMemberState==CMD_JOIN_REQUESTED)
		{
			resultCode=L2RC_Clans_SendJoinRequest_REQUEST_ALREADY_PENDING;
			return true;
		}
		if (clanMemberState==CMD_JOIN_INVITED)
		{
			resultCode=L2RC_Clans_SendJoinRequest_ALREADY_INVITED;
			return true;
		}

		result = pgsql->QueryVaridic("SELECT requiresInvitationsToJoin FROM lobby2.clans WHERE clanId_pk=%i",clanId);
		bool requiresInvitationsToJoin;
		PostgreSQLInterface::PQGetValueFromBinary(&requiresInvitationsToJoin, result, 0, "requiresInvitationsToJoin");
		PQclear(result);

		if (requiresInvitationsToJoin==false)
		{
			result = pgsql->QueryVaridic(
				"INSERT INTO lobby2.clanMembers (userId_fk, clanId_fk, isSubleader, memberState_fk) VALUES "
				"(%i, %i, false, (SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_Active') );"
				,command->callerUserId, clanId);
			PQclear(result);

			// Send notification all members about the new member
			// Notify all members about this new member
			DataStructures::List<ClanMemberDescriptor> clanMembers;
			GetClanMembers(clanId, clanMembers, pgsql);
			for (unsigned int i=0; i < clanMembers.Size(); i++)
			{
				if (clanMembers[i].memberState!=CMD_ACTIVE)
					continue;
				if (clanMembers[i].userId==command->callerUserId)
					continue;

				Notification_Clans_NewClanMember *notification = (Notification_Clans_NewClanMember *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_NewClanMember);
				notification->clanHandle=clanHandle;
				notification->targetHandle=command->callingUserName;	
				command->server->AddOutputFromThread(notification, clanMembers[i].userId, "");

			}

			unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);

			// Send email to leader
			SendEmail(clanLeaderId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_SendJoinRequest", pgsql);

			clanJoined=true;
		}
		else
		{
			// Add row to lobby2.clanMembers
			result = pgsql->QueryVaridic(
				"INSERT INTO lobby2.clanMembers (userId_fk, clanId_fk, isSubleader, memberState_fk) VALUES "
				"(%i, %i, false, (SELECT stateId_pk FROM lobby2.clanMemberStates WHERE description='ClanMember_JoinRequested') );"
				,command->callerUserId, clanId);
			PQclear(result);

			// Send notification to leader, subleaders about this invite
			unsigned int clanLeaderId = GetClanLeaderId(clanId, pgsql);
			DataStructures::List<ClanMemberDescriptor> clanMembers;
			GetClanMembers(clanId, clanMembers, pgsql);
			for (unsigned int i=0; i < clanMembers.Size(); i++)
			{
				if (clanMembers[i].memberState!=CMD_ACTIVE)
					continue;
				if (clanMembers[i].isSubleader==false && clanMembers[i].userId!=clanLeaderId)
					continue;

				Notification_Clans_PendingJoinStatus *notification = (Notification_Clans_PendingJoinStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_PendingJoinStatus);
				notification->clanHandle=clanHandle;
				notification->targetHandle=clanMembers[i].name;
				notification->sourceHandle=command->callingUserName;		
				notification->majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_REQUEST;
				notification->minorOp=Notification_Clans_PendingJoinStatus::JOIN_SENT;
				command->server->AddOutputFromThread(notification, clanMembers[i].userId, ""); // subleader
			}
		}

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_WithdrawJoinRequest, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;
		Notification_Clans_PendingJoinStatus notification;
		//notification.userName="Leader and subleaders of the clan.";
		notification.clanHandle="The clan handle";
		notification.targetHandle="The guy that withdrew the join request";
		notification.majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_REQUEST;
		notification.minorOp=Notification_Clans_PendingJoinStatus::JOIN_WITHDRAWN;
		notification.resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(&notification, 0, "");

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_AcceptJoinRequest, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;
		Notification_Clans_NewClanMember notification;
		//notification.userName="All members of the clan.";
		notification.clanHandle="The clan handle";
		notification.targetHandle="The guy joining the clan";
		notification.resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(&notification, 0, "");

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_RejectJoinRequest, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;
		Notification_Clans_PendingJoinStatus notification;
		//notification.userName="Leader and subleaders of the clan, other than clanMemberHandle. Also the targetHandle.";
		notification.clanHandle="The clan handle";
		notification.targetHandle="The guy sent the original join request";
		notification.clanMemberHandle="The guy that rejected the join request";
		notification.majorOp=Notification_Clans_PendingJoinStatus::JOIN_CLAN_REQUEST;
		notification.minorOp=Notification_Clans_PendingJoinStatus::JOIN_REJECTED;
		notification.resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(&notification, 0, "");

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_DownloadRequestList, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_KickAndBlacklistUser, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;
		Notification_Clans_KickAndBlacklistUser notification;
		//notification.userName="Leader and subleaders of the clan, other than blacklistingUserHandle.";
		notification.clanHandle="The clan handle";
		notification.targetHandle="The guy kicked out";
		notification.blacklistingUserHandle="The leader or subleader that initiated this operation";
		notification.targetHandleWasKicked=true;
		notification.resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(&notification, 0, "");

		// If user was in the clan, send Notification_Clans_Leave to them.
		if (notification.targetHandleWasKicked)
		{
			Notification_Clans_Leave notification;
			//notification.userName="All clan members, other than leaders and subleaders";
			notification.clanHandle=notification.clanHandle;
			notification.targetHandle=notification.targetHandle;
			notification.resultCode=L2RC_SUCCESS;
			command->server->AddOutputFromThread(&notification, 0, "");
		}

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_UnblacklistUser, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result;
		Notification_Clans_UnblacklistUser notification;
		//notification.userName="Leader and subleaders of the clan, other than unblacklistingUserHandle.";
		notification.clanHandle="The clan handle";
		notification.targetHandle="The guy no longer blacklisted";
		notification.unblacklistingUserHandle="For now, always the clan leader";
		notification.resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(&notification, 0, "");

		resultCode=L2RC_SUCCESS;
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_GetBlacklist, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_GetMembers, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_CreateBoard, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_DestroyBoard, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_CreateNewTopic, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_ReplyToTopic, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_RemovePost, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_GetBoards, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_GetTopics, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Clans_GetPosts, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		PGresult *result = pgsql->QueryVaridic("");
		if (result!=0)
		{
			PQclear(result);
			resultCode=L2RC_SUCCESS;
		}
		else
		{
			resultCode=L2RC_SUCCESS;
		}
		return true;
	}
};

__L2_MSG_DB_HEADER(Notification_Friends_StatusChange, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
		if (command->callerSystemAddress==UNASSIGNED_SYSTEM_ADDRESS)
		{
			OutputFriendsNotification(Notification_Friends_StatusChange::FRIEND_LOGGED_OFF, command, pgsql);
		}

		// Don't let the thread return this notification with UNASSIGNED_SYSTEM_ADDRESS to the user. It's just a message to the thread.
		return false;
	}
};



// --------------------------------------------- Database specific factory class for all messages --------------------------------------------

#define __L2_MSG_FACTORY_IMPL(__NAME__,__DB__) {case L2MID_##__NAME__ : return RakNet::OP_NEW< __NAME__##_##__DB__ >() ;}

struct Lobby2MessageFactory_PGSQL : public Lobby2MessageFactory
{
	virtual Lobby2Message *Alloc(Lobby2MessageID id)
	{
		switch (id)
		{
			__L2_MSG_FACTORY_IMPL(Platform_Startup, PGSQL);
			__L2_MSG_FACTORY_IMPL(Platform_Shutdown, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_CreateDatabase, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_DestroyDatabase, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_CreateTitle, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_DestroyTitle, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_GetTitleRequiredAge, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_GetTitleBinaryData, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_RegisterProfanity, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_BanUser, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_UnbanUser, PGSQL);
			__L2_MSG_FACTORY_IMPL(CDKey_Add, PGSQL);
			__L2_MSG_FACTORY_IMPL(CDKey_GetStatus, PGSQL);
			__L2_MSG_FACTORY_IMPL(CDKey_Use, PGSQL);
			__L2_MSG_FACTORY_IMPL(CDKey_FlagStolen, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_Login, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_Logoff, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_RegisterAccount, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_SetEmailAddressValidated, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_ValidateHandle, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_DeleteAccount, PGSQL);
			__L2_MSG_FACTORY_IMPL(System_PruneAccounts, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_GetEmailAddress, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_GetPasswordRecoveryQuestionByHandle, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_GetPasswordByPasswordRecoveryAnswer, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_ChangeHandle, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_UpdateAccount, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_StartIgnore, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_StopIgnore, PGSQL);
			__L2_MSG_FACTORY_IMPL(Client_GetIgnoreList, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_SendInvite, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_AcceptInvite, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_RejectInvite, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_GetInvites, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_GetFriends, PGSQL);
			__L2_MSG_FACTORY_IMPL(Friends_Remove, PGSQL);
			__L2_MSG_FACTORY_IMPL(BookmarkedUsers_Add, PGSQL);
			__L2_MSG_FACTORY_IMPL(BookmarkedUsers_Remove, PGSQL);
			__L2_MSG_FACTORY_IMPL(BookmarkedUsers_Get, PGSQL);
			__L2_MSG_FACTORY_IMPL(Emails_Send, PGSQL);
			__L2_MSG_FACTORY_IMPL(Emails_Get, PGSQL);
			__L2_MSG_FACTORY_IMPL(Emails_Delete, PGSQL);
			__L2_MSG_FACTORY_IMPL(Emails_SetStatus, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_SubmitMatch, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_GetMatches, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_GetMatchBinaryData, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_GetTotalScore, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_WipeScoresForPlayer, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_WipeMatches, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_PruneMatches, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_UpdateRating, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_WipeRatings, PGSQL);
			__L2_MSG_FACTORY_IMPL(Ranking_GetRating, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_Create, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SetProperties, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetProperties, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SetMyMemberProperties, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GrantLeader, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SetSubleaderStatus, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SetMemberRank, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetMemberProperties, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_ChangeHandle, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_Leave, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_Get, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SendJoinInvitation, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_WithdrawJoinInvitation, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_AcceptJoinInvitation, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_RejectJoinInvitation, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_DownloadInvitationList, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_SendJoinRequest, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_WithdrawJoinRequest, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_AcceptJoinRequest, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_RejectJoinRequest, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_DownloadRequestList, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_KickAndBlacklistUser, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_UnblacklistUser, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetBlacklist, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetMembers, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_CreateBoard, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_DestroyBoard, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_CreateNewTopic, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_ReplyToTopic, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_RemovePost, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetBoards, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetTopics, PGSQL);
			__L2_MSG_FACTORY_IMPL(Clans_GetPosts, PGSQL);
			__L2_MSG_FACTORY_BASE(Notification_Client_IgnoreStatus);
			__L2_MSG_FACTORY_IMPL(Notification_Friends_StatusChange, PGSQL);
			__L2_MSG_FACTORY_BASE(Notification_User_ChangedHandle);
			__L2_MSG_FACTORY_BASE(Notification_Friends_CreatedClan);
			__L2_MSG_FACTORY_BASE(Notification_Emails_Received);
			__L2_MSG_FACTORY_BASE(Notification_Clans_GrantLeader);
			__L2_MSG_FACTORY_BASE(Notification_Clans_SetSubleaderStatus);
			__L2_MSG_FACTORY_BASE(Notification_Clans_SetMemberRank);
			__L2_MSG_FACTORY_BASE(Notification_Clans_ChangeHandle);
			__L2_MSG_FACTORY_BASE(Notification_Clans_Leave);
			__L2_MSG_FACTORY_BASE(Notification_Clans_PendingJoinStatus);
			__L2_MSG_FACTORY_BASE(Notification_Clans_NewClanMember);
			__L2_MSG_FACTORY_BASE(Notification_Clans_KickAndBlacklistUser);
			__L2_MSG_FACTORY_BASE(Notification_Clans_UnblacklistUser);
			__L2_MSG_FACTORY_BASE(Notification_Clans_Destroyed);


		default:
			return 0;
		};
	};
};
}; // namespace RakNet

#endif
