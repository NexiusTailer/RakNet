#include "NatPunchthroughServer.h"
#include "SocketLayer.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "MTUSize.h"
#include "GetTime.h"

int NatPunchthroughServer::NatPunchthroughUserComp( const RakNetGUID &key, User * const &data )
{
	if (key < data->guid)
		return -1;
	if (key > data->guid)
		return 1;
	return 0;
}

NatPunchthroughServer::NatPunchthroughServer()
{
}
NatPunchthroughServer::~NatPunchthroughServer()
{
	unsigned int i;
	for (i=0; i < users.Size(); i++)
		RakNet::OP_DELETE(users[i], __FILE__, __LINE__ );
}

void NatPunchthroughServer::Update(void)
{
	CloseUnresponsiveAttempts();
}
PluginReceiveResult NatPunchthroughServer::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_NAT_PUNCHTHROUGH_REQUEST:
		OnNATPunchthroughRequest(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_NAT_GET_MOST_RECENT_PORT:
		OnGetMostRecentPort(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_NAT_CLIENT_READY:
		OnClientReady(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	return RR_CONTINUE_PROCESSING;
}
void NatPunchthroughServer::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) systemAddress;

	unsigned int i=0;

	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_CONNECTION_TO_TARGET_LOST);
	outgoingBs.Write(rakNetGUID);
	i=0;
	while (i < connectionAttempts.Size())
	{
		if (rakNetGUID==connectionAttempts[i].recipient->guid)
		{
			connectionAttempts[i].sender->isReady=true;
			rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].sender->guid),false);
			connectionAttempts.RemoveAtIndex(i);
		}
		else if (rakNetGUID==connectionAttempts[i].sender->guid)
		{
			connectionAttempts[i].recipient->isReady=true;
			connectionAttempts.RemoveAtIndex(i);
		}
		else
			i++;
	}

	bool objectExists;
	i = users.GetIndexFromKey(rakNetGUID, &objectExists);
	if (objectExists)
	{
		RakNet::OP_DELETE(users[i], __FILE__, __LINE__);
		users.RemoveAtIndex(i);
	}

	StartPendingPunchthrough();
}

void NatPunchthroughServer::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) systemAddress;
	(void) isIncoming;

	User *user = RakNet::OP_NEW<User>(__FILE__,__LINE__);
	user->guid=rakNetGUID;
	user->isReady=true;
	user->mostRecentPort=0;
	users.Insert(rakNetGUID, user, true);
}
void NatPunchthroughServer::OnNATPunchthroughRequest(Packet *packet)
{
	RakNet::BitStream outgoingBs;
	ConnectionAttempt ca;
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(sizeof(MessageID));
	RakNetGUID recipientGuid, senderGuid;
	incomingBs.Read(recipientGuid);
	senderGuid=packet->guid;
	unsigned int i;
	ca.sender=0;
	ca.recipient=0;
	bool objectExists;
	i = users.GetIndexFromKey(senderGuid, &objectExists);
	RakAssert(objectExists);
	ca.sender=users[i];
	i = users.GetIndexFromKey(recipientGuid, &objectExists);
	if (objectExists==false)
	{
		outgoingBs.Write((MessageID)ID_NAT_TARGET_NOT_CONNECTED);
		outgoingBs.Write(recipientGuid);
		rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
		return;
	}
	ca.recipient=users[i];

	unsigned int connectionAttemptIndex;
	connectionAttemptIndex = GetConnectionAttemptIndex(ca.recipient->guid, packet->guid);
	if (connectionAttemptIndex!=(unsigned int)-1)
	{
		outgoingBs.Write((MessageID)ID_NAT_ALREADY_IN_PROGRESS);
		outgoingBs.Write(recipientGuid);
		rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
		return;
	}

	ca.attemptPhase=ConnectionAttempt::NAT_ATTEMPT_PHASE_NOT_STARTED;
	ca.startTime=0;

	// Track this connection attempt. After both systems have sent to the sockets, we tell them to connect at a given time.
	connectionAttempts.Insert(ca, __FILE__, __LINE__);

	StartPendingPunchthrough();
}

unsigned int NatPunchthroughServer::GetConnectionAttemptIndex(RakNetGUID g1, RakNetGUID g2)
{
	unsigned int i;
	for (i=0; i < connectionAttempts.Size(); i++)
	{
		if (
			(g1==connectionAttempts[i].sender->guid &&
			g2==connectionAttempts[i].recipient->guid)
			||
			(g1==connectionAttempts[i].recipient->guid &&
			g2==connectionAttempts[i].sender->guid)
			)
			return i;
	}
	return (unsigned int) -1;
}
void NatPunchthroughServer::OnGetMostRecentPort(Packet *packet)
{
	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	unsigned short mostRecentPort;
	bsIn.Read(mostRecentPort);
	unsigned int i;
	bool objectExists;
	i = users.GetIndexFromKey(packet->guid, &objectExists);
	if (objectExists)
	{
		users[i]->mostRecentPort=mostRecentPort;
		SendTimestamps();
	}
}
void NatPunchthroughServer::OnClientReady(Packet *packet)
{
	unsigned int i;
	bool objectExists;
	i = users.GetIndexFromKey(packet->guid, &objectExists);
	if (objectExists)
	{
		users[i]->isReady=true;
		StartPendingPunchthrough();
	}
}
void NatPunchthroughServer::StartPendingPunchthrough(void)
{
	unsigned int i;
	RakNetTime time=RakNet::GetTime();
	for (i=0; i < connectionAttempts.Size(); i++)
	{
		if (connectionAttempts[i].attemptPhase==ConnectionAttempt::NAT_ATTEMPT_PHASE_NOT_STARTED &&
			connectionAttempts[i].sender->isReady==true &&
			connectionAttempts[i].recipient->isReady==true)
		{
			connectionAttempts[i].sender->isReady=false;
			connectionAttempts[i].recipient->isReady=false;
			connectionAttempts[i].attemptPhase=ConnectionAttempt::NAT_ATTEMPT_PHASE_GETTING_RECENT_PORTS;
			connectionAttempts[i].startTime=time;

			connectionAttempts[i].sender->mostRecentPort=0;
			connectionAttempts[i].recipient->mostRecentPort=0;

			RakNet::BitStream outgoingBs;
			outgoingBs.Write((MessageID)ID_NAT_GET_MOST_RECENT_PORT);
			SystemAddress sa;
			sa = rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].sender->guid);
			rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,sa,false);
			sa = rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].recipient->guid);
			rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,sa,false);
		}
	}
}
void NatPunchthroughServer::CloseUnresponsiveAttempts(void)
{
	unsigned int i;
	RakNetTime time=RakNet::GetTime();
	i=0;
	bool readyStateChanged=false;
	while (i < connectionAttempts.Size())
	{
		if (connectionAttempts[i].attemptPhase!=ConnectionAttempt::NAT_ATTEMPT_PHASE_NOT_STARTED &&
			time > connectionAttempts[i].startTime &&
#ifdef _DEBUG
			time - connectionAttempts[i].startTime > 15000
#else
			time - connectionAttempts[i].startTime > 5000
#endif
			)
		{
			RakNet::BitStream outgoingBs;
			// that other system might not be running the plugin
			outgoingBs.Write((MessageID)ID_NAT_TARGET_UNRESPONSIVE);
			outgoingBs.Write(connectionAttempts[i].recipient->guid);
			rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].sender->guid),false);
			connectionAttempts[i].sender->isReady=true;
			connectionAttempts[i].recipient->isReady=true;
			connectionAttempts.RemoveAtIndex(i);
			readyStateChanged=true;
		}
		else
			i++;
	}

	if (readyStateChanged)
		StartPendingPunchthrough();
}
void NatPunchthroughServer::SendTimestamps(void)
{
	unsigned int i,j;
	RakNetTime time=RakNet::GetTime();

	i=0;
	while (i < connectionAttempts.Size())
	{
		if (connectionAttempts[i].attemptPhase==ConnectionAttempt::NAT_ATTEMPT_PHASE_GETTING_RECENT_PORTS &&
			connectionAttempts[i].sender->mostRecentPort!=0 &&
			connectionAttempts[i].recipient->mostRecentPort!=0)
		{
			SystemAddress senderSystemAddress = rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].sender->guid);
			SystemAddress recipientSystemAddress = rakPeerInterface->GetSystemAddressFromGuid(connectionAttempts[i].recipient->guid);
			if (senderSystemAddress!=UNASSIGNED_SYSTEM_ADDRESS && recipientSystemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
			{
				SystemAddress recipientTargetAddress = recipientSystemAddress;
				SystemAddress senderTargetAddress = senderSystemAddress;
				recipientTargetAddress.port=connectionAttempts[i].recipient->mostRecentPort;
				senderTargetAddress.port=connectionAttempts[i].sender->mostRecentPort;

				// Pick a time far enough in the future that both systems will have gotten the message
				int targetPing = rakPeerInterface->GetAveragePing(recipientTargetAddress);
				int senderPing = rakPeerInterface->GetAveragePing(senderSystemAddress);
				RakNetTime simultaneousAttemptTime;
				if (targetPing==-1 || senderPing==-1)
					simultaneousAttemptTime = time + 1500;
				else
				{
					int largerPing = targetPing > senderPing ? targetPing : senderPing;
					if (largerPing * 4 < 100)
						simultaneousAttemptTime = time + 100;
					else
						simultaneousAttemptTime = time + (largerPing * 4);
				}

				// Send to recipient timestamped message to connect at time
				RakNet::BitStream bsOut;
				bsOut.Write((MessageID)ID_TIMESTAMP);
				bsOut.Write(simultaneousAttemptTime);
				bsOut.Write((MessageID)ID_NAT_CONNECT_AT_TIME);
				bsOut.Write(senderTargetAddress); // Public IP, using most recent port
				for (j=0; j < MAXIMUM_NUMBER_OF_INTERNAL_IDS; j++) // Internal IP
					bsOut.Write(rakPeerInterface->GetInternalID(senderSystemAddress,j));
				bsOut.Write(connectionAttempts[i].sender->guid);
				bsOut.Write(false);
				rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,recipientSystemAddress,false);

				// Same for sender
				bsOut.Reset();
				bsOut.Write((MessageID)ID_TIMESTAMP);
				bsOut.Write(simultaneousAttemptTime);
				bsOut.Write((MessageID)ID_NAT_CONNECT_AT_TIME);
				bsOut.Write(recipientTargetAddress); // Public IP, using most recent port
				for (j=0; j < MAXIMUM_NUMBER_OF_INTERNAL_IDS; j++) // Internal IP
					bsOut.Write(rakPeerInterface->GetInternalID(recipientSystemAddress,j));						
				bsOut.Write(connectionAttempts[i].recipient->guid);
				bsOut.Write(true);
				rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,senderSystemAddress,false);

				// Done with this attempt. Both systems are busy so won't do anything else until they notify us
				connectionAttempts.RemoveAtIndex(i);
			}			
		}
		else
			i++;
	}
}
