#pragma once

#include "RakString.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakPeer.h"
#include "RakSleep.h"
#include "RakNetTime.h"
#include "GetTime.h"
#include "DebugTools.h"
	#include "RakTimer.h"

class CommonFunctions
{
public:
	CommonFunctions(void);
	~CommonFunctions(void);

	static bool WaitAndConnect(RakPeerInterface *peer,char* ip,unsigned short int port,int millisecondsToWait);
	static bool WaitForMessageWithID(RakPeerInterface *reciever,int id,int millisecondsToWait);
	static Packet * WaitAndReturnMessageWithID(RakPeerInterface *reciever,int id,int millisecondsToWait);
	static void DisconnectAndWait(RakPeerInterface *peer,char* ip,unsigned short int port);

};
