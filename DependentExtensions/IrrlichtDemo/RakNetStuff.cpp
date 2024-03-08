#include "RakNetStuff.h"
#include "RakNetworkFactory.h"
#include "NetworkIDManager.h"
#include "CDemo.h"
#include "RakNetTime.h"
#include "GetTime.h"

using namespace RakNet;

RakPeerInterface *rakPeer;
NetworkIDManager *networkIDManager;
ReplicaManager3Irrlicht *replicaManager3;
NatPunchthroughClient *natPunchthroughClient;
UDPProxyClient *udpProxyClient;
TCPInterface *tcpInterface;
HTTPConnection *httpConnection;
PHPDirectoryServer *phpDirectoryServer;
PlayerReplica *playerReplica;

/*
class DebugBoxSceneNode : public scene::ISceneNode 
{
public:
	DebugBoxSceneNode(scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id = -1);
	virtual const core::aabbox3d<f32>& getBoundingBox() const;
	virtual void OnRegisterSceneNode();
	virtual void render();

	CDemo *demo;
};
DebugBoxSceneNode::DebugBoxSceneNode(
									 scene::ISceneNode* parent,
									 scene::ISceneManager* mgr,
									 s32 id)
									 : scene::ISceneNode(parent, mgr, id)
{
#ifdef _DEBUG
	setDebugName("DebugBoxSceneNode");
#endif
	setAutomaticCulling(scene::EAC_OFF);
} 
const core::aabbox3d<f32>& DebugBoxSceneNode::getBoundingBox() const
{
	return demo->GetSyndeyBoundingBox();
}
void DebugBoxSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		demo->GetSceneManager()->registerNodeForRendering(this, scene::ESNRP_SOLID);
}
void DebugBoxSceneNode::render()
{
	if (DebugDataVisible)
	{ 
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation); 

		video::SMaterial m;
		m.Lighting = false;
		demo->GetDevice()->getVideoDriver()->setMaterial(m);
		demo->GetDevice()->getVideoDriver()->draw3DBox(demo->GetSyndeyBoundingBox());
	}
}
*/

DataStructures::Multilist<ML_UNORDERED_LIST, PlayerReplica*> PlayerReplica::playerList;

// Take this many milliseconds to move the visible position to the real position
static const float INTERP_TIME_MS=100.0f;

void InstantiateRakNetClasses(void)
{
	static const int MAX_PLAYERS=32;
	static const unsigned short TCP_PORT=0;
	static const RakNetTime UDP_SLEEP_TIMER=30;

	// Basis of all UDP communications
	rakPeer=RakNetworkFactory::GetRakPeerInterface();
	// +1 is for the connection to the NAT punchthrough server
	rakPeer->Startup(MAX_PLAYERS+1,UDP_SLEEP_TIMER,&SocketDescriptor(0,0),1);
	rakPeer->SetMaximumIncomingConnections(MAX_PLAYERS);
	// ReplicaManager3 replies on NetworkIDManager. It assigns numbers to objects so they can be looked up over the network
	// It's a class in case you wanted to have multiple worlds, then you could have multiple instances of NetworkIDManager
	networkIDManager=new NetworkIDManager;
	networkIDManager->SetIsNetworkIDAuthority(true);
	rakPeer->SetNetworkIDManager(networkIDManager);
	// Automatically sends around new / deleted / changed game objects
	replicaManager3=new ReplicaManager3Irrlicht;
	rakPeer->AttachPlugin(replicaManager3);
	// Automatically destroy connections, but don't create them so we have more control over when a system is considered ready to play
	replicaManager3->SetAutoManageConnections(false,true);
	// Create and register the network object that represents the player
	playerReplica = new PlayerReplica;
	replicaManager3->Reference(playerReplica);
	// Lets you connect through routers
	natPunchthroughClient=new NatPunchthroughClient;
	rakPeer->AttachPlugin(natPunchthroughClient);
	// If NatPunchthroughClient fails, this will forward messages
	udpProxyClient=new UDPProxyClient;
	rakPeer->AttachPlugin(udpProxyClient);
	// All the rest is to connect to http://www.jenkinssoftware.com/raknet/DirectoryServer.php and upload/download the player list so we know about other people
	tcpInterface=new TCPInterface;
	tcpInterface->Start(TCP_PORT,0);
	httpConnection=new HTTPConnection(*tcpInterface, "jenkinssoftware.com");
	phpDirectoryServer=new PHPDirectoryServer(*httpConnection, "/raknet/DirectoryServer.php");
	phpDirectoryServer->SetField("RakNetGUID",rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());
	// Upload our own game instance
	phpDirectoryServer->UploadTable(60, "IrrlichtDemo", rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS).port, "");
	// Download all other game instances
	phpDirectoryServer->DownloadTable("");
	// Connect to the NAT punchthrough server
	rakPeer->Connect(DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_IP, DEFAULT_NAT_PUNCHTHROUGH_FACILITATOR_PORT,0,0);
}
void DeinitializeRakNetClasses(void)
{
	// Shutdown so the server knows we stopped
	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
	delete networkIDManager;
	delete replicaManager3;
	delete natPunchthroughClient;
	delete udpProxyClient;
	delete httpConnection;
	delete tcpInterface;
	delete phpDirectoryServer;
	// ReplicaManager3 deletes all referenced objects, including this one
	//playerReplica->PreDestruction(0);
	//delete playerReplica;
}
BaseIrrlichtReplica::BaseIrrlichtReplica()
{
}
BaseIrrlichtReplica::~BaseIrrlichtReplica()
{

}
void BaseIrrlichtReplica::SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)
{
	constructionBitstream->Write(position);
}
bool BaseIrrlichtReplica::DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection)
{
	constructionBitstream->Read(position);
	return true;
}
RM3SerializationResult BaseIrrlichtReplica::Serialize(RakNet::SerializeParameters *serializeParameters)
{
	return RM3SR_BROADCAST_IDENTICALLY;
}
void BaseIrrlichtReplica::Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection)
{
}
void BaseIrrlichtReplica::Update(RakNetTime curTime)
{
}
PlayerReplica::PlayerReplica()
{
	model=0;
	rotationDeltaPerMS=0.0f;
	isMoving=false;
	deathTimeout=0;
	lastUpdate=RakNet::GetTime();
	playerList.Push(this,__FILE__,__LINE__);
}
PlayerReplica::~PlayerReplica()
{
	playerList.RemoveAtKey(this,true,__FILE__,__LINE__);
}
void PlayerReplica::WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const
{
	allocationIdBitstream->Write(RakNet::RakString("PlayerReplica"));
}
void PlayerReplica::SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)
{
	BaseIrrlichtReplica::SerializeConstruction(constructionBitstream, destinationConnection);
	constructionBitstream->Write(rotationAroundYAxis);
	constructionBitstream->Write(playerName);
	constructionBitstream->Write(IsDead());
}
bool PlayerReplica::DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection)
{
	if (!BaseIrrlichtReplica::DeserializeConstruction(constructionBitstream, sourceConnection))
		return false;
	constructionBitstream->Read(rotationAroundYAxis);
	constructionBitstream->Read(playerName);
	constructionBitstream->Read(isDead);
	return true;
}
void PlayerReplica::PostDeserializeConstruction(RakNet::Connection_RM3 *sourceConnection)
{
	// Object was remotely created and all data loaded. Now we can make the object visible
	scene::IAnimatedMesh* mesh = 0;
	scene::ISceneManager *sm = demo->GetSceneManager();
	mesh = sm->getMesh(IRRLICHT_MEDIA_PATH "sydney.md2");
	model = sm->addAnimatedMeshSceneNode(mesh, 0);

//	DebugBoxSceneNode * debugBox = new DebugBoxSceneNode(model,sm);
//	debugBox->demo=demo;
//	debugBox->setDebugDataVisible(true); 

	model->setPosition(position);
	model->setRotation(core::vector3df(0, rotationAroundYAxis, 0));
	model->setScale(core::vector3df(2,2,2));
	model->setMD2Animation(scene::EMAT_STAND);
	curAnim=scene::EMAT_STAND;
	model->setMaterialTexture(0, demo->GetDevice()->getVideoDriver()->getTexture(IRRLICHT_MEDIA_PATH "sydney.bmp"));
	model->setMaterialFlag(video::EMF_LIGHTING, true);
	model->addShadowVolumeSceneNode();
	model->setAutomaticCulling ( scene::EAC_BOX );
	model->setVisible(true);
	model->setAnimationEndCallback(this);
	wchar_t playerNameWChar[1024];
	mbstowcs(playerNameWChar, playerName.C_String(), 1024);
	scene::IBillboardSceneNode *bb = sm->addBillboardTextSceneNode(0, playerNameWChar, model);
	bb->setSize(core::dimension2df(40,20));
	bb->setPosition(core::vector3df(0,model->getBoundingBox().MaxEdge.Y+bb->getBoundingBox().MaxEdge.Y-bb->getBoundingBox().MinEdge.Y+5.0,0));
	bb->setColor(video::SColor(255,255,128,128), video::SColor(255,255,128,128));
}
void PlayerReplica::PreDestruction(RakNet::Connection_RM3 *sourceConnection)
{
	if (model)
		model->remove();
}
RM3SerializationResult PlayerReplica::Serialize(RakNet::SerializeParameters *serializeParameters)
{
	BaseIrrlichtReplica::Serialize(serializeParameters);
	serializeParameters->outputBitstream.Write(position);
	serializeParameters->outputBitstream.Write(rotationAroundYAxis);
	serializeParameters->outputBitstream.Write(isMoving);
	serializeParameters->outputBitstream.Write(IsDead());
	return RM3SR_BROADCAST_IDENTICALLY;
}
void PlayerReplica::Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection)
{
	BaseIrrlichtReplica::Deserialize(serializationBitstream, timeStamp, sourceConnection);
	serializationBitstream->Read(position);
	serializationBitstream->Read(rotationAroundYAxis);
	serializationBitstream->Read(isMoving);
	bool wasDead=isDead;
	serializationBitstream->Read(isDead);
	if (isDead==true && wasDead==false)
	{
		demo->PlayDeathSound(position);
	}

	core::vector3df positionOffset;
	positionOffset=position-model->getPosition();
	positionDeltaPerMS = positionOffset / INTERP_TIME_MS;
	float rotationOffset;
	rotationOffset=GetRotationDifference(rotationAroundYAxis,model->getRotation().Y);
	rotationDeltaPerMS = rotationOffset / INTERP_TIME_MS;
	interpEndTime = RakNet::GetTime() + (RakNetTime) INTERP_TIME_MS;
}
void PlayerReplica::Update(RakNetTime curTime)
{
	// Is a locally created object?
	if (creatingSystemGUID==rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
	{
		// Local player has no mesh to interpolate
		// Input our camera position as our player position
		playerReplica->position=demo->GetSceneManager()->getActiveCamera()->getPosition()-irr::core::vector3df(0,CAMERA_HEIGHT,0);
		playerReplica->rotationAroundYAxis=demo->GetSceneManager()->getActiveCamera()->getRotation().Y-90.0f;
		isMoving=demo->IsMovementKeyDown();

		// Ack, makes the screen messed up and the mouse move off the window
		// Find another way to keep the dead player from moving
	//	demo->EnableInput(IsDead()==false);

		return;
	}

	// Update interpolation
	RakNetTime elapsed = curTime-lastUpdate;
	if (elapsed<=1)
		return;
	if (elapsed>100)
		elapsed=100;

	lastUpdate=curTime;
	irr::core::vector3df curPositionDelta = position-model->getPosition();
	irr::core::vector3df interpThisTick = positionDeltaPerMS*(float) elapsed;
	if (curTime < interpEndTime && interpThisTick.getLengthSQ() < curPositionDelta.getLengthSQ())
	{
		model->setPosition(model->getPosition()+positionDeltaPerMS*(float) elapsed);
	}
	else
	{
		model->setPosition(position);
	}

	float curRotationDelta = GetRotationDifference(rotationAroundYAxis,model->getRotation().Y);
	float interpThisTickRotation = rotationDeltaPerMS*(float)elapsed;
	if (curTime < interpEndTime && fabs(interpThisTickRotation) < fabs(curRotationDelta))
	{
		model->setRotation(model->getRotation()+core::vector3df(0,interpThisTickRotation,0));
	}
	else
	{
		model->setRotation(core::vector3df(0,rotationAroundYAxis,0));
	}

	if (isDead)
	{
		UpdateAnimation(scene::EMAT_DEATH_FALLBACK);
		model->setLoopMode(false);		
	}
	else if (curAnim!=scene::EMAT_ATTACK)
	{
		if (isMoving)
		{
			UpdateAnimation(scene::EMAT_RUN);
			model->setLoopMode(true);
		}
		else
		{
			UpdateAnimation(scene::EMAT_STAND);
			model->setLoopMode(true);
		}
	}	
}
void PlayerReplica::UpdateAnimation(irr::scene::EMD2_ANIMATION_TYPE anim)
{
	if (anim!=curAnim)
		model->setMD2Animation(anim);
	curAnim=anim;
}
float PlayerReplica::GetRotationDifference(float r1, float r2)
{
	float diff = r1-r2;
	while (diff>180.0f)
		diff-=360.0f;
	while (diff<-180.0f)
		diff+=360.0f;
	return diff;
}
void PlayerReplica::OnAnimationEnd(scene::IAnimatedMeshSceneNode* node)
{
	if (curAnim==scene::EMAT_ATTACK)
	{
		if (isMoving)
		{
			UpdateAnimation(scene::EMAT_RUN);
			model->setLoopMode(true);
		}
		else
		{
			UpdateAnimation(scene::EMAT_STAND);
			model->setLoopMode(true);
		}
	}
}
void PlayerReplica::PlayAttackAnimation(void)
{
	if (isDead==false)
	{
		UpdateAnimation(scene::EMAT_ATTACK);
		model->setLoopMode(false);		
	}
}
bool PlayerReplica::IsDead(void) const
{
	return deathTimeout > RakNet::GetTime();
}
BallReplica::BallReplica()
{
	creationTime=RakNet::GetTime();
}
BallReplica::~BallReplica()
{
}
void BallReplica::WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const 
{
	allocationIdBitstream->Write(RakNet::RakString("BallReplica"));
}
void BallReplica::SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)
{
	BaseIrrlichtReplica::SerializeConstruction(constructionBitstream, destinationConnection);
	constructionBitstream->Write(shotDirection);
}
bool BallReplica::DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection)
{
	if (!BaseIrrlichtReplica::DeserializeConstruction(constructionBitstream, sourceConnection))
		return false;
	constructionBitstream->Read(shotDirection);
	return true;
}
void BallReplica::PostDeserializeConstruction(RakNet::Connection_RM3 *sourceConnection)
{
	// Shot visible effect and BallReplica classes are not linked, but they update the same way, such that
	// they are in the same spot all the time
	demo->shootFromOrigin(position, shotDirection);

	// Find the owner of this ball, and make them play the attack animation
	DataStructures::DefaultIndexType idx;
	for (idx=0; idx < PlayerReplica::playerList.GetSize(); idx++)
	{
		if (PlayerReplica::playerList[idx]->creatingSystemGUID==creatingSystemGUID)
		{
			PlayerReplica::playerList[idx]->PlayAttackAnimation();
			break;
		}
	}
}
void BallReplica::PreDestruction(RakNet::Connection_RM3 *sourceConnection)
{
	// The system that shot this ball destroyed it, or disconnected
	// Technically we should clear out the node visible effect too, but it's not important for now
}
RM3SerializationResult BallReplica::Serialize(RakNet::SerializeParameters *serializeParameters)
{
	BaseIrrlichtReplica::Serialize(serializeParameters);
	return RM3SR_BROADCAST_IDENTICALLY;
}
void BallReplica::Deserialize(RakNet::BitStream *serializationBitstream, RakNetTime timeStamp, RakNet::Connection_RM3 *sourceConnection)
{
	BaseIrrlichtReplica::Deserialize(serializationBitstream, timeStamp, sourceConnection);
}
void BallReplica::Update(RakNetTime curTime)
{
	// Is a locally created object?
	if (creatingSystemGUID==rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
	{
		// Destroy if shot expired
		if (curTime > shotLifetime)
		{
			// Destroy on network
			BroadcastDestruction();
			delete this;
			return;
		}
	}

	// Keep at the same position as the visible effect
	// Deterministic, so no need to actually transmit position
	// The variable position is the origin that the ball was created at. For the player, it is their actual position
	RakNetTime elapsedTime = curTime - creationTime;
	irr::core::vector3df updatedPosition = position + shotDirection * (float) elapsedTime * SHOT_SPEED;

	// See if the bullet hit us
	if (creatingSystemGUID!=rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
	{
		if (playerReplica->IsDead()==false)
		{
			float playerHalfHeight=demo->GetSyndeyBoundingBox().getExtent().Y/2;
			irr::core::vector3df positionRelativeToCharacter = updatedPosition-playerReplica->position;//+core::vector3df(0,playerHalfHeight,0);
			if (demo->GetSyndeyBoundingBox().isPointInside(positionRelativeToCharacter))
			//if ((playerReplica->position+core::vector3df(0,playerHalfHeight,0)-updatedPosition).getLengthSQ() < BALL_DIAMETER*BALL_DIAMETER/4.0f)
			{
				// We're dead for 3 seconds
				playerReplica->deathTimeout=curTime+3000;
			}
		}
	}
}
RakNet::Replica3 *Connection_RM3Irrlicht::AllocReplica(RakNet::BitStream *allocationId)
{
	RakNet::RakString typeName; allocationId->Read(typeName);
	if (typeName=="PlayerReplica") {BaseIrrlichtReplica *r = new PlayerReplica; r->demo=demo; return r;}
	if (typeName=="BallReplica") {BaseIrrlichtReplica *r = new BallReplica; r->demo=demo; return r;}
	return 0;
}
