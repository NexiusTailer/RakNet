#pragma once
#include "TestInterface.h"

#include <stdio.h>
#include <string.h>
#include "RakString.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakPeer.h"
#include "RakSleep.h"
#include "RakNetTime.h"
#include "GetTime.h"



class MaximumConnectTest : public TestInterface
{
public:
    MaximumConnectTest(void);
    ~MaximumConnectTest(void);
    int RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses);//should return 0 if no error, or the error number
    RakNet::RakString GetTestName();
    RakNet::RakString ErrorCodeToString(int errorCode);

};
