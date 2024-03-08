#ifndef __CLOUD_SERVER_HELPER_H
#define __CLOUD_SERVER_HELPER_H

#include "CloudServer.h"

namespace RakNet
{

class DynDNS;
class CloudClient;
class TwoWayAuthentication;
class FullyConnectedMesh2;
class ConnectionGraph2;

class SampleFilter : public CloudServerQueryFilter
{
public:
	virtual bool OnPostRequest(RakNetGUID clientGuid, SystemAddress clientAddress, CloudKey key, uint32_t dataLength, const char *data);
	virtual bool OnReleaseRequest(RakNetGUID clientGuid, SystemAddress clientAddress, DataStructures::List<CloudKey> &cloudKeys);
	virtual bool OnGetRequest(RakNetGUID clientGuid, SystemAddress clientAddress, CloudQuery &query, DataStructures::List<RakNetGUID> &specificSystems);
	virtual bool OnUnsubscribeRequest(RakNetGUID clientGuid, SystemAddress clientAddress, DataStructures::List<CloudKey> &cloudKeys, DataStructures::List<RakNetGUID> &specificSystems);

	RakNetGUID serverGuid;
};

struct CloudServerHelper
{
	static const char *dnsHost;
	static const char *usernameAndPassword;
	static const char *serverToServerPassword;
	static unsigned short serverPort;
	static unsigned short allowedIncomingConnections;
	static unsigned short allowedOutgoingConnections;

	static void OnPacket(Packet *packet, RakPeerInterface *rakPeer, CloudClient *cloudClient, RakNet::CloudServer *cloudServer, RakNet::FullyConnectedMesh2 *fullyConnectedMesh2, TwoWayAuthentication *twoWayAuthentication, ConnectionGraph2 *connectionGraph2, DynDNS *dynDNS);
	// Returns false on DNS update failure
	static bool Update(DynDNS *dynDNS);

	static bool ParseCommandLineParameters(int argc, char **argv);
	static void PrintHelp(void);
	static bool StartRakPeer(RakNet::RakPeerInterface *rakPeer);
	static Packet *ConnectToRakPeer(const char *host, unsigned short port, RakPeerInterface *rakPeer);
	static bool UpdateHostDNS(RakNet::DynDNS *dynDNS);
	static MessageID AuthenticateRemoteServerBlocking(RakPeerInterface *rakPeer, TwoWayAuthentication *twoWayAuthentication, RakNetGUID remoteSystem);
	static void SetupPlugins(
		RakNet::CloudServer *cloudServer,
		RakNet::SampleFilter *sampleFilter,
		RakNet::CloudClient *cloudClient,
		RakNet::FullyConnectedMesh2 *fullyConnectedMesh2,
		RakNet::TwoWayAuthentication *twoWayAuthentication,
		RakNet::ConnectionGraph2 *connectionGraph2,
		const char *serverToServerPassword
		);

	static int JoinCloud(
		RakNet::RakPeerInterface *rakPeer,
		RakNet::CloudServer *cloudServer,
		RakNet::CloudClient *cloudClient,
		RakNet::FullyConnectedMesh2 *fullyConnectedMesh2,
		RakNet::TwoWayAuthentication *twoWayAuthentication,
		RakNet::ConnectionGraph2 *connectionGraph2,
		DynDNS *dynDNS
		);

protected:
	// Call when you get ID_FCM2_NEW_HOST
	static void OnFCMNewHost(Packet *packet, RakPeerInterface *rakPeer, DynDNS *dynDNS);
	// Call when the number of client connections change
	static void OnConnectionCountChange(RakPeerInterface *rakPeer, CloudClient *cloudClient);
};

} // namespace RakNet

#endif // __CLOUD_SERVER_HELPER_H
