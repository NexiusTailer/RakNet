#include "RakNetDataProvider_PC.h"
#include "Lobby2Client.h"
#include "RoomsPlugin.h"
#include "RakNetworkFactory.h"
#include "MessageIdentifiers.h"
#include "SocketLayer.h"

using namespace RakNet;

// ---------------------------------------------------------------------------------------

// Why was there no accessor here to begin with?
class RakNetLobbyServer : public LobbyServer
{
public:
	RakNetLobbyServer(NetworkType net) : LobbyServer(net) {}

	void ClearLobbyPlayers(void) {Team0.Clear(); Team1.Clear();}
	void AddLobbyPlayer(const PlayerBrowserData &lobbyPlayer)
	{
		GPtr<LobbyPlayer> p = new LobbyPlayer(RakString::ToInteger(lobbyPlayer.name));
		p->SetTeam(lobbyPlayer.team);
		p->SetName(lobbyPlayer.name);
		p->SetScore(lobbyPlayer.score);
		p->SetDeaths(lobbyPlayer.deaths);
		p->SetPing(lobbyPlayer.ping);

		if (lobbyPlayer.team==1)
		{
			Team1.PushBack(p);
		}
		else
		{
			Team0.PushBack(p);
		}
	}
};

// ---------------------------------------------------------------------------------------

void PlayerBrowserData::Serialize(RakNet::BitStream *bitStream, bool writeToBitstream)
{
	bitStream->Serialize(writeToBitstream, name);
	bitStream->Serialize(writeToBitstream, team);
	bitStream->Serialize(writeToBitstream, score);
	bitStream->Serialize(writeToBitstream, deaths);
	bitStream->Serialize(writeToBitstream, ping);
}

// ---------------------------------------------------------------------------------------

void ServerAndRoomBrowserData::Serialize(RakNet::BitStream *bitStream, bool writeToBitstream)
{
	bitStream->Serialize(writeToBitstream, gameName);
	unsigned short numPlayers=players.Size();
	bitStream->Serialize(writeToBitstream, numPlayers);
	for (unsigned short i=0; i < numPlayers; i++)
	{
		if (writeToBitstream)
		{
			players[i].Serialize(bitStream,writeToBitstream);
		}
		else
		{
			PlayerBrowserData pld;
			pld.Serialize(bitStream,writeToBitstream);
			players.Push(pld,__FILE__,__LINE__);
		}
	}
	bitStream->Serialize(writeToBitstream, maxPlayers);
	bitStream->Serialize(writeToBitstream, mapName);
	bitStream->Serialize(writeToBitstream, gameMode);
	bitStream->Serialize(writeToBitstream, serverHasAPassword);
	bitStream->Serialize(writeToBitstream, autoBalanced);
	bitStream->Serialize(writeToBitstream, gameVersion);
	bitStream->Serialize(writeToBitstream, mapSize);
	bitStream->Serialize(writeToBitstream, friendlyFire);
}
void ServerAndRoomBrowserData::WriteToTable(DataStructures::Table *table)
{
	table->Clear();
	DataStructures::Table::Cell cells[9];
	table->AddColumn("gameName", DataStructures::Table::STRING);
	cells[0].Set(gameName.C_String());
	table->AddColumn("maxPlayers", DataStructures::Table::NUMERIC);
	cells[1].Set((unsigned int) maxPlayers);
	table->AddColumn("mapName", DataStructures::Table::STRING);
	cells[2].Set(mapName.C_String());
	table->AddColumn("gameMode", DataStructures::Table::STRING);
	cells[3].Set(gameMode.C_String());
	table->AddColumn("serverHasAPassword", DataStructures::Table::NUMERIC);
	cells[4].Set((unsigned int)serverHasAPassword);
	table->AddColumn("autoBalanced", DataStructures::Table::STRING);
	cells[5].Set(autoBalanced.C_String());
	table->AddColumn("gameVersion", DataStructures::Table::STRING);
	cells[6].Set(gameVersion.C_String());
	table->AddColumn("mapSize", DataStructures::Table::STRING);
	cells[7].Set(mapSize.C_String());
	table->AddColumn("friendlyFire", DataStructures::Table::STRING);
	cells[8].Set(friendlyFire.C_String());
	RakAssert(table->GetColumnCount()==9);
	DataStructures::List<DataStructures::Table::Cell*> initialCellValues;
	for (int i=0; i < 9; i++)
		initialCellValues.Push(&cells[i],__FILE__,__LINE__);
	table->AddRow(table->GetRowCount(), initialCellValues, true);
}
void ServerAndRoomBrowserData::SetAsOfflinePingResponse(RakPeerInterface *rakPeer)
{
	RakNet::BitStream bs;
	Serialize(&bs,true);
	rakPeer->SetOfflinePingResponse((const char*) bs.GetData(),bs.GetNumberOfBytesUsed());
}
void ServerAndRoomBrowserData::WriteToLobbyServer(GPtr<LobbyServer> lobbyServer)
{
	lobbyServer->SetName(gameName);
	lobbyServer->SetNumPlayers(players.Size());
	lobbyServer->SetMaxPlayers(maxPlayers);
	char temp[128];
	G_sprintf(temp, sizeof(temp), "%d/%d", players.Size(), maxPlayers);
	lobbyServer->SetPlayers(temp);
	lobbyServer->SetMapName(mapName);
	lobbyServer->SetGameMode(gameMode);
	lobbyServer->SetLocked(serverHasAPassword);
	lobbyServer->SetAutoBalanced(autoBalanced);
	lobbyServer->SetGameVersion(gameVersion);
	lobbyServer->SetMapSize(mapSize);
	lobbyServer->SetFriendlyFire(friendlyFire);

	RakNetLobbyServer *rnls = (RakNetLobbyServer*) lobbyServer.GetPtr();
	rnls->ClearLobbyPlayers();
	for (unsigned short i=0; i < players.Size(); i++)
	{
		rnls->AddLobbyPlayer(players[i]);
	}
}

// ---------------------------------------------------------------------------------------

RakNetDataProvider_PC::RakNetDataProvider_PC()
{
	serverAddress=UNASSIGNED_SYSTEM_ADDRESS;
	lobby2Client=0;
	msgFactory=0;
	roomsPlugin=0;
	rakPeer=0;
	instancesWereAutocreated=false;
}
RakNetDataProvider_PC::~RakNetDataProvider_PC()
{

}
UPInt RakNetDataProvider_PC::GetTotalServerCount(LobbyServer::NetworkType net)
{
	return 0;
}
void RakNetDataProvider_PC::SetExternalInterfaces(const RakNetDataProviderExternalInterfaces &externalInterfaces)
{
	GASSERT(externalInterfaces.serverAddress!=UNASSIGNED_SYSTEM_ADDRESS);
	GASSERT(externalInterfaces.externallyAllocatedLobby2Client!=0);
	GASSERT(externalInterfaces.externallyAllocatedLobby2MessageFactory!=0);
	GASSERT(externalInterfaces.externallyAllocatedRoomsPlugin!=0);

	serverAddress=externalInterfaces.serverAddress;
	lobby2Client=externalInterfaces.externallyAllocatedLobby2Client;
	lobby2Client->SetServerAddress(serverAddress);

	msgFactory=externalInterfaces.externallyAllocatedLobby2MessageFactory;
	lobby2Client->SetMessageFactory(msgFactory);
	lobby2Client->AddCallbackInterface(this);
	roomsPlugin=externalInterfaces.externallyAllocatedRoomsPlugin;
	roomsPlugin->SetServerAddress(serverAddress);
	roomsPlugin->SetRoomsCallback(this);
	rakPeer=lobby2Client->GetRakPeerInterface();
	GASSERT(rakPeer);
	titleName=externalInterfaces.titleName;
	titleSecretKey=externalInterfaces.titleSecretKey;
	lanServerPort=externalInterfaces.wanServerPort;

	// TODO
	// Connect to rooms server

	if (rakPeer->IsConnected(serverAddress,false,false)==false)
	{
		disconnectFromServerOnLogoff=true;
	}
	else
	{
		disconnectFromServerOnLogoff=false;
	}
}
void RakNetDataProvider_PC::Startup(void)
{
	connectionState=UNKNOWN;

	// This is a bug, but Scaleform didn't provide any way to supply external data to the data provider
	// This code should not be used outside of the sample
#ifdef AUTOMANAGE_RAKNET_INSTANCES
	if (lobby2Client==0 && roomsPlugin==0)
	{
		instancesWereAutocreated=true;

		rakPeer = RakNetworkFactory::GetRakPeerInterface();
//		rakPeer->SetTimeoutTime(3000,UNASSIGNED_SYSTEM_ADDRESS);
		SocketDescriptor sd;
		sd.port=1000;
		// This is so I can test multiple instances on the same computer
		while (SocketLayer::IsPortInUse(sd.port))
			sd.port++;
		rakPeer->Startup(32,0,&sd,1);
		rakPeer->SetMaximumIncomingConnections(32);

		RakNetDataProviderExternalInterfaces externalInterfaces;
		externalInterfaces.externallyAllocatedLobby2Client=OP_NEW<Lobby2Client>(__FILE__,__LINE__);
		externalInterfaces.externallyAllocatedLobby2MessageFactory=OP_NEW<Lobby2MessageFactory>(__FILE__,__LINE__);
		externalInterfaces.externallyAllocatedRoomsPlugin=OP_NEW<RoomsPlugin>(__FILE__,__LINE__);
		rakPeer->AttachPlugin(externalInterfaces.externallyAllocatedLobby2Client);
		rakPeer->AttachPlugin(externalInterfaces.externallyAllocatedRoomsPlugin);
		externalInterfaces.serverAddress.SetBinaryAddress("127.0.0.1");
		externalInterfaces.serverAddress.port=10000; // See Lobby2ServerSample_PGSQL.cpp
		externalInterfaces.titleName="Test Title Name"; // See Lobby2ClientSample.cpp
		externalInterfaces.titleSecretKey="Test secret key";
		externalInterfaces.wanServerPort=sd.port;
		SetExternalInterfaces(externalInterfaces);
	}
#endif
}
void RakNetDataProvider_PC::Shutdown(void)
{
	if (disconnectFromServerOnLogoff)
		rakPeer->CloseConnection(serverAddress,true);

#ifdef AUTOMANAGE_RAKNET_INSTANCES
	if (instancesWereAutocreated)
	{
		RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
		rakPeer=0;
		OP_DELETE(lobby2Client,__FILE__,__LINE__);
		lobby2Client=0;
		OP_DELETE(roomsPlugin,__FILE__,__LINE__);
		roomsPlugin=0;
	}
#endif
}
SInt RakNetDataProvider_PC::Run()
{
	// This is in a Scaleform provided thread!
	Startup();
	while (bAlive)
	{
#ifdef AUTOMANAGE_RAKNET_INSTANCES
		if (instancesWereAutocreated)
		{
			Packet *packet;
			for (packet = rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet = rakPeer->Receive())
			{
				switch (packet->data[0])
				{
				case ID_ALREADY_CONNECTED:
				case ID_CONNECTION_ATTEMPT_FAILED:
				case ID_NO_FREE_INCOMING_CONNECTIONS:
				case ID_RSA_PUBLIC_KEY_MISMATCH:
				case ID_CONNECTION_BANNED:
				case ID_INVALID_PASSWORD:
				case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				case ID_IP_RECENTLY_CONNECTED:
					OnFailedConnectionAttempt(packet->data[0], packet->systemAddress);
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
				case ID_NEW_INCOMING_CONNECTION:
					OnNewConnection(packet->data[0], packet->systemAddress);
					break;
				case ID_PONG:
					OnOfflinePong(packet);
					break;
				case ID_CONNECTION_LOST:
				case ID_DISCONNECTION_NOTIFICATION:
					OnDroppedConnection(packet);
					break;
				}
			}
		}
#endif

		Tick();
		MSleep(10); // Yield CPU
	};
	Shutdown();

	return 0;
}
void RakNetDataProvider_PC::Tick(void)
{
	ProcessCommands();

	// LAN updates expire after one second
	if (timeWhenWANUpdateFinished!=0 && RakNet::GetTimeMS()>timeWhenWANUpdateFinished)
	{
		// In one second, push this
		LDPResult* r = new LDPResult(LDPResult::R_ServerListUpdateCompleted);
		r->Network = LobbyServer::NET_LAN;
		QueueLowPriorityResult(r);

		timeWhenWANUpdateFinished=0;
	}
}
void RakNetDataProvider_PC::ProcessCommand(LDPCommand* cmd)
{
	switch (cmd->Type)
	{
	case LDPCommand::C_Login:  
		{
			RN_ConnectUser(cmd);
			break;
		}
	case LDPCommand::C_CreateAccount:
		{
			RN_ConnectNewUser(cmd);
			break;
		}
	case LDPCommand::C_Logout:
		{
			RN_Logout();
			break;
		}
	case LDPCommand::C_GetBuddyInfo:
		{
			RN_GetBuddyInfo(cmd);
			break;
		}
	case LDPCommand::C_SendChatMessage:
		{
			RN_SendMessage(cmd);
			break;
		}
	case LDPCommand::C_RemoveBuddy:
		{
			RN_RemoveBuddy(cmd);
			break;
		}
	case LDPCommand::C_AddBuddy:
		{
			RN_AddBuddy(cmd);
			break;
		}
	case LDPCommand::C_AcceptBuddyRequest:
		{
			RN_AcceptBuddyRequest(cmd);
			break;
		}
	case LDPCommand::C_DenyBuddyRequest:
		{
			RN_DenyBuddyRequest(cmd);
			break;
		}
	case LDPCommand::C_StopServerUpdating:
		{
			RN_StopUpdatingServerList(cmd);
			break;
		}
	case LDPCommand::C_PopulateServerList:
		{
			RN_PopulateServerList(cmd);
			break;
		}
	case LDPCommand::C_RefreshServer:
		{
			RN_RefreshServer(cmd);
			break;
		}
	default:
		{
			GFC_DEBUG_ASSERT1(0, "*****WARNING***** UNKNOWN COMMAND TYPE: %d", cmd->Type);
		}
	}
}
void RakNetDataProvider_PC::RN_ConnectUser(LDPCommand* cmd)
{
	// Used by SendLoginMessage
	loginUsername=cmd->UserName;
	loginPassword=cmd->Password;

	if (rakPeer->IsConnected(serverAddress,false,false)==false)
	{
		// Try connecting, and set appropriate state
		ConnectToServer(CONNECTING_THEN_LOGIN);
	}
	else
	{
		// Try logging in
		SendLoginMessage();
	}
}
void RakNetDataProvider_PC::ConnectToServer(ConnectionState newConnectionState)
{
	connectionState=newConnectionState;
	char serverIP[64];
	serverAddress.ToString(false,serverIP);
	rakPeer->Connect(serverIP,serverAddress.port,0,0);
}
void RakNetDataProvider_PC::SendLoginMessage(void)
{
	connectionState=LOGGING_IN;

	Client_Login *arg = (Client_Login *) msgFactory->Alloc(L2MID_Client_Login);
	arg->titleName=titleName;
	arg->titleSecretKey=titleSecretKey;
	arg->userName=loginUsername;
	arg->userPassword=loginPassword;


	// --------------------------------------------------------------------------------------
	// Make debugging easier
// 	arg->userName="user0";
// 	arg->userPassword="asdf";
	// --------------------------------------------------------------------------------------


	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::RN_ConnectNewUser(LDPCommand* cmd)
{
	loginUsername=cmd->UserName;
	loginPassword=cmd->Password;
	createAccountEmail=cmd->Email;

	// Scaleform bug: If server does not respond, user is stuck on the registration screen
	if (rakPeer->IsConnected(serverAddress,false,false)==false)
	{
		// Try connecting, and set appropriate state
		ConnectToServer(CONNECTING_THEN_CREATE_ACCOUNT);
	}
	else
	{
		// Try logging in
		SendRegisterAccountMessage();
	}
}
void RakNetDataProvider_PC::SendRegisterAccountMessage(void)
{
	RakNet::Client_RegisterAccount *arg = (RakNet::Client_RegisterAccount *) msgFactory->Alloc(L2MID_Client_RegisterAccount);
	arg->createAccountParameters.password=loginPassword;
	arg->createAccountParameters.emailAddress=createAccountEmail;
	arg->userName=loginUsername;
	arg->titleName=titleName;
	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::OnFailedConnectionAttempt(MessageID messageId, SystemAddress systemAddress)
{
	if (systemAddress==serverAddress)
	{
		connectionState=UNKNOWN;
		LDPResult* r = new LDPResult(LDPResult::R_LoginErrorReceived);
		r->Error.Type = LobbyError::ET_Connection;
		r->Error.Message = MessageIdToString(messageId);
		QueueHighPriorityResult(r);
	}
}
void RakNetDataProvider_PC::OnNewConnection(MessageID messageId, SystemAddress systemAddress)
{
	if (systemAddress==serverAddress)
	{
		if (connectionState==CONNECTING_THEN_LOGIN)
		{
			SendLoginMessage();
		}
		else
		{
			GASSERT(connectionState==CONNECTING_THEN_CREATE_ACCOUNT);
			SendRegisterAccountMessage();
		}
	}
}
void RakNetDataProvider_PC::AddServer(SystemAddress systemAddress, GPtr<LobbyServer> server)
{
	ServerMap.Add(systemAddress, server);
}
GPtr<LobbyServer> RakNetDataProvider_PC::GetServer(SystemAddress systemAddress)
{
	GPtr<LobbyServer> *ls = ServerMap.Get(systemAddress);
	if (ls)
		return *ls;
	else
		return 0;
}
GPtr<LobbyServer> RakNetDataProvider_PC::AllocOrGetServer(SystemAddress systemAddress, LDPResult **r, LobbyServer::NetworkType network)
{
	GPtr<LobbyServer> serverPtr = GetServer(systemAddress);
	if (serverPtr.GetPtr())
	{
		// If server already exists, push this
		*r = new LDPResult(LDPResult::R_ServerUpdated);
		(*r)->pServer = serverPtr;
	}
	else
	{
		// Spool a server added result if the server has basic keys
		// This makes sure that the server displayed in the UI will
		// have some valid data to display.
		*r = new LDPResult(LDPResult::R_ServerAdded);
		serverPtr = *new RakNetLobbyServer(network);
		(*r)->pServer = serverPtr;
		AddServer(systemAddress,serverPtr);
	}
	(*r)->Network=network;
	char ipAndPort[64];
	systemAddress.ToString(true,ipAndPort);
	serverPtr->SetIPPort(ipAndPort);
	return serverPtr;
}

void RakNetDataProvider_PC::ClearServers(void)
{
	ServerMap.Clear();
}
int RakNetDataProvider_PC::BuddyComp( RakString const &key, GPtr<LobbyBuddy> const &data )
{
	return strcmp(key.C_String(),data->GetName().ToCStr());
}
void RakNetDataProvider_PC::AddBuddy(RakNet::RakString buddyName, GPtr<LobbyBuddy> buddy)
{
	//BuddyMap.Add(RakString::ToInteger(buddyName), buddy);
	// BuddyMap.Add(GString(buddyName.C_String()), buddy);
	buddyList.Insert(buddyName,buddy,true,__FILE__,__LINE__);
}
GPtr<LobbyBuddy> RakNetDataProvider_PC::GetBuddy(RakNet::RakString buddyName)
{
	//GPtr<LobbyBuddy> *lb = BuddyMap.Get(RakString::ToInteger(buddyName));
// 	GPtr<LobbyBuddy> *lb = BuddyMap.Get(GString(buddyName.C_String()));
// 	if (lb)
// 		return *lb;
// 	else
// 		return 0;

	bool objectExists;
	unsigned int index = buddyList.GetIndexFromKey(buddyName,&objectExists);
	if (objectExists==false)
		return 0;
	return buddyList[index];
}
void RakNetDataProvider_PC::ClearBuddies(void)
{
	// BuddyMap.Clear();
	buddyList.Clear(false,__FILE__,__LINE__);
}
void RakNetDataProvider_PC::OnDroppedConnection(Packet *packet)
{
	if (packet->systemAddress==serverAddress)
	{
		// Scaleform bug: This doesn't kick you out of the server list screen back to login as it should
		LDPResult* r = new LDPResult(LDPResult::R_ServerErrorReceived);
		r->Error.Type = LobbyError::ET_Connection;
		r->Error.Message = "Connection to server lost";
		QueueHighPriorityResult(r);

		connectionState=UNKNOWN;
	}
}
void RakNetDataProvider_PC::OnOfflinePong(Packet *packet)
{
	if (packet->length<1+sizeof(RakNetTime))
		return;

#ifndef _DEBUG
	// Allow for testing
	// Don't show a pong response from my own system
 	if (packet->guid==rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
 		return;
#endif

	/// RakPeer - Pong from an unconnected system.  First byte is ID_PONG, second sizeof(RakNetTime) bytes is the ping, following bytes is system specific enumeration data.
	RakNet::BitStream bsIn(packet->data+1,packet->length-1,false);
	RakNetTime pingResponse;
	bsIn.Read(pingResponse);
	RakNetTime elapsedPing=RakNet::GetTime()-pingResponse;
	RakNetTimeMS elapsedPingMs;
#if __GET_TIME_64BIT==1
	elapsedPingMs=elapsedPing/1000;
#else
	elapsedPingMs=elapsedPing;
#endif

	// This was set in RakPeerInterface::SetOfflinePingResponse, by using ServerWANUpdate::SetAsOfflinePingResponse()
	ServerAndRoomBrowserData browserData;
	browserData.Serialize(&bsIn,false);

	LDPResult* r;
	GPtr<LobbyServer> serverPtr = AllocOrGetServer(packet->systemAddress, &r, LobbyServer::NET_LAN);

	// Apply to smart pointer
	browserData.WriteToLobbyServer(serverPtr);
	serverPtr->SetPing((UPInt) elapsedPingMs);

	if (r->Type==LDPResult::R_ServerUpdated)
		QueueHighPriorityResult(r);
	else
		QueueLowPriorityResult(r);
}
const char* RakNetDataProvider_PC::MessageIdToString(MessageID messageId) const
{
	switch (messageId)
	{
	case ID_CONNECTION_ATTEMPT_FAILED:
		return "Failed to connect to server.";
	case ID_ALREADY_CONNECTED:
		return "Already connected. Try again in 10 seconds.";
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		return "Server is full.";
	case ID_RSA_PUBLIC_KEY_MISMATCH:
		return "Login security check failed.";
	case ID_CONNECTION_BANNED:
		return "Banned from server.";
	case ID_INVALID_PASSWORD:
		return "Bad login password.";
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		return "Incorrect protocol version.";
	case ID_IP_RECENTLY_CONNECTED:
		return "Too many recent connections from this IP address. Try again in 1 second.";
	default:
		return "Unknown";
	}
}

void RakNetDataProvider_PC::MessageResult(Client_Login *message)
{
	if (message->resultCode==L2RC_SUCCESS)
	{
		connectionState=LOGGED_IN;

#ifdef AUTOMANAGE_RAKNET_INSTANCES
		// Setup this program to reply to other instances of the program
		// You want this if this is the host of a peer to peer session, or a server
		// You can automatically determine the host of a peer to peer session with the FullyConnectedMesh2 plugin.
		// See void OfflineServer::Initialize()
		ServerAndRoomBrowserData browserData;
		browserData.gameName=RakNet::RakString("Test Title Name") + loginUsername;
		browserData.maxPlayers=32;
		browserData.mapName="mapName";
		browserData.gameMode="gameMode";
		browserData.serverHasAPassword="serverHasAPassword";
		browserData.autoBalanced="ON";
		browserData.gameVersion="gameVersion";
		browserData.mapSize="mapSize";
		browserData.friendlyFire="OFF";

		PlayerBrowserData myself;
		myself.name=message->userName;
		myself.team=0;
		myself.score=0;
		myself.deaths=0;
		myself.ping=0;
		browserData.players.Push(myself,__FILE__,__LINE__);

		browserData.SetAsOfflinePingResponse(rakPeer);

		/// Create a room for the WAN browser
		RakNet::CreateRoom_Func func;
		func.networkedRoomCreationParameters.slots.publicSlots=browserData.maxPlayers;
		func.networkedRoomCreationParameters.slots.reservedSlots=0;
		func.networkedRoomCreationParameters.slots.spectatorSlots=0;
		func.networkedRoomCreationParameters.hiddenFromSearches=false;
		func.networkedRoomCreationParameters.destroyOnModeratorLeave=false;
		func.networkedRoomCreationParameters.autoLockReadyStatus=true;
		func.networkedRoomCreationParameters.inviteToRoomPermission=NetworkedRoomCreationParameters::INVITE_MODE_ANYONE_CAN_INVITE;
		func.networkedRoomCreationParameters.inviteToSpectatorSlotPermission=NetworkedRoomCreationParameters::INVITE_MODE_ANYONE_CAN_INVITE;
		func.networkedRoomCreationParameters.clearInvitesOnNewModerator=true;
		func.networkedRoomCreationParameters.roomName=browserData.gameName;
		func.gameIdentifier=titleName;
		browserData.WriteToTable(&func.initialRoomProperties);
		func.userName=loginUsername;
		roomsPlugin->ExecuteFunc(&func);
		
#endif

		RakNet::Client_SetPresence *arg = (RakNet::Client_SetPresence *) msgFactory->Alloc(L2MID_Client_SetPresence);
		arg->presence.isVisible=true;
		arg->presence.titleName=titleName;
		arg->presence.status=Lobby2Presence::IN_LOBBY;
		lobby2Client->SendMsgAndDealloc(arg);

		LDPResult* r = new LDPResult(LDPResult::R_UserLoggedIn);
		r->UserName = message->userName.C_String();
		QueueHighPriorityResult(r);
	}
	else
	{
		connectionState=NOT_LOGGED_IN;

		LDPResult* r = new LDPResult(LDPResult::R_LoginErrorReceived);
		if (message->resultCode==L2RC_Client_Login_HANDLE_NOT_IN_USE_OR_BAD_SECRET_KEY)
			r->Error.Type = LobbyError::ET_LoginBadPass;
		else
			r->Error.Type = LobbyError::ET_Login;

		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
	}
}
void RakNetDataProvider_PC::MessageResult(Client_RegisterAccount *message)
{
	if (message->resultCode==L2RC_SUCCESS)
	{
		connectionState=NOT_LOGGED_IN;

		// Account created, now log in
		loginUsername=message->userName;
		loginPassword=message->createAccountParameters.password;
		SendLoginMessage();
	}
	else
	{
		LDPResult* r = new LDPResult(LDPResult::R_LoginErrorReceived);
		switch (message->resultCode)
		{
		case L2RC_Client_RegisterAccount_HANDLE_ALREADY_IN_USE:
			r->Error.Type = LobbyError::ET_NewLoginBadNick;
			break;
		case L2RC_PASSWORD_IS_EMPTY:
		case L2RC_PASSWORD_IS_TOO_SHORT:
		case L2RC_PASSWORD_IS_TOO_LONG:
			r->Error.Type = LobbyError::ET_NewLoginBadPass;
			break;
		case L2RC_EMAIL_ADDRESS_IS_EMPTY:
		case L2RC_EMAIL_ADDRESS_IS_INVALID:
			r->Error.Type = LobbyError::ET_NewLoginBadEmail;
			break;
		default:
			r->Error.Type = LobbyError::ET_NewLogin;
			break;
		}

		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
	}
}
void RakNetDataProvider_PC::RN_Logout()
{
	RakNet::Client_Logoff *arg = (RakNet::Client_Logoff *) msgFactory->Alloc(L2MID_Client_Logoff);
	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::RN_GetBuddyInfo(LDPCommand* cmd)
{
}
void RakNetDataProvider_PC::RN_SendMessage(LDPCommand* cmd)
{

}
void RakNetDataProvider_PC::RN_AddBuddy(LDPCommand* cmd)
{
	RakNet::Friends_SendInvite *arg = (RakNet::Friends_SendInvite *) msgFactory->Alloc(L2MID_Friends_SendInvite);
	arg->targetHandle=cmd->UserName;
	arg->body=cmd->Message;
	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::RN_RemoveBuddy(LDPCommand* cmd)
{
	RakNet::Friends_Remove *arg = (RakNet::Friends_Remove *) msgFactory->Alloc(L2MID_Friends_Remove);
	arg->targetHandle=cmd->UserName;
	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::RN_AcceptBuddyRequest(LDPCommand* cmd)
{
	RakNet::Friends_AcceptInvite *arg = (RakNet::Friends_AcceptInvite *) msgFactory->Alloc(L2MID_Friends_AcceptInvite);
	arg->targetHandle=cmd->UserName;
	lobby2Client->SendMsgAndDealloc(arg);
}
void RakNetDataProvider_PC::RN_DenyBuddyRequest(LDPCommand* cmd)
{
	RakNet::Friends_RejectInvite *arg = (RakNet::Friends_RejectInvite *) msgFactory->Alloc(L2MID_Friends_RejectInvite);
	arg->targetHandle=cmd->UserName;
	lobby2Client->SendMsgAndDealloc(arg);
}

void RakNetDataProvider_PC::MessageResult(RakNet::Friends_SendInvite *message)
{
	if (message->resultCode!=L2RC_SUCCESS)
	{
		LDPResult* r = new LDPResult(LDPResult::R_BuddyErrorReceived);
		r->Error.Type = LobbyError::ET_BuddyAdd;
		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
		GFC_DEBUG_MESSAGE1(1, "Friends_SendInvite returned with error! %s", 
			r->Error.Message.ToCStr());

		return;
	}

}
void RakNetDataProvider_PC::MessageResult(RakNet::Friends_AcceptInvite *message)
{
	if (message->resultCode!=L2RC_SUCCESS)
	{
		LDPResult* r = new LDPResult(LDPResult::R_BuddyErrorReceived);
		r->Error.Type = LobbyError::ET_BuddyAuth;
		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
		GFC_DEBUG_MESSAGE1(1, "Friends_AcceptInvite returned with error! %s", 
			r->Error.Message.ToCStr());

		return;
	}

	// Add to buddy list. The target will get Notification_Friends_StatusChange
	PushBuddyOnlineStatus(message->targetHandle,message->presence.status!=Lobby2Presence::NOT_ONLINE,message->presence);
}
void RakNetDataProvider_PC::MessageResult(RakNet::Friends_RejectInvite *message)
{
	if (message->resultCode!=L2RC_SUCCESS)
	{
		LDPResult* r = new LDPResult(LDPResult::R_BuddyErrorReceived);
		r->Error.Type = LobbyError::ET_BuddyDeny;
		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
		GFC_DEBUG_MESSAGE1(1, "Friends_RejectInvite returned with error! %s", 
			r->Error.Message.ToCStr());

		return;
	}

}
void RakNetDataProvider_PC::MessageResult(RakNet::Friends_Remove *message)
{
	if (message->resultCode!=L2RC_SUCCESS)
	{
		LDPResult* r = new LDPResult(LDPResult::R_BuddyErrorReceived);
		r->Error.Type = LobbyError::ET_BuddyRemove;
		RakNet::RakString errorStr;
		message->DebugMsg(errorStr);
		// Scaleform bug: If the error string has a space, it gets truncated
		r->Error.Message = errorStr.C_String();
		QueueHighPriorityResult(r);
		GFC_DEBUG_MESSAGE1(1, "Friends_Remove returned with error! %s", 
			r->Error.Message.ToCStr());

		return;
	}

	// Remove from buddy list. The target will get Notification_Friends_StatusChange
	DeleteBuddy(message->targetHandle);
}
void RakNetDataProvider_PC::RN_PopulateServerList(LDPCommand* cmd)
{
	if (cmd->Network==LobbyServer::NET_LAN)
	{
		// Ping out looking for game servers. We can't find servers unless they all use the same port
		// Up to 4 instances is reasonable
		for (int i=0; i < 4; i++)
		{
			rakPeer->Ping("255.255.255.255", lanServerPort+i, false);
		}

		// Allow updates for 500 milliseconds, which should be plenty of time for a WAN to respond
		timeWhenWANUpdateFinished=RakNet::GetTimeMS()+500;
	}
	else
	{
		// Ask rooms plugin for rooms
		// DataStructures::Table::FilterQuery RoomQuery::fq[32];
		// DataStructures::Table::Cell RoomQuery::cells[32];
		SearchByFilter_Func func;
		func.roomQuery.queries=0;
		func.roomQuery.numQueries=0;
		func.onlyJoinable=false;
		func.gameIdentifier=titleName;
		func.userName=loginUsername;
		roomsPlugin->ExecuteFunc(&func);
	}

	LDPResult* r = new LDPResult(LDPResult::R_ServerListUpdateStarted);
	r->Network = cmd->Network;
	QueueHighPriorityResult(r);

}
void RakNetDataProvider_PC::OnBuddyPanelStateChange(bool buddyPanelIsAvailable)
{
	if (buddyPanelIsAvailable)
	{
		// Scaleform bug: >>>> CANNOT NOTIFY SINCE BUDDY PANEL IS INACTIVE! is returned if I update the buddy list after logging in
		// I added this callback so I only update the list when it can use the data
		RakNet::Friends_GetFriends *arg = (RakNet::Friends_GetFriends *) msgFactory->Alloc(L2MID_Friends_GetFriends);
		lobby2Client->SendMsgAndDealloc(arg);
		RakNet::Friends_GetInvites *arg2 = (RakNet::Friends_GetInvites *) msgFactory->Alloc(L2MID_Friends_GetInvites);
		lobby2Client->SendMsgAndDealloc(arg2);	
	}
}
void RakNetDataProvider_PC::RN_RefreshServer(LDPCommand* cmd)
{
	RakNetLobbyServer* pgs = (RakNetLobbyServer*)cmd->pServer.GetPtr();
	if (pgs->GetNetwork()==LobbyServer::NET_LAN)
	{
		SystemAddress sa;
		sa.SetBinaryAddress(pgs->GetIPPort());
		char dest[64];
		sa.ToString(false,dest);
		rakPeer->Ping(dest, sa.port, false);
	}
	else
	{
		GetRoomProperties_Func func;
		func.roomName=pgs->GetName();
		func.userName=loginUsername;
		roomsPlugin->ExecuteFunc(&func);
	}
}
void RakNetDataProvider_PC::RN_StopUpdatingServerList(LDPCommand* cmd)
{
	if (cmd->bClearServers)
	{
		ClearServers();
		// notify UI to clear the server list manager
		LDPResult* r = new LDPResult(LDPResult::R_ServerListCleared);
		r->Network = cmd->Network;
		QueueHighPriorityResult(r);
	}

	LDPResult* r = new LDPResult(LDPResult::R_ServerListUpdateStopped);
	r->Network = cmd->Network;
	QueueHighPriorityResult(r);
}
void RakNetDataProvider_PC::MessageResult(Client_Logoff *message)
{
	LDPResult* r = new LDPResult(LDPResult::R_UserLoggedOut);
	QueueHighPriorityResult(r);

	if (disconnectFromServerOnLogoff)
	{
		rakPeer->CloseConnection(serverAddress,true);
	}

	ClearBuddies();

#ifdef AUTOMANAGE_RAKNET_INSTANCES
	// Not logged in, so don't respond to pings
	rakPeer->SetOfflinePingResponse(0,0);
#endif
}
void RakNetDataProvider_PC::DeleteBuddy(RakNet::RakString buddyName)
{
	LDPResult* r = new LDPResult(LDPResult::R_BuddyRemoved);
	GPtr<LobbyBuddy> buddy = GetBuddy(buddyName);
	if (buddy.GetPtr())
		r->pBuddy = buddy;
	QueueHighPriorityResult(r);
}
void RakNetDataProvider_PC::PushBuddyOnlineStatus(RakNet::RakString buddyName, bool isOnline, const RakNet::Lobby2Presence &presence)
{
	LDPResult* r;
	GPtr<LobbyBuddy> buddy = GetBuddy(buddyName);
	if (! buddy.GetPtr())
	{
		buddy = *new LobbyBuddy(RakString::ToInteger(buddyName), isOnline ? LobbyBuddy::ST_Online : LobbyBuddy::ST_Offline);
		AddBuddy(buddyName,buddy);
		buddy->SetName(buddyName.C_String());
		r = new LDPResult(LDPResult::R_BuddyArrived);
	}
	else
	{
		buddy->SetStatus( isOnline ? LobbyBuddy::ST_Online : LobbyBuddy::ST_Offline );
		r = new LDPResult(LDPResult::R_BuddyStatusChanged);
	}

	SetBuddyPresence(buddy.GetPtr(), presence);

	r->pBuddy = buddy;
	QueueHighPriorityResult(r);

}
void RakNetDataProvider_PC::MessageResult(RakNet::Friends_GetFriends *message)
{
	unsigned int i;
	for (i=0; i < message->myFriends.Size(); i++)
	{
		PushBuddyOnlineStatus(message->myFriends[i].usernameAndStatus.handle, message->myFriends[i].usernameAndStatus.isOnline, message->myFriends[i].usernameAndStatus.presence);
	}
}
void RakNetDataProvider_PC::MessageResult(RakNet::Friends_GetInvites *message)
{
	unsigned int i;
	for (i=0; i < message->invitesReceived.Size(); i++)
	{
		LDPResult* r = new LDPResult(LDPResult::R_BuddyRequestReceived);
		r->ProfileID = RakString::ToInteger(message->invitesReceived[i].usernameAndStatus.handle);
		r->UserName = message->invitesReceived[i].usernameAndStatus.handle;
		r->Message = "Buddy invite sent while offline.";
		QueueLowPriorityResult(r);
	}
}
void RakNetDataProvider_PC::MessageResult(RakNet::Notification_Friends_StatusChange *message)
{
	switch (message->op)
	{
		case Notification_Friends_StatusChange::FRIEND_LOGGED_IN:
		{
			PushBuddyOnlineStatus(message->otherHandle,true,message->presence);
		}
		break;
		case Notification_Friends_StatusChange::FRIEND_LOGGED_OFF:
		{
			PushBuddyOnlineStatus(message->otherHandle,false,message->presence);
		}
		break;
		case Notification_Friends_StatusChange::FRIEND_ACCOUNT_WAS_DELETED:
		case Notification_Friends_StatusChange::YOU_WERE_REMOVED_AS_A_FRIEND:
		{
			DeleteBuddy(message->otherHandle);
		}
		break;
		case Notification_Friends_StatusChange::GOT_INVITATION_TO_BE_FRIENDS:
		{
			LDPResult* r = new LDPResult(LDPResult::R_BuddyRequestReceived);
			r->ProfileID = RakString::ToInteger(message->otherHandle);
			r->UserName = message->otherHandle;
			r->Message = message->body;
			QueueHighPriorityResult(r);
		}
		break;
	}
}
void RakNetDataProvider_PC::MessageResult(RakNet::Notification_Friends_PresenceUpdate *message)
{
	GPtr<LobbyBuddy> buddy = GetBuddy(message->otherHandle.C_String());
	SetBuddyPresence(buddy.GetPtr(), message->newPresence);

	LDPResult* r = new LDPResult(LDPResult::R_BuddyStatusChanged);
	r->pBuddy = buddy;
	QueueHighPriorityResult(r);
}
void RakNetDataProvider_PC::SetBuddyPresence(LobbyBuddy* buddy, const RakNet::Lobby2Presence &presence)
{
	if (buddy)
	{
		if (presence.status==Lobby2Presence::UNDEFINED)
			return;

		if (presence.titleName!=titleName ||
			presence.isVisible==false)
		{
			buddy->SetStatus(LobbyBuddy::ST_Offline);
			return;
		}

		if (presence.status==Lobby2Presence::IN_GAME)
		{
			buddy->SetStatus(LobbyBuddy::ST_Playing);
		}
		else if (presence.status==Lobby2Presence::AWAY)
		{
			buddy->SetStatus(LobbyBuddy::ST_Away);
		}
		else
		{
			buddy->SetStatus(LobbyBuddy::ST_Online);
		}
	}
}

void RakNetDataProvider_PC::RemoveBuddy(const LobbyBuddy* pbuddy)
{
	GFC_DEBUG_MESSAGE1(1, "LobbyDataProvider::RemoveBuddy(%s)", pbuddy->GetName().ToCStr());

	LDPCommand* c = new LDPCommand(LDPCommand::C_RemoveBuddy);
	c->ProfileID = pbuddy->GetProfileID();
	// The base class does not give you the name
	c->UserName = pbuddy->GetName();
	QueueCommand(c);
}
void RakNetDataProvider_PC::WriteTableToLobbyServer( DataStructures::Table *table, GPtr<LobbyServer> lobbyServer)
{
	DataStructures::Table::Row *row = table->GetRowByIndex(0,0);
	lobbyServer->SetName(row->cells[table->ColumnIndex("gameName")]->c);
	lobbyServer->SetMapName(row->cells[table->ColumnIndex("mapName")]->c);
	lobbyServer->SetGameMode(row->cells[table->ColumnIndex("gameMode")]->c);
	lobbyServer->SetLocked(row->cells[table->ColumnIndex("serverHasAPassword")]->i!=0.0);
	lobbyServer->SetAutoBalanced(row->cells[table->ColumnIndex("autoBalanced")]->c);
	lobbyServer->SetGameVersion(row->cells[table->ColumnIndex("gameVersion")]->c);
	lobbyServer->SetMapSize(row->cells[table->ColumnIndex("mapSize")]->c);
	lobbyServer->SetFriendlyFire(row->cells[table->ColumnIndex("friendlyFire")]->c);
}
void RakNetDataProvider_PC::WriteRoomCreationParametersToLobbyServer( CreateRoom_Func *callResult, GPtr<LobbyServer> lobbyServer)
{
	lobbyServer->SetNumPlayers(1);
	DataStructures::Table::Row *row =callResult->initialRoomProperties.GetRowByIndex(0,0);
	lobbyServer->SetMaxPlayers(row->cells[callResult->initialRoomProperties.ColumnIndex("maxPlayers")]->i);
	char temp[128];
	G_sprintf(temp, sizeof(temp), "%d/%d", 1, lobbyServer->GetMaxPlayers());
	lobbyServer->SetPlayers(temp);

	WriteTableToLobbyServer(&callResult->initialRoomProperties, lobbyServer);
}
void RakNetDataProvider_PC::WriteRoomDescriptorToLobbyServer( RakNet::RoomDescriptor *roomDescriptor, GPtr<LobbyServer> lobbyServer)
{
	lobbyServer->SetNumPlayers(roomDescriptor->roomMemberList.Size());
	DataStructures::Table::Row *row = roomDescriptor->roomProperties.GetRowByIndex(0,0);
	lobbyServer->SetMaxPlayers(row->cells[roomDescriptor->roomProperties.ColumnIndex("maxPlayers")]->i);
	char temp[128];
	G_sprintf(temp, sizeof(temp), "%d/%d", 1, lobbyServer->GetMaxPlayers());
	lobbyServer->SetPlayers(temp);

	RakNetLobbyServer *rnls = (RakNetLobbyServer*) lobbyServer.GetPtr();
	rnls->ClearLobbyPlayers();
	for (int i=0; i < roomDescriptor->roomMemberList.Size(); i++)
	{
		// TODO - for properties other than name, I'd actually have to connect to and query the server
		PlayerBrowserData pld;
		pld.name=roomDescriptor->roomMemberList[i].name;
		pld.team=9999;
		pld.score=9999;
		pld.deaths=9999;
		pld.ping=9999;
		rnls->AddLobbyPlayer(pld);
	}
	WriteTableToLobbyServer(&roomDescriptor->roomProperties, lobbyServer);
}
void RakNetDataProvider_PC::CreateRoom_Callback( SystemAddress senderAddress, CreateRoom_Func *callResult)
{
	if (senderAddress!=serverAddress)
		return;

	if (callResult->resultCode==REC_SUCCESS)
	{
// 		LDPResult* r1 = new LDPResult(LDPResult::R_ServerListUpdateStarted);
// 		r1->Network = LobbyServer::NET_WAN;
// 		QueueHighPriorityResult(r1);

		LDPResult* r2;
		GPtr<LobbyServer> serverPtr = AllocOrGetServer(callResult->roomDescriptor.roomMemberList[0].systemAddress, &r2, LobbyServer::NET_WAN);
		WriteRoomCreationParametersToLobbyServer(callResult, serverPtr);

		// 0 ping to myself
		serverPtr->SetPing(0);
		
		QueueHighPriorityResult(r2);

// 		LDPResult* r3 = new LDPResult(LDPResult::R_ServerListUpdateCompleted);
// 		r3->Network = LobbyServer::NET_WAN;
// 		QueueHighPriorityResult(r3);
	}
	else
	{
		// TODO - failure codes. Need expanded ErrorType enums
		// REC_CREATE_ROOM_UNKNOWN_TITLE,
		// REC_CREATE_ROOM_CURRENTLY_IN_QUICK_JOIN,
		// REC_CREATE_ROOM_CURRENTLY_IN_A_ROOM,
		// 	REC_ROOM_CREATION_PARAMETERS_EMPTY_ROOM_NAME;
		// 	REC_ROOM_CREATION_PARAMETERS_ROOM_NAME_HAS_PROFANITY;
		// 	REC_ROOM_CREATION_PARAMETERS_ROOM_NAME_IN_USE;
		// 	REC_ROOM_CREATION_PARAMETERS_NO_PLAYABLE_SLOTS;
		// 	REC_ROOM_CREATION_PARAMETERS_RESERVED_QUICK_JOIN_ROOM_NAME;
		// REC_SET_CUSTOM_ROOM_PROPERTIES_CONTAINS_DEFAULT_COLUMNS

	}

}
void RakNetDataProvider_PC::SearchByFilter_Callback( SystemAddress senderAddress, SearchByFilter_Func *callResult)
{
	if (senderAddress!=serverAddress)
		return;

	unsigned int i;
	for (i=0; i < callResult->roomsOutput.Size(); i++)
	{
		LDPResult* r2;
		GPtr<LobbyServer> serverPtr = AllocOrGetServer(callResult->roomsOutput[i]->roomMemberList[0].systemAddress, &r2, LobbyServer::NET_WAN);
		WriteRoomDescriptorToLobbyServer(callResult->roomsOutput[i], serverPtr);
		serverPtr->SetPing(999);

		// Ping the room owner. If no response, the ping will just stay at 999
		char str[64];
		callResult->roomsOutput[i]->roomMemberList[0].systemAddress.ToString(false,str);
		rakPeer->Ping((const char*) str, callResult->roomsOutput[i]->roomMemberList[0].systemAddress.port, false);

		QueueHighPriorityResult(r2);		
	}
}
void RakNetDataProvider_PC::GetRoomProperties_Callback( SystemAddress senderAddress, GetRoomProperties_Func *callResult)
{
	if (senderAddress!=serverAddress)
		return;

	LDPResult* r2;
	GPtr<LobbyServer> serverPtr = AllocOrGetServer(callResult->roomDescriptor.roomMemberList[0].systemAddress, &r2, LobbyServer::NET_WAN);
	WriteRoomDescriptorToLobbyServer(&callResult->roomDescriptor, serverPtr);
	QueueHighPriorityResult(r2);	
}
