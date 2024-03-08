#include "RakPeerInterface.h"
#include "RakSleep.h"
#include <stdio.h>
#include <stdlib.h>
#include "RakNetworkFactory.h"
#include <string.h>
#include "Kbhit.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "UDPProxyServer.h"
#include "UDPProxyCoordinator.h"
#include "NatPunchthroughServer.h"
#include "NatTypeDetectionServer.h"
#include "SocketLayer.h"
#include "Getche.h"

using namespace RakNet;

enum FeatureSupport
{
	SUPPORTED,
	UNSUPPORTED,
	QUERY
};

enum FeatureList
{
	NAT_TYPE_DETECTION_SERVER,
	NAT_PUNCHTHROUGH_SERVER,
	UDP_PROXY_COORDINATOR,
	UDP_PROXY_SERVER,
	FEATURE_LIST_COUNT,
};

#define RAKPEER_PORT 61111

struct SampleFramework
{
	virtual const char * QueryName(void)=0;
	virtual const char * QueryRequirements(void)=0;
	virtual const char * QueryFunction(void)=0;
	virtual void Init(RakPeerInterface *rakPeer)=0;
	virtual void ProcessPacket(Packet *packet)=0;
	virtual void Shutdown(RakPeerInterface *rakPeer)=0;

	FeatureSupport isSupported;
};

struct NatTypeDetectionServerFramework : public SampleFramework
{
	NatTypeDetectionServerFramework() {isSupported=QUERY; ntds=0;}
	virtual const char * QueryName(void) {return "NatTypeDetectionServer";}
	virtual const char * QueryRequirements(void) {return "Requires 4 IP addresses";}
	virtual const char * QueryFunction(void) {return "Determines router type to filter by connectable systems.\nOne instance needed, multiple instances may exist to spread workload.";}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (isSupported==SUPPORTED)
		{
			ntds = new NatTypeDetectionServer;
			rakPeer->AttachPlugin(ntds);

			char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ];
			unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
			SocketLayer::Instance()->GetMyIP( ipList, binaryAddresses );
			for (int i=0; i<5; i++)
			{
				if (ipList[i][0]==0 && i < MAXIMUM_NUMBER_OF_INTERNAL_IDS)
				{
					printf("Failed. Not enough IP addresses to bind to.\n");
					rakPeer->DetachPlugin(ntds);
					delete ntds;
					ntds=0;
					isSupported=UNSUPPORTED;
					return;
				}
			}
			printf("Starting %s on %s, %s, %s.\n", QueryName(), ipList[1], ipList[2], ipList[3]);
			ntds->Startup(ipList[1], ipList[2], ipList[3]);
		}
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		if (ntds)
		{
			rakPeer->DetachPlugin(ntds);
			delete ntds;
		}
		ntds=0;
	}

	NatTypeDetectionServer *ntds;
};
struct NatPunchthroughServerFramework : public SampleFramework, public NatPunchthroughServerDebugInterface_Printf
{
	NatPunchthroughServerFramework() {isSupported=QUERY; nps=0;}
	virtual const char * QueryName(void) {return "NatPunchthroughServerFramework";}
	virtual const char * QueryRequirements(void) {return "None";}
	virtual const char * QueryFunction(void) {return "Coordinates NATPunchthroughClient.";}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (isSupported==SUPPORTED)
		{
			nps = new NatPunchthroughServer;
			rakPeer->AttachPlugin(nps);
			nps->SetDebugInterface(this);
		}
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		if (nps)
		{
			rakPeer->DetachPlugin(nps);
			delete nps;
		}
		nps=0;
	}

	NatPunchthroughServer *nps;
};
struct UDPProxyCoordinatorFramework : public SampleFramework
{
	UDPProxyCoordinatorFramework() {isSupported=QUERY;}
	virtual const char * QueryName(void) {return "UDPProxyCoordinator";}
	virtual const char * QueryRequirements(void) {return "Bandwidth to handle a few hundred bytes per game session.";}
	virtual const char * QueryFunction(void) {return "Coordinates UDPProxyClient to find available UDPProxyServer.\nExactly one instance required.";}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (isSupported==SUPPORTED)
		{
			udppc = new UDPProxyCoordinator;
			rakPeer->AttachPlugin(udppc);

			char password[512];
			do
			{
				printf("Create password for UDPProxyCoordinator: ");
				gets(password);
			} while (password[0]==0);
			udppc->SetRemoteLoginPassword(password);
		}
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		if (udppc)
		{
			rakPeer->DetachPlugin(udppc);
			delete udppc;
			udppc=0;
		}
	}

	UDPProxyCoordinator *udppc;
};
SystemAddress SelectAmongConnectedSystems(RakPeerInterface *rakPeer, const char *hostName)
{
	DataStructures::List<SystemAddress> addresses;
	DataStructures::List<RakNetGUID> guids;
	rakPeer->GetSystemList(addresses, guids);
	if (addresses.Size()==0)
	{
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
	if (addresses.Size()>1)
	{
		printf("Select IP address for %s.\n", hostName);
		char buff[64];
		for (unsigned int i=0; i < addresses.Size(); i++)
		{
			addresses[i].ToString(true, buff);
			printf("%i. %s\n", i+1, buff);
		}
		gets(buff);
		if (buff[0]==0)
		{
			return UNASSIGNED_SYSTEM_ADDRESS;
		}
		unsigned int idx = atoi(buff);
		if (idx<=0 || idx > addresses.Size())
		{
			return UNASSIGNED_SYSTEM_ADDRESS;
		}
		return addresses[idx-1];
	}
	else
		return addresses[0];
};
SystemAddress ConnectBlocking(RakPeerInterface *rakPeer, const char *hostName)
{
	char ipAddr[64];
	printf("Enter IP of system %s is running on: ", hostName);
	gets(ipAddr);
	if (ipAddr[0]==0)
	{
		printf("Failed. Not connected to %s.\n", hostName);
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
	char port[64];
	printf("Enter port of system %s is running on: ", hostName);
	gets(port);
	if (port[0]==0)
	{
		printf("Failed. Not connected to %s.\n", hostName);
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
	if (rakPeer->Connect(ipAddr, atoi(port), 0, 0)==false)
	{
		printf("Failed connect call for %s.\n", hostName);
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
	printf("Connecting...\n");
	Packet *packet;
	while (1)
	{
		for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
		{
			if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
			{
				return packet->systemAddress;
			}
			else
			{
				return UNASSIGNED_SYSTEM_ADDRESS;
			}
			RakSleep(100);
		}
	}
}
struct UDPProxyServerFramework : public SampleFramework, public UDPProxyServerResultHandler
{
	UDPProxyServerFramework() {isSupported=QUERY;}
	virtual const char * QueryName(void) {return "UDPProxyServer";}
	virtual const char * QueryRequirements(void) {return "Bandwidth to handle forwarded game traffic.";}
	virtual const char * QueryFunction(void) {return "Allows game clients to forward network traffic transparently.\nOne or more instances required, can be added at runtime.";}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (isSupported==SUPPORTED)
		{
			printf("Logging into UDPProxyCoordinator...\n");
			SystemAddress coordinatorAddress=SelectAmongConnectedSystems(rakPeer, "UDPProxyCoordinator");
			if (coordinatorAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				printf("Warning: RakPeer is not currently connected to any system.\nEnter option:\n(1). UDPProxyCoordinator is on localhost\n(2). Connect to a remote system\n(3). Fail.\nOption: ");
				char ch=getche();
				printf("\n");
				if (ch=='1')
				{
					coordinatorAddress==rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS);
				}
				else if (ch=='2')
				{
					coordinatorAddress=ConnectBlocking(rakPeer, "UDPProxyCoordinator");
					if (coordinatorAddress==UNASSIGNED_SYSTEM_ADDRESS)
					{
						printf("Failed to connect.\n");
						isSupported=QUERY;
						return;
					}
				}
				else
				{
					printf("Failed. Not connected to UDPProxyCoordinator.\n");
					isSupported=QUERY;
					return;
				}
			}
			
			char password[512];
			do
			{
				printf("Enter password used with UDPProxyCoordinator: ");
				gets(password);
			} while (password[0]==0);

			udpps = new UDPProxyServer;
			udpps->SetResultHandler(this);
			rakPeer->AttachPlugin(udpps);
			if (udpps->LoginToCoordinator(password, coordinatorAddress)==false)
			{
				printf("LoginToCoordinator call failed.\n");
				isSupported=QUERY;
				rakPeer->DetachPlugin(udpps);
				delete udpps;
				udpps=0;
				return;
			}
		}
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		if (udpps)
		{
			rakPeer->DetachPlugin(udpps);
			delete udpps;
			udpps=0;
		}
	}

	virtual void OnLoginSuccess(RakNet::RakString usedPassword, RakNet::UDPProxyServer *proxyServerPlugin)
	{
		printf("%s logged into UDPProxyCoordinator.\n", QueryName());
	}
	virtual void OnAlreadyLoggedIn(RakNet::RakString usedPassword, RakNet::UDPProxyServer *proxyServerPlugin)
	{
		printf("%s already logged into UDPProxyCoordinator.\n", QueryName());
	}
	virtual void OnNoPasswordSet(RakNet::RakString usedPassword, RakNet::UDPProxyServer *proxyServerPlugin)
	{
		printf("%s failed login to UDPProxyCoordinator. No password set.\n", QueryName());
		isSupported=QUERY;
		delete udpps;
		udpps=0;
	}
	virtual void OnWrongPassword(RakNet::RakString usedPassword, RakNet::UDPProxyServer *proxyServerPlugin)
	{
		printf("%s failed login to UDPProxyCoordinator. %s was the wrong password.\n", QueryName(), usedPassword.C_String());
		isSupported=QUERY;
		delete udpps;
		udpps=0;
	}

	UDPProxyServer *udpps;
};
int main()
{
	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ];
	unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
	SocketLayer::Instance()->GetMyIP( ipList, binaryAddresses );
	SocketDescriptor sd(RAKPEER_PORT,ipList[0]);
	if (rakPeer->Startup(32,10,&sd,1)==false)
	{
		SocketDescriptor sd2(RAKPEER_PORT,0);
		if (rakPeer->Startup(32,10,&sd2,1)==false)
		{
			printf("Failed to start rakPeer! Quitting\n");
			RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
			return 1;
		}
	}
	rakPeer->SetMaximumIncomingConnections(32);

	SampleFramework *samples[FEATURE_LIST_COUNT];
	unsigned int i=0;
	samples[i++] = new NatTypeDetectionServerFramework;
	samples[i++] = new NatPunchthroughServerFramework;
	samples[i++] = new UDPProxyCoordinatorFramework;
	samples[i++] = new UDPProxyServerFramework;
	assert(i==FEATURE_LIST_COUNT);

	bool isFirstPrint=true;
	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		if (samples[i]->isSupported==QUERY)
		{
			if (isFirstPrint)
			{
				printf("NAT traversal server.\nSee http://www.dx.net/raknet_dx.php for discounted server hosting\nSelect which features to support.\n");
				isFirstPrint=false;
			}
			printf("\n%s\nRequirements: %s\nDescription: %s\n", samples[i]->QueryName(), samples[i]->QueryRequirements(), samples[i]->QueryFunction());
			printf("Support %s? (y/n): ", samples[i]->QueryName());
			char supported=getche();
			if (supported=='y' || supported=='Y')
			{
				samples[i]->isSupported=SUPPORTED;
			}
			printf("\n");
		}
	}

	printf("\n");

	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		if (samples[i]->isSupported==SUPPORTED)
		{
			printf("Starting %s...\n", samples[i]->QueryName());
			samples[i]->Init(rakPeer);
			if (samples[i]->isSupported!=SUPPORTED)
			{
				printf("Failed to start %s.", samples[i]->QueryName());
				if (samples[i]->isSupported==QUERY)
				{
					printf(" Retry? (y/n): ");
					char supported=getche();
					if (supported=='y' || supported=='Y')
					{
						samples[i]->isSupported=SUPPORTED;
						i--;
					}
				}
				else
					printf("\n");
				printf("\n");
			}
			else
				printf("Success.\n\n");
		}
	}

	bool anySupported=false;
	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		if (samples[i]->isSupported==SUPPORTED)
		{
			anySupported=true;
			break;
		}
	}

	if (anySupported==false)
	{
		printf("No features supported! Quitting.\n");
		rakPeer->Shutdown(100);
		RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
		return 1;
	}

	bool firstComma=true;
	printf("Supported features: ");
	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		if (samples[i]->isSupported==SUPPORTED)
		{
			if (firstComma==false)
				printf(", ");
			else
				firstComma=false;
			printf("%s", samples[i]->QueryName());
		}
	}
	
	printf("\nEntering update loop. Press 'q' to quit.\n");

	Packet *packet;
	bool quit=false;
	while (!quit)
	{
		for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
		{
			for (i=0; i < FEATURE_LIST_COUNT; i++)
			{
				samples[i]->ProcessPacket(packet);
			}
		}

		if (kbhit())
		{
			if (getch()=='q')
				quit=true;
		}
		RakSleep(100);
	}

	printf("Quitting.\n");
	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		samples[i]->Shutdown(rakPeer);
	}
	rakPeer->Shutdown(100);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
	return 0;
}

