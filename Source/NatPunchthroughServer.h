/// \file
/// \brief Contains the NAT-punchthrough plugin for the server.
///
/// This file is part of RakNet Copyright 2003 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation; either version 2 of the License, or (at your
/// option) any later version.

#ifndef __NAT_PUNCHTHROUGH_SERVER_H
#define __NAT_PUNCHTHROUGH_SERVER_H

#include "RakNetTypes.h"
#include "Export.h"
#include "PluginInterface2.h"
#include "PacketPriority.h"
#include "SocketIncludes.h"
#include "DS_OrderedList.h"

class RakPeerInterface;
struct Packet;

/// \defgroup NAT_PUNCHTHROUGH_GROUP NatPunchthrough
/// \ingroup PLUGINS_GROUP

/// Maintain connection to NatPunchthroughServer to process incoming connection attempts through NatPunchthroughClient
/// Server maintains two sockets clients can connect to so as to estimate the next port choice
/// Server tells other client about port estimate, current public port to the server, and a time to start connection attempts
/// \ingroup NAT_PUNCHTHROUGH_GROUP
class RAK_DLL_EXPORT NatPunchthroughServer : public PluginInterface2
{
public:
	/// Constructor
	NatPunchthroughServer();

	/// Destructor
	virtual ~NatPunchthroughServer();

	/// \internal For plugin handling
	virtual void Update(void);

	/// \internal For plugin handling
	virtual PluginReceiveResult OnReceive(Packet *packet);

	/// \internal For plugin handling
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );
	virtual void OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming);

	// Each connected user has a ready state. Ready means ready for nat punchthrough.
	struct User
	{
		RakNetGUID guid;
		bool isReady;
		unsigned short mostRecentPort;
	};
	static int NatPunchthroughUserComp( const RakNetGUID &key, User * const &data );
protected:
	void OnNATPunchthroughRequest(Packet *packet);	

	//DataStructures::List<User*> users;
	DataStructures::OrderedList<RakNetGUID, User*, NatPunchthroughServer::NatPunchthroughUserComp> users;

	struct ConnectionAttempt
	{
		User *sender, *recipient;
		RakNetTime startTime;
		bool inProgress;
		enum
		{
			NAT_ATTEMPT_PHASE_NOT_STARTED,
			NAT_ATTEMPT_PHASE_GETTING_RECENT_PORTS,
		} attemptPhase;
	};
	DataStructures::List<ConnectionAttempt> connectionAttempts;
	unsigned int GetConnectionAttemptIndex(RakNetGUID g1, RakNetGUID g2);
	void OnGetMostRecentPort(Packet *packet);
	void OnClientReady(Packet *packet);

	void SendTimestamps(void);
	void CloseUnresponsiveAttempts(void);
	void StartPendingPunchthrough(void);

};

#endif
