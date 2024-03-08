/**********************************************************************

Filename    :   LobbyDataProvider.h
Content     :   The Lobby application controller
Created     :   7/31/2009
Authors     :   Prasad Silva
Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_LobbyController_H
#define INC_LobbyController_H

#include "GRefCount.h"
#include "FxGameDelegate.h"

#include "LobbyDataProvider.h"
#include "LobbyProfileManager.h"


// This is the main lobby application. It interfaces with both the UI and
// the data stores (online and offline).


//////////////////////////////////////////////////////////////////////////
// Localization interface for the lobby application
//
class LobbyLocalizer 
{
public:
    virtual const char*     GetCurrentLanguage() = 0;
    virtual void            ApplyLanguage(const char* lang) = 0;
};


//////////////////////////////////////////////////////////////////////////
// The lobby application controller
//
class LobbyController : public FxDelegateHandler, public LDPResultListener
{
public:
    LobbyController(GFxMovieView* pmovie, FxDelegate* pdg, LobbyLocalizer* ploc);
    virtual ~LobbyController();

    // FxDelegate callback installer
    void            Accept(FxDelegateHandler::CallbackProcessor* cp);

    // DataProvider listener for results
    void            OnDataProviderResult(LDPResult* r);

    // Login panel callbacks
    static void     OnProfilesListRequestLength(const FxDelegateArgs& params);
    static void     OnProfilesListRequestItemRange(const FxDelegateArgs& params);
    static void     OnProfilesListRequestItemAt(const FxDelegateArgs& params);
    static void     OnLogin(const FxDelegateArgs& params);
    static void     OnLogout(const FxDelegateArgs& params);
    static void     OnCreateProfile(const FxDelegateArgs& params);
    static void     OnDeleteProfile(const FxDelegateArgs& params);
    static void     OnGetRemLastAccount(const FxDelegateArgs& params);
    static void     OnSetRemLastAccount(const FxDelegateArgs& params);

    // Language panel callbacks
    static void     OnGetCurrentLanguage(const FxDelegateArgs& params);
    static void     OnApplyLanguage(const FxDelegateArgs& params);    

    // Buddy panel callbacks
    static void     OnBuddyPanelStateChange(const FxDelegateArgs& params);
    static void     OnBuddyListRequestLength(const FxDelegateArgs& params);
    static void     OnBuddyListRequestItemRange(const FxDelegateArgs& params);
    static void     OnBuddyListRequestItemAt(const FxDelegateArgs& params);
    static void     OnSelectChatLog(const FxDelegateArgs& params);
    static void     OnSubmitChat(const FxDelegateArgs& params);
    static void     OnAddBuddy(const FxDelegateArgs& params);
    static void     OnRemoveSelectedBuddy(const FxDelegateArgs& params);
    static void     OnGetBuddyRequestsCount(const FxDelegateArgs& params);
    static void     OnProcessBuddyRequest(const FxDelegateArgs& params);
    static void     OnAcceptBuddyRequest(const FxDelegateArgs& params);

    // Browser panel callbacks
    static void     OnServerBrowserPanelStateChange(const FxDelegateArgs& params);
    static void     OnServerListRequestLength(const FxDelegateArgs& params);
    static void     OnServerListRequestItemRange(const FxDelegateArgs& params);
    static void     OnPopulateServers(const FxDelegateArgs& params);
    static void     OnPollServerList(const FxDelegateArgs& params);
    static void     OnSortServerList(const FxDelegateArgs& params);
    static void     OnRefreshServer(const FxDelegateArgs& params);
    static void     OnSpectateServer(const FxDelegateArgs& params);
    static void     OnJoinServer(const FxDelegateArgs& params);
    static void     OnConnect(const FxDelegateArgs& params);
    static void     OnGetServerFilter(const FxDelegateArgs& params);
    static void     OnSetServerFilter(const FxDelegateArgs& params);
    static void     OnSetInitialServerSort(const FxDelegateArgs& params);
    static void     OnServersScrolled(const FxDelegateArgs& params);

    // Players panel callbacks
    static void     OnPlayersListRequestLength(const FxDelegateArgs& params);
    static void     OnPlayersListRequestItemAt(const FxDelegateArgs& params);
    static void     OnPlayersListRequestItemRange(const FxDelegateArgs& params);
    static void     OnSortPlayerLists(const FxDelegateArgs& params);
    static void     OnSetInitialPlayersSort(const FxDelegateArgs& params);

    // Callback to change data provider
    static void     OnModeChange(const FxDelegateArgs& params);
    static void     OnHasOnlineMode(const FxDelegateArgs& params);
    static void     OnGetMode(const FxDelegateArgs& params);

    // Process pending tasks that may be queued up and require synchronous 
    // execution with the main thread
    void            ProcessTasks();

    GINLINE bool            IsOfflineMode() { return bOfflineMode; }

private:
    // If true, then the current data provider is an offline data provider 
    bool                        bOfflineMode;

    // GFx specific references
    GFxMovieView*               pMovie;
    FxDelegate*                 pDelegate;

    // *** Login panel specific members
    // The current data provider
    GPtr<LobbyDataProvider>     pDataProvider;
    GPtr<LobbyProfileManager>   pProfileMgr;
    // A lobby localizer to retrieve localization specific information
    LobbyLocalizer*             pLocalizer;
    // The current login profile
    GPtr<LobbyProfile>          pLoginProfile;

    // *** Buddy panel specific members
    GArray< GPtr<LobbyBuddy> >  BuddyList;
    // If true, then the buddy panel UI is available
    bool                        bBuddyPanelActive;
    int                         SelectedBuddyIndex;
    // List of buddy requests from remote users
    GArray< LobbyBuddyRequest > PendingBuddyRequests;

    // *** Server/Player panel specific members
    // Total list of servers retrieved from the data provider
    GArray< GPtr<LobbyServer> > UnfilteredServerList;
    // List of servers filtered from the unfiltered list (via the server filter)
    GArray< LobbyServer* >      FilteredServerList;
    // Temporary storage for incoming servers. They must be processed when the
    // UI requests to refresh the view
    GArray< GPtr<LobbyServer> >      UnprocessedServers;
    // If true, then the server panel UI is available
    bool                        bServerBrowserPanelActive;
    // Current network type
    LobbyServer::NetworkType    ActiveServerListNetwork;
    // Comparator for the server list
    LobbyServerLess             ServerComparator;
    // Currently selected server (can be set by both application and UI)
    int                         SelectedServerIndex;
    LobbyServerFilter           ServerFilter;
    // Comparator for the player lists
    LobbyPlayerLess             PlayerComparator;
    // Index of the top most server in the server list view (set by the UI)
    int                         TopServerIndex;

    // Temporary buffer used to compile a chat log before sending it 
    // to the UI
    GStringBuffer               CompiledMessage;

    // Initialize and startup the dataprovider
    void            BootDataProvider();
    // Clear all local server/player data
    void            ClearData();
    // Generate a formatted chat message (timestamp, html, etc.)
    void            GenerateChatMessage(LobbyBuddy* pbuddy, 
                        const LobbyBuddyMessage& msg, 
                        LobbyBuddyMessage::Direction dir);
    LobbyBuddy*     GetSelectedBuddy();
    // Helper to notify the UI to refresh its buddy list view
    void            RefreshBuddyList();

    // Process the unprocessed server list
    void            FlushUnprocessedServers();
    LobbyServer*    GetSelectedServer();
    void            ClearServerStorage();
    void            RefreshServerList(LobbyServer::NetworkType network);
    void            ApplyServerFilter();
    void            NotifyPlayerListsClear();
    LobbyServer*    GetTopServer();
};

#endif  // INC_LobbyController_H
