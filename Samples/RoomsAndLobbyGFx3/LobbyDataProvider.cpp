/**********************************************************************

Filename    :   LobbyDataProvider.cpp
Content     :   Data provider base used by the Lobby application
Created     :   7/31/2009
Authors     :   Prasad Silva
Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "LobbyDataProvider.h"

LobbyDataProvider::LobbyDataProvider()
{
    bAlive     = true;
    bStopRequested  = false;
}

LobbyDataProvider::~LobbyDataProvider()
{
    // free all commands and results
    while (!Commands.IsEmpty())
    {
        LDPCommand* cmd = Commands.PopFront();
        delete cmd;
    }
    while (!HighPriorityResults.IsEmpty())
    {
        LDPResult* rlt = HighPriorityResults.PopFront();
        delete rlt;
    }
    while (!LowPriorityResults.IsEmpty())
    {
        LDPResult* rlt = LowPriorityResults.PopFront();
        delete rlt;
    }
}


//////////////////////////////////////////////////////////////////////////
// Login to the master servers
//
void LobbyDataProvider::ConnectNick(const char *uniqueNick, const char *password)
{
    GFC_DEBUG_MESSAGE2(1, "LobbyDataProvider::ConnectNick(%s, %s)", uniqueNick, password);

    // Spool a connect nick command in the command queue
    LDPCommand* c = new LDPCommand(LDPCommand::C_Login);
    c->UserName = uniqueNick;
    c->Password = password;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Create a new account and login immediately afterwards
//
void LobbyDataProvider::ConnectNewUser(const char *uniqueNick, const char *password,
                                         const char* email)
{
    GFC_DEBUG_MESSAGE(1, "LobbyDataProvider::ConnectNewUser");

    LDPCommand* c = new LDPCommand(LDPCommand::C_CreateAccount);
    c->UserName = uniqueNick;
    c->Password = password;
    c->Email = email;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Logout from the GameSpy master servers
//
void LobbyDataProvider::Logout()
{
    GFC_DEBUG_MESSAGE(1, "LobbyDataProvider::Logout");

    // Spool a logout command in the command queue
    LDPCommand* c = new LDPCommand(LDPCommand::C_Logout);
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Send a message to a buddy in the buddy list. This base implementation 
// only queues up the appropriate command.
//
void LobbyDataProvider::SendChatMessage(const LobbyBuddy* pbuddy, const char* msg)
{
    GFC_DEBUG_MESSAGE2(1, "LobbyDataProvider::SendChatMessage(%s, %s)", pbuddy->GetName().ToCStr(), msg);

    LDPCommand* c = new LDPCommand(LDPCommand::C_SendChatMessage);
    c->ProfileID = pbuddy->GetProfileID();
    c->Message = msg;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Remove a buddy from the buddy list. This base implementation only 
// queues up the appropriate command.
//
void LobbyDataProvider::RemoveBuddy(const LobbyBuddy* pbuddy)
{
    GFC_DEBUG_MESSAGE1(1, "LobbyDataProvider::RemoveBuddy(%s)", pbuddy->GetName().ToCStr());

    LDPCommand* c = new LDPCommand(LDPCommand::C_RemoveBuddy);
    c->ProfileID = pbuddy->GetProfileID();
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Invite a buddy to be added to the buddy list. This base implementation 
// only queues up the appropriate command.
//
void LobbyDataProvider::AddBuddy(const char* buddyName, const char* message)
{
    GFC_DEBUG_MESSAGE1(1, "LobbyDataProvider::AddBuddy(%s)", buddyName);

    LDPCommand* c = new LDPCommand(LDPCommand::C_AddBuddy);
    c->UserName = buddyName;
    c->Message = message;
    QueueCommand(c);
}

//////////////////////////////////////////////////////////////////////////
// Accept a buddy request. This base implementation only queues up the 
// appropriate command.
//
void LobbyDataProvider::AcceptBuddyRequest(int id, const char* buddyName)
{
    GFC_DEBUG_MESSAGE1(1, "LobbyDataProvider::AcceptBuddyRequest(%d)", id);

    LDPCommand* c = new LDPCommand(LDPCommand::C_AcceptBuddyRequest);
    c->ProfileID = id;
	c->UserName=buddyName;
    QueueCommand(c);
}

//////////////////////////////////////////////////////////////////////////
// Deny a buddy request. This base implementation only queues up the 
// appropriate command.
//
void LobbyDataProvider::DenyBuddyRequest(int id, const char* buddyName)
{
    GFC_DEBUG_MESSAGE1(1, "LobbyDataProvider::DenyBuddyRequest(%d)", id);

    LDPCommand* c = new LDPCommand(LDPCommand::C_DenyBuddyRequest);
    c->ProfileID = id;
	c->UserName=buddyName;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Update the server list (called after clearing it). This base 
// implementation only queues up the appropriate command.
//
void LobbyDataProvider::PopulateNewServerList(LobbyServer::NetworkType net)
{
    GFC_DEBUG_MESSAGE(1, "LobbyDataProvider::PopulateNewServerList");

    LDPCommand* c = new LDPCommand(LDPCommand::C_PopulateServerList);
    c->Network = net;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Refresh a specific server. This base implementation only queues up the
// appropriate command.
//
void LobbyDataProvider::RefreshServer(LobbyServer::NetworkType net, LobbyServer* pserver)
{
    //GFC_DEBUG_MESSAGE(1, "LobbyDataProvider::RefreshServer");

    LDPCommand* c = new LDPCommand(LDPCommand::C_RefreshServer);
    c->Network = net;
    c->pServer = pserver;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Stop server updating. This base implementation only queues up the 
// appropriate command.
//
void LobbyDataProvider::StopUpdating(LobbyServer::NetworkType net, bool clearList)
{
    GFC_DEBUG_MESSAGE(1, "LobbyDataProvider::StopUpdating");

    LDPCommand* c = new LDPCommand(LDPCommand::C_StopServerUpdating);
    c->Network = net;
    c->bClearServers = clearList;
    QueueCommand(c);
}


//////////////////////////////////////////////////////////////////////////
// Stop the worker thread
//
void LobbyDataProvider::Stop()
{
    GFC_DEBUG_MESSAGE(1, "IDataProvider::Stop");

    GMutex::Locker lock(&Commands.GetLock());
    bAlive = false;
    bStopRequested = true;
}


//////////////////////////////////////////////////////////////////////////
// Register a result listener with the data provider. All registered 
// result listeners will receive all results generated by the data 
// provider.
//
void LobbyDataProvider::AddResultListener(LDPResultListener* l)
{
    ResultListeners.PushBack(l);
}


//////////////////////////////////////////////////////////////////////////
// Process results
//
// This is always synchronous with the render thread
//
void LobbyDataProvider::ProcessResults()
{
    // Offload the results from the thread safe containers to a local 
    // container to unblock the worker thread.

    // Service all high priority results
    HighPriorityResults.Lock();
    while (!HighPriorityResults.IsEmpty())
    {
        LDPResult* rlt = HighPriorityResults.PopFront();
        ResultsToProcess.PushBack(rlt);
    }
    HighPriorityResults.Unlock();

    // Service only one low priority result
    LowPriorityResults.Lock();
    if (!LowPriorityResults.IsEmpty())
    {
        LDPResult* rlt = LowPriorityResults.PopFront();
        ResultsToProcess.PushBack(rlt);
    }
    LowPriorityResults.Unlock();

    while (!ResultsToProcess.IsEmpty())
    {
        LDPResult* rlt = ResultsToProcess.GetFirst();
        ResultsToProcess.Remove(rlt);

        BroadcastResult(rlt);

        //GFC_DEBUG_MESSAGE1(1, "RESULT POPPED! (%d)", rlt->Type);        
        delete rlt;
        rlt = ResultsToProcess.GetFirst();
    }
}


//////////////////////////////////////////////////////////////////////////
// Notify all result listeners of a result
//
void LobbyDataProvider::BroadcastResult(LDPResult* r)
{
    for (UPInt i=0; i < ResultListeners.GetSize(); i++) 
    {
        ResultListeners[i]->OnDataProviderResult(r);
    }
}


//////////////////////////////////////////////////////////////////////////
// Process commands
//
// This is executed on the worker thread
//
void LobbyDataProvider::ProcessCommands()
{
    // Process one command at a time
    Commands.Lock();
    LDPCommand* cmd = NULL;
    if (!Commands.IsEmpty())
    {
        cmd = Commands.PopFront();
        //GFC_DEBUG_MESSAGE1(1, "COMMAND POPPED! (%d)",  cmd->Type);
    }
    Commands.Unlock();

    if (cmd != NULL)
    {
        // Invoke the impelmentation specific command processor
        ProcessCommand(cmd);
        delete cmd;
    }
}


//////////////////////////////////////////////////////////////////////////
// Process pending tasks that may be queued up and require synchronous 
// execution with the main thread
//
void LobbyDataProvider::ExecuteSynchronousTasks()
{
    ProcessResults();
}
