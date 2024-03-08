#ifndef __LOBBY_2_CLIENT_360_H
#define __LOBBY_2_CLIENT_360_H

#include "Lobby2Plugin.h"
#include "DS_OrderedList.h"
#include "XBox360Includes.h"


namespace RakNet
{
struct Lobby2Message;

class RAK_DLL_EXPORT Lobby2Client_360 : public RakNet::Lobby2Plugin
{
public:	
	Lobby2Client_360();
	virtual ~Lobby2Client_360();

	/// Send a command to the server
	/// \param[in] msg The message that represents the command
	virtual void SendMessage(Lobby2Message *msg);

	// Base class implementation
	virtual void Update(RakPeerInterface *peer);

	/// \internal
	static int Lobby2MessageComp( const unsigned int &key, Lobby2Message * const &data );

	bool IsOperationPending(Lobby2MessageID messageId);

	bool ClientStartup(void);
	HANDLE m_hLiveListener;         // Live notification listener
	HANDLE m_hSysListener;          // System notification listener


protected:
	void CheckForAcceptedInvitation(void);
	void CheckForMuteListChanged(void);
	void CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc);
	void PushDeferredCallback(Lobby2Message *msg);
	Lobby2Message *GetDeferredCallback(unsigned int requestId, bool peek);
	Lobby2Message *GetDeferredCallbackByMessageId(Lobby2MessageID messageId, bool peek);
	DataStructures::OrderedList<const unsigned int, Lobby2Message *, Lobby2MessageComp> deferredCallbacks;

	// Plugin interface functions
	void OnAttach(RakPeerInterface *peer);

};
};

#endif
