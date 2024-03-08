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


// KevinJ:
#define ACTIONSCRIPT_CALLABLE_HEADER(functionName) \
static void functionName##_static(const FxDelegateArgs& pparams); \
virtual void functionName(const FxDelegateArgs& pparams);
#define ACTIONSCRIPT_CALLABLE_FUNCTION(className, functionName) \
void className::functionName##_static(const FxDelegateArgs& pparams) \
{ \
	((className*)pparams.GetHandler())->functionName(pparams);  \
} \
void className::functionName(const FxDelegateArgs& pparams)
#define ACCEPT_ACTION_SCRIPT_CALLABLE_FUNCTION(callbackProcessor, className, functionName) \
callbackProcessor->Process( #functionName , className::functionName##_static);


// This is the main lobby application. It interfaces with both the UI and
// the data stores (online and offline).

// #define TELNET_DEBUGGING

#ifdef TELNET_DEBUGGING
#include "RakNetTransport2.h"
#include "ConsoleServer.h"
#include "CommandParserInterface.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"

class LobbyController;

class LobbyControllerCommandParser : public CommandParserInterface
{
public:
	LobbyControllerCommandParser(LobbyController *_lobbyController) {lobbyController=_lobbyController;}

	/// Given \a command with parameters \a parameterList , do whatever processing you wish.
	/// \param[in] command The command to process
	/// \param[in] numParameters How many parameters were passed along with the command
	/// \param[in] parameterList The list of parameters.  parameterList[0] is the first parameter and so on.
	/// \param[in] transport The transport interface we can use to write to
	/// \param[in] systemAddress The player that sent this command.
	/// \param[in] originalString The string that was actually sent over the network, in case you want to do your own parsing
	bool OnCommand(const char *command, unsigned numParameters, char **parameterList, TransportInterface *transport, SystemAddress systemAddress, const char *originalString);

	/// You are responsible for overriding this function and returning a static string, which will identifier your parser.
	/// This should return a static string
	/// \return The name that you return.
	const char *GetName(void) const;

	/// A callback for when you are expected to send a brief description of your parser to \a systemAddress
	/// \param[in] transport The transport interface we can use to write to
	/// \param[in] systemAddress The player that requested help.
	void SendHelp(TransportInterface *transport, SystemAddress systemAddress);

protected:
	LobbyController *lobbyController;
};
#endif

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


	// --------------- KEVINJ ---------------------
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Platform_Query);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_InviteBuddyToParty);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_Leave);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_StartPrivate);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_StartRanked);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_OnProgressStatusPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_GetBuddies);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Parties_GetPartyMembers);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_CreateRoom);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnMakePrivateSlotPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnKickPlayerPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnLockTeamsPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnInviteBuddyPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnSwitchTeamsPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnToggleReadyPressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnToggleSpectatorPresed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnChatMessage);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_OnLeavePressed);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_Join);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_Rooms_QuickJoin);
	ACTIONSCRIPT_CALLABLE_HEADER(f2c_LoadedSWF);

	struct RoomPlayer
	{
		GString playerName;
		GString playerConnectionStatus;
		GString playerCurrentTeam;
		GString playerDesiredTeam;
		bool playerIsReady;
		bool playerIsTalking;
		int playerPing;

		void WriteToArgs(FxResponseArgsList &rargs) const;
	};
	struct RoomSlot
	{
		enum SlotStatus
		{
			SS_IS_OPEN,
			SS_IS_PRIVATE,
			SS_IS_SPECTATOR,
			SS_IS_PLAYER,
		};
		
		SlotStatus slotStatus;
		RoomPlayer *roomPlayer; // If 0, then this slot is not used.		

		void WriteToArgs(FxResponseArgsList &rargs) const;
	};
	virtual void c2f_ShowProgress(bool showCancel, bool showOK, GString dialogId);
	virtual void c2f_HideProgress(GString dialogId);
	virtual void c2f_LoadSWF(GString swfId);
	virtual void c2f_Parties_AddOrUpdateBuddy(GString name, GString presenceString);
	virtual void c2f_Parties_RemoveBuddy(GString name);
	virtual void c2f_Parties_AddOrUpdatePartyMember(GString name, GString connectionStatus);
	virtual void c2f_Parties_RemovePartyMember(GString name);
	virtual void c2f_Rooms_Show(GString roomName, bool teamsAreLocked, bool showGameStartCountdown, int gameStartCountdownInSeconds, int numSlots, const RoomSlot *roomSlots);
	virtual void c2f_Rooms_UpdateTeamsAreLocked(bool isLocked);
	virtual void c2f_Rooms_UpdateSlots(int numSlots, RoomSlot *roomSlots);
	virtual void c2f_Rooms_Update_Player(const RoomPlayer *roomPlayer);
	virtual void c2f_Rooms_ShowGameStartCountdown(void);
	virtual void c2f_Rooms_HideGameStartCountdown(void);
	virtual void c2f_Rooms_UpdateGameStartCountdown(int gameStartCountdownInSeconds);
	virtual void c2f_Rooms_OnGameStarted(void);
	virtual void c2f_Rooms_ShowChatMessage(GString senderName, GString message);


#ifdef TELNET_DEBUGGING
	ConsoleServer *consoleServer;
	RakNetTransport2 *raknetTransport;
	RakPeerInterface *rakPeer;
	LobbyControllerCommandParser *lobbyControllerCommandParser;
#endif

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
