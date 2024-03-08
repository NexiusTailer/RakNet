/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "FullyConnectedMesh2.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakAssert.h"
#include "GetTime.h"

DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(FullyConnectedMesh2::RemoteSystemWithTime, RakNetGUID, guid)

FullyConnectedMesh2::FullyConnectedMesh2()
{
	startupTime=0;
	lastReceivedElapsedRuntime=0;
	lastRemoteHostGuid=UNASSIGNED_RAKNET_GUID;
	lastRemoteHostAddress=UNASSIGNED_SYSTEM_ADDRESS;
}
FullyConnectedMesh2::~FullyConnectedMesh2()
{
	Clear();
}
RakNetGUID FullyConnectedMesh2::GetConnectedHost(void) const
{
	if (remoteSystemList.GetSize()==0)
		return UNASSIGNED_RAKNET_GUID;

	return lastRemoteHostGuid;
}
RakNetGUID FullyConnectedMesh2::GetHostSystem(void) const
{
	return lastRemoteHostGuid;
}
void FullyConnectedMesh2::Update(void)
{
	// If got lastReceivedElapsedRuntime and one second passed, move all newElapsedRuntime to elapsedRuntime
	RakNetTime curTime=RakNet::GetTime();
	if (lastReceivedElapsedRuntime && curTime > lastReceivedElapsedRuntime && lastReceivedElapsedRuntime > lastReceivedElapsedRuntime+1000)
	{
		DataStructures::DefaultIndexType lastHostIndex=CalculateHostSystemIndex();
		lastReceivedElapsedRuntime=0;
		DataStructures::DefaultIndexType i;
		for (i=1; i < remoteSystemList.GetSize(); i++)
		{
			remoteSystemList[i]->elapsedRuntime=remoteSystemList[i]->newElapsedRuntime;
		}
		DataStructures::DefaultIndexType newHostIndex=CalculateHostSystemIndex();
		if (lastHostIndex!=newHostIndex)
		{
			if (newHostIndex==(DataStructures::DefaultIndexType)-1)
			{
				lastRemoteHostGuid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
				lastRemoteHostAddress=rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS);
			}
			else
			{
				lastRemoteHostGuid=remoteSystemList[newHostIndex]->guid;
				lastRemoteHostAddress=remoteSystemList[newHostIndex]->systemAddress;
			}
			PushNewHost();
		}
	}

}
PluginReceiveResult FullyConnectedMesh2::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		{
			unsigned int count;
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(MessageID));
			bsIn.Read(count);
			SystemAddress remoteAddress;
			RakNetGUID remoteGuid;
			char str[64];
			for (unsigned int i=0; i < count; i++)
			{
				bsIn.Read(remoteAddress);
				bsIn.Read(remoteGuid);
				remoteAddress.ToString(false,str);
				rakPeerInterface->Connect(str,remoteAddress.port,0,0);
			}
		}
		break;

		case ID_FCM2_ELAPSED_RUNTIME:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(MessageID));
			RakNetTimeUS elapsedRuntime;
			bsIn.Read(elapsedRuntime);
			DataStructures::DefaultIndexType i = remoteSystemList.GetIndexOf(packet->guid);
			if (i!=(DataStructures::DefaultIndexType)-1)
			{
				remoteSystemList[i]->newElapsedRuntime=elapsedRuntime;
				lastReceivedElapsedRuntime=RakNet::GetTime();
			}
		}
		break;
	}
	return RR_CONTINUE_PROCESSING;
}
void FullyConnectedMesh2::OnStartup(void)
{
	startupTime=RakNet::GetTimeUS();
}
void FullyConnectedMesh2::OnAttach(void)
{
	// In case Startup() was called first
	if (rakPeerInterface->IsActive())
		startupTime=RakNet::GetTimeUS();
	lastRemoteHostGuid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	lastRemoteHostAddress=rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS);
}
void FullyConnectedMesh2::OnShutdown(void)
{
	Clear();
	startupTime=0;
	lastReceivedElapsedRuntime=0;
}
void FullyConnectedMesh2::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) systemAddress;

	DataStructures::DefaultIndexType droppedSystemIndex = remoteSystemList.GetIndexOf(rakNetGUID);
	if (droppedSystemIndex==(DataStructures::DefaultIndexType)-1)
		return;

	if (remoteSystemList[droppedSystemIndex]->guid==lastRemoteHostGuid)
	{
		DataStructures::DefaultIndexType newHostIndex=CalculateHostSystemIndex();
		// New host
		if (newHostIndex==(DataStructures::DefaultIndexType)-1)
		{
			lastRemoteHostGuid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
			lastRemoteHostAddress=rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS);
		}
		else
		{
			lastRemoteHostGuid=remoteSystemList[newHostIndex]->guid;
			lastRemoteHostAddress=remoteSystemList[newHostIndex]->systemAddress;
		}
		PushNewHost();
	}
}
void FullyConnectedMesh2::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) isIncoming;
	(void) rakNetGUID;
	(void) systemAddress;

	RakNetTimeUS curTime=RakNet::GetTimeUS();
	RakNetTimeUS elapsedTime;
	if (curTime>startupTime)
		elapsedTime=curTime-startupTime;
	else
		elapsedTime=0;
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_FCM2_ELAPSED_RUNTIME);
	bsOut.Write(elapsedTime);
	// Intentional broadcast
	DataStructures::DefaultIndexType i;
	for (i=0; i < remoteSystemList.GetSize(); i++)
	{
		rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,remoteSystemList[i]->systemAddress,false);
	}

}
void FullyConnectedMesh2::Clear(void)
{
	remoteSystemList.ClearPointers(false,__FILE__,__LINE__);
}
void FullyConnectedMesh2::PushNewHost(void)
{
	Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID));
	p->data[0]=ID_FCM2_NEW_HOST;
	p->systemAddress=lastRemoteHostAddress;
	p->systemIndex=(SystemIndex)-1;
	p->guid=lastRemoteHostGuid;
	rakPeerInterface->PushBackPacket(p, true);
}

DataStructures::DefaultIndexType FullyConnectedMesh2::CalculateHostSystemIndex(void) const
{
	if (remoteSystemList.GetSize()==0)
		return (DataStructures::DefaultIndexType)-1;

	// Return system with lowest time. If none, return ourselves
	DataStructures::DefaultIndexType i, lowestRuntimeIndex=0;
	RakNetTimeUS lowestRuntime=remoteSystemList[0]->elapsedRuntime;
	for (i=1; i < remoteSystemList.GetSize(); i++)
	{
		if (remoteSystemList[i]->elapsedRuntime < lowestRuntime)
		{
			lowestRuntime=remoteSystemList[i]->elapsedRuntime;
			lowestRuntimeIndex=i;
		}
	}

	return lowestRuntimeIndex;
}
