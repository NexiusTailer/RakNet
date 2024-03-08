#include "Lobby2Client_360.h"
#include "Lobby2Message_360.h"
//#include "AtgSignIn.h"
#include <stdlib.h>

using namespace RakNet;

static RakNet::Lobby2Client_360 *lobby2Client;

int Lobby2Client_360::Lobby2MessageComp( const unsigned int &key, Lobby2Message * const &data )
{
	if (key < data->requestId)
		return -1;
	if (key > data->requestId)
		return 1;
	return 0;
}


Lobby2Client_360::Lobby2Client_360()
{
	m_hLiveListener=INVALID_HANDLE_VALUE;
	m_hSysListener=INVALID_HANDLE_VALUE;
}

Lobby2Client_360::~Lobby2Client_360()
{
	unsigned int i;
	// To free memory, which should be freed in the callback
	for (i=0; i < deferredCallbacks.Size(); i++)
	{
		msgFactory->Dealloc(deferredCallbacks[i]);
	}
	deferredCallbacks.Clear();

	if (m_hLiveListener!=INVALID_HANDLE_VALUE)
		CloseHandle( m_hLiveListener );
	if (m_hSysListener!=INVALID_HANDLE_VALUE)
		CloseHandle( m_hSysListener );
}

void Lobby2Client_360::SendMessage(Lobby2Message *msg)
{
	if (msg->ClientImpl((Lobby2Client*) this))
	{
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}
	else
	{
		// Won't be deleted by the user's call to Deref.
		msg->resultCode=L2RC_PROCESSING;
		msg->refCount++;
		msg->extendedResultCode=msg->m_Overlapped.InternalContext;
		PushDeferredCallback(msg);
	}
}
void Lobby2Client_360::Update(RakPeerInterface *peer)
{
	CheckForAcceptedInvitation();
	CheckForMuteListChanged();

	unsigned int i;
	i=0;
	for (; i < deferredCallbacks.Size(); i++)
	{
		Lobby2Message *lobby2Message = deferredCallbacks[i];
		if (lobby2Message->extendedResultCode!=0)
		{
			if (XHasOverlappedIoCompleted( &lobby2Message->m_Overlapped ))
			{
				HRESULT hr = XGetOverlappedExtendedError( &lobby2Message->m_Overlapped  );
				deferredCallbacks[i]->extendedResultCode=hr;
				if (!FAILED(deferredCallbacks[i]->extendedResultCode))
					CallCBWithResultCode(deferredCallbacks[i], L2RC_SUCCESS);
				else
					CallCBWithResultCode(deferredCallbacks[i], L2RC_GENERAL_ERROR);
				msgFactory->Dealloc(deferredCallbacks[i]);
				deferredCallbacks.RemoveAtIndex(i);
			}
			else
				i++;
		}
		else
			i++;
	}	
}
void Lobby2Client_360::PushDeferredCallback(Lobby2Message *msg)
{
	deferredCallbacks.Insert(msg->requestId, msg, true);
}
Lobby2Message *Lobby2Client_360::GetDeferredCallback(unsigned int requestId, bool peek)
{
	unsigned int index;
	bool objectExists;
	index = deferredCallbacks.GetIndexFromKey(requestId, &objectExists);
	if (objectExists)
	{
		Lobby2Message *msg = deferredCallbacks[index];
		if (peek==false)
			deferredCallbacks.RemoveAtIndex(index);
		return msg;
	}
	else
		return 0;
}
Lobby2Message *Lobby2Client_360::GetDeferredCallbackByMessageId(Lobby2MessageID messageId, bool peek)
{
	unsigned int index;
	for (index=0; index < deferredCallbacks.Size(); index++)
	{
		Lobby2MessageID id = deferredCallbacks[index]->GetID();
		printf("%i\n", (int) id);
		if (id==messageId)
		{
			Lobby2Message *msg = deferredCallbacks[index];
			if (peek==false)
				deferredCallbacks.RemoveAtIndex(index);
			return msg;
		}
	}
	return 0;
}
void Lobby2Client_360::CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc)
{
	if (msg)
	{
		msg->resultCode=rc;
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}	
}
void Lobby2Client_360::OnAttach(RakPeerInterface *peer)
{
	// Global pointer to use if callback does not have context data
	lobby2Client=this;
}
bool Lobby2Client_360::ClientStartup(void)
{

	// Start up Xbox Live functionality using default Secure Network Layer settings
	if( XOnlineStartup() != ERROR_SUCCESS )
		return false;

	// KevinJ: Move processing to lobby2Client, and callback via to L2MID_Notification_Console_LobbyGotRoomInvitation
	// Register our Live (invitation) listener
	m_hLiveListener = XNotifyCreateListener( XNOTIFY_LIVE );
	if( m_hLiveListener == NULL || m_hLiveListener == INVALID_HANDLE_VALUE )
		return false;

	// Register our System (mute list change) listener
	m_hSysListener = XNotifyCreateListener( XNOTIFY_SYSTEM );
	if( m_hSysListener == NULL || m_hSysListener == INVALID_HANDLE_VALUE )
		return false;

	return true;
}
void Lobby2Client_360::CheckForAcceptedInvitation(void)
{
	// Check for system notifications
	DWORD dwNotificationID;


	Notification_Console_LobbyGotRoomInvitation_360 lobbyGotRoomInvitation;
//	ULONG_PTR ulParam;

	if( XNotifyGetNext( m_hLiveListener, 0, &dwNotificationID, &lobbyGotRoomInvitation.owner ) )
	{
		switch( dwNotificationID )
		{
		case XN_LIVE_INVITE_ACCEPTED:
			XInviteGetAcceptedInfo( lobbyGotRoomInvitation.owner, &lobbyGotRoomInvitation.inviteInfo );
			CallCBWithResultCode(&lobbyGotRoomInvitation, L2RC_SUCCESS);
		}
	}
}
void Lobby2Client_360::CheckForMuteListChanged(void)
{
	DWORD dwNotificationID;
	ULONG_PTR ulParam;
	if( XNotifyGetNext( m_hSysListener, 0, &dwNotificationID, &ulParam ) &&
		dwNotificationID == XN_SYS_MUTELISTCHANGED )
	{
		Notification_Console_MuteListChanged_360 notification;
		CallCBWithResultCode(&notification, L2RC_SUCCESS);
	}

}
//bool Lobby2Client_360::IsUserSignedIn(int controllerIndex)
//{
//	return (bool) ATG::SignIn::IsUserSignedIn( controllerIndex );
//}
bool Lobby2Client_360::IsOperationPending(Lobby2MessageID messageId)
{
	unsigned int i;
	for (i=0; i < deferredCallbacks.Size(); i++)
		if (deferredCallbacks[i]->GetID()==messageId)
			return true;
	return false;
}
