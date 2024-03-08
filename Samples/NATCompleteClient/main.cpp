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
#include "NatPunchthroughClient.h"
#include "NatTypeDetectionClient.h"
#include "Getche.h"
#include "UPNPPortForwarder.h"
#include "GetTime.h"
#include "Router2.h"
#include "UDPProxyClient.h"

using namespace RakNet;

#define RAKPEER_PORT 61112
#define RAKPEER_PORT_STR "61112"
#define DEFAULT_SERVER_PORT "61111"
#define DEFAULT_SERVER_ADDRESS "94.198.81.195"

enum SampleResult
{
	PENDING,
	FAILED,
	SUCCEEDED
};

struct SampleFramework
{
	virtual const char * QueryName(void)=0;
	virtual bool QueryRequiresServer(void)=0;
	virtual const char * QueryFunction(void)=0;
	virtual const char * QuerySuccess(void)=0;
	virtual bool QueryQuitOnSuccess(void)=0;
	virtual void Init(RakPeerInterface *rakPeer)=0;
	virtual void ProcessPacket(Packet *packet)=0;
	virtual void Update(RakPeerInterface *rakPeer)=0;
	virtual void Shutdown(RakPeerInterface *rakPeer)=0;

	SampleResult sampleResult;
};

struct UPNPFramework : public SampleFramework, public UPNPCallbackInterface
{
	UPNPFramework() { sampleResult=PENDING; upnp=0;} 
	virtual const char * QueryName(void) {return "UPNPFramework";}
	virtual bool QueryRequiresServer(void) {return false;}
	virtual const char * QueryFunction(void) {return "Use UPNP to open the router";}
	virtual const char * QuerySuccess(void) {return "Other systems can now connect to you on the opened port.";}
	virtual bool QueryQuitOnSuccess(void) {return true;}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		upnp = new UPNPPortForwarder;
		upnp->OpenPortOnInterface(this,RAKPEER_PORT, "ALL");
	}

	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Update(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		upnp->CallCallbacks();
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		delete upnp;
		upnp=0;
	}

	virtual void UPNPStatusUpdate(RakNet::RakString stringToPrint) {
		printf("%s\n", stringToPrint.C_String());
	}

	virtual void QueryUPNPSupport_Result(QueryUPNPSupportResult *result) {
		unsigned int i;
		if (result->interfacesFound.Size()==0)
		{
			printf("Router does not support UPNP\n");
			sampleResult=FAILED;
			return;
		}
		printf("UPNP supported interfaces found:\n");
		for (i=0; i < result->interfacesFound.Size(); i++)
		{
			printf("%i. %s", i+1, result->interfacesFound[i].C_String());
		}
		sampleResult=SUCCEEDED;		
	}

	virtual void OpenPortOnInterface_Result(OpenPortResult *result)
	{
		unsigned int i;
		if (result->interfacesFound.Size()==0)
		{
			printf("Router does not support UPNP\n");
			sampleResult=FAILED;
			return;
		}
		printf("UPNP supported interfaces found:\n");
		bool anySucceeded=false;
		for (i=0; i < result->interfacesFound.Size(); i++)
		{
			if (result->succeeded[i]==true)
				anySucceeded=true;
			printf("%i. %s. Supports UPNP: %s\n", i+1, result->interfacesFound[i].C_String(), result->succeeded[i]==true ? "Yes" : "No");
		}
		if (anySucceeded)
			sampleResult=SUCCEEDED;		
		else
			sampleResult=FAILED;
	}

	UPNPPortForwarder *upnp;
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
SystemAddress ConnectBlocking(RakPeerInterface *rakPeer, const char *hostName, const char *defaultAddress, const char *defaultPort)
{
	char ipAddr[64];
	if (defaultAddress==0 || defaultAddress[0]==0)
		printf("Enter IP of system %s is running on: ", hostName);
	else
		printf("Enter IP of system %s, or press enter for default: ", hostName);
	gets(ipAddr);
	if (ipAddr[0]==0)
	{
		if (defaultAddress==0 || defaultAddress[0]==0)
		{
			printf("Failed. No address entered for %s.\n", hostName);
			return UNASSIGNED_SYSTEM_ADDRESS;
		}
		else
		{
			strcpy(ipAddr, defaultAddress);
		}
	}
	char port[64];
	if (defaultAddress==0 || defaultAddress[0]==0)
		printf("Enter port of system %s is running on: ", hostName);
	else
		printf("Enter port of system %s, or press enter for default: ", hostName);
	gets(port);
	if (port[0]==0)
	{
		if (defaultPort==0 || defaultPort[0]==0)
		{
			printf("Failed. No port entered for %s.\n", hostName);
			return UNASSIGNED_SYSTEM_ADDRESS;
		}
		else
		{
			strcpy(port, defaultPort);
		}
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
struct NatTypeDetectionFramework : public SampleFramework
{
	NatTypeDetectionFramework() { sampleResult=PENDING; ntdc=0;}
	virtual const char * QueryName(void) {return "NatTypeDetectionFramework";}
	virtual bool QueryRequiresServer(void) {return true;}
	virtual const char * QueryFunction(void) {return "Determines router type to avoid NAT punch attempts that cannot\nsucceed.";}
	virtual const char * QuerySuccess(void) {return "If our NAT type is Symmetric, we can skip NAT punch to other symmetric NATs.";}
	virtual bool QueryQuitOnSuccess(void) {return false;}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		SystemAddress serverAddress=SelectAmongConnectedSystems(rakPeer, "NatTypeDetectionServer");
		if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
		{
			serverAddress=ConnectBlocking(rakPeer, "NatTypeDetectionServer", DEFAULT_SERVER_ADDRESS, DEFAULT_SERVER_PORT);
			if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				printf("Failed to connect to a server.\n");
				sampleResult=FAILED;
				return;
			}
		}
		ntdc = new NatTypeDetectionClient;
		rakPeer->AttachPlugin(ntdc);
		ntdc->DetectNATType(serverAddress);
		timeout=RakNet::GetTimeMS() + 5000;
	}

	virtual void ProcessPacket(Packet *packet)
	{
		if (packet->data[0]==ID_NAT_TYPE_DETECTION_RESULT)
		{
			RakNet::NATTypeDetectionResult r = (RakNet::NATTypeDetectionResult) packet->data[1];
			printf("NAT Type is %s (%s)\n", NATTypeDetectionResultToString(r), NATTypeDetectionResultToStringFriendly(r));
			printf("Using NATPunchthrough can connect to systems using:\n");
			for (int i=0; i < (int) RakNet::NAT_TYPE_COUNT; i++)
			{
				if (CanConnect(r,(RakNet::NATTypeDetectionResult)i))
				{
					if (i!=0)
						printf(", ");
					printf("%s", NATTypeDetectionResultToString((RakNet::NATTypeDetectionResult)i));
				}
			}
			printf("\n");
			if (r==RakNet::NAT_TYPE_PORT_RESTRICTED || r==RakNet::NAT_TYPE_SYMMETRIC)
			{
				// For UPNP, see Samples\UDPProxy
				printf("Note: Your router must support UPNP or have the user manually forward ports.\n");
				printf("Otherwise not all connections may complete.\n");
			}

			sampleResult=SUCCEEDED;
		}
	}
	virtual void Update(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		if (sampleResult==PENDING && RakNet::GetTimeMS()>timeout)
		{
			printf("No response from the server, probably not running NatTypeDetectionServer plugin.\n");
			sampleResult=FAILED;
		}
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		delete ntdc;
		ntdc=0;
	}

	NatTypeDetectionClient *ntdc;
	RakNetTimeMS timeout;
};

struct NatPunchthoughClientFramework : public SampleFramework, public NatPunchthroughDebugInterface_Printf
{
	NatPunchthoughClientFramework() { sampleResult=PENDING; npClient=0;}
	virtual const char * QueryName(void) {return "NatPunchthoughClientFramework";}
	virtual bool QueryRequiresServer(void) {return true;}
	virtual const char * QueryFunction(void) {return "Causes two systems to try to connect to each other at the same\ntime, to get through routers.";}
	virtual const char * QuerySuccess(void) {return "We can now communicate with the other system, including connecting.";}
	virtual bool QueryQuitOnSuccess(void) {return true;}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		SystemAddress serverAddress=SelectAmongConnectedSystems(rakPeer, "NatPunchthroughServer");
		if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
		{
			serverAddress=ConnectBlocking(rakPeer, "NatPunchthroughServer", DEFAULT_SERVER_ADDRESS, DEFAULT_SERVER_PORT);
			if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				printf("Failed to connect to a server.\n");
				sampleResult=FAILED;
				return;
			}
		}
		npClient->SetDebugInterface(this);
		
		char guid[128];
		do 
		{
			printf("Enter RakNetGuid of the remote system, which should have already connected to the server.\n");
			gets(guid);
		} while (guid[0]==0);
		RakNetGUID remoteSystemGuid;
		remoteSystemGuid.FromString(guid);
		npClient = new NatPunchthroughClient;
		rakPeer->AttachPlugin(npClient);
		npClient->OpenNAT(remoteSystemGuid, serverAddress);
		timeout=RakNet::GetTimeMS() + 5000;
	}

	virtual void ProcessPacket(Packet *packet)
	{
		if (
			packet->data[0]==ID_NAT_TARGET_NOT_CONNECTED ||
			packet->data[0]==ID_NAT_TARGET_UNRESPONSIVE ||
			packet->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST ||
			packet->data[0]==ID_NAT_PUNCHTHROUGH_FAILED
			)
			{
				RakNetGUID guid;
				if (packet->data[0]==ID_NAT_PUNCHTHROUGH_FAILED)
				{
					guid=packet->guid;
				}
				else
				{
					RakNet::BitStream bs(packet->data,packet->length,false);
					bs.IgnoreBytes(1);
					bool b = bs.Read(guid);
					RakAssert(b);
				}

				switch (packet->data[0])
				{
				case ID_NAT_TARGET_NOT_CONNECTED:
					printf("Failed: ID_NAT_TARGET_NOT_CONNECTED\n");
					break;
				case ID_NAT_TARGET_UNRESPONSIVE:
					printf("Failed: ID_NAT_TARGET_UNRESPONSIVE\n");
					break;
				case ID_NAT_CONNECTION_TO_TARGET_LOST:
					printf("Failed: ID_NAT_CONNECTION_TO_TARGET_LOST\n");
					break;
				case ID_NAT_PUNCHTHROUGH_FAILED:
					printf("Failed: ID_NAT_PUNCHTHROUGH_FAILED\n");
					break;
				}

				sampleResult=FAILED;
				return;
			}
		else if (packet->data[0]==ID_NAT_PUNCHTHROUGH_SUCCEEDED)
		{
			unsigned char weAreTheSender = packet->data[1];
			if (weAreTheSender)
				printf("NAT punch success to remote system %s.\n", packet->systemAddress.ToString(true));
			else
				printf("NAT punch success from remote system %s.\n", packet->systemAddress.ToString(true));
		}
	}
	virtual void Update(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		if (sampleResult==PENDING && RakNet::GetTimeMS()>timeout)
		{
			printf("No response from the server, probably not running NatPunchthroughServer plugin.\n");
			sampleResult=FAILED;
		}
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		delete npClient;
		npClient=0;
	}

	NatPunchthroughClient *npClient;
	RakNetTimeMS timeout;
};

struct Router2Framework : public SampleFramework
{
	Router2Framework() { sampleResult=PENDING; router2=0;}
	virtual const char * QueryName(void) {return "Router2Framework";}
	virtual bool QueryRequiresServer(void) {return false;}
	virtual const char * QueryFunction(void) {return "Connect to a peer we cannot directly connect to using the\nbandwidth of a shared peer.";}
	virtual const char * QuerySuccess(void) {return "Router2 assumes we will now connect to the other system.";}
	virtual bool QueryQuitOnSuccess(void) {return true;}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		printf("Given your application's bandwidth, how much traffic can be forwarded through a single peer?\nIf you use more than half the available bandwidth, then this plugin won't work for you.\n");;
		char supportedStr[64];
		do 
		{
			printf("Enter a number greater than or equal to 0: ");
			gets(supportedStr);
		} while (supportedStr[0]==0);
		int supported=atoi(supportedStr);
		if (supported<=0)
		{
			printf("Aborting Router2\n");
			sampleResult=FAILED;
			return;
		}

		SystemAddress peerAddress = SelectAmongConnectedSystems(rakPeer, "shared peer");
		if (peerAddress==UNASSIGNED_SYSTEM_ADDRESS)
		{
			peerAddress=ConnectBlocking(rakPeer, "shared peer", "", RAKPEER_PORT_STR);
			if (peerAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				printf("Failed to connect to a shared peer.\n");
				sampleResult=FAILED;
				return;
			}
		}

		char guid[64];
		printf("Destination system must be connected to the shared peer.\n");
		do 
		{
			printf("Enter RakNetGUID of destination system: ");
			gets(guid);
		} while (guid[0]==0);
		RakNetGUID endpointGuid;
		endpointGuid.FromString(guid);
		router2 = new Router2;
		rakPeer->AttachPlugin(router2);
		router2->EstablishRouting(endpointGuid);

		timeout=RakNet::GetTimeMS() + 5000;
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Update(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		if (sampleResult==PENDING && RakNet::GetTimeMS()>timeout)
		{
			printf("No response from any system, probably not running Router2 plugin.\n");
			sampleResult=FAILED;
		}
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		delete router2;
		router2=0;
	}
	Router2 *router2;
	RakNetTimeMS timeout;
};
struct UDPProxyClientFramework : public SampleFramework, public UDPProxyClientResultHandler
{
	UDPProxyClientFramework() { sampleResult=PENDING; udpProxy=0;}
	virtual const char * QueryName(void) {return "UDPProxyClientFramework";}
	virtual bool QueryRequiresServer(void) {return true;}
	virtual const char * QueryFunction(void) {return "Connect to a peer using a shared server connection.";}
	virtual const char * QuerySuccess(void) {return "We can now communicate with the other system, including connecting, within 5 seconds.";}
	virtual bool QueryQuitOnSuccess(void) {return true;}
	virtual void Init(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		SystemAddress serverAddress=SelectAmongConnectedSystems(rakPeer, "UDPProxyCoordinator");
		if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
		{
			serverAddress=ConnectBlocking(rakPeer, "UDPProxyCoordinator", DEFAULT_SERVER_ADDRESS, DEFAULT_SERVER_PORT);
			if (serverAddress==UNASSIGNED_SYSTEM_ADDRESS)
			{
				printf("Failed to connect to a server.\n");
				sampleResult=FAILED;
				return;
			}
		}
		udpProxy = new UDPProxyClient;
		rakPeer->AttachPlugin(udpProxy);
		udpProxy->SetResultHandler(this);
		timeout=RakNet::GetTimeMS() + 5000;
	}
	virtual void ProcessPacket(Packet *packet)
	{
	}
	virtual void Update(RakPeerInterface *rakPeer)
	{
		if (sampleResult==FAILED) return;

		if (sampleResult==PENDING && RakNet::GetTimeMS()>timeout)
		{
			printf("No response from the server, probably not running UDPProxyCoordinator plugin.\n");
			sampleResult=FAILED;
		}
	}
	virtual void Shutdown(RakPeerInterface *rakPeer)
	{
		delete udpProxy;
		udpProxy=0;
	}
	
	virtual void OnForwardingSuccess(const char *proxyIPAddress, unsigned short proxyPort, unsigned short reverseProxyPort,
		SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Datagrams forwarded successfully by proxy %s:%i to target %s.\n", proxyIPAddress, proxyPort, targetAddress.ToString(false));
		printf("Connecting to proxy, which will be received by target.\n");
		// rakPeer->Connect(proxyIPAddress, proxyPort, 0, 0);
		sampleResult=SUCCEEDED;
	}
	virtual void OnForwardingNotification(const char *proxyIPAddress, unsigned short proxyPort, unsigned short reverseProxyPort,
		SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Source %s has setup forwarding to us through proxy %s:%i.\n", sourceAddress.ToString(false), proxyIPAddress, reverseProxyPort);
	}
	virtual void OnNoServersOnline(SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Failure: No servers logged into coordinator.\n");
		sampleResult=FAILED;
	}
	virtual void OnRecipientNotConnected(SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNetGUID targetGuid, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Failure: Recipient not connected to coordinator.\n");
		sampleResult=FAILED;
	}
	virtual void OnAllServersBusy(SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Failure: No servers have available forwarding ports.\n");
		sampleResult=FAILED;
	}
	virtual void OnForwardingInProgress(SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddress, RakNet::UDPProxyClient *proxyClientPlugin)
	{
		printf("Notification: Forwarding already in progress.\n");
	}

	UDPProxyClient *udpProxy;
	RakNetTimeMS timeout;
};
enum FeatureList
{
	_UPNPFramework,
	_NatTypeDetectionFramework,
	_NatPunchthoughFramework,
	_Router2Framework,
	_UDPProxyClientFramework,
	FEATURE_LIST_COUNT
};
int main(void)
{
	RakPeerInterface *rakPeer=RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor sd(RAKPEER_PORT,0);
	if (rakPeer->Startup(32,10,&sd,1)==false)
	{
		printf("Failed to start rakPeer! Quitting\n");
		RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
		return 1;
	}

	SampleFramework *samples[FEATURE_LIST_COUNT];
	unsigned int i=0;
	samples[i++] = new UPNPFramework;
	samples[i++] = new NatTypeDetectionFramework;
	samples[i++] = new NatPunchthoughClientFramework;
	samples[i++] = new Router2Framework;
	samples[i++] = new UDPProxyClientFramework;
	assert(i==FEATURE_LIST_COUNT);

	bool isFirstPrint=true;
	for (i=0; i < FEATURE_LIST_COUNT; i++)
	{
		if (isFirstPrint)
		{
			printf("NAT traversal client\nSupported operations:\n");
			isFirstPrint=false;
		}
		printf("\n%s\nRequires server: %s\nDescription: %s\n", samples[i]->QueryName(), samples[i]->QueryRequiresServer()==1 ? "Yes" : "No", samples[i]->QueryFunction());
	}

	printf("\nDo you have a server running the NATCompleteServer project? (y/n): ");

	char responseLetter=getche();
	bool hasServer=responseLetter=='y' || responseLetter=='Y';
	printf("\n");
	if (hasServer==false)
		printf("Note: Only UPNP and Router2 are supported without a server\nYou may want to consider using the Lobby2/Steam project. They host the\nservers for you.\n\n");

	FeatureList currentStage=_UPNPFramework;

	if (hasServer==false)
	{
		while (samples[(int) currentStage]->QueryRequiresServer()==true)
		{
			printf("No server: Skipping %s\n", samples[(int) currentStage]->QueryName());
			int stageInt = (int) currentStage;
			stageInt++;
			currentStage=(FeatureList)stageInt;
			if (currentStage==FEATURE_LIST_COUNT)
			{
				printf("Connectivity not possible. Exiting\n");
				return 1;
			}
		}
	}

	while (1)
	{
		printf("Executing %s\n", samples[(int) currentStage]->QueryName());
		samples[(int) currentStage]->Init(rakPeer);

		bool thisSampleDone=false;
		while (1)
		{
			samples[(int) currentStage]->Update(rakPeer);
			Packet *packet;
			for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
			{
				for (i=0; i < FEATURE_LIST_COUNT; i++)
				{
					samples[i]->ProcessPacket(packet);
				}
			}

			if (samples[(int) currentStage]->sampleResult==FAILED ||
				samples[(int) currentStage]->sampleResult==SUCCEEDED)
			{
				printf("\n");
				thisSampleDone=true;
				if (samples[(int) currentStage]->sampleResult==FAILED)
				{
					printf("Failed %s\n", samples[(int) currentStage]->QueryName());

					int stageInt = (int) currentStage;
					stageInt++;
					currentStage=(FeatureList)stageInt;
					if (currentStage==FEATURE_LIST_COUNT)
					{
						printf("Connectivity not possible. Exiting\n");
						rakPeer->Shutdown(100);
						RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
						return 1;
					}
					else
					{
						printf("Proceeding to next stage.\n");
						break;
					}
				}
				else
				{
					printf("Passed %s\n", samples[(int) currentStage]->QueryName());
					if (samples[(int) currentStage]->QueryQuitOnSuccess())
					{
						printf("You should now be able to connect. Sample complete.\n");
						rakPeer->Shutdown(100);
						RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
						return 1;
					}
					printf("Proceeding to next stage.\n");
					break;
				}
			}

			RakSleep(100);
		}
	}

	return 0;
}
