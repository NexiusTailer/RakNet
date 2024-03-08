#include "FCMHost.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakPeerInterface.h"

FCMHost::FCMHost()
{
	rakPeer=0;
	autoAddConnections=false;
	DataStructures::Map<FCMHostGroupID, DataStructures::List<RakNetGUID>* >::IMPLEMENT_DEFAULT_COMPARISON();
}
FCMHost::~FCMHost()
{
	Clear();
}
void FCMHost::AddParticipant(RakNetGUID guid, FCMHostGroupID groupId)
{
	// ID_FCM_HOST_ADD_PARTICIPANT_REQUEST if not already there
	if (participantList.Has(groupId))
	{
		unsigned long index = participantList.Get(groupId)->GetIndexOf(guid);
		if (index!=MAX_UNSIGNED_LONG)
			return;
	}
 
	// Query the remote system to add us as a participant. If we are host of a group, tell the other system about that as well.
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_FCM_HOST_ADD_PARTICIPANT_REQUEST);
	bs.Write(groupId);
	if (AmIConnectedHost(groupId))
	{
		bs.Write(true);
		WriteHostListGroup(groupId, &bs);
	}
	else
	{
		bs.Write(false);
	}
	rakPeer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, rakPeer->GetSystemAddressFromGuid(guid), false);
}
void FCMHost::RemoveParticipantFromAllGroups(RakNetGUID guid)
{
	int i;
	for (i=hostListsGroup.Size(); i > 0; i--)
	{
		RemoveParticipantFromGroup(guid, hostListsGroup.GetKeyAtIndex(i-1));
	}
}
void FCMHost::RemoveParticipantFromGroup(RakNetGUID guid, FCMHostGroupID groupId)
{
	DataStructures::List<RakNetGUID>* theList, *theList2;
	if (participantList.Has(groupId))
	{
		theList=participantList.Get(groupId);
		unsigned long index = theList->GetIndexOf(guid);
		if (index!=MAX_UNSIGNED_LONG)
		{
			theList->RemoveAtIndex(index);
			if (theList->Size()==0)
			{
				RakNet::OP_DELETE(theList);
				participantList.Delete(groupId);
			}
		}
	}

	if (hostListsGroup.Has(groupId))
	{
		theList=hostListsGroup.Get(groupId);
		unsigned long index = theList->GetIndexOf(guid);
		if (index!=MAX_UNSIGNED_LONG)
		{
			theList->RemoveAtIndex(index);
			if (theList->Size()<=1)
			{
				if (index==0)
				{
					// I am hosting myself
					NotifyUserOfHost(groupId, rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
				}

				RakNet::OP_DELETE(theList);
				hostListsGroup.Delete(groupId);
			}
			// Old host dropped, got a new host
			else if (theList->Size()>1 &&
				index==0)
			{
				NotifyUserOfHost(groupId, (*theList)[0]);

				if ((*theList)[0]==rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
				{
					// Add all participants in this group that are not in hostListsGroup.
					// These are participants we know about, but the previous host didn't
					unsigned int j;
					if (participantList.Has(groupId))
					{
						theList2=participantList.Get(groupId);
						for (j=0; j < theList2->Size(); j++)
						{
							if (theList->GetIndexOf((*theList2)[j])==MAX_UNSIGNED_LONG)
								theList->Insert((*theList2)[j]);
						}
					}
				}
				else
				{
					// Got a new host.
				}
			}
		}
	}
}
void FCMHost::RemoveParticipant(RakNetGUID guid, FCMHostGroupID groupId)
{
	RemoveParticipantFromGroup(guid, groupId);
}
void FCMHost::SetAutoAddNewConnections(bool autoAdd, FCMHostGroupID groupId)
{
	autoAddConnections=autoAdd;
	autoAddConnectionsTargetGroup=groupId;
}
RakNetGUID FCMHost::GetHost(FCMHostGroupID groupId) const
{
	unsigned int i;
	if (hostListsGroup.Has(groupId)==false)
	{
		return rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	}
	else
	{
		DataStructures::List<RakNetGUID>* theList;
		theList=hostListsGroup.Get(groupId);
		for (i=0; i < theList->Size(); i++)
		{
			if (rakPeer->IsConnected(rakPeer->GetSystemAddressFromGuid((*theList)[i]), true, true)==true)
				return (*theList)[i];

			if (rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS)==(*theList)[i])
				return (*theList)[i];
		}
	}
	return rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
}
bool FCMHost::AmIHost(FCMHostGroupID groupId) const
{
	return GetHost(groupId)==rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
}
bool FCMHost::AmIConnectedHost(FCMHostGroupID groupId) const
{
	if (HasConnectedHost())
		return AmIHost(groupId);
	return false;
}
bool FCMHost::HasConnectedHost(FCMHostGroupID groupId) const
{
	return hostListsGroup.Has(groupId) &&
		hostListsGroup.Get(groupId)->Size()>1;
}

void FCMHost::OnAttach(RakPeerInterface *peer)
{
	rakPeer=peer;
}
PluginReceiveResult FCMHost::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_NEW_INCOMING_CONNECTION:
	case ID_CONNECTION_REQUEST_ACCEPTED:
		OnNewConnection(peer, packet);
		return RR_CONTINUE_PROCESSING;
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_LOST:
		OnCloseConnection(peer, packet->systemAddress);
		return RR_CONTINUE_PROCESSING;
	case ID_FCM_HOST_ADD_PARTICIPANT_REQUEST:
		OnAddParticipantRequest(peer, packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FCM_HOST_LIST_UPDATE:
		OnHostListUpdate(peer, packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}

	return RR_CONTINUE_PROCESSING;
}
void FCMHost::OnShutdown(RakPeerInterface *peer)
{
	(void) peer;
	Clear();
}
void FCMHost::OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress)
{
	RemoveParticipantFromAllGroups(peer->GetGuidFromSystemAddress(systemAddress));
}
void FCMHost::Clear(void)
{
	unsigned int i;
	for (i=0; i < hostListsGroup.Size(); i++)
		RakNet::OP_DELETE(hostListsGroup[i]);
	hostListsGroup.Clear();
	for (i=0; i < participantList.Size(); i++)
		RakNet::OP_DELETE(participantList[i]);
	participantList.Clear();
}

void FCMHost::OnAddParticipantRequest(RakPeerInterface *peer, Packet *packet)
{
	(void) peer;

	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(1); // ID_FCM_HOST_ADD_PARTICIPANT_REQUEST
	FCMHostGroupID groupId;
	incomingBs.Read(groupId);
	bool isRemoteSystemHost;
	incomingBs.Read(isRemoteSystemHost);
	DataStructures::List<RakNetGUID> remoteHostList;
	if (isRemoteSystemHost)
		ReadHostListGroup(&incomingBs, &remoteHostList);

	if (remoteHostList.Size()>0)
	{
		if (AmIConnectedHost(groupId))
		{
			// Both systems are hosts
			// Lowest guid wins
			if (rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS) < remoteHostList[0])
			{
				// I have lowest guid
				// Append unique elements of other list to my list
				unsigned int i;
				DataStructures::List<RakNetGUID>* theList = hostListsGroup.Get(groupId);
				for (i=0; i < remoteHostList.Size(); i++)
				{
					if (theList->GetIndexOf(remoteHostList[i])==MAX_UNSIGNED_LONG)
						theList->Insert(remoteHostList[i]);
				}

				// Broadcast list
				BroadcastGroupList(groupId);
			}
			else
			{
				// They have the lowest guid
				// They will append my list to their list
				// They will broadcast to us the updated list
				// (Do nothing)
			}

		}
		else
		{
			// Only the other system is a host
			// When this system sends out ID_FCM_HOST_ADD_PARTICIPANT_REQUEST to the other system, we will be added and notified of the new list
			// (Do nothing)
		}
	}
	else if (AmIConnectedHost(groupId))
	{
		// Add other system
		DataStructures::List<RakNetGUID>* theList = hostListsGroup.Get(groupId);
		theList->Insert(packet->guid);

		// Broadcast new list to all systems
		BroadcastGroupList(groupId);
	}
	else if (isRemoteSystemHost==false && hostListsGroup.Has(groupId)==false)
	{
		// Neither system is host
		// Lowest GUID host wins
		// Lowest GUID host adds to list, and tells remote system of this list
		RakNetGUID myGuid = rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
		if (myGuid < packet->guid)
		{
			DataStructures::List<RakNetGUID>* theList = RakNet::OP_NEW< DataStructures::List<RakNetGUID> >();
			theList->Insert(myGuid);
			theList->Insert(packet->guid);
			hostListsGroup.Set(groupId, theList);

			// Broadcast new list to the requesting system
			BroadcastGroupList(groupId);

			// Tell the system that we are now a connected host
			NotifyUserOfHost(groupId, packet->guid);
		}
	}
}
void FCMHost::OnNewConnection(RakPeerInterface *peer, Packet *packet)
{
	(void) peer;

	if (autoAddConnections)
		AddParticipant(packet->guid, autoAddConnectionsTargetGroup);

	// If this new connection changed the host, tell the user
	RakNetGUID newHost;
	unsigned int i;
	for (i=0; i < hostListsGroup.Size(); i++)
	{
		newHost = GetHost(hostListsGroup.GetKeyAtIndex(i));
		if (newHost==packet->guid)
			NotifyUserOfHost(hostListsGroup.GetKeyAtIndex(i), newHost);
	}
}
void FCMHost::OnHostListUpdate(RakPeerInterface *peer, Packet *packet)
{
	(void) peer;

	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(1);

	FCMHostGroupID groupId;
	incomingBs.Read(groupId);

	// Got host list update.
	DataStructures::List<RakNetGUID>* remoteHostList = RakNet::OP_NEW<DataStructures::List<RakNetGUID> >();
	ReadHostListGroup(&incomingBs, remoteHostList);

	// Only accept updates from the current host, or another host with a lower head guid
	RakNetGUID curHost = GetHost(groupId);
	if (hostListsGroup.Has(groupId) &&
		hostListsGroup.Get(groupId)>0 &&
		packet->guid > curHost )
	{
		RakNet::OP_DELETE(remoteHostList);
		return;
	}

	if (hostListsGroup.Has(groupId)==true)
		RakNet::OP_DELETE(hostListsGroup.Get(groupId));

	hostListsGroup.Set(groupId, remoteHostList);

	RakNetGUID newHost = GetHost(groupId);
	if (curHost!=newHost)
		NotifyUserOfHost(groupId, newHost);

//	printf("My   GUID=%s\n", rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
//	printf("Host GUID=%s\n", (*remoteHostList)[0].ToString());
//	printf("Get Host =%s\n", GetHost().ToString());

}
void FCMHost::WriteHostListGroup(FCMHostGroupID groupId, RakNet::BitStream *bs)
{
	DataStructures::List<RakNetGUID>* theList = hostListsGroup.Get(groupId);
	unsigned short size = (unsigned short) theList->Size();
	bs->Write(size);
	unsigned int i;
	for (i=0; i < theList->Size(); i++)
		bs->Write((*theList)[i]);
}
void FCMHost::ReadHostListGroup(RakNet::BitStream *bs, DataStructures::List<RakNetGUID> *theList)
{
	theList->Clear();
	unsigned short size;
	bs->Read(size);
	unsigned int i;
	RakNetGUID g;
	for (i=0; i < size; i++)
	{
		bs->Read(g);
		theList->Insert(g);
	}
}
void FCMHost::BroadcastToGroup(FCMHostGroupID groupId, RakNet::BitStream *bs)	
{
	DataStructures::List<RakNetGUID>* theList = hostListsGroup.Get(groupId);
	unsigned int i;
	for (i=0; i < theList->Size(); i++)
	{
		SystemAddress targetAddress = rakPeer->GetSystemAddressFromGuid((*theList)[i]);
//		printf("BroadcastToGroup to %s\n", targetAddress.ToString(true));
		// remove this next line
	//	if (targetAddress.port==60000)
		rakPeer->Send(bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, targetAddress, false);
	}
}
void FCMHost::BroadcastGroupList(FCMHostGroupID groupId)
{
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_FCM_HOST_LIST_UPDATE);
	bs.Write(groupId);
	WriteHostListGroup(groupId, &bs);
	BroadcastToGroup(groupId, &bs);
}
void FCMHost::NotifyUserOfHost(FCMHostGroupID groupId, RakNetGUID guid)
{
	Packet *p = rakPeer->AllocatePacket(sizeof(MessageID)+sizeof(FCMHostGroupID));
	RakNet::BitStream bs2(p->data, sizeof(MessageID)+sizeof(FCMHostGroupID), false);
	bs2.SetWriteOffset(0);
	bs2.Write((MessageID)ID_FCM_HOST_CHANGED);
	bs2.Write(groupId);
	p->systemAddress=rakPeer->GetSystemAddressFromGuid(guid);
	p->systemIndex=(SystemIndex)-1;
	p->guid=guid;
	// Push at head, to replace the incoming notification message.
	// Otherwise the notification message is effectively moved to the end of the queue
	rakPeer->PushBackPacket(p, true);
}
