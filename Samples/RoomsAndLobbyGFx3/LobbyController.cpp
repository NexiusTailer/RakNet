/**********************************************************************

Filename    :   LobbyDataProvider.cpp
Content     :   The Lobby application controller
Created     :   7/31/2009
Authors     :   Prasad Silva
Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

// WindowsIncludes must be first to avoid
// 1>c:\program files (x86)\microsoft visual studio 8\vc\platformsdk\include\winsock2.h(112) : error C2011: 'fd_set' : 'struct' type redefinition
// 1>        c:\program files (x86)\microsoft visual studio 8\vc\platformsdk\include\winsock.h(54) : see declaration of 'fd_set'
// 1>c:\program files (x86)\microsoft visual studio 8\vc\platformsdk\include\winsock2.h(147) : warning C4005: 'FD_SET' : macro redefinition
// 1>        c:\program files (x86)\microsoft visual studio 8\vc\platformsdk\include\winsock.h(88) : see previous definition of 'FD_SET'
#include "WindowsIncludes.h"
#ifdef _WIN32
#include "RakNetDataProvider_PC.h"
#else
#include "RakNetDataProvider_PS3.h"
#endif
#include "LobbyController.h"
#include "Offline\OfflineDataProvider.h"


LobbyController::LobbyController(GFxMovieView* pmovie, 
                           FxDelegate* pdg, 
                           LobbyLocalizer* ploc) :
pMovie(pmovie), pDelegate(pdg), pLocalizer(ploc),
SelectedBuddyIndex(-1), bBuddyPanelActive(false), bServerBrowserPanelActive(false),
ActiveServerListNetwork(LobbyServer::NET_WAN), SelectedServerIndex(-1), 
TopServerIndex(-1)
{
    pProfileMgr = *new LobbyProfileManager("profiles.xml", pMovie->GetFileOpener());
    
#ifndef LOBBY_NO_ONLINE
    bOfflineMode = false;
#else
    bOfflineMode = true;
#endif

    BootDataProvider();

#ifdef TELNET_DEBUGGING
	lobbyControllerCommandParser = new LobbyControllerCommandParser(this);

	lobbyControllerCommandParser->RegisterCommand(3,"c2f_ShowProgress","bool showCancel, bool showOK, GString dialogId");
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_HideProgress","GString dialogId");
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_LoadSWF","GString swfId");
	lobbyControllerCommandParser->RegisterCommand(2,"c2f_Parties_AddOrUpdateBuddy","GString name, GString presenceString");
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_Parties_RemoveBuddy","GString name");
	lobbyControllerCommandParser->RegisterCommand(2,"c2f_Parties_AddOrUpdatePartyMember","GString name, GString connectionStatus");
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_Parties_RemovePartyMember","GString name");
	lobbyControllerCommandParser->RegisterCommand(CommandParserInterface::VARIABLE_NUMBER_OF_PARAMETERS,
		"c2f_Rooms_Show",
		"GString roomName, bool teamsAreLocked, bool showGameStartCountdown, int gameStartCountdownInSeconds, int numSlots\r\n"
		"(For each slot in numSlots)\r\n"
		"GString slotStatus (open, private, spectator, player)\r\n"
		"(If player, then the following parameters also apply)\r\n"
		"GString playerName, GString playerConnectionStatus, int playerCurrentTeamNumber, int playerDesiredTeamNumber, bool playerIsReady, bool playerIsTalking, int playerPing\r\n"
		);
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_Rooms_UpdateTeamsAreLocked","bool isLocked");
	lobbyControllerCommandParser->RegisterCommand(CommandParserInterface::VARIABLE_NUMBER_OF_PARAMETERS,"c2f_Rooms_UpdateSlots",
		"int numSlots\r\n"
		"(For each slot in numSlots)\r\n"
		"GString slotStatus (open, private, spectator, player)\r\n"
		"(If player, then the following parameters also apply)\r\n"
		"GString playerName, GString playerConnectionStatus, int playerCurrentTeamNumber, int playerDesiredTeamNumber, bool playerIsReady, bool playerIsTalking, int playerPing\r\n"
		);
	lobbyControllerCommandParser->RegisterCommand(7,"c2f_Rooms_Update_Player",
		"GString playerName, GString playerConnectionStatus, int playerCurrentTeamNumber, int playerDesiredTeamNumber, bool playerIsReady, bool playerIsTalking, int playerPing\r\n"
		);
	lobbyControllerCommandParser->RegisterCommand(0,"c2f_Rooms_ShowGameStartCountdown","");
	lobbyControllerCommandParser->RegisterCommand(0,"c2f_Rooms_HideGameStartCountdown","");
	lobbyControllerCommandParser->RegisterCommand(1,"c2f_Rooms_UpdateGameStartCountdown","gameStartCountdownInSeconds");
	lobbyControllerCommandParser->RegisterCommand(0,"c2f_Rooms_OnGameStarted","");
	lobbyControllerCommandParser->RegisterCommand(2,"c2f_Rooms_ShowChatMessage","GString senderName, GString message");

	consoleServer = new ConsoleServer;
	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor sd(23,0);
	rakPeer->Startup(1,10,&sd,1);
	rakPeer->SetMaximumIncomingConnections(1);
	raknetTransport = new RakNetTransport2;
	rakPeer->AttachPlugin(raknetTransport);
	raknetTransport->Start(23,true);
	consoleServer->AddCommandParser(lobbyControllerCommandParser);
	consoleServer->SetTransportProvider(raknetTransport, 23);
	consoleServer->SetPrompt("> "); // Show this character when waiting for user input
#endif
}


LobbyController::~LobbyController()
{
#ifdef TELNET_DEBUGGING
	delete lobbyControllerCommandParser;
	delete consoleServer;
	delete raknetTransport;
#endif

    if (pDataProvider)
    {
        pDataProvider->Stop();
        pDataProvider->Wait();
    }
}


//////////////////////////////////////////////////////////////////////////
// Initialize and startup the dataprovider
//
void LobbyController::BootDataProvider()
{
    if (pDataProvider) 
    {
        pDataProvider->Stop();
        // Wait for thread to finish
        pDataProvider->Wait();
    }
#ifndef LOBBY_NO_ONLINE
    if (bOfflineMode)
	{
        pDataProvider = *new OfflineDataProvider();
	}
    else 
	{
#ifdef _WIN32
        pDataProvider = *new RakNetDataProvider_PC();
#else
		pDataProvider = *new RakNetDataProvider_PS3();
#endif
	}
#else
    pDataProvider = *new OfflineDataProvider();
#endif
    pDataProvider->AddResultListener(this);
    pDataProvider->Start();

}

//////////////////////////////////////////////////////////////////////////
// Clear all local server/buddy data
//
void LobbyController::ClearData()
{
    BuddyList.Clear();
    SelectedBuddyIndex = -1;
    PendingBuddyRequests.Clear();

    ClearServerStorage();

    ServerFilter.Clear();
}


//////////////////////////////////////////////////////////////////////////
// Process pending tasks that may be queued up and require synchronous 
// execution with the main thread
//
void LobbyController::ProcessTasks()
{
    pDataProvider->ExecuteSynchronousTasks();

#ifdef TELNET_DEBUGGING
	Packet *packet;
	for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
	{
	}
	consoleServer->Update();
#endif
}
//////////////////////////////////////////////////////////////////////////
// FxDelegate callback installer. This method registers all the callbacks
// for the user interface.
//
void LobbyController::Accept(FxDelegateHandler::CallbackProcessor* cp)
{
    cp->Process("Profiles.requestLength", LobbyController::OnProfilesListRequestLength);    
    cp->Process("Profiles.requestItemAt", LobbyController::OnProfilesListRequestItemAt);
    cp->Process("Profiles.requestItemRange", LobbyController::OnProfilesListRequestItemRange);

    cp->Process("Buddies.requestLength", LobbyController::OnBuddyListRequestLength);
    cp->Process("Buddies.requestItemAt", LobbyController::OnBuddyListRequestItemAt);
    cp->Process("Buddies.requestItemRange", LobbyController::OnBuddyListRequestItemRange);

    cp->Process("Servers.requestLength", LobbyController::OnServerListRequestLength);
    cp->Process("Servers.requestItemRange", LobbyController::OnServerListRequestItemRange);

    cp->Process("Players.requestLength", LobbyController::OnPlayersListRequestLength);
    cp->Process("Players.requestItemAt", LobbyController::OnPlayersListRequestItemAt);
    cp->Process("Players.requestItemRange", LobbyController::OnPlayersListRequestItemRange);

    cp->Process("login", LobbyController::OnLogin);
    cp->Process("logout", LobbyController::OnLogout);
    cp->Process("createProfile", LobbyController::OnCreateProfile);
    cp->Process("deleteProfile", LobbyController::OnDeleteProfile);
    cp->Process("getRemLastAccount", LobbyController::OnGetRemLastAccount);
    cp->Process("setRemLastAccount", LobbyController::OnSetRemLastAccount);

    cp->Process("buddyPanelActive", LobbyController::OnBuddyPanelStateChange);    
    cp->Process("addBuddy", LobbyController::OnAddBuddy);
    cp->Process("removeSelectedBuddy", LobbyController::OnRemoveSelectedBuddy);
    cp->Process("getBuddyRequestsCount", LobbyController::OnGetBuddyRequestsCount);
    cp->Process("processBuddyRequest", LobbyController::OnProcessBuddyRequest);
    cp->Process("acceptBuddyRequest", LobbyController::OnAcceptBuddyRequest);
    cp->Process("selectChatLog", LobbyController::OnSelectChatLog);
    cp->Process("submitChat", LobbyController::OnSubmitChat);

    cp->Process("serverBrowserPanelActive", LobbyController::OnServerBrowserPanelStateChange);
    cp->Process("populateServerList", LobbyController::OnPopulateServers);
    cp->Process("pollServerList", LobbyController::OnPollServerList);
    cp->Process("sortServerList", LobbyController::OnSortServerList);
    cp->Process("refreshServer", LobbyController::OnRefreshServer);
    cp->Process("spectateServer", LobbyController::OnSpectateServer);
    cp->Process("joinServer", LobbyController::OnJoinServer);
    cp->Process("connect", LobbyController::OnConnect);
    cp->Process("getFilter", LobbyController::OnGetServerFilter);
    cp->Process("setFilter", LobbyController::OnSetServerFilter);
    cp->Process("setInitialServerSort", LobbyController::OnSetInitialServerSort);
    cp->Process("serversScrolled", LobbyController::OnServersScrolled);

    cp->Process("sortPlayerLists", LobbyController::OnSortPlayerLists);
    cp->Process("setInitialPlayersSort", LobbyController::OnSetInitialPlayersSort);

    cp->Process("modeChange", LobbyController::OnModeChange);
    cp->Process("hasOnlineMode", LobbyController::OnHasOnlineMode);
    cp->Process("getMode", LobbyController::OnGetMode);

    cp->Process("applyLanguage", LobbyController::OnApplyLanguage);
    cp->Process("getCurrentLang", LobbyController::OnGetCurrentLanguage);

	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Platform_Query);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_InviteBuddyToParty);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_Leave);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_StartPrivate);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_StartRanked);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_OnProgressStatusPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_GetBuddies);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Parties_GetPartyMembers);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_CreateRoom);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnMakePrivateSlotPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnKickPlayerPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnLockTeamsPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnInviteBuddyPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnSwitchTeamsPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnToggleReadyPressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnToggleSpectatorPresed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnChatMessage);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_OnLeavePressed);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_Join);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_Rooms_QuickJoin);
	ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(cp,LobbyController,f2c_LoadedSWF);
}


//////////////////////////////////////////////////////////////////////////
// DataProvider listener for results. These results are queued up by the 
// data provider and broadcast on the main thread.
//
void LobbyController::OnDataProviderResult(LDPResult* r)
{
    //GFC_DEBUG_MESSAGE2(1, "LobbyController::OnDataProviderResult (%d) : %s", 
    //    r->Type, r->Error.Message.ToCStr());
    
    switch (r->Type)
    {
		case LDPResult::R_ServerErrorReceived:
		{	
			GFC_DEBUG_MESSAGE(1, "> Unable to connect with server.");
			FxResponseArgs<1> args;
			args.Add(GFxValue(r->Error.Message));
			pDelegate->Invoke(pMovie, "openErrorDialog", args);

			pDataProvider->StopUpdating(ActiveServerListNetwork);
			pDataProvider->Logout();
			ClearData();
			break;
		}
    case LDPResult::R_UserLoggedIn:
        {
            GASSERT(pLoginProfile.GetPtr());
            pProfileMgr->AddProfile(pLoginProfile);
            FxResponseArgs<1> args;
            args.Add(true);
            pDelegate->Invoke(pMovie, "loginStatus", args);
            break;
        }
    case LDPResult::R_LoginErrorReceived:
        {
            FxResponseArgs<2> args;
            args.Add(false);
            args.Add(GFxValue(r->Error.Message));
            pDelegate->Invoke(pMovie, "loginStatus", args);
            break;
        }
    case LDPResult::R_BuddyArrived:
        {
            GFC_DEBUG_MESSAGE1(1, "> Buddy arrived: '%s'", r->pBuddy->GetName().ToCStr());
            BuddyList.PushBack(r->pBuddy);
            if (SelectedBuddyIndex == -1) SelectedBuddyIndex = 0;
            RefreshBuddyList();
            break;
        }
    case LDPResult::R_BuddyStatusChanged:
        {
            GFC_DEBUG_MESSAGE2(1, "> Buddy status changed: '%s' (%d)", r->pBuddy->GetName().ToCStr(),
                r->pBuddy->GetStatus());
            RefreshBuddyList();
            break;
        }
    case LDPResult::R_ChatMessageArrived:
        {
            GFC_DEBUG_MESSAGE2(1, "> Buddy chat message from '%s': '%s'", r->pBuddy->GetName().ToCStr(),
                r->Message.ToCStr());
            
            GenerateChatMessage(r->pBuddy, LobbyBuddyMessage(r->Message, r->TimeStamp), 
                LobbyBuddyMessage::MSG_Incoming);

            LobbyBuddy* pchatBuddy = GetSelectedBuddy();
            if (pchatBuddy == r->pBuddy) 
            {
                LobbyBuddy::MsgBufferType& msgBuffer = r->pBuddy->GetMessages();
                CompiledMessage.Clear();
                CompiledMessage.AppendString(msgBuffer.GetPartA());
                CompiledMessage.AppendString(msgBuffer.GetPartB());
                FxResponseArgs<2> args;
                args.Add( GFxValue(r->pBuddy->GetName()) );
                args.Add( GFxValue(CompiledMessage.ToCStr()) );
                pDelegate->Invoke(pMovie, "refreshChatLog", args);
            }
            else 
            {
                r->pBuddy->SetPendingMsg(true);
                RefreshBuddyList();                
            }
            break;
        }
    case LDPResult::R_ChatErrorReceived:
        {
            FxResponseArgs<1> args;
            args.Add(GFxValue(r->Error.Message));
            pDelegate->Invoke(pMovie, "chatError", args);
            break;
        }
    case LDPResult::R_BuddyRemoved:
        {
            UPInt idx = G_LowerBound(BuddyList, r->pBuddy, LobbyBuddy::Less);
            GASSERT(idx < BuddyList.GetSize());
            BuddyList.RemoveAt(idx);
            if (idx == (UPInt)SelectedBuddyIndex)
            {
                SelectedBuddyIndex--;
                if (SelectedBuddyIndex < 0) SelectedBuddyIndex = (BuddyList.GetSize() ? 0 : -1);
            }
            RefreshBuddyList();
            break;
        }
    case LDPResult::R_BuddyRequestReceived:
        {
            // Skip if buddy request already exists
            for (UPInt i=0; i < PendingBuddyRequests.GetSize(); i++)
            {
                if (PendingBuddyRequests[i].ProfileID == r->ProfileID)
                    return;
            }
            LobbyBuddyRequest req;
            req.ProfileID = r->ProfileID;
            req.Message = r->Message;
            req.Name = r->UserName;            
            PendingBuddyRequests.PushBack(req);
            if (bBuddyPanelActive)
            {
                FxResponseArgs<1> args;
                args.Add( (Double)PendingBuddyRequests.GetSize() );
                pDelegate->Invoke(pMovie, "buddyRequestNotify", args);
            }
            else 
            {
                GFC_DEBUG_MESSAGE(1, ">>>> CANNOT NOTIFY SINCE BUDDY PANEL IS INACTIVE!");
            }
            break;
        }
    case LDPResult::R_BuddyErrorReceived:
        {
            FxResponseArgs<1> args;
            args.Add(GFxValue(r->Error.Message));
            pDelegate->Invoke(pMovie, "buddyError", args);
            break;            
        }
    case LDPResult::R_ServerAdded:
        {
            GASSERT(r->pServer);
            UnprocessedServers.PushBack(r->pServer);
            break;
        }
    case LDPResult::R_ServerUpdated:
        {
            RefreshServerList(r->Network);
            LobbyServer* pserver = r->pServer;
            pserver->UpdateProperties();
            if (pserver == GetSelectedServer())
            {
                FxResponseArgs<6> args;
                args.Add( GFxValue(pserver->DisplayGetName()) );
                args.Add( GFxValue(pserver->GetIPPort()) );
                args.Add( GFxValue(pserver->GetGameVersion()) );
                args.Add( GFxValue(pserver->GetAutoBalanced()) );
                args.Add( GFxValue(pserver->GetMapSize()) );
                args.Add( GFxValue(pserver->GetFriendlyFire()) );
                pDelegate->Invoke(pMovie, "loadServerDetails", args);
                // Sort player lists
                pserver->SortPlayerArrays(PlayerComparator);
                FxResponseArgs<2> args2;
                args2.Add( (Double)pserver->GetPlayersArray(0).GetSize() );
                args2.Add( (Double)pserver->GetPlayersArray(1).GetSize() );
                pDelegate->Invoke(pMovie, "refreshPlayerLists", args2);
            }
            break;
        }
    case LDPResult::R_ServerUpdateFailed:
        {
            // Do nothing
            break;
        }
    case LDPResult::R_ServerListUpdateStarted:
        {
            if (bServerBrowserPanelActive)
            {
                GFC_DEBUG_MESSAGE1(1, "> Started server list update [%d]", ActiveServerListNetwork);
                FxResponseArgs<1> args;
                args.Add(true);
                pDelegate->Invoke(pMovie, "updatingServerList", args);
            }
            break;
        }
    case LDPResult::R_ServerListUpdateCompleted:
        {
            RefreshServerList(r->Network);
            if (bServerBrowserPanelActive)
            {
                GFC_DEBUG_MESSAGE1(1, "> Completed server/server-list update [%d]", r->Network);
                FxResponseArgs<1> args;
                args.Add(false);
                pDelegate->Invoke(pMovie, "updatingServerList", args);
            }
            break;
        }
    case LDPResult::R_ServerListUpdateStopped:
        {
            if (bServerBrowserPanelActive)
            {
                GFC_DEBUG_MESSAGE1(1, "> Stopped server list update [%d]", ActiveServerListNetwork);
                FxResponseArgs<1> args;
                args.Add(false);
                pDelegate->Invoke(pMovie, "updatingServerList", args);
            }            
            break;
        }
    case LDPResult::R_ServerListCleared:
        {
            GFC_DEBUG_MESSAGE1(1, "> Cleared server list [%d]", r->Network);
            ClearServerStorage();
            RefreshServerList(r->Network);
            NotifyPlayerListsClear();
            break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// Helper to get the selected buddy
//
LobbyBuddy* LobbyController::GetSelectedBuddy()
{
    if (SelectedBuddyIndex < 0) return NULL;
    return BuddyList[SelectedBuddyIndex];
}


//////////////////////////////////////////////////////////////////////////
// Helper to refresh the buddy list view
//
void LobbyController::RefreshBuddyList()
{
    if (SelectedBuddyIndex > -1) 
    {
        LobbyBuddy* pselectedBuddy = GetSelectedBuddy();
        G_QuickSort(BuddyList, LobbyBuddy::Less);
        SelectedBuddyIndex = G_LowerBound(BuddyList, pselectedBuddy, LobbyBuddy::Less);
    }
    GASSERT(SelectedBuddyIndex < (int)BuddyList.GetSize());
    if (!bBuddyPanelActive) return;
    FxResponseArgs<2> args;
    args.Add( GFxValue((Double)BuddyList.GetSize()) );
    args.Add( GFxValue((Double)SelectedBuddyIndex) );
    pDelegate->Invoke(pMovie, "refreshBuddyList", args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the number of profiles for the Profiles list
//
void LobbyController::OnProfilesListRequestLength(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnProfilesListRequestLength");
    LobbyController* pthis = (LobbyController*)params.GetHandler();
    
    FxResponseArgs<1>   args;
    args.Add((Double)pthis->pProfileMgr->GetNumProfiles());
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the elements within a range for the Profiles list
//
void LobbyController::OnProfilesListRequestItemRange(const FxDelegateArgs& params)
{  
    UInt    sidx = (UInt)params[0].GetNumber();
    UInt    eidx = (UInt)params[1].GetNumber();
    GFC_DEBUG_MESSAGE2(1, "LobbyController::OnProfilesListRequestItemRange [%d, %d]", 
        sidx, eidx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    LobbyProfileManager* pprofileMgr = pthis->pProfileMgr;
    SInt lastIdx = pprofileMgr->GetNumProfiles() - 1;

    FxResponseArgsList rargs;
    SInt    numRet = (eidx - sidx + 1);
    for (SInt i = 0; i < numRet; i++)
    {
        UInt idx = sidx + i;
        if ((SInt)idx <= lastIdx)
        {
            const LobbyProfile* profile = pprofileMgr->GetProfile(idx);
            rargs.Add( GFxValue(profile->GetName()) );
            rargs.Add( GFxValue(profile->GetPassword()) );
            rargs.Add( GFxValue(profile->IsRememberingPassword()) );
        }
    }
    params.Respond(rargs);  
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the element at a specific index from the Profiles list
//
void LobbyController::OnProfilesListRequestItemAt(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount() == 1);
    int idx = (int)params[0].GetNumber();
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnProfilesListRequestItemAt");
    LobbyController* pthis = (LobbyController*)params.GetHandler();
	GASSERT(idx < (int)pthis->pProfileMgr->GetNumProfiles());
	const LobbyProfile* profile = pthis->pProfileMgr->GetProfile(idx);
    
	if (profile == NULL) return;
	
	FxResponseArgs<3> args;
	args.Add( GFxValue(profile->GetName()) );
	args.Add( GFxValue(profile->GetPassword()) );
	args.Add( profile->IsRememberingPassword() );
	params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the last used account
//
void LobbyController::OnGetRemLastAccount(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnGetRemLastAccount");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    int id = pthis->pProfileMgr->GetLastUsedProfileID();
    if (id < 0)
    {
        FxResponseArgs<2> args;
        args.Add(pthis->pProfileMgr->IsRememberingLastProfile());
        args.Add( (Double)id );
        params.Respond(args);
    }
    else
    {
        FxResponseArgs<5> args;
        args.Add(pthis->pProfileMgr->IsRememberingLastProfile());
        args.Add( (Double)id );
        const LobbyProfile* profile = pthis->pProfileMgr->GetProfile(id);
        args.Add( GFxValue(profile->GetName()) );
        args.Add( GFxValue(profile->GetPassword()) );
        args.Add( profile->IsRememberingPassword() );
        params.Respond(args);
    }        
}


//////////////////////////////////////////////////////////////////////////
// Callback to set the last used account
//
void LobbyController::OnSetRemLastAccount(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnSetRemLastAccount");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    bool remlast = params[0].GetBool();
    pthis->pProfileMgr->SetRememberLastUsedProfile(remlast);
}


//////////////////////////////////////////////////////////////////////////
// Callback to log in 
//
void LobbyController::OnLogin(const FxDelegateArgs& params)
{
    // Async response required: 'loginStatus'
    const char* name = params[0].GetString();
    const char* pass = params[1].GetString();
    bool        rempass = params[2].GetBool();
    bool        remlast = params[3].GetBool();
    GFC_DEBUG_MESSAGE4(1, "LobbyController::OnLogin ('%s', '%s', %s, %s)", 
        name, pass, rempass?"true":"false", remlast?"true":"false");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    bool error = false;
    FxResponseArgs<2> args;
    args.Add(false);
    if (G_strlen(name) == 0)
    {
        args.Add("Username cannot be empty!");
        error = true;
    }
    else if (G_strlen(pass) == 0)
    {
        args.Add("Password cannot be empty!");
        error = true;
    }
    if (error) 
    {
        params.Respond(args);
        return;
    }

    pthis->pLoginProfile = *new LobbyProfile(name, pass, rempass);
    pthis->pProfileMgr->SetRememberLastUsedProfile(remlast);
    pthis->pDataProvider->ConnectNick(name, pass);
}


//////////////////////////////////////////////////////////////////////////
// Callback to log out 
//
void LobbyController::OnLogout(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnLogout");
    LobbyController* pthis = (LobbyController*)params.GetHandler();
    
    pthis->pDataProvider->StopUpdating(pthis->ActiveServerListNetwork);

    pthis->pDataProvider->Logout();

    pthis->ClearData();
}


//////////////////////////////////////////////////////////////////////////
// Callback to create a new profile
//
void LobbyController::OnCreateProfile(const FxDelegateArgs& params)
{
    // Async response required: 'loginStatus'
    const char* name = params[0].GetString();
    const char* email = params[1].GetString();
    const char* pass = params[2].GetString();
    const char* pass2 = params[3].GetString();
    GFC_DEBUG_MESSAGE4(1, "LobbyController::OnCreateProfile ('%s', '%s', '%s', '%s')",
        name, email, pass, pass2);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    bool error = false;
    FxResponseArgs<2> args;
    args.Add(false);
    if (G_strlen(name) == 0)
    {
        args.Add("Username cannot be empty!");
        error = true;
    }
    else if (G_strlen(email) == 0)
    {
        args.Add("Email cannot be empty!");
        error = true;
    }
    else if (G_strlen(pass) == 0)
    {
        args.Add("Password cannot be empty!");
        error = true;
    }
    else if (G_strcmp(pass, pass2))
    {
        args.Add("Password mismatch!");
        error = true;
    }
    if (error)
    {
        params.Respond(args);
        return;
    }

    pthis->pLoginProfile = *new LobbyProfile(name, pass, false);
    pthis->pDataProvider->ConnectNewUser(name, pass, email);
}


//////////////////////////////////////////////////////////////////////////
// Callback to delete a profile
//
void LobbyController::OnDeleteProfile(const FxDelegateArgs& params)
{
    UPInt idx = (UPInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnDeleteProfile (%d)", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->pProfileMgr->DeleteProfile(idx);
    FxResponseArgs<1> args;
    args.Add((Double)pthis->pProfileMgr->GetNumProfiles());
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to switch dataproviders
//
void LobbyController::OnModeChange(const FxDelegateArgs& params)
{
#ifndef LOBBY_NO_ONLINE
    GASSERT(params.GetArgCount()==1);
    bool offline = params[0].GetBool();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnModeChange : '%s'", (offline)?"offline":"online");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->bOfflineMode = offline;
    pthis->BootDataProvider();
#else
    GUNUSED(params);
    GASSERT(0); // Should never be a mode change when online mode is disabled
#endif
}


//////////////////////////////////////////////////////////////////////////
// Callback to notify the UI whether online mode is supported or not
//
void LobbyController::OnHasOnlineMode(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnHasOnlineMode");

    FxResponseArgs<1> args;
#ifndef LOBBY_NO_ONLINE
    args.Add( true );
#else
    args.Add( false );
#endif
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to retreive the current mode
//
void LobbyController::OnGetMode(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnGetMode");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<1> args;
    args.Add( pthis->bOfflineMode );
    params.Respond( args );
}

//////////////////////////////////////////////////////////////////////////
// Callback to retrieve the current loc language
//
void LobbyController::OnGetCurrentLanguage(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnGetCurrentLanguage");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<1> args;
    args.Add(pthis->pLocalizer->GetCurrentLanguage());

    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to apply a new loc language
//
void LobbyController::OnApplyLanguage(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnApplyLanguage");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    if (params.GetArgCount() > 0)
        pthis->pLocalizer->ApplyLanguage(params[0].GetString());
}


//////////////////////////////////////////////////////////////////////////
// Callback to notify when the buddy panel is available
//
void LobbyController::OnBuddyPanelStateChange(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnBuddyPanelStateChange [%d]", params[0].GetBool());
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->bBuddyPanelActive = params[0].GetBool();

	// KevinJ: Tell the data provider when the buddy list is active, so the buddy list can be populated
	pthis->pDataProvider->OnBuddyPanelStateChange(pthis->bBuddyPanelActive);

    if (pthis->bBuddyPanelActive)
        pthis->RefreshBuddyList();
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the number of pending buddy requests
//
void LobbyController::OnGetBuddyRequestsCount(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnGetBuddyRequestsCount");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<1> args;
    args.Add( (Double)pthis->PendingBuddyRequests.GetSize() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to process the next buddy request 
//
void LobbyController::OnProcessBuddyRequest(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnProcessBuddyRequest");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    GASSERT(pthis->PendingBuddyRequests.GetSize() > 0);
    UInt idx = pthis->PendingBuddyRequests.GetSize()-1;
    const LobbyBuddyRequest& req = pthis->PendingBuddyRequests[idx];
    FxResponseArgs<3> args;
    args.Add( (Double)idx );
    args.Add( GFxValue(req.Name) );
    args.Add( GFxValue(req.Message) );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback when a buddy request is accepted
//
void LobbyController::OnAcceptBuddyRequest(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount() == 2);
    bool accept = params[0].GetBool();
    UInt idx = (UInt)params[1].GetNumber();
    GFC_DEBUG_MESSAGE2(1, "LobbyController::OnAcceptBuddyRequest [%s, %d]", accept?"true":"false", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    GASSERT(idx == pthis->PendingBuddyRequests.GetSize()-1);

	// KevinJ: Modified to also take buddyName as an input parameters
    if (accept)
        pthis->pDataProvider->AcceptBuddyRequest(pthis->PendingBuddyRequests[idx].ProfileID, pthis->PendingBuddyRequests[idx].Name);
    else
        pthis->pDataProvider->DenyBuddyRequest(pthis->PendingBuddyRequests[idx].ProfileID, pthis->PendingBuddyRequests[idx].Name);
    pthis->PendingBuddyRequests.PopBack();

    FxResponseArgs<1> args;
    args.Add( (Double)pthis->PendingBuddyRequests.GetSize() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the number of buddies for the Buddy list
//
void LobbyController::OnBuddyListRequestLength(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::BuddiesRequestLength");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<1>   args;
    args.Add((Double)pthis->BuddyList.GetSize());
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the elements within a range for the Buddy list
//
void LobbyController::OnBuddyListRequestItemRange(const FxDelegateArgs& params)
{  
    UInt    sidx = (UInt)params[0].GetNumber();
    UInt    eidx = (UInt)params[1].GetNumber();
    GFC_DEBUG_MESSAGE2(1, "LobbyController::BuddiesRequestItemRange [%d, %d]", 
        sidx, eidx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    SInt lastIdx = pthis->BuddyList.GetSize() - 1;

    FxResponseArgsList rargs;
    SInt    numRet = (eidx - sidx + 1);
    for (SInt i = 0; i < numRet; i++)
    {
        UInt idx = sidx + i;
        if ((SInt)idx <= lastIdx)
        {
            const LobbyBuddy* pbuddy = pthis->BuddyList[idx];
            GASSERT(pbuddy);
            rargs.Add( GFxValue(pbuddy->GetName()) );
            rargs.Add( GFxValue( (Double)pbuddy->GetStatus() ) );
        }
    }
    params.Respond(rargs);    
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the element at a specific index from the Buddy List
//
void LobbyController::OnBuddyListRequestItemAt(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    UInt idx = (UInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnBuddyListRequestItemAt [%d]", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    LobbyBuddy* pbuddy = pthis->BuddyList[idx];

    FxResponseArgs<1> rargs;
    rargs.Add( GFxValue(pbuddy->GetName()) );
    params.Respond(rargs);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get select a new chat log
//
void LobbyController::OnSelectChatLog(const FxDelegateArgs& params)
{
    int    buddyidx = (int)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::SelectChatLog [%d]", buddyidx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();
    
    // Special case; clear the chat log
    if (buddyidx == -1) 
    {
        FxResponseArgs<2> rargs;
        rargs.Add( GFxValue("") );
        rargs.Add( GFxValue("") );
        params.Respond(rargs);
        return;
    }

    GASSERT(buddyidx < (int)pthis->BuddyList.GetSize());
    LobbyBuddy* pbuddy = pthis->BuddyList[buddyidx];
    GASSERT(pbuddy);
    pthis->SelectedBuddyIndex = buddyidx;

    if (pbuddy->HasPendingMsg())
    {
        pbuddy->SetPendingMsg(false);
        pthis->RefreshBuddyList();
    }

    LobbyBuddy::MsgBufferType& msgBuffer = pbuddy->GetMessages();
    pthis->CompiledMessage.Clear();
    pthis->CompiledMessage.AppendString( msgBuffer.GetPartA() );
    pthis->CompiledMessage.AppendString( msgBuffer.GetPartB() );

    FxResponseArgs<2> rargs;
    rargs.Add( GFxValue(pbuddy->GetName()) );
    rargs.Add( GFxValue(pthis->CompiledMessage) );
    params.Respond(rargs);
}


//////////////////////////////////////////////////////////////////////////
// Callback to send a new chat message to the currently selected buddy
//
void LobbyController::OnSubmitChat(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE1(1, "LobbyController::SubmitChat '%s'", params[0].GetString());
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    LobbyBuddy* pchatBuddy = pthis->GetSelectedBuddy();
    if (!pchatBuddy) return;
    pthis->pDataProvider->SendChatMessage(pchatBuddy, params[0].GetString());

    LobbyBuddy::MsgBufferType& msgBuffer = pchatBuddy->GetMessages();
    pthis->GenerateChatMessage(pchatBuddy, LobbyBuddyMessage(params[0].GetString(), time(0)), 
        LobbyBuddyMessage::MSG_Outgoing);

    pthis->CompiledMessage.Clear();
    pthis->CompiledMessage.AppendString( msgBuffer.GetPartA() );
    pthis->CompiledMessage.AppendString( msgBuffer.GetPartB() );

    FxResponseArgs<2> rargs;
    rargs.Add( GFxValue(pchatBuddy->GetName()) );
    rargs.Add( GFxValue(pthis->CompiledMessage) );
    params.Respond(rargs);
}


//////////////////////////////////////////////////////////////////////////
// Helper to format a chat message (timestamp, html, etc.)
//
void LobbyController::GenerateChatMessage(LobbyBuddy* pbuddy, const LobbyBuddyMessage& msg, 
                                       LobbyBuddyMessage::Direction dir)
{
    GASSERT(pbuddy);
    LobbyBuddy::MsgBufferType& buff = pbuddy->GetMessages();

    GString str;
    GString::EscapeSpecialHTML(msg.GetMessage().ToCStr(), msg.GetMessage().GetLength(), &str);

    if (buff.GetNumMessages() > 0)
    {
        buff.Append("\n");
    }

    // Append timestamp
    tm timeStamp;
    time_t t = msg.GetTimeStamp();
    localtime_s(&timeStamp, &t);
    char timeStr[16];
    G_sprintf(timeStr, sizeof(timeStr), "%2d:%02d:%02d", timeStamp.tm_hour, 
        timeStamp.tm_min, timeStamp.tm_sec);
    buff.Append("<FONT COLOR=\"#79B5B5\">").Append(timeStr).Append(" - </FONT>");

    // Append nick
    if (dir == LobbyBuddyMessage::MSG_Outgoing)
    {
        buff.Append("<FONT COLOR=\"#387734\">[").Append(pLoginProfile->GetName());
    }
    else
    {
        buff.Append("<FONT COLOR=\"#A0302E\">[").Append(pbuddy->GetName());
    }

    // Append message
    buff.Append("]</FONT>: ").Append(str.ToCStr()).Compile();
}


//////////////////////////////////////////////////////////////////////////
// Callback to remove the currently selected buddy from the buddy list
//
void LobbyController::OnRemoveSelectedBuddy(const FxDelegateArgs& params)
{    
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    LobbyBuddy* pchatBuddy = pthis->GetSelectedBuddy();
    if (!pchatBuddy) return;
    GFC_DEBUG_MESSAGE1(1, "LobbyController::RemoveSelectedBuddy '%s'", pchatBuddy->GetName().ToCStr());

    pthis->pDataProvider->RemoveBuddy(pchatBuddy);
}


//////////////////////////////////////////////////////////////////////////
// Callback to add a buddy
//
void LobbyController::OnAddBuddy(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==2);
    GFC_DEBUG_MESSAGE2(1, "LobbyController::AddBuddy ['%s', '%s']", 
        params[0].GetString(), params[1].GetString());
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->pDataProvider->AddBuddy(params[0].GetString(), params[1].GetString());
}


//////////////////////////////////////////////////////////////////////////
// Callback to notify when the server panel is available
//
void LobbyController::OnServerBrowserPanelStateChange(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnServerBrowserPanelStateChange [%d]", params[0].GetBool());
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->bServerBrowserPanelActive = params[0].GetBool();
}


//////////////////////////////////////////////////////////////////////////
// Callback to update the server list
//
void LobbyController::OnPopulateServers(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    UInt networkID = (UInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnPopulateServers [%d]", networkID);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    // Stop UI server list updating
    FxResponseArgs<1> args;
    args.Add(false);
    pthis->pDelegate->Invoke(params.GetMovie(), "updatingServerList", args);

    pthis->pDataProvider->StopUpdating(pthis->ActiveServerListNetwork);

    LobbyServer::NetworkType net = (networkID==0)?LobbyServer::NET_WAN:LobbyServer::NET_LAN;
    pthis->ActiveServerListNetwork = net;
    pthis->pDataProvider->PopulateNewServerList( net );
}


//////////////////////////////////////////////////////////////////////////
// Callback to notify the UI of the current server list properties
//
void LobbyController::OnPollServerList(const FxDelegateArgs& params)
{
    //GFC_DEBUG_MESSAGE(1, "LobbyController::OnPollServerList");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->FlushUnprocessedServers();

    FxResponseArgs<5> args;
    args.Add( (Double)pthis->FilteredServerList.GetSize() );
    args.Add( (Double)pthis->UnfilteredServerList.GetSize() );
    args.Add( (Double)pthis->pDataProvider->GetTotalServerCount(pthis->ActiveServerListNetwork));
    args.Add( (Double)pthis->SelectedServerIndex );
    args.Add( (Double)pthis->TopServerIndex );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Helper to clear local server storage
//
void LobbyController::ClearServerStorage()
{
    FilteredServerList.Clear();
    UnfilteredServerList.Clear();
    UnprocessedServers.Clear();
    SelectedServerIndex = -1;
    TopServerIndex = -1;
}


//////////////////////////////////////////////////////////////////////////
// Helper to notify the UI of a server list refresh
//
void LobbyController::RefreshServerList(LobbyServer::NetworkType network)
{
    if (bServerBrowserPanelActive) 
    {
        FlushUnprocessedServers();

        FxResponseArgs<5> args;
        args.Add( (Double)FilteredServerList.GetSize() );
        args.Add( (Double)UnfilteredServerList.GetSize() );
        args.Add( (Double)pDataProvider->GetTotalServerCount(network));
        args.Add( (Double)SelectedServerIndex );
        args.Add( (Double)TopServerIndex );
        pDelegate->Invoke(pMovie, "refreshServerList", args);
    }
}


//////////////////////////////////////////////////////////////////////////
// Process unprocessed servers (move them to the UnfilteredServers list
// and to the FilteredServers list if they pass the server filter 
// conditions)
//
void LobbyController::FlushUnprocessedServers()
{
    LobbyServer* pselectedServer = GetSelectedServer();
    LobbyServer* ptopServer = GetTopServer();

    for (UPInt i=0; i < UnprocessedServers.GetSize(); i++) 
    {
        LobbyServer* pserver = UnprocessedServers[i];
        UnfilteredServerList.PushBack(pserver);

        if (!ServerFilter.IsActive() || ServerFilter.Pass(pserver))
            FilteredServerList.PushBack(pserver);
    }

    if (FilteredServerList.GetSize()) 
    {
        G_QuickSort(FilteredServerList, ServerComparator);
        if (SelectedServerIndex > -1) 
            SelectedServerIndex = G_LowerBound(FilteredServerList, pselectedServer, ServerComparator);
        if (TopServerIndex > -1)
            TopServerIndex = G_LowerBound(FilteredServerList, ptopServer, ServerComparator);
    }
    GASSERT(SelectedServerIndex < (int)FilteredServerList.GetSize());

    UnprocessedServers.Clear();
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the number of servers for the Server list
//
void LobbyController::OnServerListRequestLength(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnServerListRequestLength");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<1>   args;
    args.Add((Double)pthis->FilteredServerList.GetSize());
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the elements within a range for the Server list
//
void LobbyController::OnServerListRequestItemRange(const FxDelegateArgs& params)
{  
    UInt    sidx = (UInt)params[0].GetNumber();
    UInt    eidx = (UInt)params[1].GetNumber();
    //GFC_DEBUG_MESSAGE2(1, "LobbyController::OnServerListRequestItemRange [%d, %d]", 
    //    sidx, eidx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    SInt lastIdx = pthis->FilteredServerList.GetSize() - 1;

    FxResponseArgsList rargs;
    SInt    numRet = (eidx - sidx + 1);
    for (SInt i = 0; i < numRet; i++)
    {
        UInt idx = sidx + i;
        if ((SInt)idx <= lastIdx)
        {
            const LobbyServer* pserver = pthis->FilteredServerList[idx];
            GASSERT(pserver);
            // Need the following returned: name, ping, players, mapName, gameMode, locked
            rargs.Add( GFxValue(pserver->DisplayGetName()) );
            rargs.Add( (Double)pserver->DisplayGetPing() );
            rargs.Add( GFxValue(pserver->DisplayGetPlayers()) );
            rargs.Add( GFxValue(pserver->DisplayGetMapName()) );
            rargs.Add( GFxValue(pserver->DisplayGetGameMode()) );
            rargs.Add( pserver->DisplayIsLocked() );
        }
    }
    params.Respond(rargs);    
}

//////////////////////////////////////////////////////////////////////////
// Callback to sort the server list on a specific column
//
void LobbyController::OnSortServerList(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    SInt key = (SInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnSortServerList [%d]", key);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    if (key == pthis->ServerComparator.GetKey())
        pthis->ServerComparator.FlipOrder();
    else
        pthis->ServerComparator.SetKey(key);

    LobbyServer* pselectedServer = pthis->GetSelectedServer();
    LobbyServer* ptopServer = pthis->GetTopServer();
    for (UPInt idx=0; idx < pthis->FilteredServerList.GetSize(); idx++)
    {
        LobbyServer* pserver = pthis->FilteredServerList[idx];
        pserver->CommitCache();
    }
    G_QuickSort(pthis->FilteredServerList, pthis->ServerComparator);
    if (pselectedServer)
    {
        pthis->SelectedServerIndex = G_LowerBound(pthis->FilteredServerList, pselectedServer, pthis->ServerComparator);
        pthis->TopServerIndex = G_LowerBound(pthis->FilteredServerList, ptopServer, pthis->ServerComparator);;
    }

    FxResponseArgs<2> args;
    args.Add( (Double)pthis->ServerComparator.GetKey() );
    args.Add( pthis->ServerComparator.GetAscending() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to set the initial server list sort column and order
//
void LobbyController::OnSetInitialServerSort(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==2);
    SInt key = (SInt)params[0].GetNumber();
    bool asc = params[1].GetBool();
    GFC_DEBUG_MESSAGE2(1, "LobbyController::OnSetInitialServerSort [%d, %d]", key, asc);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->ServerComparator.SetKey(key);
    pthis->ServerComparator.SetAscending(asc);

    FxResponseArgs<2> args;
    args.Add( (Double)pthis->ServerComparator.GetKey() );
    args.Add( pthis->ServerComparator.GetAscending() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Helper to get the selected server
//
LobbyServer* LobbyController::GetSelectedServer()
{
    if (SelectedServerIndex < 0) return NULL;
    return FilteredServerList[SelectedServerIndex];
}


/////////////////////////////////////////////////////////////////////////
// Helper to get the topmost server in the list view
//
LobbyServer* LobbyController::GetTopServer()
{
    if (TopServerIndex < 0) return NULL;
    return FilteredServerList[TopServerIndex];
}


//////////////////////////////////////////////////////////////////////////
// Callback when the server list has been scrolled in the UI. This 
// callback receives the most current index of the topmost element in 
// the server list.
//
void LobbyController::OnServersScrolled(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    UInt topIdx = (UInt)params[0].GetNumber();
    //GFC_DEBUG_MESSAGE1(1, "LobbyController::OnServersScrolled [%d]", topIdx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->TopServerIndex = topIdx;
}


//////////////////////////////////////////////////////////////////////////
// Callback to refresh a server
//
void LobbyController::OnRefreshServer(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==2);
    UInt selIdx = (UInt)params[0].GetNumber();
    UInt topIdx = (UInt)params[1].GetNumber();
    //GFC_DEBUG_MESSAGE2(1, "LobbyController::OnRefreshServer [%d, %d]", selIdx, topIdx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    GASSERT(selIdx < pthis->FilteredServerList.GetSize());
    pthis->SelectedServerIndex = selIdx;
    pthis->TopServerIndex = topIdx;

    LobbyServer* pserver = pthis->GetSelectedServer();
    pthis->pDataProvider->RefreshServer(pthis->ActiveServerListNetwork, pserver);

    pthis->NotifyPlayerListsClear();

    FxResponseArgs<6> args;
    args.Add( GFxValue(pserver->DisplayGetName()) );
    args.Add( GFxValue(pserver->GetIPPort()) );
    args.Add( GFxValue(pserver->GetGameVersion()) );
    args.Add( GFxValue(pserver->GetAutoBalanced()) );
    args.Add( GFxValue(pserver->GetMapSize()) );
    args.Add( GFxValue(pserver->GetFriendlyFire()) );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to spectate a server
//
void LobbyController::OnSpectateServer(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    UInt idx = (UInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnSpectateServer [%d]", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    GASSERT(idx < pthis->FilteredServerList.GetSize());

    LobbyServer* pserver = pthis->FilteredServerList[idx];
    
    // Add game logic to spectate the server
    GUNUSED(pserver);
}


//////////////////////////////////////////////////////////////////////////
// Callback to join a server
//
void LobbyController::OnJoinServer(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    UInt idx = (UInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnJoinServer [%d]", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    GASSERT(idx < pthis->FilteredServerList.GetSize());

    LobbyServer* pserver = pthis->FilteredServerList[idx];
    
    // Add game logic to join the server
    GUNUSED(pserver);
}


//////////////////////////////////////////////////////////////////////////
// Callback to connect to a server by ip/port/pass
//
void LobbyController::OnConnect(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==3);
    const char* ip = params[0].GetString();
    const char* port = params[1].GetString();
    const char* password = params[2].GetString();
    GFC_DEBUG_MESSAGE3(1, "LobbyController::OnConnect [%s:%s, pass:'%s']", ip, port, password);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

     // Add game logic to connect to a server
    GUNUSED4(pthis, ip, port, password);
}


//////////////////////////////////////////////////////////////////////////
// Callback to retrieve the current server filter properties
//
void LobbyController::OnGetServerFilter(const FxDelegateArgs& params)
{
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnGetServerFilter");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgs<7> args;
    args.Add( pthis->ServerFilter.GetServerName() );
    args.Add( pthis->ServerFilter.GetMapName() );
    args.Add( pthis->ServerFilter.GetGameMode() );
    args.Add( pthis->ServerFilter.GetMaxPing() );
    args.Add( pthis->ServerFilter.GetNotFull() );
    args.Add( pthis->ServerFilter.GetNotEmpty() );
    args.Add( pthis->ServerFilter.GetNoPass() );
    params.Respond(args);
}

//////////////////////////////////////////////////////////////////////////
// Callback to set the current server filter properties
//
void LobbyController::OnSetServerFilter(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==7);
    const char* serverName = params[0].GetString();
    const char* map = params[1].GetString();
    const char* game = params[2].GetString();
    const char* maxPing = params[3].GetString();
    bool notFull = params[4].GetBool();
    bool notEmpty = params[5].GetBool();
    bool noPass = params[6].GetBool();
    GFC_DEBUG_MESSAGE(1, "LobbyController::OnSetServerFilter");
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->ServerFilter.Set(serverName, map, game, maxPing, notFull, notEmpty, noPass);
    pthis->ApplyServerFilter();
}


//////////////////////////////////////////////////////////////////////////
// Helper to apply the latest server filter
//
void LobbyController::ApplyServerFilter()
{
    FilteredServerList.Clear();
    if (ServerFilter.IsActive())
    {
        for (UPInt i=0; i < UnfilteredServerList.GetSize(); i++)
        {
            LobbyServer* pserver = UnfilteredServerList[i];
            if (ServerFilter.Pass(pserver))
            {
                pserver->CommitCache();
                FilteredServerList.PushBack(pserver);
            }
        }
    }
    else
    {
        for (UPInt i=0; i < UnfilteredServerList.GetSize(); i++)
        {
            LobbyServer* pserver = UnfilteredServerList[i];
            pserver->CommitCache();
            FilteredServerList.PushBack(pserver);
        }
    }
    
    SelectedServerIndex = -1;
    TopServerIndex = -1;
    if (FilteredServerList.GetSize())
    {
        G_QuickSort(FilteredServerList, ServerComparator);
        SelectedServerIndex = 0;
        TopServerIndex = 0;
        pDataProvider->RefreshServer(ActiveServerListNetwork, FilteredServerList[0]);
    }
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the number of players for a players list
//
void LobbyController::OnPlayersListRequestLength(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1); // team id
    UInt team = (UInt)params[0].GetNumber();    
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnPlayersListRequestLength team=%d", team);
    LobbyController* pthis = (LobbyController*)params.GetHandler();    

    UInt numPlayers = 0;
    LobbyServer* pserver = pthis->GetSelectedServer();    
    if (pserver) 
    {
        const LobbyServer::PlayersArray& players = pserver->GetPlayersArray(team);
        numPlayers = players.GetSize();
    }
    FxResponseArgs<1>   args;
    args.Add((Double)numPlayers);
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the element at a specific index from the Buddy List
//
void LobbyController::OnPlayersListRequestItemAt(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==2);
    UInt team = (UInt)params[0].GetNumber();
    UInt idx = (UInt)params[1].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnPlayersListRequestItemAt [%d]", idx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    LobbyServer* pserver = pthis->GetSelectedServer();
    if (!pserver) 
        return;
    GUNUSED(team);

    const LobbyServer::PlayersArray& players = pserver->GetPlayersArray(team);
    if (idx >= players.GetSize())
        return;
    const LobbyPlayer* pplayer = players[idx];

    FxResponseArgs<8> rargs;
    Double kills = rand() % 500;
    Double deaths = rand() % 500;
    Double ratio = kills/((!deaths) ? 1 : deaths);
    rargs.Add( GFxValue(pplayer->GetName()) );
    rargs.Add( Double( rand() % 10  ) );  // rank
    rargs.Add( Double( rand() % 250 ) );  // score
    rargs.Add( Double( rand() % 200 ) );  // enlisted (negative delta)
    rargs.Add( Double( rand() % 400 ) );  // time played (hrs)
    rargs.Add( kills );
    rargs.Add( deaths );
    rargs.Add( ratio );

    params.Respond(rargs);
}


//////////////////////////////////////////////////////////////////////////
// Callback to get the elements within a range for a players list
//
void LobbyController::OnPlayersListRequestItemRange(const FxDelegateArgs& params)
{  
    GASSERT(params.GetArgCount()==3); // team id, start idx, end idx
    UInt    team = (UInt)params[0].GetNumber();    
    UInt    sidx = (UInt)params[1].GetNumber();
    UInt    eidx = (UInt)params[2].GetNumber();
    GFC_DEBUG_MESSAGE3(1, "LobbyController::OnPlayersListRequestItemRange team=%d [%d, %d]", 
        team, sidx, eidx);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    FxResponseArgsList rargs;
    LobbyServer* pserver = pthis->GetSelectedServer();    
    if (pserver) 
    {
		// Scaleform bug:
		/*
		When void LobbyController::OnPlayersListRequestItemRange is called, and only one player is in the list, then eidx is -1 and sidx is 0.

		This calculation then amounts to -1 - 0 + 1 = 0.
		SInt    numRet = (eidx - sidx + 1);

		So no players are returned, although 1 player is actually in the list.
		*/

        const LobbyServer::PlayersArray& players = pserver->GetPlayersArray(team);
        SInt    lastIdx = players.GetSize() - 1;
        SInt    numRet = (eidx - sidx + 1);
        for (SInt i = 0; i < numRet; i++)
        {
            UInt idx = sidx + i;
            if ((SInt)idx <= lastIdx)
            {
                const LobbyPlayer* pplayer = players[idx];
                GASSERT(pplayer);
                // Need the following returned: name, score, deaths, ping
                rargs.Add( GFxValue(pplayer->GetName()) );
                rargs.Add( (Double)pplayer->GetScore() );
                rargs.Add( (Double)pplayer->GetDeaths() );
                rargs.Add( (Double)pplayer->GetPing() );
            }
        }
    }    
    params.Respond(rargs);    
}


//////////////////////////////////////////////////////////////////////////
// Callback when the player lists are to be sorted by a specific column
//
void LobbyController::OnSortPlayerLists(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==1);
    SInt key = (SInt)params[0].GetNumber();
    GFC_DEBUG_MESSAGE1(1, "LobbyController::OnSortPlayerLists [%d]", key);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    if (key == pthis->PlayerComparator.GetKey())
        pthis->PlayerComparator.FlipOrder();
    else
        pthis->PlayerComparator.SetKey(key);

    LobbyServer* pselectedServer = pthis->GetSelectedServer();
    if (pselectedServer)
        pselectedServer->SortPlayerArrays(pthis->PlayerComparator);

    FxResponseArgs<2> args;
    args.Add( (Double)pthis->PlayerComparator.GetKey() );
    args.Add( pthis->PlayerComparator.GetAscending() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Callback to set the intiail sort properties of the players filter
//
void LobbyController::OnSetInitialPlayersSort(const FxDelegateArgs& params)
{
    GASSERT(params.GetArgCount()==2);
    SInt key = (SInt)params[0].GetNumber();
    bool asc = params[1].GetBool();
    GFC_DEBUG_MESSAGE2(1, "LobbyController::OnSetInitialPlayersSort [%d, %d]", key, asc);
    LobbyController* pthis = (LobbyController*)params.GetHandler();

    pthis->PlayerComparator.SetKey(key);
    pthis->PlayerComparator.SetAscending(asc);

    FxResponseArgs<2> args;
    args.Add( (Double)pthis->PlayerComparator.GetKey() );
    args.Add( pthis->PlayerComparator.GetAscending() );
    params.Respond(args);
}


//////////////////////////////////////////////////////////////////////////
// Helper to notify the UI to clear the player lists
//
void LobbyController::NotifyPlayerListsClear()
{
    if (bServerBrowserPanelActive)
    {
        // Notify UI to clear player lists
        FxResponseArgs<2> args2;
        args2.Add( (Double)0 );
        args2.Add( (Double)0 );
        pDelegate->Invoke(pMovie, "refreshPlayerLists", args2);
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////

void LobbyController::c2f_ShowProgress(bool showCancel, bool showOK, GString dialogId)
{
	FxResponseArgs<3> rargs;
	rargs.Add(showCancel);
	rargs.Add(showOK);
	rargs.Add(GFxValue(dialogId.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_ShowProgress", rargs);
}
void LobbyController::c2f_HideProgress(GString dialogId)
{
	FxResponseArgs<1> rargs;
	rargs.Add(GFxValue(dialogId.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_HideProgress", rargs);
}
void LobbyController::c2f_LoadSWF(GString swfId)
{
	FxResponseArgs<1> rargs;
	rargs.Add(GFxValue(swfId.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_LoadSWF", rargs);
}
void LobbyController::c2f_Parties_AddOrUpdateBuddy(GString name, GString presenceString)
{
	FxResponseArgs<2> rargs;
	rargs.Add(GFxValue(name.ToCStr()));
	rargs.Add(GFxValue(presenceString.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_Parties_AddOrUpdateBuddy", rargs);
}
void LobbyController::c2f_Parties_RemoveBuddy(GString name)
{
	FxResponseArgs<1> rargs;
	rargs.Add(GFxValue(name.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_Parties_RemoveBuddy", rargs);
}
void LobbyController::c2f_Parties_AddOrUpdatePartyMember(GString name, GString connectionStatus)
{
	FxResponseArgs<2> rargs;
	rargs.Add(GFxValue(name.ToCStr()));
	rargs.Add(GFxValue(connectionStatus.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_Parties_AddOrUpdatePartyMember", rargs);
}
void LobbyController::c2f_Parties_RemovePartyMember(GString name)
{
	FxResponseArgs<1> rargs;
	rargs.Add(GFxValue(name.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_Parties_RemovePartyMember", rargs);
}
void LobbyController::RoomPlayer::WriteToArgs(FxResponseArgsList &rargs) const
{
	rargs.Add(GFxValue(playerName.ToCStr()));
	rargs.Add(GFxValue(playerConnectionStatus.ToCStr()));
	rargs.Add(GFxValue(playerCurrentTeam.ToCStr()));
	rargs.Add(GFxValue(playerDesiredTeam.ToCStr()));
	rargs.Add(playerIsReady);
	rargs.Add(playerIsTalking);
	rargs.Add((Double) (playerPing));
}
void LobbyController::RoomSlot::WriteToArgs(FxResponseArgsList &rargs) const
{
	if (slotStatus==SS_IS_OPEN)
		rargs.Add( GFxValue( "open" )  );
	else if (slotStatus==SS_IS_PRIVATE)
		rargs.Add( GFxValue( "private" ) );
	else if (slotStatus==SS_IS_SPECTATOR)
		rargs.Add( GFxValue( "spectator" ) );
	else if (slotStatus==SS_IS_PLAYER)
	{
		rargs.Add( GFxValue( "player" ));
		roomPlayer->WriteToArgs(rargs);
	}
}
void LobbyController::c2f_Rooms_Show(GString roomName, bool teamsAreLocked, bool showGameStartCountdown, int gameStartCountdownInSeconds, int numSlots, const RoomSlot *roomSlots)
{
	FxResponseArgsList rargs;
	rargs.Add(GFxValue(roomName.ToCStr()));
	rargs.Add(teamsAreLocked);
	rargs.Add(showGameStartCountdown);
	rargs.Add((Double) gameStartCountdownInSeconds);
	rargs.Add((Double) numSlots);
	for (int i=0; i < numSlots; i++)
		roomSlots[i].WriteToArgs(rargs);
	FxDelegate::Invoke(pMovie, "c2f_Rooms_Show", rargs);
}
void LobbyController::c2f_Rooms_UpdateTeamsAreLocked(bool isLocked)
{
	FxResponseArgs<1> rargs;
	rargs.Add(isLocked);
	FxDelegate::Invoke(pMovie, "c2f_Rooms_UpdateTeamsAreLocked", rargs);
}
void LobbyController::c2f_Rooms_UpdateSlots(int numSlots, RoomSlot *roomSlots)
{
	FxResponseArgsList rargs;
	rargs.Add((Double) numSlots);
	for (int i=0; i < numSlots; i++)
		roomSlots[i].WriteToArgs(rargs);
	FxDelegate::Invoke(pMovie, "c2f_Rooms_UpdateSlots", rargs);
}
void LobbyController::c2f_Rooms_Update_Player(const RoomPlayer *roomPlayer)
{
	FxResponseArgsList rargs;
	roomPlayer->WriteToArgs(rargs);
	FxDelegate::Invoke(pMovie, "c2f_Rooms_Update_Player", rargs);
}
void LobbyController::c2f_Rooms_ShowGameStartCountdown(void)
{
	FxResponseArgsList rargs;
	FxDelegate::Invoke(pMovie, "c2f_Rooms_ShowGameStartCountdown", rargs);
}
void LobbyController::c2f_Rooms_HideGameStartCountdown(void)
{
	FxResponseArgsList rargs;
	FxDelegate::Invoke(pMovie, "c2f_Rooms_HideGameStartCountdown", rargs);
}
void LobbyController::c2f_Rooms_UpdateGameStartCountdown(int gameStartCountdownInSeconds)
{
	FxResponseArgs<1> rargs;
	rargs.Add((Double) gameStartCountdownInSeconds);
	FxDelegate::Invoke(pMovie, "c2f_Rooms_UpdateGameStartCountdown", rargs);
}
void LobbyController::c2f_Rooms_OnGameStarted(void)
{
	FxResponseArgsList rargs;
	FxDelegate::Invoke(pMovie, "c2f_Rooms_OnGameStarted", rargs);
}
void LobbyController::c2f_Rooms_ShowChatMessage(GString senderName, GString message)
{
	FxResponseArgs<2> rargs;
	rargs.Add(GFxValue(senderName.ToCStr()));
	rargs.Add(GFxValue(message.ToCStr()));
	FxDelegate::Invoke(pMovie, "c2f_Rooms_ShowChatMessage", rargs);
}


ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Platform_Query) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_InviteBuddyToParty) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_Leave) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_StartPrivate) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_StartRanked) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_OnProgressStatusPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_GetBuddies) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Parties_GetPartyMembers) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_CreateRoom) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnMakePrivateSlotPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnKickPlayerPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnLockTeamsPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnInviteBuddyPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnSwitchTeamsPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnToggleReadyPressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnToggleSpectatorPresed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnChatMessage) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_OnLeavePressed) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_Join)
{
	pDataProvider->f2c_Rooms_Join(pparams[0].GetNumber());
}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_Rooms_QuickJoin) {}
ACTIONSCRIPT_CALLABLE_FUNCTION(LobbyController, f2c_LoadedSWF) {}


#ifdef TELNET_DEBUGGING
bool LobbyControllerCommandParser::OnCommand(const char *command, unsigned numParameters, char **parameterList, TransportInterface *transport, SystemAddress systemAddress, const char *originalString)
{
	(void) originalString;

	if (strcmp(command, "c2f_ShowProgress")==0)
	{
		lobbyController->c2f_ShowProgress(atoi(parameterList[0]), atoi(parameterList[1]), parameterList[2]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_HideProgress")==0)
	{
		lobbyController->c2f_HideProgress(parameterList[0]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_LoadSWF")==0)
	{
		lobbyController->c2f_LoadSWF(parameterList[0]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Parties_AddOrUpdateBuddy")==0)
	{
		lobbyController->c2f_Parties_AddOrUpdateBuddy(parameterList[0],parameterList[1]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Parties_RemoveBuddy")==0)
	{
		lobbyController->c2f_Parties_RemoveBuddy(parameterList[0]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Parties_AddOrUpdatePartyMember")==0)
	{
		lobbyController->c2f_Parties_AddOrUpdatePartyMember(parameterList[0],parameterList[1]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Parties_RemovePartyMember")==0)
	{
		lobbyController->c2f_Parties_RemovePartyMember(parameterList[0]);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_Show")==0)
	{
		LobbyController::RoomSlot roomSlots[32];
		LobbyController::RoomPlayer roomPlayers[32];
		int numSlots=atoi(parameterList[4]);
		int parameterListIndex=5;
		int roomPlayerIndex=0;
		for (int i=0; i < numSlots; i++)
		{
			char *param = parameterList[parameterListIndex++];
			if (stricmp(param,"open")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_OPEN;
			else if (stricmp(param,"private")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_PRIVATE;
			else if (stricmp(param,"spectator")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_SPECTATOR;
			else if (stricmp(param,"player")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_PLAYER;
			else
				GASSERT(0);

			 if (roomSlots[i].slotStatus==LobbyController::RoomSlot::SS_IS_PLAYER)
			 {
				 roomSlots[i].roomPlayer=roomPlayers+roomPlayerIndex;
				 roomPlayers[roomPlayerIndex].playerName=parameterList[parameterListIndex++];
				 roomPlayers[roomPlayerIndex].playerConnectionStatus=parameterList[parameterListIndex++];
				 roomPlayers[roomPlayerIndex].playerCurrentTeam=parameterList[parameterListIndex++];
				 roomPlayers[roomPlayerIndex].playerDesiredTeam=parameterList[parameterListIndex++];
				 roomPlayers[roomPlayerIndex].playerIsReady=atoi(parameterList[parameterListIndex++]);
				 roomPlayers[roomPlayerIndex].playerIsTalking=atoi(parameterList[parameterListIndex++]);
				 roomPlayers[roomPlayerIndex].playerPing=atoi(parameterList[parameterListIndex++]);
				 roomPlayerIndex++;
			 }
			 else
			 {
				 roomSlots[i].roomPlayer=0;
			 }
		}

		lobbyController->c2f_Rooms_Show(parameterList[0], atoi(parameterList[1]), atoi(parameterList[2]), atoi(parameterList[3]), atoi(parameterList[4]), (LobbyController::RoomSlot*) roomSlots);
		ReturnResult(command, transport, systemAddress);

	}
	else if (strcmp(command, "c2f_Rooms_UpdateTeamsAreLocked")==0)
	{
		lobbyController->c2f_Rooms_UpdateTeamsAreLocked(atoi(parameterList[0]));
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_UpdateSlots")==0)
	{
		LobbyController::RoomSlot roomSlots[32];
		LobbyController::RoomPlayer roomPlayers[32];
		int numSlots=atoi(parameterList[0]);
		int parameterListIndex=1;
		int roomPlayerIndex=0;
		for (int i=0; i < numSlots; i++)
		{
			char *param = parameterList[parameterListIndex++];
			if (stricmp(param,"open")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_OPEN;
			else if (stricmp(param,"private")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_PRIVATE;
			else if (stricmp(param,"spectator")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_SPECTATOR;
			else if (stricmp(param,"player")==0)
				roomSlots[i].slotStatus=LobbyController::RoomSlot::SS_IS_PLAYER;
			else
				GASSERT(0);

			if (roomSlots[i].slotStatus==LobbyController::RoomSlot::SS_IS_PLAYER)
			{
				roomSlots[i].roomPlayer=roomPlayers+roomPlayerIndex;
				roomPlayers[roomPlayerIndex].playerName=parameterList[parameterListIndex++];
				roomPlayers[roomPlayerIndex].playerConnectionStatus=parameterList[parameterListIndex++];
				roomPlayers[roomPlayerIndex].playerCurrentTeam=parameterList[parameterListIndex++];
				roomPlayers[roomPlayerIndex].playerDesiredTeam=parameterList[parameterListIndex++];
				roomPlayers[roomPlayerIndex].playerIsReady=atoi(parameterList[parameterListIndex++]);
				roomPlayers[roomPlayerIndex].playerIsTalking=atoi(parameterList[parameterListIndex++]);
				roomPlayers[roomPlayerIndex].playerPing=atoi(parameterList[parameterListIndex++]);
				roomPlayerIndex++;
			}
			else
			{
				roomSlots[i].roomPlayer=0;
			}
		}

		lobbyController->c2f_Rooms_UpdateSlots(atoi(parameterList[0]), (LobbyController::RoomSlot*) roomSlots);
		ReturnResult(command, transport, systemAddress);

	}
	else if (strcmp(command, "c2f_Rooms_Update_Player")==0)
	{
		LobbyController::RoomPlayer roomPlayer;
		roomPlayer.playerName=parameterList[0];
		roomPlayer.playerConnectionStatus=parameterList[1];
		roomPlayer.playerCurrentTeam=parameterList[2];
		roomPlayer.playerDesiredTeam=parameterList[3];
		roomPlayer.playerIsReady=atoi(parameterList[4]);
		roomPlayer.playerIsTalking=atoi(parameterList[5]);
		roomPlayer.playerPing=atoi(parameterList[6]);

		lobbyController->c2f_Rooms_Update_Player(&roomPlayer);
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_ShowGameStartCountdown")==0)
	{
		lobbyController->c2f_Rooms_ShowGameStartCountdown();
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_HideGameStartCountdown")==0)
	{
		lobbyController->c2f_Rooms_HideGameStartCountdown();
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_UpdateGameStartCountdown")==0)
	{
		lobbyController->c2f_Rooms_UpdateGameStartCountdown(atoi(parameterList[0]));
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_OnGameStarted")==0)
	{
		lobbyController->c2f_Rooms_OnGameStarted();
		ReturnResult(command, transport, systemAddress);
	}
	else if (strcmp(command, "c2f_Rooms_ShowChatMessage")==0)
	{
		lobbyController->c2f_Rooms_ShowChatMessage(parameterList[0],parameterList[1]);
		ReturnResult(command, transport, systemAddress);
	}

	return true;
}
const char *LobbyControllerCommandParser::GetName(void) const
{
	return "LobbyControllerCommandParser";
}
void LobbyControllerCommandParser::SendHelp(TransportInterface *transport, SystemAddress systemAddress)
{
	transport->Send(systemAddress, "Run c2f functions manually, for debugging.\r\n");
}
#endif
