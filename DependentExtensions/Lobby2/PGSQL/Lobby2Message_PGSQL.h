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
int GetActiveClanCount(unsigned int userId, PostgreSQLInterface *pgsql);
bool CreateAccountParametersFailed( CreateAccountParameters &createAccountParameters, RakNet::Lobby2ResultCode &resultCode, Lobby2ServerCommand *command, PostgreSQLInterface *pgsql);
void UpdateAccountFromMissingCreationParameters(CreateAccountParameters &createAccountParameters, unsigned int userPrimaryKey, Lobby2ServerCommand *command, PostgreSQLInterface *pgsql);

__L2_MSG_DB_HEADER(Platform_Startup, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface ) { (void)command; (void)databaseInterface; return false; }};
__L2_MSG_DB_HEADER(Platform_Shutdown, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface ) { (void)command; (void)databaseInterface; return false; }};
__L2_MSG_DB_HEADER(System_CreateDatabase, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_DestroyDatabase, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_CreateTitle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_DestroyTitle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_GetTitleRequiredAge, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(System_GetTitleBinaryData, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
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
__L2_MSG_DB_HEADER(Clans_Create, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetProperties, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetProperties, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetMyMemberProperties, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GrantLeader, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetSubleaderStatus, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SetMemberRank, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetMemberProperties, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_ChangeHandle, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_Leave, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_Get, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SendJoinInvitation, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_WithdrawJoinInvitation, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_AcceptJoinInvitation, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_RejectJoinInvitation, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_DownloadInvitationList, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_SendJoinRequest, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_WithdrawJoinRequest, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_AcceptJoinRequest, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_RejectJoinRequest, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_DownloadRequestList, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_KickAndBlacklistUser, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_UnblacklistUser, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetBlacklist, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};
__L2_MSG_DB_HEADER(Clans_GetMembers, PGSQL){virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface );};

__L2_MSG_DB_HEADER(Clans_CreateBoard, PGSQL)
{
	virtual bool ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
	{
		(void)command;

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
		(void)command;

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
		(void)command;

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
		(void)command;

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
		(void)command;

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
		(void)command;

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
		(void)command;

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
		(void)command;

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

#define __L2_MSG_FACTORY_IMPL(__NAME__,__DB__) {case L2MID_##__NAME__ : return RakNet::OP_NEW< __NAME__##_##__DB__ >( __FILE__, __LINE__ ) ;}

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
