// Demonstrates ReplicaManager 3: A system to automatically create, destroy, and serialize objects

#include "StringTable.h"
#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include <stdio.h>
#include "Kbhit.h"
#include <string.h>
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "ReplicaManager3.h"
#include "NetworkIDManager.h"
#include "RakSleep.h"
#include "FormatString.h"
#include "RakString.h"
#include "GetTime.h"
#include "SocketLayer.h"

enum
{
	CLIENT,
	SERVER,
	P2P
} topology;

// ReplicaManager3 is in the namespace RakNet
using namespace RakNet;

struct SampleReplica : public Replica3
{
	virtual RakNet::RakString GetName(void) const=0;
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	void PrintOutput(RakNet::BitStream *bs)
	{
		if (bs->GetNumberOfBitsUsed()==0)
			return;
		RakNet::RakString rakString;
		bs->Read(rakString);
		printf("Receive: %s\n", rakString.C_String());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		constructionBitstream->Write(GetName() + RakNet::RakString(" SerializeConstruction"));
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		PrintOutput(constructionBitstream);
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		destructionBitstream->Write(GetName() + RakNet::RakString(" SerializeDestruction"));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		PrintOutput(destructionBitstream);
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		RakNetTime time = RakNet::GetTime()/1000;

		if (time%2) // Swap channels randomly for testing
			serializeParameters->outputBitstream[0].Write(GetName() + RakNet::RakString(" Channel 0. Serialize. Time = %i", time));
		else
			serializeParameters->outputBitstream[1].Write(GetName() + RakNet::RakString(" Channel 1. Serialize. Time = %i", time));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		PrintOutput(&deserializeParameters->serializationBitstream[0]);
		PrintOutput(&deserializeParameters->serializationBitstream[1]);
	}

	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(" SerializeConstructionRequestAccepted"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(" SerializeConstructionRequestRejected"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

};

struct ClientCreatible_ClientSerialized : public SampleReplica {
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("ClientCreatible_ClientSerialized");}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)
	{
		return SampleReplica::Serialize(serializeParameters);
	}
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
		return QueryConstruction_ClientConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
		return QueryRemoteConstruction_ClientConstruction(sourceConnection);
	}

	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
		return QuerySerialization_ClientSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		return QueryActionOnPopConnection_Client(droppedConnection);
	}
};
struct ServerCreated_ClientSerialized : public SampleReplica {
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("ServerCreated_ClientSerialized");}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)
	{
		return SampleReplica::Serialize(serializeParameters);
	}
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
		return QueryConstruction_ServerConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
		return QueryRemoteConstruction_ServerConstruction(sourceConnection);
	}
	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
		return QuerySerialization_ClientSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		return QueryActionOnPopConnection_Server(droppedConnection);
	}
};
struct ClientCreatible_ServerSerialized : public SampleReplica {
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("ClientCreatible_ServerSerialized");}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)
	{
		if (topology==CLIENT)
			return RM3SR_DO_NOT_SERIALIZE;
		return SampleReplica::Serialize(serializeParameters);
	}
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
		return QueryConstruction_ClientConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
		return QueryRemoteConstruction_ClientConstruction(sourceConnection);
	}
	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
		return QuerySerialization_ServerSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		return QueryActionOnPopConnection_Client(droppedConnection);
	}
};
struct ServerCreated_ServerSerialized : public SampleReplica {
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("ServerCreated_ServerSerialized");}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)
	{
		if (topology==CLIENT)
			return RM3SR_DO_NOT_SERIALIZE;
		RakNetTime time = RakNet::GetTime()/1000;

		if (time%2) // Swap channels randomly for testing
			serializeParameters->outputBitstream[0].Write(GetName() + RakNet::RakString(" Serialize. Channel 0. Time = %i", time));
		else
			serializeParameters->outputBitstream[1].Write(GetName() + RakNet::RakString(" Serialize. Channel 1. Time = %i", time));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
		return QueryConstruction_ServerConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
		return QueryRemoteConstruction_ServerConstruction(sourceConnection);
	}
	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
		return QuerySerialization_ServerSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		return QueryActionOnPopConnection_Server(droppedConnection);
	}
};
struct P2PReplica : public SampleReplica {
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("P2PReplica");}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)
	{
		RakNetTime time = RakNet::GetTime()/1000;

		if (time%2) // Swap channels randomly for testing
			serializeParameters->outputBitstream[0].Write(GetName() + RakNet::RakString(" Serialize. Channel 0. Time = %i", time));
		else
			serializeParameters->outputBitstream[1].Write(GetName() + RakNet::RakString(" Serialize. Channel 1. Time = %i", time));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
		return QueryConstruction_PeerToPeer(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
		return QueryRemoteConstruction_PeerToPeer(sourceConnection);
	}
	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
		return QuerySerialization_PeerToPeer(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		return QueryActionOnPopConnection_PeerToPeer(droppedConnection);
	}
};

class SampleConnection : public Connection_RM3 {
public:
	SampleConnection(SystemAddress _systemAddress, RakNetGUID _guid) : Connection_RM3(_systemAddress, _guid) {}
	virtual ~SampleConnection() {}

	virtual Replica3 *AllocReplica(RakNet::BitStream *allocationId, ReplicaManager3 *replicaManager3)
	{
		RakNet::RakString typeName;
		allocationId->Read(typeName);
		if (typeName=="ClientCreatible_ClientSerialized") return new ClientCreatible_ClientSerialized;
		if (typeName=="ServerCreated_ClientSerialized") return new ServerCreated_ClientSerialized;
		if (typeName=="ClientCreatible_ServerSerialized") return new ClientCreatible_ServerSerialized;
		if (typeName=="ServerCreated_ServerSerialized") return new ServerCreated_ServerSerialized;
		if (typeName=="P2PReplica") return new P2PReplica;
		return 0;
	}
protected:
};

class ReplicaManager3Sample : public ReplicaManager3
{
	virtual Connection_RM3* AllocConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID) const {
		return new SampleConnection(systemAddress,rakNetGUID);
	}
	virtual void DeallocConnection(Connection_RM3 *connection) const {
		delete connection;
	}
};

int main(void)
{
	char ch;
	SocketDescriptor sd;
	char ip[128];
	static const int SERVER_PORT=12345;

	// ReplicaManager3 requires NetworkIDManager to lookup pointers from numbers.
	NetworkIDManager networkIdManager;
	// Each application has one instance of RakPeerInterface
	RakPeerInterface *rakPeer;
	// The system that performs most of our functionality for this demo
	ReplicaManager3Sample replicaManager;


	printf("Demonstration of ReplicaManager3.\n");
	printf("1. Demonstrates creating objects created by the server and client.\n");
	printf("2. Demonstrates automatic serialization data members\n");
	printf("Difficulty: Intermediate\n\n");

	printf("Start as (c)lient, (s)erver, (p)eer? ");
	ch=getche();

	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	if (ch=='c' || ch=='C')
	{
		topology=CLIENT;
		sd.port=0;
	}
	else if (ch=='s' || ch=='S')
	{
		topology=SERVER;
		sd.port=SERVER_PORT;
	}
	else
	{
		topology=P2P;
		sd.port=SERVER_PORT;
		while (SocketLayer::IsPortInUse(sd.port)==true)
			sd.port++;
	}

	// ObjectMemberRPC, AutoRPC for objects, and ReplicaManager3 require that you call SetNetworkIDManager()
	rakPeer->SetNetworkIDManager(&networkIdManager);
	// The network ID authority is the system that creates the common numerical identifier used to lookup pointers.
	// For client/server this is the server
	// For peer to peer this would be true on every system, and NETWORK_ID_SUPPORTS_PEER_TO_PEER should be defined in RakNetDefines.h
	networkIdManager.SetIsNetworkIDAuthority(topology==SERVER||topology==P2P);
	// Start RakNet, up to 32 connections if the server
	rakPeer->Startup(32,100,&sd,1);
	rakPeer->AttachPlugin(&replicaManager);
	rakPeer->SetMaximumIncomingConnections(32);

	printf("\n");
	if (topology==CLIENT)
	{
		printf("Enter server IP: ");
		gets(ip);
		if (ip[0]==0)
			strcpy(ip, "127.0.0.1");
		rakPeer->Connect(ip,SERVER_PORT,0,0,0);
		printf("Connecting...\n");
	}

	printf("Commands:\n(Q)uit\n'C'reate objects\n'D'estroy my objects\n");

	// Enter infinite loop to run the system
	Packet *packet;
	bool quit=false;
	while (!quit)
	{
		for (packet = rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet = rakPeer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("ID_CONNECTION_ATTEMPT_FAILED\n");
				quit=true;
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
				quit=true;
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_CONNECTION_LOST:
				printf("ID_CONNECTION_LOST\n");
				break;
			case ID_ADVERTISE_SYSTEM:
				// The conditional is needed because ID_ADVERTISE_SYSTEM may be from a system we are connected to, but replying on a different address.
				if (rakPeer->GetSystemAddressFromGuid(packet->guid)==UNASSIGNED_SYSTEM_ADDRESS)
				{
					printf("Connecting to %s\n", packet->systemAddress.ToString(true));
					rakPeer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.port,0,0);
				}
				break;
			}
		}

		if (kbhit())
		{
			ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
			if (ch=='c' || ch=='C')
			{
				printf("Objects created.\n");
				if (topology==SERVER||topology==CLIENT)
				{
					replicaManager.Reference(new ClientCreatible_ClientSerialized);
					replicaManager.Reference(new ServerCreated_ClientSerialized);
					replicaManager.Reference(new ClientCreatible_ServerSerialized);
					replicaManager.Reference(new ServerCreated_ServerSerialized);
				}
				else
				{
				//	for (int i=0; i < 20; i++)
						replicaManager.Reference(new P2PReplica);
				}
			}
			if (ch=='d' || ch=='D')
			{
				printf("My objects destroyed.\n");
				DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
				// The reason for ClearPointers is that in the sample, I don't track which objects have and have not been allocated at the application level. So ClearPointers will call delete on every object in the returned list, which is every object that the application has created. Another way to put it is
				// 	A. Send a packet to tell other systems to delete these objects
				// 	B. Delete these objects on my own system
				replicaManager.GetReplicasCreatedByMe(replicaListOut);
				replicaManager.BroadcastDestructionList(replicaListOut, UNASSIGNED_SYSTEM_ADDRESS);
				replicaListOut.ClearPointers( true, __FILE__, __LINE__ );
			}

		}

		RakSleep(30);
		for (int i=0; i < 32; i++)
		{
			if (rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS,0).port!=SERVER_PORT+i)
				rakPeer->AdvertiseSystem("255.255.255.255", SERVER_PORT+i, 0,0,0);
		}
	}

	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
}
