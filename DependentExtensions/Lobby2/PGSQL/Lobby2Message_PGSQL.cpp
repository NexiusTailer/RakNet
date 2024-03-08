#include "Lobby2Message_PGSQL.h"

using namespace RakNet;

unsigned int RakNet::GetUserRowFromHandle(RakNet::RakString userName, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT (userId_pk) from lobby2.users WHERE handleLower=lower(%s)", userName.C_String());
	if (result)
	{
		unsigned int primaryKey;
		int numRowsReturned = PQntuples(result);
		if (numRowsReturned>0)
		{
			PostgreSQLInterface::PQGetValueFromBinary(&primaryKey, result, 0, "userId_pk");
			PQclear(result);
			return primaryKey;
		}
		PQclear(result);
		return 0;
	}
	return 0;
}
unsigned int RakNet::GetClanIdFromHandle(RakNet::RakString clanName, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT (clanId_pk) from lobby2.clans WHERE clanHandleLower=lower(%s)", clanName.C_String());
	if (result)
	{
		unsigned int primaryKey;
		int numRowsReturned = PQntuples(result);
		if (numRowsReturned>0)
		{
			PostgreSQLInterface::PQGetValueFromBinary(&primaryKey, result, 0, "clanId_pk");
			PQclear(result);
			return primaryKey;
		}
		PQclear(result);
		return 0;
	}
	return 0;
}
bool RakNet::IsClanLeader(RakNet::RakString clanName, unsigned int userId, PostgreSQLInterface *pgsql)
{
	unsigned int clanId = GetClanIdFromHandle(clanName, pgsql);
	if (clanId==0)
		return false;
	return IsClanLeader(clanId, userId, pgsql);
}
unsigned int RakNet::GetClanLeaderId(unsigned int clanId, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT leaderUserId_fk FROM lobby2.clans WHERE clanId_pk=%i", clanId);
	if (result==0)
		return 0;
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		return 0;
	}
	int leaderId;
	PostgreSQLInterface::PQGetValueFromBinary(&leaderId, result, 0, "leaderUserId_fk");
	PQclear(result);
	return leaderId;
}
bool RakNet::IsClanLeader(unsigned int clanId, unsigned int userId, PostgreSQLInterface *pgsql)
{
	return userId!=0 && GetClanLeaderId(clanId, pgsql)==userId;
}
RakNet::ClanMemberState RakNet::GetClanMemberState(unsigned int clanId, unsigned int userId, bool *isSubleader, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT memberState_fk FROM lobby2.clanMembers WHERE userId_fk=%i AND clanId_fk=%i", userId, clanId);
	if (result==0)
		return CMD_UNDEFINED;
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		return CMD_UNDEFINED;
	}
	int memberState;
	PostgreSQLInterface::PQGetValueFromBinary(&memberState, result, 0, "memberState_fk");
	PostgreSQLInterface::PQGetValueFromBinary(isSubleader, result, 0, "isSubleader");
	PQclear(result);
	return (ClanMemberState) memberState;
}
void RakNet::GetClanMembers(unsigned int clanId, DataStructures::List<ClanMemberDescriptor> &clanMembers, PostgreSQLInterface *pgsql)
{
	ClanMemberDescriptor cmd;

	PGresult *result = pgsql->QueryVaridic(
		"SELECT lobby2.clanMembers.userId_fk, lobby2.clanMembers.isSubleader, lobby2.clanMembers.memberState_fk, lobby2.clanMembers.banReason, lobby2.users.handle "
		"FROM lobby2.clanMembers, lobby2.users WHERE clanId_fk=%i AND lobby2.users.userId_pk=lobby2.clanMembers.userId_fk", clanId);
	if (result==0)
		return;
	int numRowsReturned = PQntuples(result);
	int idx;
	for (idx=0; idx < numRowsReturned; idx++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&cmd.userId, result, idx, "userId_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&cmd.isSubleader, result, idx, "isSubleader");
		int cms;
		PostgreSQLInterface::PQGetValueFromBinary(&cms, result, idx, "memberState_fk");
		cmd.memberState=(ClanMemberState)cms;
		PostgreSQLInterface::PQGetValueFromBinary(&cmd.banReason, result, idx, "banReason");
		PostgreSQLInterface::PQGetValueFromBinary(&cmd.name, result, idx, "handle");
		clanMembers.Insert(cmd);
	}
	PQclear(result);
}
bool RakNet::IsTitleInUse(RakNet::RakString titleName, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT titleName_pk FROM lobby2.titles where titleName_pk=%s", titleName.C_String());
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned==0)
		return false;
	return true;
}
bool RakNet::StringContainsProfanity(RakNet::RakString string, PostgreSQLInterface *pgsql)
{
	RakNet::RakString strLower1 = " " + string;
	RakNet::RakString strLower2 = string + " ";
	RakNet::RakString strLower3 = " " + string + " ";
	RakNet::RakString strLower4 = string;
	strLower1.ToLower();
	strLower2.ToLower();
	strLower3.ToLower();
	strLower4.ToLower();
	PGresult *result = pgsql->QueryVaridic("SELECT wordLower FROM lobby2.profanity WHERE "
		"wordLower LIKE %s OR wordLower LIKE %s OR wordLower LIKE %s OR wordLower LIKE %s"
		, strLower1.C_String(), strLower2.C_String(), strLower3.C_String(), strLower4.C_String());
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned==0)
		return false;
	return true;
}
bool RakNet::IsValidCountry(RakNet::RakString string, bool *countryHasStates, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT country_name,country_has_states FROM lobby2.country where lower(country_name)=lower(%s)", string.C_String());
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	if (countryHasStates && numRowsReturned>0)
		PostgreSQLInterface::PQGetValueFromBinary(countryHasStates, result, 0, "country_has_states");
	PQclear(result);
	if (numRowsReturned==0)
		return false;
	return true;
}
bool RakNet::IsValidState(RakNet::RakString string, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT state_name FROM lobby2.state WHERE lower(state_name)=lower(%s)", string.C_String());
		if (result==0)
			return false;
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned==0)
		return false;
	return true;
}
bool RakNet::IsValidRace(RakNet::RakString string, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic("SELECT race_text FROM lobby2.race WHERE lower(race_text)=lower(%s)", string.C_String());
		if (result==0)
			return false;
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned==0)
		return false;
	return true;
}
void RakNet::GetFriendIDs(unsigned int callerUserId, bool excludeIfIgnored, PostgreSQLInterface *pgsql, DataStructures::List<unsigned int> &output)
{
	PGresult *result = pgsql->QueryVaridic("SELECT userTwo_fk from lobby2.friends WHERE userOne_fk=%i AND "
		"actionId_fk=(SELECT actionId_pk from lobby2.friendActions WHERE description='isFriends');", callerUserId);
	if (result==0)
		return;
	int numRowsReturned = PQntuples(result);
	int idx;
	unsigned int id;
	for (idx=0; idx < numRowsReturned; idx++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&id, result, idx, "userTwo_fk");
		if (excludeIfIgnored==false || IsIgnoredByTarget(callerUserId, id, pgsql)==false)
			output.Insert(id);
	}
	PQclear(result);
}
void RakNet::GetClanMateIDs(unsigned int callerUserId, bool excludeIfIgnored, PostgreSQLInterface *pgsql, DataStructures::List<unsigned int> &output)
{
	PGresult *result = pgsql->QueryVaridic(
		"select userId_fk from lobby2.clanMembers where clanId_fk="
		"(select clanId_fk from lobby2.clanMembers where userId_fk=%i)"
		"AND userId_fk !=%i;"
		, callerUserId, callerUserId);
	if (result==0)
		return;
	int numRowsReturned = PQntuples(result);
	int idx;
	unsigned int id;
	for (idx=0; idx < numRowsReturned; idx++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&id, result, idx, "userId_fk");
		if (excludeIfIgnored==false || IsIgnoredByTarget(callerUserId, id, pgsql)==false)
			output.Insert(id);
	}
	PQclear(result);
}

bool RakNet::IsIgnoredByTarget(unsigned int callerUserId, unsigned int targetUserId, PostgreSQLInterface *pgsql)
{
	PGresult *result = pgsql->QueryVaridic(
		"select userMe_fk from lobby2.ignore where userMe_fk=%i AND userOther_fk=%i"
		, callerUserId, targetUserId);
	if (result==0)
		return false;
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	return numRowsReturned>0;
}

void RakNet::OutputFriendsNotification(RakNet::Notification_Friends_StatusChange::Status notificationType, Lobby2ServerCommand *command, PostgreSQLInterface *pgsql)
{
	// Tell all friends about this new login
	DataStructures::List<unsigned int> output;
	GetFriendIDs(command->callerUserId, true, pgsql, output);
	
	unsigned int idx;
	for (idx=0; idx < output.Size(); idx++)
	{
		Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
		RakAssert(command->callingUserName.IsEmpty()==false);
		notification->otherHandle=command->callingUserName;
		notification->op=notificationType;
		notification->resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(notification, output[idx], "");
	}
}


void RakNet::GetFriendHandlesByStatus(unsigned int callerUserId, RakNet::RakString status, PostgreSQLInterface *pgsql, DataStructures::List<RakNet::RakString> &output, bool callerIsUserOne)
{
	RakNet::RakString query;
	if (callerIsUserOne)
	{
		query = "SELECT handle from lobby2.users WHERE userId_pk ="
			"(SELECT userTwo_fk from lobby2.friends WHERE userOne_fk=%i AND actionId_fk ="
			"(SELECT actionId_pk FROM lobby2.friendActions where description='";
	}
	else
	{
		query = "SELECT handle from lobby2.users WHERE userId_pk ="
			"(SELECT userOne_fk from lobby2.friends WHERE userTwo_fk=%i AND actionId_fk ="
			"(SELECT actionId_pk FROM lobby2.friendActions where description='";
	}
	
	query+=status;
	query+="'));";

	PGresult *result = pgsql->QueryVaridic( query.C_String(), callerUserId );
	RakAssert(result);

	int numRowsReturned = PQntuples(result);
	int i;
	for (i=0; i < numRowsReturned; i++)
	{
		RakNet::RakString handle;
		PostgreSQLInterface::PQGetValueFromBinary(&handle, result, i, "handle");
		output.Insert(handle);
	}

	PQclear(result);
}

void RakNet::SendEmail(DataStructures::List<RakNet::RakString> &recipientNames, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql)
{
	DataStructures::List<unsigned int> targetUserIds;
	unsigned int targetUserId;
	for (unsigned int i=0; i < recipientNames.Size(); i++)
	{
		targetUserId = GetUserRowFromHandle(recipientNames[i], pgsql);
		if (targetUserId!=0)
			targetUserIds.Insert(targetUserId);
	}
	SendEmail(targetUserIds, senderUserId, senderUserName, server, subject, body, binaryData, status, triggerString, pgsql);
}
void RakNet::SendEmail(unsigned int targetUserId, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql)
{
	DataStructures::List<unsigned int> targetUserIds;
	targetUserIds.Insert(targetUserId);
	SendEmail(targetUserIds, senderUserId, senderUserName, server, subject, body, binaryData, status, triggerString, pgsql);
}
void RakNet::SendEmail(DataStructures::List<unsigned int> &targetUserIds, unsigned int senderUserId, RakNet::RakString senderUserName, Lobby2Server *server, RakNet::RakString subject, RakNet::RakString body, BinaryDataBlock *binaryData, int status, RakNet::RakString triggerString, PostgreSQLInterface *pgsql)
{
	if (targetUserIds.Size()==0)
		return;

	PGresult *result;
	if (binaryData)
	{
		result = pgsql->QueryVaridic(
			"INSERT INTO lobby2.emails (subject, body, binaryData, triggerId_fk) VALUES "
			"(%s, %s, %a, (SELECT triggerId_pk FROM lobby2.emailTriggers WHERE description=%s) ) RETURNING emailId_pk;"
			,subject.C_String(), body.C_String(), binaryData->binaryData, binaryData->binaryDataLength, triggerString.C_String()
			);
	}
	else
	{
		result = pgsql->QueryVaridic(
			"INSERT INTO lobby2.emails (subject, body, triggerId_fk) VALUES "
			"(%s, %s, %a, (SELECT triggerId_pk FROM lobby2.emailTriggers WHERE description=%s) ) RETURNING emailId_pk;"
			,subject.C_String(), body.C_String(), triggerString.C_String()
			);
	}
	
	RakAssert(result);
	unsigned int emailId_pk;
	PostgreSQLInterface::PQGetValueFromBinary(&emailId_pk, result, 0, "emailId_pk");
	PQclear(result);

	unsigned int i;
	for (i=0; i < targetUserIds.Size(); i++)
	{
		// Once in my inbox
		result = pgsql->QueryVaridic(
			"INSERT INTO lobby2.emailTargets (emailId_fk, userMe_fk, userOther_fk, status, wasRead, ISentThisEmail) VALUES "
			"(%i, %i, %i, %i, %b, %b);", emailId_pk, senderUserId, targetUserIds[i], status, true, true);
		RakAssert(result);
		PQclear(result);

		// Once in the destination inbox
		result = pgsql->QueryVaridic(
			"INSERT INTO lobby2.emailTargets (emailId_fk, userMe_fk, userOther_fk, status, wasRead, ISentThisEmail) VALUES "
			"(%i, %i, %i, %i, %b, %b);", emailId_pk, targetUserIds[i], senderUserId, status, false, false);
		RakAssert(result);
		PQclear(result);

		// Notify recipient that they got an email
		Notification_Emails_Received *notification = (Notification_Emails_Received *) server->GetMessageFactory()->Alloc(L2MID_Notification_Emails_Received);
		notification->sender=senderUserName;
		notification->subject=subject;
		notification->emailId=emailId_pk;
		notification->resultCode=L2RC_SUCCESS;
		server->AddOutputFromThread(notification, targetUserIds[i], "");
	}
}
bool RakNet::System_CreateDatabase_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	bool success;
	PGresult *result;
	pgsql->ExecuteBlockingCommand("DROP SCHEMA lobby2 CASCADE;", &result, false);
	PQclear(result);
	pgsql->ExecuteBlockingCommand("DROP LANGUAGE plpgsql;", &result, false);
	PQclear(result);
	FILE *fp = fopen("Lobby2Schema.txt", "rb");
	fseek( fp, 0, SEEK_END );
	unsigned int fileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	char *cmd = (char*) rakMalloc(fileSize+1);
	fread(cmd, 1, fileSize, fp);
	fclose(fp);
	cmd[fileSize]=0;
	success = pgsql->ExecuteBlockingCommand(cmd, &result, false);
	PQclear(result);
	if (success)
	{
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		printf(cmd);
		printf(pgsql->GetLastError());
	}
	rakFree(cmd);
	return true;
}

bool RakNet::System_DestroyDatabase_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	bool success;
	PGresult *result;
	success=pgsql->ExecuteBlockingCommand("DROP SCHEMA lobby2 CASCADE;", &result, false);
	PQclear(result);
	if (success)
		resultCode=L2RC_SUCCESS;
	else
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
	return true;
}

bool RakNet::System_CreateTitle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("INSERT INTO lobby2.titles (titleName_pk, titleSecretKey, requiredAge, binaryData) VALUES (%s,%s,%i,%a)",
		titleName.C_String(),
		titleSecretKey.C_String(),
		requiredAge,
		binaryData.binaryData,
		binaryData.binaryDataLength);
	if (result!=0)
	{
		PQclear(result);		
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_CreateTitle_TITLE_ALREADY_IN_USE;
	}
	return true;
}

bool RakNet::System_DestroyTitle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	result = pgsql->QueryVaridic("DELETE FROM lobby2.titles WHERE titlename_pk=%s", titleName.C_String());
	if (result!=0)
	{
		PQclear(result);		
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_DestroyTitle_TITLE_NOT_IN_USE;
	}
	return true;
}

bool RakNet::System_GetTitleRequiredAge_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("SELECT requiredAge FROM lobby2.titles where titlename_pk=%s", titleName.C_String());
	if (result!=0)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&requiredAge, result, 0, "requiredAge");
		PQclear(result);
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_GetTitleRequiredAge_TITLE_NOT_IN_USE;
	}
	return true;
}

bool RakNet::System_GetTitleBinaryData_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("SELECT binaryData FROM lobby2.titles where titlename_pk=%s", titleName.C_String());
	if (result!=0)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&binaryData.binaryData, &binaryData.binaryDataLength, result, 0, "binaryData");
		PQclear(result);
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_GetTitleBinaryData_TITLE_NOT_IN_USE;
	}
	return true;
}

bool RakNet::System_RegisterProfanity_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	//		pgsql->PrepareVaridic("INSERT INTO lobby2.profanity (word) VALUES (%s)");
	//		for (unsigned int i=0; i < profanityWords.Size(); i++)
	//			pgsql->PrepareVaridicArgs(0, profanityWords[i].C_String());
	//		PGresult *result = pgsql->ExecutePreparedStatement();
	PGresult *result;
	resultCode=L2RC_SUCCESS;
	for (unsigned int i=0; i < profanityWords.Size(); i++)
	{
		result = pgsql->QueryVaridic("INSERT INTO lobby2.profanity (word) VALUES (%s)", profanityWords[i].C_String());
		if (result==0)
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		PQclear(result);
		if (resultCode==L2RC_DATABASE_CONSTRAINT_FAILURE)
			return true;
	}
	return true;
}

bool RakNet::System_BanUser_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}
	PGresult *result = pgsql->QueryVaridic("INSERT INTO lobby2.bannedUsers (userId_fk, description, timeout) VALUES (%i, %s, now () + %i * interval '1 hours')", userRow, banReason.C_String(), durationHours);
	if (result!=0)
	{
		PQclear(result);
		result = pgsql->QueryVaridic("INSERT INTO lobby2.userHistory (userId_fk, description, triggerId_fk) "
			"VALUES (%i, %s, (SELECT triggerId_pk FROM lobby2.userHistoryTriggers WHERE description='Banned'))", userRow, banReason.C_String());
		RakAssert(result);
		PQclear(result);

		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_BanUser_ALREADY_BANNED;
	}
	
	return true;
}

bool RakNet::System_UnbanUser_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}
	PGresult *result = pgsql->QueryVaridic("DELETE FROM lobby2.bannedUsers WHERE userId_fk=%i", userRow);
	if (result!=0)
	{
		PQclear(result);
		result = pgsql->QueryVaridic("INSERT INTO lobby2.userHistory (userId_fk, description, triggerId_fk) "
			"VALUES (%i, %s, (SELECT triggerId_pk FROM lobby2.userHistoryTriggers WHERE description='Unbanned'))", userRow, reason.C_String());
		RakAssert(result);
		PQclear(result);
		resultCode=L2RC_SUCCESS;
	}
	else
	{
		resultCode=L2RC_System_BanUser_ALREADY_BANNED;
	}
	return true;
}

bool RakNet::CDKey_Add_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_CDKey_Add_TITLE_NOT_IN_USE;
		return true;
	}
	unsigned int i;
	for (i=0; i < cdKeys.Size(); i++)
	{
		result = pgsql->QueryVaridic("INSERT INTO lobby2.cdkeys (cdKey, usable, stolen, titleName_fk) VALUES (%s, true, false, %s);", cdKeys[i].C_String(), titleName.C_String());
		RakAssert(result);
		PQclear(result);
	}
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::CDKey_GetStatus_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_CDKey_GetStatus_TITLE_NOT_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT (lobby2.cdkeys.usable, lobby2.cdkeys.stolen, lobby2.cdkeys.activationDate, lobby2.users.handle) "
		"FROM lobby2.cdkeys, lobby2.users WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s AND lobby2.users.userId_pk=lobby2.cdkeys.userId_fk",
		cdKey.C_String(), titleName.C_String());
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_CDKey_GetStatus_UNKNOWN_CD_KEY;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&usable, result, 0, "lobby2.cdkeys.usable");
	PostgreSQLInterface::PQGetValueFromBinary(&wasStolen, result, 0, "lobby2.cdkeys.stolen");
	PostgreSQLInterface::PQGetValueFromBinary(&usedBy, result, 0, "lobby2.users.handle");
	PostgreSQLInterface::PQGetValueFromBinary(&activationDate, result, 0, "lobby2.cdkeys.activationDate");

	PQclear(result);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::CDKey_Use_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_CDKey_Use_TITLE_NOT_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT (lobby2.cdkeys.usable, lobby2.cdkeys.stolen, lobby2.cdkeys.userId_fk) "
		"FROM lobby2.cdkeys, lobby2.users WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s",
		cdKey.C_String(), titleName.C_String());
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_CDKey_Use_UNKNOWN_CD_KEY;
		return true;
	}
	bool usable, wasStolen, alreadyUsed;
	unsigned int userId;
	PostgreSQLInterface::PQGetValueFromBinary(&usable, result, 0, "lobby2.cdkeys.usable");
	PostgreSQLInterface::PQGetValueFromBinary(&wasStolen, result, 0, "lobby2.cdkeys.stolen");
	alreadyUsed=PostgreSQLInterface::PQGetValueFromBinary(&userId, result, 0, "lobby2.cdkeys.userId_fk");
	PQclear(result);

	if (wasStolen)
		resultCode=L2RC_CDKey_Use_CD_KEY_STOLEN;
	else if (alreadyUsed)
		resultCode=L2RC_CDKey_Use_CD_KEY_ALREADY_USED;
	else if (usable==false)
		resultCode=L2RC_CDKey_Use_NOT_USABLE;
	else
	{
		unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
		if (userRow==0)
		{
			resultCode=L2RC_UNKNOWN_USER;
			return true;
		}
		result = pgsql->QueryVaridic("UPDATE lobby2.cdKeys SET activationDate=now(),"
			"userId_fk=%i WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s", userRow, cdKey.C_String(), titleName.C_String());
		if (result==0)
		{
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
			return true;
		}
		PQclear(result);
		resultCode=L2RC_SUCCESS;
	}
	return true;
}

bool RakNet::CDKey_FlagStolen_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_CDKey_FlagStolen_TITLE_NOT_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT lobby2.cdkeys.cdKey, lobby2.cdkeys.userId_fk FROM lobby2.cdkeys WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s",
		cdKey.C_String(), titleName.C_String());
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_CDKey_Use_UNKNOWN_CD_KEY;
		return true;
	}
	unsigned int userId_fk;
	if (PostgreSQLInterface::PQGetValueFromBinary(&userId_fk, result, 0, "userId_fk"))
	{
		PQclear(result);
		result = pgsql->QueryVaridic("SELECT handle from lobby2.users WHERE userId_pk=%i", userId_fk);
		PostgreSQLInterface::PQGetValueFromBinary(&userUsingThisKey, result, 0, "handle");
		PQclear(result);

	}
	else
		PQclear(result);

	result = pgsql->QueryVaridic("UPDATE lobby2.cdKeys SET stolen=%b WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s", wasStolen, cdKey.C_String(), titleName.C_String());
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_Login_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	unsigned int userRow = GetUserRowFromHandle(userName.C_String(), pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_Client_Login_HANDLE_NOT_IN_USE_OR_BAD_SECRET_KEY;
		return true;
	}
	result = pgsql->QueryVaridic("SELECT (password) FROM lobby2.users WHERE userId_pk=%i", userRow);
	RakNet::RakString password;
	PostgreSQLInterface::PQGetValueFromBinary(&password, result, 0, "password");
	PQclear(result);
	if (password!=userPassword)
	{
		resultCode=L2RC_Client_Login_HANDLE_NOT_IN_USE_OR_BAD_SECRET_KEY;
		return true;
	}
	if (command->server->GetConfigurationProperties()->requiresEmailAddressValidationToLogin)
	{
		result = pgsql->QueryVaridic("SELECT (emailAddressValidated) FROM lobby2.users WHERE userId_pk=%i", userRow);
		bool emailAddressValidated;
		PostgreSQLInterface::PQGetValueFromBinary(&emailAddressValidated, result, 0, "emailAddressValidated");
		PQclear(result);
		if (emailAddressValidated==false)
		{
			resultCode=L2RC_Client_Login_EMAIL_ADDRESS_NOT_VALIDATED;
			return true;
		}
	}
	if (command->server->GetConfigurationProperties()->requiresTitleToLogin)
	{
		RakNet::RakString titleDBSecretKey;
		result = pgsql->QueryVaridic("SELECT titleSecretKey FROM lobby2.titles where titleName_pk=%s", titleName.C_String());
		int numRowsReturned = PQntuples(result);
		if (numRowsReturned==0)
		{
			resultCode=L2RC_Client_Login_BAD_TITLE_OR_TITLE_SECRET_KEY;
			PQclear(result);
			return false;
		}
		PostgreSQLInterface::PQGetValueFromBinary(&titleDBSecretKey, result, 0, "titleSecretKey");
		PQclear(result);
		// title can have no secret key, in which case you just have to specify a valid title
		if (titleDBSecretKey.IsEmpty()==false && titleDBSecretKey!=titleSecretKey)
		{
			resultCode=L2RC_Client_Login_BAD_TITLE_OR_TITLE_SECRET_KEY;
			return false;
		}
	}

	// Does this user have any stolen CD keys?
	result = pgsql->QueryVaridic("SELECT stolen FROM lobby2.cdkeys WHERE userId_fk=%i AND stolen=TRUE",	userRow);
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned!=0)
	{
		resultCode=L2RC_Client_Login_CDKEY_STOLEN;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT description, timeout, creationDate from lobby2.bannedUsers WHERE userId_fk=%i AND now() < timeout", userRow);
	numRowsReturned = PQntuples(result);
	if (numRowsReturned!=0)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&bannedReason, result, 0, "description");
		PostgreSQLInterface::PQGetValueFromBinary(&bannedExpiration, result, 0, "timeout");
		PostgreSQLInterface::PQGetValueFromBinary(&whenBanned, result, 0, "creationDate");
		PQclear(result);
		resultCode=L2RC_Client_Login_BANNED;
		return true;
	}
	PQclear(result);

	result = pgsql->QueryVaridic("INSERT INTO lobby2.userHistory (userId_fk, triggerId_fk) "
		"VALUES "
		"(%i,"
		"(SELECT triggerId_pk FROM lobby2.userHistoryTriggers WHERE description='Login'))", userRow);
	PQclear(result);

	command->callingUserName=userName;
	command->callerUserId=userRow;

	// Let the user do this if they want. Not all users may want ignore lists
	/*
	// Trigger GetIgnoreList for this user, so they download the client ignore list used for rooms when they logon
	Client_GetIgnoreList *ignoreListRequest = (Client_GetIgnoreList *) command->server->GetMessageFactory()->Alloc(L2MID_Client_GetIgnoreList);
	ignoreListRequest->ServerDBImpl(command, databaseInterface);
	command->server->AddOutputFromThread(ignoreListRequest, userRow, userName);
	*/

	OutputFriendsNotification(Notification_Friends_StatusChange::FRIEND_LOGGED_IN, command, pgsql);

	// Have the server record in memory this new login
	command->server->OnLogin(command, true);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_Logoff_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	if (command->callerUserId==0)
		return false;

	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	result = pgsql->QueryVaridic("INSERT INTO lobby2.userHistory (userId_fk, triggerId_fk) "
		"VALUES "
		"(%i,"
		"(SELECT triggerId_pk FROM lobby2.userHistoryTriggers WHERE description='Logoff'));", command->callerUserId);
	PQclear(result);

	// Notification is done below
	command->server->OnLogoff(command, true);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_RegisterAccount_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	bool hasStates=true;
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	if (StringContainsProfanity(userName, pgsql))
	{
		resultCode=L2RC_PROFANITY_FILTER_CHECK_FAILED;
		return true;
	}

	if (createAccountParameters.homeCountry.IsEmpty()==false)
	{
		if (IsValidCountry(createAccountParameters.homeCountry, &hasStates, pgsql)==false)
		{
			resultCode=L2RC_Client_RegisterAccount_INVALID_COUNTRY;
			return true;
		}
	}
	if (hasStates==true)
	{
		if (createAccountParameters.homeState.IsEmpty()==false && IsValidState(createAccountParameters.homeState, pgsql)==false)
		{
			resultCode=L2RC_Client_RegisterAccount_INVALID_STATE;
			return true;
		}
	}
	else
		createAccountParameters.homeState.Clear();

	if (createAccountParameters.billingCountry.IsEmpty()==false)
	{
		if (IsValidCountry(createAccountParameters.billingCountry, &hasStates, pgsql)==false)
		{
			resultCode=L2RC_Client_RegisterAccount_INVALID_COUNTRY;
			return true;
		}
	}
	if (hasStates==true)
	{
		if (createAccountParameters.billingState.IsEmpty()==false && IsValidState(createAccountParameters.billingState, pgsql)==false)
		{
			resultCode=L2RC_Client_RegisterAccount_INVALID_STATE;
			return true;
		}
	}
	else
		createAccountParameters.billingState.Clear();

	if (createAccountParameters.race.IsEmpty()==false &&
		IsValidRace(createAccountParameters.race, pgsql)==false)
	{
		resultCode=L2RC_Client_RegisterAccount_INVALID_RACE;
		return true;
	}
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow!=0)
	{
		resultCode=L2RC_Client_RegisterAccount_HANDLE_ALREADY_IN_USE;
		return true;
	}
	unsigned int requiredAgeYears = command->server->GetConfigurationProperties()->accountRegistrationRequiredAgeYears;
	if (createAccountParameters.ageInDays < requiredAgeYears*365 )
	{
		resultCode=L2RC_Client_RegisterAccount_REQUIRED_AGE_NOT_MET;
		return true;
	}

	if (command->server->GetConfigurationProperties()->accountRegistrationRequiresCDKey)
	{
		if (cdKey.IsEmpty())
		{
			resultCode=L2RC_Client_RegisterAccount_REQUIRES_CD_KEY;
			return true;
		}

		if (titleName.IsEmpty())
		{
			resultCode=L2RC_Client_RegisterAccount_REQUIRES_CD_KEY;
			return true;
		}
		result = pgsql->QueryVaridic("SELECT usable, stolen "
			"FROM lobby2.cdkeys WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s",
			cdKey.C_String(), titleName.C_String());

		int numRowsReturned = PQntuples(result);
		if (numRowsReturned==0)
		{
			PQclear(result);
			resultCode=L2RC_Client_RegisterAccount_CD_KEY_NOT_USABLE;
			return true;
		}

		bool usable;
		bool wasStolen;
		RakNet::RakString usedBy;
		PostgreSQLInterface::PQGetValueFromBinary(&usable, result, 0, "usable");
		PostgreSQLInterface::PQGetValueFromBinary(&wasStolen, result, 0, "stolen");
		PQclear(result);
		if (usable==false)
		{
			PQclear(result);
			resultCode=L2RC_Client_RegisterAccount_CD_KEY_NOT_USABLE;
			return true;
		}
		if (wasStolen)
		{
			PQclear(result);
			resultCode=L2RC_Client_RegisterAccount_CD_KEY_STOLEN;
			return true;
		}
		if (usedBy.IsEmpty()==false)
		{
			PQclear(result);
			resultCode=L2RC_Client_RegisterAccount_CD_KEY_ALREADY_USED;
			return true;
		}
	}

	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.users ("
		"handle, firstname, middlename, lastname,"
		"sex_male, homeaddress1, homeaddress2, homecity, "
		"homezipcode, billingaddress1, billingaddress2, billingcity,"
		"billingzipcode, emailaddress, password, passwordrecoveryquestion,"
		"passwordrecoveryanswer, caption1, caption2, dateOfBirth) "
		"VALUES ("
		"%s, %s, %s, %s,"
		"%b, %s, %s, %s,"
		"%s, %s, %s, %s,"
		"%s, %s, %s, %s,"
		"%s, %s, %s, (select now() - %i * interval '1 day')) RETURNING userId_pk",
		userName.C_String(), createAccountParameters.firstName.C_String(), createAccountParameters.middleName.C_String(), createAccountParameters.lastName.C_String(),
		createAccountParameters.sex_male, createAccountParameters.homeAddress1.C_String(), createAccountParameters.homeAddress2.C_String(), createAccountParameters.homeCity.C_String(),
		createAccountParameters.homeZipCode.C_String(), createAccountParameters.billingAddress1.C_String(),	createAccountParameters.billingAddress2.C_String(), createAccountParameters.billingCity.C_String(),
		createAccountParameters.billingZipCode.C_String(), createAccountParameters.emailAddress.C_String(), createAccountParameters.password.C_String(), createAccountParameters.passwordRecoveryQuestion.C_String(),
		createAccountParameters.passwordRecoveryAnswer.C_String(), createAccountParameters.caption1.C_String(),createAccountParameters.caption2.C_String(),
		createAccountParameters.ageInDays);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	unsigned int userPrimaryKey, key;
	PostgreSQLInterface::PQGetValueFromBinary(&userPrimaryKey, result, 0, "userId_pk");
	PQclear(result);

	// Assign the cd key, already validated earlier
	if (command->server->GetConfigurationProperties()->accountRegistrationRequiresCDKey)
	{
		PQclear(pgsql->QueryVaridic("UPDATE lobby2.cdKeys SET activationDate=now(),"
			"userId_fk=%i WHERE lobby2.cdkeys.cdKey=%s AND lobby2.cdkeys.titleName_fk=%s", userPrimaryKey, cdKey.C_String(), titleName.C_String()));
	}

	if (createAccountParameters.homeState.IsEmpty()==false)
	{
		result = pgsql->QueryVaridic("SELECT state_id FROM lobby2.state where lower(state_name)=lower(%s)", createAccountParameters.homeState.C_String());
		if (result)
		{
			if (PQntuples(result))
			{
				PostgreSQLInterface::PQGetValueFromBinary(&key, result, 0, "state_id");
				PQclear( pgsql->QueryVaridic("UPDATE lobby2.users SET homeStateId_fk=%i WHERE userId_pk=%i", key, userPrimaryKey ));
			}
			PQclear(result);
		}
	}
	if (createAccountParameters.homeCountry.IsEmpty()==false)
	{
		result = pgsql->QueryVaridic("SELECT country_id FROM lobby2.country where lower(country_name)=lower(%s)", createAccountParameters.homeCountry.C_String());
		if (result)
		{
			if (PQntuples(result))
			{
				PostgreSQLInterface::PQGetValueFromBinary(&key, result, 0, "country_id");
				PQclear( pgsql->QueryVaridic("UPDATE lobby2.users SET homeCountryId_fk=%i WHERE userId_pk=%i", key, userPrimaryKey ));
			}
			PQclear(result);
		}
	}
	if (createAccountParameters.billingState.IsEmpty()==false)
	{
		result = pgsql->QueryVaridic("SELECT state_id FROM lobby2.state where lower(state_name)=lower(%s)", createAccountParameters.billingState.C_String());
		if (result)
		{
			if (PQntuples(result))
			{
				PostgreSQLInterface::PQGetValueFromBinary(&key, result, 0, "state_id");
				PQclear( pgsql->QueryVaridic("UPDATE lobby2.users SET billingStateId_fk=%i WHERE userId_pk=%i", key, userPrimaryKey ));
			}
			PQclear(result);
		}
	}
	if (createAccountParameters.billingCountry.IsEmpty()==false)
	{
		result = pgsql->QueryVaridic("SELECT country_id FROM lobby2.country where lower(country_name)=lower(%s)", createAccountParameters.billingCountry.C_String());
		if (result)
		{
			if (PQntuples(result))
			{
				PostgreSQLInterface::PQGetValueFromBinary(&key, result, 0, "country_id");
				PQclear( pgsql->QueryVaridic("UPDATE lobby2.users SET billingCountryId_fk=%i WHERE userId_pk=%i", key, userPrimaryKey ));
			}
			PQclear(result);
		}
	}
	if (createAccountParameters.race.IsEmpty()==false)
	{
		result = pgsql->QueryVaridic("SELECT race_id FROM lobby2.race where lower(race_text)=lower(%s)", createAccountParameters.race.C_String());
		if (result)
		{
			if (PQntuples(result))
			{
				PostgreSQLInterface::PQGetValueFromBinary(&key, result, 0, "race_id");
				PQclear( pgsql->QueryVaridic("UPDATE lobby2.users SET raceId_fk=%i WHERE userId_pk=%i", key, userPrimaryKey ));
			}
			PQclear(result);
		}
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::System_SetEmailAddressValidated_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}
	PGresult *result = pgsql->QueryVaridic("UPDATE lobby2.users SET emailAddressValidated=%b WHERE userId_pk=%i", validated, userRow);
	PQclear(result);
	if (result!=0)
		resultCode=L2RC_SUCCESS;
	else
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
	return true;
}

bool RakNet::Client_ValidateHandle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow!=0)
	{
		resultCode=L2RC_Client_ValidateHandle_HANDLE_ALREADY_IN_USE;
		return true;
	}
	if (StringContainsProfanity(userName, pgsql))
	{
		resultCode=L2RC_PROFANITY_FILTER_CHECK_FAILED;
		return true;
	}
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::System_DeleteAccount_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	// Notify friends of account deletion
	command->callingUserName=userName;
	command->callerUserId=userRow;
	OutputFriendsNotification(Notification_Friends_StatusChange::FRIEND_ACCOUNT_WAS_DELETED, command, pgsql);

	// Trigger logoff as well
	Client_Logoff *logoffRequest = (Client_Logoff *) command->server->GetMessageFactory()->Alloc(L2MID_Client_Logoff);
	logoffRequest->ServerDBImpl( command, databaseInterface );
	command->server->AddOutputFromThread(logoffRequest, userRow, userName);

	// Delete the account
	PGresult *result = pgsql->QueryVaridic("DELETE FROM lobby2.users WHERE userId_pk=%i", userRow);
	PQclear(result);
	if (result!=0)
		resultCode=L2RC_SUCCESS;
	else
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
	return true;
}

bool RakNet::System_PruneAccounts_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic(
		"SELECT handle from lobby2.users WHERE userId_pk ="
		"(SELECT userId_fk FROM lobby2.userHistory WHERE creationDate < now() - %i * interval '1 day' ORDER by creationDate DESC LIMIT 1 AND triggerid_fk="
		"(SELECT triggerId_pk from lobby2.userHistoryTriggers where description='Login')"
		") ;", deleteAccountsNotLoggedInDays);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_SUCCESS;
		return true;
	}

	// Delete all accounts where the user has not logged in deleteAccountsNotLoggedInDays
	System_DeleteAccount *deleteAccount = (System_DeleteAccount *) command->server->GetMessageFactory()->Alloc(L2MID_System_DeleteAccount);

	RakNet::RakString userName;
	for (int i=0; i < numRowsReturned; i++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&userName, result, i, "handle");
		deleteAccount->userName=userName;
		deleteAccount->ServerDBImpl( command, databaseInterface );
	}
	command->server->GetMessageFactory()->Dealloc(deleteAccount);
	PQclear(result);

	return true;
}

bool RakNet::Client_GetEmailAddress_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"SELECT emailaddress, emailAddressValidated from lobby2.users WHERE userId_pk = %i", userRow);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&emailAddress, result, 0, "emailAddress");
	PostgreSQLInterface::PQGetValueFromBinary(&emailAddressValidated, result, 0, "emailAddressValidated");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_GetPasswordRecoveryQuestionByHandle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"SELECT passwordRecoveryQuestion from lobby2.users WHERE userId_pk = %i", userRow);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}


	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&passwordRecoveryQuestion, result, 0, "passwordRecoveryQuestion");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_GetPasswordByPasswordRecoveryAnswer_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"SELECT password from lobby2.users WHERE lower(passwordrecoveryanswer) = lower(%s) AND userId_pk = %i", passwordRecoveryAnswer.C_String(), userRow);

	if (result==0)
	{
		resultCode=L2RC_Client_GetPasswordByPasswordRecoveryAnswer_BAD_ANSWER;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_Client_GetPasswordByPasswordRecoveryAnswer_BAD_ANSWER;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&password, result, 0, "password");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_ChangeHandle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int userRow = GetUserRowFromHandle(userName, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	unsigned int userRow2 = GetUserRowFromHandle(newHandle, pgsql);
	if (userRow2!=0)
	{
		resultCode=L2RC_Client_ChangeHandle_NEW_HANDLE_ALREADY_IN_USE;
		return true;
	}

	if (StringContainsProfanity(newHandle, pgsql))
	{
		resultCode=L2RC_PROFANITY_FILTER_CHECK_FAILED;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"UPDATE lobby2.users SET handle=%s WHERE userId_pk=%i", newHandle.C_String(), userRow);
	PQclear(result);

	// Tell all friends and clanMembers
	DataStructures::List<unsigned int> output;
	GetFriendIDs(command->callerUserId, false, pgsql, output);
	GetClanMateIDs(command->callerUserId, false, pgsql, output);

	unsigned int i;
	for (i=0; i < output.Size(); i++)
	{
		Notification_User_ChangedHandle *notification = (Notification_User_ChangedHandle *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_User_ChangedHandle);
		notification->oldHandle=userName;
		notification->newHandle=newHandle;
		notification->resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(notification, output[i], "");
	}

	command->callingUserName=newHandle;
	command->server->OnChangeHandle(command, true);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_UpdateAccount_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	// TODO - Lots of copy paste, finish this later when the specification is solid

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_StartIgnore_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Client_StartIgnore_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Client_StartIgnore_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.ignore (userMe_fk, userOther_fk) VALUES (%i, %i)", command->callerUserId, targetUserId);

	if (result==0)
	{
		resultCode=L2RC_Client_StartIgnore_ALREADY_IGNORED;
		return true;
	}
	PQclear(result);

	Notification_Client_IgnoreStatus *notification = (Notification_Client_IgnoreStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Client_IgnoreStatus);
	notification->otherHandle=command->callingUserName;
	notification->nowIgnored=true;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, targetHandle);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_GetIgnoreList_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("SELECT handle FROM lobby2.users WHERE userId_pk="
	"(SELECT userOther_fk FROM lobby2.ignore WHERE userMe_fk=%i);", command->callerUserId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	RakNet::RakString handle;
	ignoredHandles.Clear();
	int numRowsReturned = PQntuples(result);
	int i;
	for (i=0; i < numRowsReturned; i++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&handle, result, i, "handle");
		ignoredHandles.Insert(handle);
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Client_StopIgnore_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Client_StopIgnore_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Client_StopIgnore_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	PGresult *result = pgsql->QueryVaridic(
		"DELETE FROM lobby2.ignore WHERE userMe_fk=%i AND userOther_fk=%i", command->callerUserId, targetUserId);
	PQclear(result);

	Notification_Client_IgnoreStatus *notification = (Notification_Client_IgnoreStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Client_IgnoreStatus);
	notification->otherHandle=command->callingUserName;
	notification->nowIgnored=false;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, targetHandle);

	resultCode=L2RC_SUCCESS;
	return true;
}
bool RakNet::Friends_SendInvite_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Friends_SendInvite_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Friends_SendInvite_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	// Don't do if already in friends table (already friends, or already has an invite)
	result = pgsql->QueryVaridic(
		"SELECT description FROM lobby2.friendActions WHERE actionId_pk="
		"(SELECT actionId_fk from lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i);"
	, command->callerUserId, targetUserId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned!=0)
	{
		RakNet::RakString description;
		PostgreSQLInterface::PQGetValueFromBinary(&description, result, 0, "description");
		if (description=="sentInvite")
			resultCode=L2RC_Friends_SendInvite_ALREADY_SENT_INVITE;
		else if (description=="isFriends")
			resultCode=L2RC_Friends_SendInvite_ALREADY_FRIENDS;
		else
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		PQclear(result);
		return true;
	}
	PQclear(result);

	// Add friend invite
	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.friends (userOne_fk, userTwo_fk, actionId_fk) VALUES "
		"(%i, %i, (SELECT actionId_pk FROM lobby2.friendActions WHERE description='sentInvite'));"
		, command->callerUserId, targetUserId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	// Notify by email
	SendEmail(targetUserId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Friends_SendInvite", pgsql);

	// Tell the other system the invitation was sent
	Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
	RakAssert(command->callingUserName.IsEmpty()==false);
	notification->otherHandle=command->callingUserName;
	notification->op=Notification_Friends_StatusChange::GOT_INVITATION_TO_BE_FRIENDS;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, "");

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Friends_AcceptInvite_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Friends_AcceptInvite_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Friends_AcceptInvite_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	// Make sure we have an invite from the other user
	result = pgsql->QueryVaridic(
		"SELECT description FROM lobby2.friendActions WHERE actionId_pk="
		"(SELECT actionId_fk from lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i);"
		, targetUserId, command->callerUserId );

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned!=0)
	{
		RakNet::RakString description;
		PostgreSQLInterface::PQGetValueFromBinary(&description, result, 0, "description");
		PQclear(result);
		if (description!=RakNet::RakString("sentInvite"))
		{
			resultCode=L2RC_Friends_AcceptInvite_NO_INVITE;
			return true;
		}
	}
	else
	{
		PQclear(result);
		resultCode=L2RC_Friends_AcceptInvite_NO_INVITE;
		return true;
	}

	// Change from invited to friends, insert twice
	result = pgsql->QueryVaridic(
		"UPDATE lobby2.friends SET actionId_fk=(SELECT actionId_pk from lobby2.friendActions WHERE description='isFriends') WHERE userOne_fk=%i AND userTwo_fk=%i;"
		, targetUserId, command->callerUserId );
	RakAssert(result);
	PQclear(result);

	// Delete any existing invites, etc. if there are any
	result = pgsql->QueryVaridic(
		"DELETE FROM lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i;"
		, command->callerUserId, targetUserId );
	PQclear(result);

	// Insert as a friend
	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.friends (userOne_fk, userTwo_fk, actionId_fk) VALUES (%i, %i, (SELECT actionId_pk from lobby2.friendActions WHERE description='isFriends'));"
		,command->callerUserId, targetUserId);
	RakAssert(result);
	PQclear(result);

	SendEmail(targetUserId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Friends_AcceptInvite", (PostgreSQLInterface *) databaseInterface);

	// Tell the other system the invitation was accepted
	Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
	RakAssert(command->callingUserName.IsEmpty()==false);
	notification->otherHandle=command->callingUserName;
	notification->op=Notification_Friends_StatusChange::THEY_ACCEPTED_OUR_INVITATION_TO_BE_FRIENDS;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, "");

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Friends_RejectInvite_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Friends_RejectInvite_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Friends_RejectInvite_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	// Make sure we have an invite from the other user
	result = pgsql->QueryVaridic(
		"SELECT description FROM lobby2.friendActions WHERE actionId_pk="
		"(SELECT actionId_fk from lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i);"
		, targetUserId, command->callerUserId );

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned!=0)
	{
		RakNet::RakString description;
		PostgreSQLInterface::PQGetValueFromBinary(&description, result, 0, "description");
		PQclear(result);
		if (description!=RakNet::RakString("sentInvite"))
		{
			resultCode=L2RC_Friends_RejectInvite_NO_INVITE;
			return true;
		}
	}
	else
	{
		PQclear(result);
		resultCode=L2RC_Friends_RejectInvite_NO_INVITE;
		return true;
	}

	// Delete friend invite (both ways)
	result = pgsql->QueryVaridic(
		"DELETE FROM lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i;"
		, targetUserId, command->callerUserId );
	PQclear(result);
	result = pgsql->QueryVaridic(
		"DELETE FROM lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i;"
		, command->callerUserId, targetUserId );
	PQclear(result);

	SendEmail(targetUserId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Friends_AcceptInvite", (PostgreSQLInterface *) databaseInterface);

	// Tell the other system the invitation was rejected
	Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
	RakAssert(command->callingUserName.IsEmpty()==false);
	notification->otherHandle=command->callingUserName;
	notification->op=Notification_Friends_StatusChange::THEY_REJECTED_OUR_INVITATION_TO_BE_FRIENDS;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, "");

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Friends_GetInvites_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	GetFriendHandlesByStatus(command->callerUserId, "sentInvite", pgsql, invitesSent, true);
	GetFriendHandlesByStatus(command->callerUserId, "sentInvite", pgsql, invitesReceived, false);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Friends_GetFriends_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	GetFriendHandlesByStatus(command->callerUserId, "isFriends", pgsql, myFriends, true);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Friends_Remove_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Friends_Remove_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Friends_Remove_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	result = pgsql->QueryVaridic(
		"SELECT userOne_fk FROM lobby2.friends WHERE userOne_fk=%i AND userTwo_fk=%i;"
		, command->callerUserId, targetUserId );
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	PQclear(result);
	if (numRowsReturned==0)
	{
		resultCode=L2RC_Friends_Remove_NOT_FRIENDS;
		return true;
	}

	// Bidirectional delete
	result = pgsql->QueryVaridic("DELETE FROM lobby2.friends WHERE (userOne_fk=%i AND userTwo_fk=%i) OR (userOne_fk=%i AND userTwo_fk=%i)", 
		command->callerUserId, targetUserId, targetUserId, command->callerUserId);
	RakAssert(result);
	PQclear(result);

	SendEmail(targetUserId, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Friends_Remove", (PostgreSQLInterface *) databaseInterface);

	Notification_Friends_StatusChange *notification = (Notification_Friends_StatusChange *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_StatusChange);
	RakAssert(command->callingUserName.IsEmpty()==false);
	notification->otherHandle=command->callingUserName;
	notification->op=Notification_Friends_StatusChange::YOU_WERE_REMOVED_AS_A_FRIEND;
	notification->resultCode=L2RC_SUCCESS;
	command->server->AddOutputFromThread(notification, targetUserId, "");

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::BookmarkedUsers_Add_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_BookmarkedUsers_Add_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_BookmarkedUsers_Add_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.bookmarkedUsers (userMe_fk, userOther_fk, type, description) VALUES (%i, %i, %i, %s)",
		command->callerUserId, targetUserId, type, description.C_String() );
	if (result==0)
	{
		resultCode=L2RC_BookmarkedUsers_Add_ALREADY_BOOKMARKED;
		return true;
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::BookmarkedUsers_Remove_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_BookmarkedUsers_Remove_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_BookmarkedUsers_Remove_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	result = pgsql->QueryVaridic(
		"DELETE FROM lobby2.bookmarkedUsers WHERE userOther_fk=%i AND type=%i",
		targetUserId, type);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::BookmarkedUsers_Get_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	result = pgsql->QueryVaridic(
		"SELECT (SELECT handle FROM lobby2.users where userId_pk=userOther_fk) as handle, *"
		"FROM (SELECT userOther_fk, type, description, creationDate from lobby2.bookmarkedUsers WHERE userMe_fk=%i) as bm ORDER BY type, creationDate ASC;",
		command->callerUserId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	int i;
	BookmarkedUser bm;
	for (i=0; i < numRowsReturned; i++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&bm.targetHandle, result, i, "handle");
		PostgreSQLInterface::PQGetValueFromBinary(&bm.type, result, i, "type");
		PostgreSQLInterface::PQGetValueFromBinary(&bm.description, result, i, "description");
		PostgreSQLInterface::PQGetValueFromBinary(&bm.dateWhenAdded, result, i, "creationDate");
		bookmarkedUsers.Insert(bm);
	}

	PQclear(result);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Emails_Send_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	SendEmail(recipients, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, status, "Emails_Send", (PostgreSQLInterface *) databaseInterface);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Emails_Get_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic(
		"SELECT handle, tbl2.* from lobby2.users, ("
		"SELECT tbl1.*, lobby2.emails.subject, lobby2.emails.body, lobby2.emails.binaryData, lobby2.emails.creationDate FROM"
		"(SELECT emailId_fk, userMe_fk, userOther_fk, status, wasRead, ISentThisEmail, isDeleted FROM lobby2.emailTargets) as tbl1, lobby2.emails "
		"WHERE tbl1.emailId_fk=lobby2.emails.emailId_pk AND tbl1.userMe_fk=%i AND tbl1.isDeleted=FALSE"
		") as tbl2 "
		"WHERE userId_pk=tbl2.userother_fk ORDER BY creationDate ASC;"
		, command->callerUserId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	int i;
	EmailResult emailResult;
	for (i=0; i < numRowsReturned; i++)
	{
		RakNet::RakString otherHandle;
		RakNet::RakString myHandle = command->callingUserName;
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.emailID, result, i, "emailId_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&otherHandle, result, i, "handle");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.status, result, i, "status");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.wasReadByMe, result, i, "wasRead");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.wasSendByMe, result, i, "ISentThisEmail");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.subject, result, i, "subject");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.body, result, i, "body");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.binaryData.binaryData, &emailResult.binaryData.binaryDataLength, result, i, "binaryData");
		PostgreSQLInterface::PQGetValueFromBinary(&emailResult.creationDate, result, i, "creationDate");
		if (emailResult.wasSendByMe)
		{
			emailResult.sender=myHandle;
			emailResult.recipient=otherHandle;
		}
		else
		{
			emailResult.sender=otherHandle;
			emailResult.recipient=myHandle;
		}
		emailResults.Insert(emailResult);
	}

	resultCode=L2RC_SUCCESS;
	PQclear(result);
	return true;
}

bool RakNet::Emails_Delete_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("SELECT isDeleted FROM lobby2.emailTargets WHERE emailTarget_pk = %i", emailId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_Emails_Delete_UNKNOWN_EMAIL_ID;
		return true;
	}
	bool isDeleted;
	PostgreSQLInterface::PQGetValueFromBinary(&isDeleted, result, 0, "isDeleted");
	PQclear(result);
	if (isDeleted)
	{
		resultCode=L2RC_Emails_Delete_ALREADY_DELETED;
		return true;
	}
	// Don't actually delete, just flag as deleted. This is so the admin can investigate reports of abuse.
	result = pgsql->QueryVaridic("UPDATE lobby2.emailTargets SET isDeleted=TRUE WHERE emailTarget_pk = %i", emailId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	PQclear(result);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Emails_SetStatus_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result = pgsql->QueryVaridic("SELECT isDeleted FROM lobby2.emailTargets WHERE emailTarget_pk = %i", emailId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_Emails_SetStatus_UNKNOWN_EMAIL_ID;
		return true;
	}
	bool isDeleted;
	PostgreSQLInterface::PQGetValueFromBinary(&isDeleted, result, 0, "isDeleted");
	PQclear(result);
	if (isDeleted)
	{
		resultCode=L2RC_Emails_SetStatus_WAS_DELETED;
		return true;
	}
	if (updateStatusFlag)
	{
		result = pgsql->QueryVaridic("UPDATE lobby2.emailTargets SET status=%i WHERE emailTarget_pk = %i", newStatusFlag, emailId);
		if (result==0)
		{
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
			return true;
		}
	}
	PQclear(result);
	if (updateMarkedRead)
	{
		result = pgsql->QueryVaridic("UPDATE lobby2.emailTargets SET wasRead=%b WHERE emailTarget_pk = %i", isNowMarkedRead, emailId);
		if (result==0)
		{
			resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
			return true;
		}
	}
	PQclear(result);


	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_SubmitMatch_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_SubmitMatch_TITLE_NOT_IN_USE;
		return true;
	}

	// Insert
	result = pgsql->QueryVaridic("INSERT INTO lobby2.matches (gameTypeName, titleName_fk, matchNote, binaryData) VALUES "
		"(%s, %s, %s, %a) RETURNING matchId_pk;",
		gameType.C_String(), titleName.C_String(), submittedMatch.matchNote.C_String(), submittedMatch.binaryData.binaryData, submittedMatch.binaryData.binaryDataLength );

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.matchID, result, 0, "matchId_pk");
	PQclear(result);

	// For each match participant, add to lobby2.matchParticipants
	unsigned int i;
	for (i=0; i < submittedMatch.matchParticipants.Size(); i++)
	{
		result = pgsql->QueryVaridic("INSERT INTO lobby2.matchParticipants (matchId_fk, userId_fk, score) VALUES "
			"(%i, (SELECT userId_pk FROM lobby2.users WHERE handleLower=lower(%s)), %f);",
			submittedMatch.matchID, submittedMatch.matchParticipants[i].handle.C_String(), submittedMatch.matchParticipants[i].score);
		// May fail if a user is deleted at the same time this is running
		if (result)
			PQclear(result);
	}

	result = pgsql->QueryVaridic("SELECT EXTRACT(EPOCH FROM now()) as whenSubmittedDate;");
	PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.whenSubmittedDate, result, 0, "whenSubmittedDate");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_GetMatches_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result1, *result2;

	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_SubmitMatch_TITLE_NOT_IN_USE;
		return true;
	}

	result1 = pgsql->QueryVaridic("SELECT matchId_pk, matchNote, binaryData, EXTRACT(EPOCH FROM creationDate) as creationDate from lobby2.matches WHERE gameTypeName=%s AND titleName_fk=%s;", gameType.C_String(), titleName.C_String());
	if (result1==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numMatchesReturned = PQntuples(result1);
	int i;
	SubmittedMatch submittedMatch;
	MatchParticipant matchParticipant;
	for (i=0; i < numMatchesReturned; i++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.matchID, result1, i, "matchId_pk");
		PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.matchNote, result1, i, "matchNote");
		//	PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.binaryData.binaryData, &submittedMatch.binaryData.binaryDataLength, result1, i, "binaryData");
		PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.whenSubmittedDate, result1, i, "creationDate");
		PostgreSQLInterface::PQGetValueFromBinary(&submittedMatch.matchID, result1, i, "matchId_pk");

		result2=pgsql->QueryVaridic("SELECT (SELECT handle FROM lobby2.users where userId_pk=userId_fk) as handle, score from lobby2.matchParticipants where matchId_fk=%i;", submittedMatch.matchID);
		int numParticipants = PQntuples(result2);
		for (int j=0; j < numParticipants; j++)
		{
			PostgreSQLInterface::PQGetValueFromBinary(&matchParticipant.handle, result2, j, "handle");
			PostgreSQLInterface::PQGetValueFromBinary(&matchParticipant.score, result2, j, "score");
			submittedMatch.matchParticipants.Insert(matchParticipant);
		}

		PQclear(result2);

		submittedMatches.Insert(submittedMatch);
	}
	PQclear(result1);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_GetMatchBinaryData_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	result = pgsql->QueryVaridic("SELECT binaryData from lobby2.matches WHERE matchId_pk=%i", matchID);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_Ranking_GetMatchBinaryData_INVALID_MATCH_ID;
		return false;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&binaryData.binaryData, &binaryData.binaryDataLength, result, 0, "binaryData");
	PQclear(result);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_GetTotalScore_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_GetTotalScore_TITLE_NOT_IN_USE;
		return true;
	}

	unsigned int userRow = GetUserRowFromHandle(targetHandle, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT COUNT(score) as count, sum(score) as sum from lobby2.matchParticipants WHERE userId_fk=%i AND matchId_fk IN"
		"(SELECT matchId_pk from lobby2.matches WHERE gameTypeName=%s AND titleName_fk=%s);", userRow, gameType.C_String(), titleName.C_String());

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int64_t count;
	PostgreSQLInterface::PQGetValueFromBinary(&count, result, 0, "count");
	numScoresSubmitted = (unsigned int) count;
	if (numScoresSubmitted>0)
		PostgreSQLInterface::PQGetValueFromBinary(&scoreSum, result, 0, "sum");
	else
		scoreSum=0.0f;
	PQclear(result);
	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_WipeScoresForPlayer_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_WipeScoresForPlayer_TITLE_NOT_IN_USE;
		return true;
	}

	unsigned int userRow = GetUserRowFromHandle(targetHandle, pgsql);
	if (userRow==0)
	{
		resultCode=L2RC_UNKNOWN_USER;
		return true;
	}

	result = pgsql->QueryVaridic("DELETE FROM lobby2.matchParticipants WHERE userId_fk=%i AND matchId_fk IN"
		"(SELECT matchId_pk from lobby2.matches WHERE gameTypeName=%s AND titleName_fk=%s);", userRow, gameType.C_String(), titleName.C_String());

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_WipeMatches_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_WipeMatches_TITLE_NOT_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic("DELETE FROM lobby2.matches WHERE gameTypeName=%s AND titleName_fk=%s;", gameType.C_String(), titleName.C_String());

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_PruneMatches_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;
	result = pgsql->QueryVaridic("DELETE FROM lobby2.matches WHERE creationDate < (select now() - %i * interval '1 day')", pruneTimeDays);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_UpdateRating_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_UpdateRating_TITLE_NOT_IN_USE;
		return true;
	}

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Ranking_UpdateRating_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	result = pgsql->QueryVaridic("INSERT INTO lobby2.ratings (userId_fk, gameTypeName, titleName_fk, userRating) VALUES (%i, %s, %s, %f)", targetUserId, gameType.C_String(), titleName.C_String(), targetRating);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_WipeRatings_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	// Verify title name
	if (IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_WipeRatings_TITLE_NOT_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic("DELETE FROM lobby2.ratings WHERE gameTypeName=%s AND titleName_fk=%s;", gameType.C_String(), titleName.C_String());

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Ranking_GetRating_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	if (RakNet::IsTitleInUse(titleName, pgsql)==false)
	{
		resultCode=L2RC_Ranking_GetRating_TITLE_NOT_IN_USE;
		return true;
	}

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Ranking_GetRating_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT userRating, creationDate FROM lobby2.ratings WHERE gameTypeName=%s AND titleName_fk=%s and userId_fk=%i ORDER by creationDate LIMIT 1;", gameType.C_String(), titleName.C_String(), targetUserId);
	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}


	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_Ranking_GetRating_NO_RATING;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&currentRating, result, 0, "userRating");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_Create_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	if (StringContainsProfanity(clanHandle, pgsql))
	{
		resultCode=L2RC_PROFANITY_FILTER_CHECK_FAILED;
		return true;
	}

	if (GetClanIdFromHandle(clanHandle, pgsql))
	{
		resultCode=L2RC_Clans_Create_CLAN_HANDLE_IN_USE;
		return true;
	}

	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.clans (leaderUserId_fk, clanHandle, requiresInvitationsToJoin, description, binaryData) VALUES "
		"(%i, %s, %b, %s, %a) RETURNING clanId_pk;", command->callerUserId, clanHandle.C_String(), requiresInvitationsToJoin, description.C_String(), binaryData.binaryData, binaryData.binaryDataLength );

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	unsigned int clanId_pk;
	PostgreSQLInterface::PQGetValueFromBinary(&clanId_pk, result, 0, "clanId_pk");
	PQclear(result);

	if (failIfAlreadyInClan)
	{
		// Checking after rather than before in case this user creates a clan in another thread at the same time
		result = pgsql->QueryVaridic("SELECT COUNT(*) as count from lobby2.clans WHERE leaderUserId_fk=%i", command->callerUserId);
		long long count;
		PostgreSQLInterface::PQGetValueFromBinary(&count, result, 0, "count");
		PQclear(result);
		if (count>1)
		{
			result = pgsql->QueryVaridic("DELETE FROM lobby2.clans WHERE (clanId_pk=%i);", clanId_pk);
			PQclear(result);
			resultCode=L2RC_Clans_Create_ALREADY_IN_A_CLAN;
			return true;
		}
	}

	// Add yourself as a clan member
	result = pgsql->QueryVaridic(
		"INSERT INTO lobby2.clanMembers (userId_fk, clanId_fk, isSubleader, memberState_fk) VALUES "
		"(%i, %i, false, (SELECT stateId_Pk FROM lobby2.clanMemberStates WHERE description='ClanMember_Active'));", command->callerUserId, clanId_pk );

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	// Tell all friends about this new clan
	DataStructures::List<unsigned int> output;
	GetFriendIDs(command->callerUserId, true, pgsql, output);

	unsigned int idx;
	for (idx=0; idx < output.Size(); idx++)
	{
		Notification_Friends_CreatedClan *notification = (Notification_Friends_CreatedClan *)command->server->GetMessageFactory()->Alloc(L2MID_Notification_Friends_CreatedClan);
		RakAssert(command->callingUserName.IsEmpty()==false);
		notification->otherHandle=command->callingUserName;
		notification->clanName=clanHandle;
		notification->resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(notification, output[idx], "");
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_SetProperties_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_SetProperties_UNKNOWN_CLAN;
		return true;
	}
	if (IsClanLeader(clanId, command->callerUserId, pgsql)==false)
	{
		resultCode=L2RC_Clans_SetProperties_MUST_BE_LEADER;
		return true;
	}

	result = pgsql->QueryVaridic("UPDATE lobby2.clans SET description=%s, binaryData=%a WHERE clanId_pk=%i", description.C_String(), binaryData.binaryData, binaryData.binaryDataLength, clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_GetProperties_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_GetProperties_UNKNOWN_CLAN;
		return true;
	}

	result = pgsql->QueryVaridic("SELECT description, binaryData FROM lobby2.clans WHERE clanId_pk=%i", clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PostgreSQLInterface::PQGetValueFromBinary(&description, result, 0, "description");
	PostgreSQLInterface::PQGetValueFromBinary(&binaryData.binaryData, &binaryData.binaryDataLength, result, 0, "binaryData");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_SetMyMemberProperties_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_SetMyMemberProperties_UNKNOWN_CLAN;
		return true;
	}

	bool isSubleader;
	RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
	if (clanMemberState!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_SetMyMemberProperties_NOT_IN_CLAN;
		return true;
	}

	result = pgsql->QueryVaridic("UPDATE lobby2.clanMembers SET description=%s, binaryData=%a WHERE userId_fk=%i AND clanId_fk=%i", description.C_String(), binaryData.binaryData, binaryData.binaryDataLength, command->callerUserId, clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}
bool RakNet::Clans_Get_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;

	// Gets all clans that I am in
	PGresult *result = pgsql->QueryVaridic(
		"SELECT users.handle, myClansWithClanHandle.clanHandle,"
		"myClansWithClanHandle.description, myClansWithClanHandle.binaryData, "
		"myClansWithClanHandle.clanId_fk "
		"FROM lobby2.users, (SELECT clanHandle, description, binaryData, leaderUserId_fk, myClans.* FROM lobby2.clans, "
		"(SELECT clanId_fk FROM lobby2.clanMembers WHERE userId_fk=%i) AS myClans "
		"WHERE clanId_pk=myClans.clanId_fk) AS myClansWithClanHandle WHERE users.userId_pk=myClansWithClanHandle.leaderUserId_fk "
		, command->callerUserId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	int numRowsReturned = PQntuples(result);
	if (numRowsReturned==0)
	{
		PQclear(result);
		resultCode=L2RC_SUCCESS;
		return true;
	}

	ClanInfo ci;
	for (int i=0; i < numRowsReturned; i++)
	{
		PGresult *result2;
		unsigned int clanId;
		PostgreSQLInterface::PQGetValueFromBinary(&ci.clanName, result, i, "clanHandle");
		PostgreSQLInterface::PQGetValueFromBinary(&ci.description, result, i, "description");
		PostgreSQLInterface::PQGetValueFromBinary(&ci.clanLeader, result, i, "handle");
		PostgreSQLInterface::PQGetValueFromBinary(&ci.binaryData.binaryData, &ci.binaryData.binaryDataLength, result, i, "binaryData");
		PostgreSQLInterface::PQGetValueFromBinary(&clanId, result, i, "clanId_fk");

		// Get the names of all other members in this clan
		result2 = pgsql->QueryVaridic("SELECT lobby2.users.handle from lobby2.clanMembers, lobby2.users WHERE clanId_fk=%i AND userId_fk!=%i AND lobby2.users.userId_pk=lobby2.clanMembers.userId_fk", clanId, command->callerUserId);

		int numRowsReturned2 = PQntuples(result2);
		RakNet::RakString memberHandle;
		for (int j=0; j < numRowsReturned2; j++)
		{
			PostgreSQLInterface::PQGetValueFromBinary(&memberHandle, result2, j, "handle");
			ci.clanMembersOtherThanLeader.Insert(memberHandle);
		}
		PQclear(result2);

		clans.Insert(ci);
	}

	PQclear(result);


	resultCode=L2RC_SUCCESS;
	return true;

};
bool RakNet::Clans_GrantLeader_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_GrantLeader_UNKNOWN_CLAN;
		return true;
	}

	if (IsClanLeader(clanId, command->callerUserId, pgsql)==false)
	{
		resultCode=L2RC_Clans_GrantLeader_MUST_BE_LEADER;
		return true;
	}

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Clans_GrantLeader_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Clans_GrantLeader_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	bool isSubleader;
	RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, targetUserId, &isSubleader, pgsql);
	if (clanMemberState!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_GrantLeader_TARGET_NOT_IN_CLAN;
		return true;
	}

	result = pgsql->QueryVaridic("UPDATE lobby2.clans SET leaderUserId_fk=%i WHERE clanId_pk=%i;",targetUserId, clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);
	resultCode=L2RC_SUCCESS;

	Notification_Clans_GrantLeader *notification;
	// Tell all clan members of the new leader
	DataStructures::List<ClanMemberDescriptor> clanMembers;
	GetClanMembers(clanId, clanMembers, pgsql);

	// Tell all clan members, except the command originator, about the new clan leader
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		if (clanMembers[i].userId==command->callerUserId)
			continue;
		notification = (Notification_Clans_GrantLeader *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_GrantLeader);
		notification->clanHandle=clanHandle;
		notification->oldLeader=command->callingUserName;
		notification->newLeader=targetHandle;
		command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
	}

	return true;
}

bool RakNet::Clans_SetSubleaderStatus_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_SetSubleaderStatus_UNKNOWN_CLAN;
		return true;
	}

	if (IsClanLeader(clanId, command->callerUserId, pgsql)==false)
	{
		resultCode=L2RC_Clans_SetProperties_MUST_BE_LEADER;
		return true;
	}

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Clans_SetSubleaderStatus_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Clans_SetSubleaderStatus_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	bool isSubleader;
	RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, targetUserId, &isSubleader, pgsql);
	if (clanMemberState!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_SetSubleaderStatus_TARGET_NOT_IN_CLAN;
		return true;
	}
	result = pgsql->QueryVaridic("UPDATE lobby2.clanMembers SET isSubleader=%b WHERE clanId_fk=%i AND userId_fk=%i;",setToSubleader,clanId,targetUserId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);


	Notification_Clans_SetSubleaderStatus *notification;
	// Tell all clan members of the new leader
	DataStructures::List<ClanMemberDescriptor> clanMembers;
	GetClanMembers(clanId, clanMembers, pgsql);

	// Tell all clan members, except the command originator, about the new clan leader
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		if (clanMembers[i].userId==command->callerUserId)
			continue;
		notification = (Notification_Clans_SetSubleaderStatus *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_SetSubleaderStatus);
		notification->clanHandle=clanHandle;
		notification->leaderHandle=command->callingUserName;
		notification->targetHandle=targetUserId;
		notification->setToSubleader=setToSubleader;
		command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_SetMemberRank_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_SetMemberRank_UNKNOWN_CLAN;
		return true;
	}

	if (IsClanLeader(clanId, command->callerUserId, pgsql)==false)
	{
		resultCode=L2RC_Clans_SetMemberRank_MUST_BE_LEADER;
		return true;
	}

	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Clans_SetMemberRank_UNKNOWN_TARGET_HANDLE;
		return true;
	}

	if (targetUserId==command->callerUserId)
	{
		resultCode=L2RC_Clans_SetMemberRank_CANNOT_PERFORM_ON_SELF;
		return true;
	}

	bool isSubleader;
	RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, targetUserId, &isSubleader, pgsql);
	if (clanMemberState!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_SetMemberRank_TARGET_NOT_IN_CLAN;
		return true;
	}
	result = pgsql->QueryVaridic("UPDATE lobby2.clanMembers SET rank=%i WHERE clanId_fk=%i AND userId_fk=%i;",newRank,clanId,targetUserId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);


	Notification_Clans_SetMemberRank *notification;
	// Tell all clan members of the new leader
	DataStructures::List<ClanMemberDescriptor> clanMembers;
	GetClanMembers(clanId, clanMembers, pgsql);

	// Tell all clan members, except the command originator, about the new clan leader
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		if (clanMembers[i].userId==command->callerUserId)
			continue;
		notification = (Notification_Clans_SetMemberRank *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_SetMemberRank);
		notification->clanHandle=clanHandle;
		notification->leaderHandle=command->callingUserName;
		notification->targetHandle=targetUserId;
		notification->newRank=newRank;
		command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_GetMemberProperties_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PGresult *result;
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_SetMemberRank_UNKNOWN_CLAN;
		return true;
	}
	unsigned int targetUserId = GetUserRowFromHandle(targetHandle, pgsql);
	if (targetUserId==0)
	{
		resultCode=L2RC_Clans_SetMemberRank_UNKNOWN_TARGET_HANDLE;
		return true;
	}
	bool isSubleader;
	RakNet::ClanMemberState clanMemberState = GetClanMemberState(clanId, targetUserId, &isSubleader, pgsql);
	if (clanMemberState!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_SetMemberRank_TARGET_NOT_IN_CLAN;
		return true;
	}
	result = pgsql->QueryVaridic("SELECT description, binaryData, isSubleader, rank, memberState_fk, banReason FROM lobby2.clanMembers where userId_fk=%i AND clanId_fk=%i",
		targetUserId, clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&description, result, 0, "description");
	PostgreSQLInterface::PQGetValueFromBinary(&binaryData.binaryData, &binaryData.binaryDataLength, result, 0, "binaryData");
	PostgreSQLInterface::PQGetValueFromBinary(&rank, result, 0, "isSubleader");
	PostgreSQLInterface::PQGetValueFromBinary(&isSubleader, result, 0, "rank");
	int cms;
	PostgreSQLInterface::PQGetValueFromBinary(&cms, result, 0, "memberState_fk");
	clanMemberState=(ClanMemberState)cms;
	PostgreSQLInterface::PQGetValueFromBinary(&banReason, result, 0, "banReason");
	PQclear(result);

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_ChangeHandle_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(oldClanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_ChangeHandle_UNKNOWN_CLAN;
		return true;
	}

	if (IsClanLeader(clanId, command->callerUserId, pgsql)==false)
	{
		resultCode=L2RC_Clans_ChangeHandle_MUST_BE_LEADER;
		return true;
	}

	result = pgsql->QueryVaridic("UPDATE lobby2.clans SET clanHandle=%s WHERE clanId_pk=%i;",newClanHandle.C_String(), clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}
	PQclear(result);

	Notification_Clans_ChangeHandle *notification;
	// Tell all clan members of the new leader
	DataStructures::List<ClanMemberDescriptor> clanMembers;
	GetClanMembers(clanId, clanMembers, pgsql);

	// Tell all clan members, except the command originator, about the new clan name
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		if (clanMembers[i].userId==command->callerUserId)
			continue;
		notification = (Notification_Clans_ChangeHandle *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_ChangeHandle);
		notification->oldClanHandle=oldClanHandle;
		notification->leaderHandle=command->callingUserName;
		notification->newClanHandle=newClanHandle;
		command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

bool RakNet::Clans_Leave_PGSQL::ServerDBImpl( Lobby2ServerCommand *command, void *databaseInterface )
{
	PostgreSQLInterface *pgsql = (PostgreSQLInterface *)databaseInterface;
	PGresult *result;

	unsigned int clanId = GetClanIdFromHandle(clanHandle, pgsql);
	if (clanId==0)
	{
		resultCode=L2RC_Clans_Leave_UNKNOWN_CLAN;
		return true;
	}

	bool isSubleader;
	ClanMemberState state = GetClanMemberState(clanId, command->callerUserId, &isSubleader, pgsql);
	if (state!=CMD_ACTIVE)
	{
		resultCode=L2RC_Clans_Leave_NOT_IN_CLAN;
		return true;
	}

	bool isClanLeader = IsClanLeader(clanId, command->callerUserId, pgsql);

	// Remove from the clanMembers table
	result = pgsql->QueryVaridic("DELETE FROM lobby2.clanMembers WHERE userId_fk=%i AND clanId_fk=%i", command->callerUserId, clanId);

	if (result==0)
	{
		resultCode=L2RC_DATABASE_CONSTRAINT_FAILURE;
		return true;
	}

	PQclear(result);

	// Tell all clan members of the new leader
	DataStructures::List<ClanMemberDescriptor> clanMembers;
	GetClanMembers(clanId, clanMembers, pgsql);

	if (subject.IsEmpty()==false || body.IsEmpty()==false)
	{
		// Send this email to the members
		DataStructures::List<unsigned int> targetUserIds;
		for (unsigned int i=0; i < clanMembers.Size(); i++)
		{
			if (clanMembers[i].memberState==CMD_ACTIVE)
				targetUserIds.Insert(clanMembers[i].userId);
		}
		SendEmail(targetUserIds, command->callerUserId, command->callingUserName, command->server, subject, body, &binaryData, emailStatus, "Clans_Leave", pgsql);
	}

	unsigned int validUserCount=0;
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		validUserCount++;
	}

	if (isClanLeader)
	{
		if (dissolveIfClanLeader || validUserCount==0)
		{
			// Send notification to clan members that the clan was destroyed, then destroy the clan
			Notification_Clans_Destroyed *notification;
			for (unsigned int i=0; i < clanMembers.Size(); i++)
			{
				if (clanMembers[i].memberState!=CMD_ACTIVE)
					continue;
				notification = (Notification_Clans_Destroyed *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_Destroyed);
				notification->clanHandle=clanHandle;
				notification->oldClanLeader=command->callingUserName;
				notification->resultCode=L2RC_SUCCESS;
				command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
			}

			// Tell the former leader the clan was destroyed too
			notification = (Notification_Clans_Destroyed *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_Destroyed);
			notification->clanHandle=clanHandle;
			notification->oldClanLeader=command->callingUserName;
			notification->resultCode=L2RC_SUCCESS;
			command->server->AddOutputFromThread(notification, command->callerUserId, command->callingUserName);

			// Destroy the clan
			result = pgsql->QueryVaridic("DELETE FROM lobby2.clans WHERE clanId_pk=%i", clanId);
			PQclear(result);

			resultCode=L2RC_SUCCESS;
			return true;
		}
		else
		{
			// Choose the oldest subleader to lead, or if no subleaders, the oldest member
			result = pgsql->QueryVaridic("SELECT handle, userId_pk FROM lobby2.users WHERE userId_pk=(SELECT userId_fk FROM lobby2.clanMembers WHERE clanId_fk=%i ORDER BY isSubleader, creationDate DESC LIMIT 1)",
				clanId);


			int numRowsReturned = PQntuples(result);
			if (numRowsReturned==0)
			{
				// Destroy the clan if no possible leader (due to asynch)
				PQclear(result);
				result = pgsql->QueryVaridic("DELETE FROM lobby2.clans WHERE clanId_pk=%i", clanId);
				PQclear(result);
				resultCode=L2RC_SUCCESS;
				return true;
			}

			RakNet::RakString newLeaderHandle;
			unsigned int newLeaderId;
			PostgreSQLInterface::PQGetValueFromBinary(&newLeaderHandle, result, 0, "handle");
			PostgreSQLInterface::PQGetValueFromBinary(&newLeaderId, result, 0, "userId_pk");

			PQclear(result);

			// Promote this member to the leader
			result = pgsql->QueryVaridic("UPDATE lobby2.clans SET leaderUserId_fk=%i WHERE clanId_pk = %i", newLeaderId, clanId);
			PQclear(result);

			result = pgsql->QueryVaridic("UPDATE lobby2.clanMembers SET isSubleader=FALSE WHERE userId_fk = %i AND clanId_pk = %i", newLeaderId, clanId);
			PQclear(result);

			// Notify users of new leader
			Notification_Clans_GrantLeader *notification;
			for (unsigned int i=0; i < clanMembers.Size(); i++)
			{
				if (clanMembers[i].memberState!=CMD_ACTIVE)
					continue;
				if (clanMembers[i].userId==command->callerUserId)
					continue;
				notification = (Notification_Clans_GrantLeader *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_GrantLeader);
				notification->clanHandle=clanHandle;
				notification->newLeader=newLeaderHandle;
				notification->oldLeader=command->callingUserName;
				notification->resultCode=L2RC_SUCCESS;
				command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
			}
		}
	}


	// Send notification to clan members that the member has left the clan
	Notification_Clans_Leave *notification;
	for (unsigned int i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i].memberState!=CMD_ACTIVE)
			continue;
		if (clanMembers[i].userId==command->callerUserId)
			continue;
		notification = (Notification_Clans_Leave *) command->server->GetMessageFactory()->Alloc(L2MID_Notification_Clans_Leave);
		notification->clanHandle=clanHandle;
		notification->targetHandle=command->callingUserName;
		notification->resultCode=L2RC_SUCCESS;
		command->server->AddOutputFromThread(notification, clanMembers[i].userId, clanMembers[i].name.C_String());
	}

	resultCode=L2RC_SUCCESS;
	return true;
}

