#include "NativeFeatureIncludes.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "Router.h"
#include "ConnectionGraph.h"
#include "FileOperations.h"
#include "RakMemoryOverride.h"
#include "DS_List.h"

void* MyMalloc (size_t size)
{
	return malloc(size);
}

void* MyRealloc (void *p, size_t size)
{
	return realloc(p,size);
}

void MyFree (void *p)
{
	free(p);
}



// This project is used to test the DLL system to make sure necessary classes are exported
int main()
{
	// Just test allocation and deallocation across the DLL.  If it crashes it failed, otherwise it worked.
	#if _RAKNET_SUPPORT_ReplicaManager==1
		ConsoleServer* a=RakNetworkFactory::GetConsoleServer( );
	#endif
	#if _RAKNET_SUPPORT_ReplicaManager==1
		ReplicaManager* b=RakNetworkFactory::GetReplicaManager( );
	#endif
	#if _RAKNET_SUPPORT_LogCommandParser==1
		LogCommandParser* c=RakNetworkFactory::GetLogCommandParser( );
		#if _RAKNET_SUPPORT_PacketLogger==1
			PacketLogger* d=RakNetworkFactory::GetPacketLogger( );
		#endif
	#endif
	#if _RAKNET_SUPPORT_RakNetCommandParser==1
		RakNetCommandParser* e=RakNetworkFactory::GetRakNetCommandParser( );
	#endif
	RakPeerInterface * f=RakNetworkFactory::GetRakPeerInterface( );
	SystemAddress sa = UNASSIGNED_SYSTEM_ADDRESS;
	f->GetIndexFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	#if _RAKNET_SUPPORT_Router==1
		Router *g=RakNetworkFactory::GetRouter( );
	#endif
	#if _RAKNET_SUPPORT_ConnectionGraph==1
		ConnectionGraph *h=RakNetworkFactory::GetConnectionGraph( );
	#endif

	// See RakMemoryOverride.h
	SetMalloc(MyMalloc);
	SetRealloc(MyRealloc);
	SetFree(MyFree);

	char *cArray = RakNet::OP_NEW_ARRAY<char>(10,__FILE__,__LINE__);
	RakNet::OP_DELETE_ARRAY(cArray,__FILE__,__LINE__);

	DataStructures::List<int> intList;
	intList.Push(5, __FILE__, __LINE__ );
	
	f->GetMTUSize(UNASSIGNED_SYSTEM_ADDRESS);
	SystemAddress p1;
	SystemAddress p2;
	p1=p2;
	g->Update();
	
	#if _RAKNET_SUPPORT_ConsoleServer==1
		RakNetworkFactory::DestroyConsoleServer(a);
	#endif
	#if _RAKNET_SUPPORT_ReplicaManager==1
		RakNetworkFactory::DestroyReplicaManager(b);
	#endif
	#if _RAKNET_SUPPORT_LogCommandParser==1
		RakNetworkFactory::DestroyLogCommandParser(c);
		#if _RAKNET_SUPPORT_PacketLogger==1
			RakNetworkFactory::DestroyPacketLogger(d);
		#endif
	#endif
	#if _RAKNET_SUPPORT_RakNetCommandParser==1
		RakNetworkFactory::DestroyRakNetCommandParser(e);
	#endif
	RakNetworkFactory::DestroyRakPeerInterface(f);
	#if _RAKNET_SUPPORT_Router==1
		RakNetworkFactory::DestroyRouter(g);
	#endif
	#if _RAKNET_SUPPORT_ConnectionGraph==1
		RakNetworkFactory::DestroyConnectionGraph(h);
	#endif
	return 0;
}

