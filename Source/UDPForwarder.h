/// \file
/// \brief Forwards UDP datagrams. Independent of RakNet's protocol.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPForwarder==1

#ifndef __UDP_FORWARDER_H
#define __UDP_FORWARDER_H

#include "Export.h"
#include "RakNetTypes.h"
#include "SocketIncludes.h"
#include "UDPProxyCommon.h"
#include "SimpleMutex.h"
#include "RakString.h"
#include "RakThread.h"
#include "DS_Queue.h"
#include "DS_OrderedList.h"
#include "LocklessTypes.h"
#include "DS_ThreadsafeAllocatingQueue.h"

namespace RakNet
{

struct RakNetSocket;

enum UDPForwarderResult
{
	UDPFORWARDER_FORWARDING_ALREADY_EXISTS,
	UDPFORWARDER_NO_SOCKETS,
	UDPFORWARDER_BIND_FAILED,
	UDPFORWARDER_INVALID_PARAMETERS,
	UDPFORWARDER_NOT_RUNNING,
	UDPFORWARDER_SUCCESS,
	UDPFORWARDER_RESULT_COUNT
};

/// \brief Forwards UDP datagrams. Independent of RakNet's protocol.
/// \ingroup NAT_PUNCHTHROUGH_GROUP
class RAK_DLL_EXPORT UDPForwarder
{
public:
	UDPForwarder();
	virtual ~UDPForwarder();

	/// Starts the system.
	/// Required to call before StartForwarding
	void Startup(void);

	/// Stops the system, and frees all sockets
	void Shutdown(void);

	/// Sets the maximum number of forwarding entries allowed
	/// Set according to your available bandwidth and the estimated average bandwidth per forwarded address.
	/// \param[in] maxEntries The maximum number of simultaneous forwarding entries. Defaults to 64 (32 connections)
	void SetMaxForwardEntries(unsigned short maxEntries);

	/// \return The \a maxEntries parameter passed to SetMaxForwardEntries(), or the default if it was never called
	int GetMaxForwardEntries(void) const;

	/// \return How many entries have been used
	int GetUsedForwardEntries(void) const;

	/// Forwards datagrams from source to destination, and vice-versa
	/// Does nothing if this forward entry already exists via a previous call
	/// \pre Call Startup()
	/// \note RakNet's protocol will ensure a message is sent at least every 15 seconds, so if routing RakNet messages, it is a reasonable value for timeoutOnNoDataMS, plus an some extra seconds for latency
	/// \param[in] source The source IP and port
	/// \param[in] destination Where to forward to (and vice-versa)
	/// \param[in] timeoutOnNoDataMS If no messages are forwarded for this many MS, then automatically remove this entry.
	/// \param[in] forceHostAddress Force binding on a particular address. 0 to use any.
	/// \param[in] socketFamily IP version: For IPV4, use AF_INET (default). For IPV6, use AF_INET6. To autoselect, use AF_UNSPEC.
	/// \param[out] forwardingPort New opened port for forwarding
	/// \param[out] forwardingSocket New opened socket for forwarding
	/// \return UDPForwarderResult
	UDPForwarderResult StartForwarding(
		SystemAddress source, SystemAddress destination, RakNet::TimeMS timeoutOnNoDataMS,
		const char *forceHostAddress, unsigned short socketFamily,
		unsigned short *forwardingPort, RakNetSocket **forwardingSocket);

	/// No longer forward datagrams from source to destination
	/// \param[in] source The source IP and port
	/// \param[in] destination Where to forward to
	void StopForwarding(SystemAddress source, SystemAddress destination);


	struct ForwardEntry
	{
		ForwardEntry();
		~ForwardEntry();
		SystemAddress addr1Unconfirmed, addr2Unconfirmed, addr1Confirmed, addr2Confirmed;
		RakNet::TimeMS timeLastDatagramForwarded;
		RakNetSocket* socket;
		RakNet::TimeMS timeoutOnNoDataMS;
		short socketFamily;
	};


protected:
	friend RAK_THREAD_DECLARATION(UpdateUDPForwarderGlobal);

	void UpdateUDPForwarder(void);
	void RecvFrom(RakNet::TimeMS curTime, ForwardEntry *forwardEntry);

	struct StartForwardingInputStruct
	{
		SystemAddress source;
		SystemAddress destination;
		RakNet::TimeMS timeoutOnNoDataMS;
		RakString forceHostAddress;
		unsigned short socketFamily;
		unsigned int inputId;
	};

	DataStructures::ThreadsafeAllocatingQueue<StartForwardingInputStruct> startForwardingInput;

	struct StartForwardingOutputStruct
	{
		unsigned short forwardingPort;
		RakNetSocket* forwardingSocket;
		UDPForwarderResult result;
		unsigned int inputId;
	};
	DataStructures::Queue<StartForwardingOutputStruct> startForwardingOutput;
	SimpleMutex startForwardingOutputMutex;

	struct StopForwardingStruct
	{
		SystemAddress source;
		SystemAddress destination;
	};
	DataStructures::ThreadsafeAllocatingQueue<StopForwardingStruct> stopForwardingCommands;
	unsigned int nextInputId;

	// New entries are added to forwardListNotUpdated
	DataStructures::List<ForwardEntry*> forwardListNotUpdated;
//	SimpleMutex forwardListNotUpdatedMutex;

	unsigned short maxForwardEntries;
	RakNet::LocklessUint32_t isRunning, threadRunning;

};

} // End namespace

#endif

#endif // #if _RAKNET_SUPPORT_UDPForwarder==1
