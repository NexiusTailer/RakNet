#include "UPNPCallBackInterface.h"

UPNPCallBackInterface::UPNPCallBackInterface(void)
{
	theLock=RakNet::OP_NEW<SimpleMutex>(__FILE__,__LINE__);
	internalReleaseTracker= new bool;
	*internalReleaseTracker=false;
	releaseTrackers= new DataStructures::List <bool *>;
}

UPNPCallBackInterface::~UPNPCallBackInterface(void)
{
	int size=releaseTrackers->Size();
	for (int i=0;i<size;i++)
	{
		if ((*releaseTrackers)[i]!=NULL)
		{*(*releaseTrackers)[i]=true;}
	}
	*internalReleaseTracker=true;
	theLock->Unlock();
	RakSleep(50);
	delete theLock;
	delete releaseTrackers;
	delete internalReleaseTracker;
	theLock=NULL;

}
