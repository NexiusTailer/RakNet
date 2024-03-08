#pragma once

#include "TestInterface.h"

#include "RakString.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakPeer.h"
#include "RakSleep.h"
#include "RakNetTime.h"
#include "GetTime.h"
#include "DebugTools.h"
//#include "../../DependentExtensions\RPC3\RPC3.h"

#include <stdlib.h> // For atoi
#include <cstring> // For strlen
#include "Rand.h"
#include <stdio.h>

class ComprehensiveConvertTest : public TestInterface
{
public:
	ComprehensiveConvertTest(void);
	~ComprehensiveConvertTest(void);
	int RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses);//should return 0 if no error, or the error number
	RakNet::RakString GetTestName();
	RakNet::RakString ErrorCodeToString(int errorCode);
	void DestroyPeers();
private:
	static const int NUM_PEERS =10;
	RakPeerInterface *peers[NUM_PEERS];

};
