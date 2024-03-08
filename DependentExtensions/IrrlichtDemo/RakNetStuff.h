// I tried to put most of the RakNet stuff here, but some of it had to go to CDemo.h too

#ifndef __RAKNET_ADDITIONS_FOR_IRRLICHT_DEMO_H
#define __RAKNET_ADDITIONS_FOR_IRRLICHT_DEMO_H

#include "RakPeerInterface.h"
#include "ReplicaManager3.h"
#include "NatPunchthroughClient.h"
#include "UDPProxyClient.h"
#include "TCPInterface.h"
#include "HTTPConnection.h"
#include "../Samples/PHPDirectoryServer/PHPDirectoryServer.h"
#include "vector3d.h"
#include "IAnimatedMeshSceneNode.h"
#include "MessageIdentifiers.h"


class ReplicaManager3Irrlicht;
class CDemo;
class PlayerReplica;

// All externs defined in the corresponding CPP file
// Most of these classes has a manual entry, all of them have a demo
extern RakPeerInterface *rakPeer; // Basic communication
extern NetworkIDManager *networkIDManager; // Unique IDs per network object
extern ReplicaManager3Irrlicht *replicaManager3; // Autoreplicate network objects
extern NatPunchthroughClient *natPunchthroughClient; // Connect peer to peer through routers
extern RakNet::UDPProxyClient *udpProxyClient; // Use a proxy if natPunchthroughClient fails
extern TCPInterface *tcpInterface; // Connect to a webserver to list and get the list of players
extern HTTPConnection *httpConnection; /// Connect to a webserver to list and get the list of players
extern PHPDirectoryServer *phpDirectoryServer; // Connect to a webserver to list and get the list of players
extern PlayerReplica *playerReplica; // Network object that represents the player

// A NAT punchthrough and proxy server Jenkins Software is hosting for free, should usually be online
#define DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_PORT 60481
#define DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP "216.224.123.180"

void InstantiateRakNetClasses(void);
void DeinitializeRakNetClasses(void);

// Base RakNet custom classes for Replica Manager 3, setup peer to peer networking
class BaseIrrlichtReplica : public RakNet::Replica3
{
public:
	BaseIrrlichtReplica();
	virtual ~BaseIrrlichtReplica();
	virtual RakNet::RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, RakNet::ReplicaManager3 *replicaManager3) {return QueryConstruction_PeerToPeer(destinationConnection);}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {return QueryRemoteConstruction_PeerToPeer(sourceConnection);}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {delete this;}
	virtual bool QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {return QuerySerialization_PeerToPeer(destinationConnection);}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection);
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) {}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {return true;}

	/// This function is not derived from Replica3, it's specific to this app
	/// Called from CDemo::UpdateRakNet
	virtual void Update(RakNetTime curTime);

	// Set when the object is constructed
	CDemo *demo;

	// real is written on the owner peer, read on the remote peer
	irr::core::vector3df position;
	RakNetTime creationTime;
};
// Game classes automatically updated by ReplicaManager3
class PlayerReplica : public BaseIrrlichtReplica, irr::scene::IAnimationEndCallBack
{
public:
	PlayerReplica();
	virtual ~PlayerReplica();
	// Every function below, before Update overriding a function in Replica3
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const;
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection);
	virtual void PostDeserializeConstruction(RakNet::Connection_RM3 *sourceConnection);
	virtual void PreDestruction(RakNet::Connection_RM3 *sourceConnection);

	virtual void Update(RakNetTime curTime);
	void UpdateAnimation(irr::scene::EMD2_ANIMATION_TYPE anim);
	float GetRotationDifference(float r1, float r2);
	virtual void OnAnimationEnd(irr::scene::IAnimatedMeshSceneNode* node);
	void PlayAttackAnimation(void);

	// playerName is only sent in SerializeConstruction, since it doesn't change
	RakNet::RakString playerName;

	// Networked rotation
	float rotationAroundYAxis;
	// Interpolation variables, not networked
	irr::core::vector3df positionDeltaPerMS;
	float rotationDeltaPerMS;
	RakNetTime interpEndTime, lastUpdate;

	// Updated based on the keypresses, to control remote animation
	bool isMoving;

	// Only instantiated for remote systems, you never see yourself
	irr::scene::IAnimatedMeshSceneNode* model;
	irr::scene::EMD2_ANIMATION_TYPE curAnim;

	// deathTimeout is set from the local player
	RakNetTime deathTimeout;
	bool IsDead(void) const;
	// isDead is set from network packets for remote players
	bool isDead;

	// List of all players, including our own
	static DataStructures::Multilist<ML_UNORDERED_LIST, PlayerReplica*> playerList;
};
class BallReplica : public BaseIrrlichtReplica
{
public:
	BallReplica();
	virtual ~BallReplica();
	// Every function except update is overriding a function in Replica3
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const;
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection);
	virtual void PostDeserializeConstruction(RakNet::Connection_RM3 *sourceConnection);
	virtual void PreDestruction(RakNet::Connection_RM3 *sourceConnection);

	virtual void Update(RakNetTime curTime);

	// shotDirection is networked
	irr::core::vector3df shotDirection;

	// shotlifetime is calculated, not networked
	RakNetTime shotLifetime;
};
class Connection_RM3Irrlicht : public RakNet::Connection_RM3 {
public:
	Connection_RM3Irrlicht(SystemAddress _systemAddress, RakNetGUID _guid, CDemo *_demo) : RakNet::Connection_RM3(_systemAddress, _guid) {demo=_demo;}
	virtual ~Connection_RM3Irrlicht() {}

	virtual RakNet::Replica3 *AllocReplica(RakNet::BitStream *allocationId);
protected:
	CDemo *demo;
};

class ReplicaManager3Irrlicht : public RakNet::ReplicaManager3
{
public:
	virtual RakNet::Connection_RM3* AllocConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID) const {
		return new Connection_RM3Irrlicht(systemAddress,rakNetGUID,demo);
	}
	virtual void DeallocConnection(RakNet::Connection_RM3 *connection) const {
		delete connection;
	}
	CDemo *demo;
};


#endif
