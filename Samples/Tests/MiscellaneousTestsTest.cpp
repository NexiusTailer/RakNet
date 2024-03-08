#include "MiscellaneousTestsTest.h"

/*
Description:
Tests:
virtual void 	SetRouterInterface (RouterInterface *routerInterface)=0
virtual void 	RemoveRouterInterface (RouterInterface *routerInterface)=0
virtual bool 	AdvertiseSystem (const char *host, unsigned short remotePort, const char *data, int dataLength, unsigned connectionSocketIndex=0)=0

Success conditions:

Failure conditions:

RakPeerInterface Functions used, tested indirectly by its use,list may not be complete:
Startup
SetMaximumIncomingConnections
Receive
DeallocatePacket
Send

RakPeerInterface Functions Explicitly Tested:
SetRouterInterface
RemoveRouterInterface
AdvertiseSystem

*/
int MiscellaneousTestsTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{	destroyList.Clear(false,__FILE__,__LINE__);

RakPeerInterface *client,*server;

TestHelpers::StandardClientPrep(client,destroyList);
TestHelpers::StandardServerPrep(server,destroyList);

printf("Testing AdvertiseSystem\n");

client->AdvertiseSystem("127.0.0.1",60000,0,0);

if (!CommonFunctions::WaitForMessageWithID(server,ID_ADVERTISE_SYSTEM,5000))
{

	if (isVerbose)
		DebugTools::ShowError(errorList[1-1],!noPauses && isVerbose,__LINE__,__FILE__);

	return 1;
}

RouterInterfaceTester * riTester= new RouterInterfaceTester();

printf("Testing SetRouterInterface\n");

client->SetRouterInterface(riTester);

if (riTester->wasTriggered())
{

	if (isVerbose)
		DebugTools::ShowError(errorList[2-1],!noPauses && isVerbose,__LINE__,__FILE__);

	return 2;
}

TestHelpers::SendTestPacketDirected(client,"127.0.0.1",60001);

if (!riTester->wasTriggered())
{

	if (isVerbose)
		DebugTools::ShowError(errorList[3-1],!noPauses && isVerbose,__LINE__,__FILE__);

	return 3;
}

printf("Testing RemoveRouterInterface\n");
client->RemoveRouterInterface(riTester);
riTester->Reset();

TestHelpers::SendTestPacketDirected(client,"127.0.0.1",60001);

if (riTester->wasTriggered())
{

	if (isVerbose)
		DebugTools::ShowError(errorList[4-1],!noPauses && isVerbose,__LINE__,__FILE__);

	return 4;
}

return 0;

}

RakNet::RakString MiscellaneousTestsTest::GetTestName()
{

	return "MiscellaneousTestsTest";

}

RakNet::RakString MiscellaneousTestsTest::ErrorCodeToString(int errorCode)
{

	if (errorCode>0&&(unsigned int)errorCode<=errorList.Size())
	{
		return errorList[errorCode-1];
	}
	else
	{
		return "Undefined Error";
	}	

}

void MiscellaneousTestsTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}

MiscellaneousTestsTest::MiscellaneousTestsTest(void)
{

	errorList.Push("Did not recieve client advertise",__FILE__,__LINE__);
	errorList.Push("The router interface should not be called because no send has happened yet",__FILE__,__LINE__);
	errorList.Push("Router failed to trigger on failed directed send",__FILE__,__LINE__);
	errorList.Push("Router was not properly removed",__FILE__,__LINE__);

}

MiscellaneousTestsTest::~MiscellaneousTestsTest(void)
{
}