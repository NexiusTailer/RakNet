#pragma once

#include "RakString.h"
#include "DS_List.h"

class TestInterface
{
public:
	TestInterface();
	virtual ~TestInterface();
	virtual int RunTest(DataStructures::List<RakNet::RakString> params,bool isVerbose,bool noPauses)=0;//should return 0 if no error, or the error number
	virtual RakNet::RakString GetTestName()=0;
	virtual RakNet::RakString ErrorCodeToString(int errorCode)=0;
	virtual void DestroyPeers()=0;
};
