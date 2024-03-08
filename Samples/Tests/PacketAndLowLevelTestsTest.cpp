#include "PacketAndLowLevelTestsTest.h"

#include "malloc.h"

/*
Description:
Tests out the sunctions:
virtual int RakPeerInterface::GetSplitMessageProgressInterval  	(  	void   	  	 )   	 const "
virtual void RakPeerInterface::PushBackPacket  	(  	Packet *   	 packet, 		bool  	pushAtHead	  	) 			"
virtual bool RakPeerInterface::SendList  	(  	char **   	 data, 		const int *  	lengths, 		const int  	numParameters, 		PacketPriority  	priority, 		PacketReliability  	reliability, 		char  	orderingChannel, 		SystemAddress  	systemAddress, 		bool  	broadcast	  	) 			"						
virtual void RakPeerInterface::SetSplitMessageProgressInterval 	( 	int  	interval 	 )  	
virtual void RakPeerInterface::SetUnreliableTimeout  	(  	RakNetTime   	 timeoutMS  	 )   	 
virtual Packet* RakPeerInterface::AllocatePacket  	(  	unsigned   	 dataSize  	 )   
AttachPlugin (PluginInterface2 *plugin)=0
DetachPlugin (PluginInterface2 *plugin)=0

Success conditions:

Failure conditions:

RakPeerInterface Functions used, tested indirectly by its use,list may not be complete:
Startup
SetMaximumIncomingConnections
Receive
DeallocatePacket
Send
IsConnected
RakPeerInterface Functions Explicitly Tested:
GetSplitMessageProgressInterval
PushBackPacket
SendList
SetSplitMessageProgressInterval
AllocatePacket
GetMTUSize
AttachPlugin
DetachPlugin

*/
int PacketAndLowLevelTestsTest::RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)
{
	RakPeerInterface *server,*client;
	destroyList.Clear(false,__FILE__,__LINE__);

	TestHelpers::StandardClientPrep(client,destroyList);
	TestHelpers::StandardServerPrep(server,destroyList);
	printf("Connecting to server\n");
	if (!TestHelpers::WaitAndConnectTwoPeersLocally(client,server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[1-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 1;
	}

	printf("Testing SendList\n");

	char* dataList2[5];
	char **dataList=(char **)dataList2;
	int lengths[5];
	char curString1[]="AAAA";
	char curString2[]="ABBB";
	char curString3[]="ACCC";
	char curString4[]="ADDD";
	char curString5[]="AEEE";

	dataList[0]=curString1;
	dataList[1]=curString2;
	dataList[2]=curString3;
	dataList[3]=curString4;
	dataList[4]=curString5;

	for (int i=0;i<5;i++)
	{
		dataList[i][0]=ID_USER_PACKET_ENUM+1+i;
		lengths[i]=5;
	}

	client->SendList((const char**)dataList,lengths,5,HIGH_PRIORITY,RELIABLE_ORDERED,0, UNASSIGNED_SYSTEM_ADDRESS, true);

	Packet* packet;
	if (!(packet=CommonFunctions::WaitAndReturnMessageWithID(server,ID_USER_PACKET_ENUM+1,1000)))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[9-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 9;
	}

	if (packet->length!=25)
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[13],!noPauses && isVerbose,__LINE__,__FILE__);

		return 14;

	}

	server->DeallocatePacket(packet);

	PluginInterface2* myPlug=new PacketChangerPlugin();

	printf("Test attach detach of plugins\n");
	client->AttachPlugin(myPlug); 
	TestHelpers::BroadCastTestPacket(client);
	if (TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[2-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 2;
	}

	client->DetachPlugin(myPlug); 
	TestHelpers::BroadCastTestPacket(client);
	if (!TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[3-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 3;
	}

	printf("Test AllocatePacket\n");
	Packet * hugePacket,*hugePacket2;
	const int dataSize=3000000;//around 30 meg didn't want to calculate the exact
	hugePacket=client->AllocatePacket(dataSize);
	hugePacket2=client->AllocatePacket(dataSize);

	/*//Couldn't find a good cross platform way for allocated memory so skipped this check
	if (somemalloccheck<3000000)
	{}
	*/

	printf("Assuming 3000000 allocation for splitpacket, testing setsplitpacket\n");

	hugePacket->data[0]=ID_USER_PACKET_ENUM+1;
	hugePacket2->data[0]=ID_USER_PACKET_ENUM+1;

	server->SetSplitMessageProgressInterval(1);

	if (server->GetSplitMessageProgressInterval()!=1)
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[4-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 4;

	}

	if (!client->Send((const char *)hugePacket->data,dataSize,HIGH_PRIORITY,RELIABLE_ORDERED,0, UNASSIGNED_SYSTEM_ADDRESS, true))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[5-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 5;
	}

	if (!CommonFunctions::WaitForMessageWithID(server,ID_DOWNLOAD_PROGRESS,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[6-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 6;
	}

	while(CommonFunctions::WaitForMessageWithID(server,ID_DOWNLOAD_PROGRESS,500))//Clear out the rest before next test
	{
	}

	printf("Making sure still connected, if not connect\n");
	if (!TestHelpers::WaitAndConnectTwoPeersLocally(client,server,5000))//Make sure connected before test
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[11-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 11;
	}

	printf("Making sure standard send/recieve still functioning\n");
	TestHelpers::BroadCastTestPacket(client);
	if (!TestHelpers::WaitForTestPacket(server,5000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[12],!noPauses && isVerbose,__LINE__,__FILE__);

		return 13;
	}

	printf("Testing PushBackPacket\n");

	server->PushBackPacket(hugePacket,false);

	if (!TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[7-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 7;
	}

	printf("Making sure still connected, if not connect\n");
	if (!TestHelpers::WaitAndConnectTwoPeersLocally(client,server,5000))//Make sure connected before test
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[11-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 11;
	}

	printf("Making sure standard send/recieve still functioning\n");
	TestHelpers::BroadCastTestPacket(client);
	if (!TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[12-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 12;
	}

	printf("PushBackPacket head true test\n");
	server->PushBackPacket(hugePacket2,true);

	if (!TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[10-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 10;
	}

	printf("Making sure still connected, if not connect\n");
	if (!TestHelpers::WaitAndConnectTwoPeersLocally(client,server,5000))//Make sure connected before test
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[11-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 11;
	}

	printf("Run recieve test\n");
	TestHelpers::BroadCastTestPacket(client);
	if (!TestHelpers::WaitForTestPacket(server,2000))
	{

		if (isVerbose)
			DebugTools::ShowError(errorList[12-1],!noPauses && isVerbose,__LINE__,__FILE__);

		return 12;
	}

	return 0;

}

void PacketAndLowLevelTestsTest::FloodWithHighPriority(RakPeerInterface* client)
{

	for (int i=0;i<60000;i++)
	{
		TestHelpers::BroadCastTestPacket(client,UNRELIABLE,HIGH_PRIORITY,ID_USER_PACKET_ENUM+2);
	}

}

RakNet::RakString PacketAndLowLevelTestsTest::GetTestName()
{

	return "PacketAndLowLevelTestsTest";

}

RakNet::RakString PacketAndLowLevelTestsTest::ErrorCodeToString(int errorCode)
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

void PacketAndLowLevelTestsTest::DestroyPeers()
{

	int theSize=destroyList.Size();

	for (int i=0; i < theSize; i++)
		RakNetworkFactory::DestroyRakPeerInterface(destroyList[i]);

}

PacketAndLowLevelTestsTest::PacketAndLowLevelTestsTest(void)
{

	errorList.Push("Client failed to connect to server",__FILE__,__LINE__);
	errorList.Push("Attached plugin failed to modify packet",__FILE__,__LINE__);
	errorList.Push("Plugin is still modifying packets after detach",__FILE__,__LINE__);
	errorList.Push("GetSplitMessageProgressInterval returned wrong value",__FILE__,__LINE__);
	errorList.Push("Send to server failed",__FILE__,__LINE__);
	errorList.Push("Large packet did not split or did not properly get ID_DOWNLOAD_PROGRESS after SetSplitMessageProgressInterval is set to 1 millisecond",__FILE__,__LINE__);
	errorList.Push("Did not recieve and put on packet made with AllocatePacket and put on recieve stack with PushBackPacket",__FILE__,__LINE__);
	errorList.Push("Client failed to connect to server",__FILE__,__LINE__);
	errorList.Push("Did not recieve all packets from SendList",__FILE__,__LINE__);
	errorList.Push("Did not recieve and put on packet made with AllocatePacket and put on recieve stack with PushBackPacket",__FILE__,__LINE__);
	errorList.Push("Client failed to connect to server",__FILE__,__LINE__);
	errorList.Push("PushBackPacket messed up future communication",__FILE__,__LINE__);
	errorList.Push("Send/Recieve failed",__FILE__,__LINE__);
	errorList.Push("Recieved size incorrect",__FILE__,__LINE__);

}

PacketAndLowLevelTestsTest::~PacketAndLowLevelTestsTest(void)
{
}