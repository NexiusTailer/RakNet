
/// \file
/// \brief Contains the NAT-punchthrough plugin for the client.
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

#ifndef __NAT_PUNCHTHROUGH_CLIENT_H
#define __NAT_PUNCHTHROUGH_CLIENT_H

#include "RakNetTypes.h"
#include "Export.h"
#include "PluginInterface2.h"
#include "PacketPriority.h"
#include "SocketIncludes.h"
#include "DS_List.h"

// Trendnet TEW-632BRP sometimes starts at port 1024 and increments sequentially.
// Zonnet zsr1134we. Replies go out on the net, but are always absorbed by the remote router??
// Dlink ebr2310 to Trendnet ok
// Trendnet TEW-652BRP to Trendnet 632BRP OK
// Trendnet TEW-632BRP to Trendnet 632BRP OK
// Buffalo WHR-HP-G54 OK
// Netgear WGR614 ok

class RakPeerInterface;
struct Packet;
class PacketLogger;
	
struct RAK_DLL_EXPORT PunchthroughConfiguration
{
	PunchthroughConfiguration() {
		TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL=50;
		TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL=125;
		UDP_SENDS_PER_PORT=5;
		INTERNAL_IP_WAIT_AFTER_ATTEMPTS=250;
		MAX_PREDICTIVE_PORT_RANGE=6;
		EXTERNAL_IP_WAIT_BETWEEN_PORTS=1000;
		EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS=EXTERNAL_IP_WAIT_BETWEEN_PORTS;
	}

	// How much time between each UDP send
	RakNetTimeMS TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL;
	RakNetTimeMS TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL;

	// How many tries for one port before giving up and going to the next port
	int UDP_SENDS_PER_PORT;

	// After giving up on one internal port, how long to wait before trying the next port
	int INTERNAL_IP_WAIT_AFTER_ATTEMPTS;

	// How many external ports to try past the last known starting port
	int MAX_PREDICTIVE_PORT_RANGE;

	// After giving up on one external  port, how long to wait before trying the next port
	int EXTERNAL_IP_WAIT_BETWEEN_PORTS;

	// After trying all external ports, how long to wait before returning ID_NAT_PUNCHTHROUGH_FAILED
	int EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS;
};

struct NatPunchthroughDebugInterface
{
	virtual void OnClientMessage(const char *msg)=0;
};

struct NatPunchthroughDebugInterface_Printf : public NatPunchthroughDebugInterface
{
	virtual void OnClientMessage(const char *msg);
};

struct NatPunchthroughDebugInterface_PacketLogger : public NatPunchthroughDebugInterface
{
	// Set to non-zero to write to the packetlogger!
	PacketLogger *pl;

	NatPunchthroughDebugInterface_PacketLogger() {pl=0;}
	~NatPunchthroughDebugInterface_PacketLogger() {}
	virtual void OnClientMessage(const char *msg);
};

/// Maintain connection to NatPunchthroughServer to process incoming connection attempts through NatPunchthroughClient
/// Client will send datagrams to port to estimate next port
/// Will simultaneously connect with another client once ports are estimated.
/// \ingroup NAT_PUNCHTHROUGH_GROUP
class RAK_DLL_EXPORT NatPunchthroughClient : public PluginInterface2
{
public:
	NatPunchthroughClient();
	~NatPunchthroughClient();

	// Punchthrough a NAT. Doesn't connect, just tries to setup the routing table
	bool OpenNAT(RakNetGUID destination, SystemAddress facilitator);

	// Modify the system configuration if desired
	// Don't modify the variables in the structure while punchthrough is in progress
	PunchthroughConfiguration* GetPunchthroughConfiguration(void);

	/// Sets a callback to be called with debug messages
	/// \param[in] i Pointer to an interface. The pointer is stored, so don't delete it while in progress. Pass 0 to clear.
	void SetDebugInterface(NatPunchthroughDebugInterface *i);

	/// \internal For plugin handling
	virtual void Update(void);

	/// \internal For plugin handling
	virtual PluginReceiveResult OnReceive(Packet *packet);

	/// \internal For plugin handling
	virtual void OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming);

	/// \internal For plugin handling
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );

	virtual void OnAttach(void);
	virtual void OnDetach(void);
	virtual void OnShutdown(void);
	void Clear(void);

protected:
	unsigned short mostRecentNewExternalPort;
	void OnGetMostRecentPort(Packet *packet);
	void OnConnectAtTime(Packet *packet);
	unsigned int GetPendingOpenNATIndex(RakNetGUID destination, SystemAddress facilitator);
	void SendPunchthrough(RakNetGUID destination, SystemAddress facilitator);
	void SendTTL(SystemAddress sa);
	void SendOutOfBand(SystemAddress sa, MessageID oobId);
	void OnPunchthroughFailure(void);
	void OnReadyForNextPunchthrough(void);
	void PushFailure(void);
	//void ProcessNextPunchthroughQueue(void);

	/*
	struct PendingOpenNAT
	{
		RakNetGUID destination;
		SystemAddress facilitator;
	};
	DataStructures::List<PendingOpenNAT> pendingOpenNAT;
	*/

	struct SendPing
	{
		RakNetTime nextActionTime;
		SystemAddress targetAddress;
		SystemAddress facilitator;
		SystemAddress internalIds[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
		RakNetGUID targetGuid;
		bool weAreSender;
		int attemptCount;
		int retryCount;
		int punchingFixedPortAttempts; // only used for TestMode::PUNCHING_FIXED_PORT
		// Give priority to internal IP addresses because if we are on a LAN, we don't want to try to connect through the internet
		enum TestMode
		{
			TESTING_INTERNAL_IPS,
			WAITING_FOR_INTERNAL_IPS_RESPONSE,
			TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT,
			TESTING_EXTERNAL_IPS_FROM_1024,
			WAITING_AFTER_ALL_ATTEMPTS,

			// The trendnet remaps the remote port to 1024.
			// If you continue punching on a different port for the same IP it bans you and the communication becomes unidirectioal
			PUNCHING_FIXED_PORT,

			// try port 1024-1028
		} testMode;
	} sp;

	PunchthroughConfiguration pc;
	NatPunchthroughDebugInterface *natPunchthroughDebugInterface;

	// The first time we fail a NAT attempt, we add it to failedAttemptList and try again, since sometimes trying again later fixes the problem
	// The second time we fail, we return ID_NAT_PUNCHTHROUGH_FAILED
	struct AddrAndGuid
	{
		SystemAddress addr;
		RakNetGUID guid;
	};
	DataStructures::List<AddrAndGuid> failedAttemptList;
};

#endif
