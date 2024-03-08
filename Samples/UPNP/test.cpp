#include "UPNPPortForwarder.h"
#include "RakSleep.h"

int cbCount=0;

using namespace RakNet;

class UPNPCallbackInterface_Printf : public UPNPCallbackInterface 
{
	virtual void UPNPStatusUpdate(RakNet::RakString stringToPrint)
	{
		printf(stringToPrint.C_String());
	}

	/// \param[out] interfacesFound List of length equal to or less than interfacesQueried. May be none.
	virtual void QueryUPNPSupport_Result(QueryUPNPSupportResult *result)
	{
		cbCount++;
	}
	virtual void OpenPortOnInterface_Result(OpenPortResult *result)
	{
		cbCount++;
	}
};

int main (int argc,char * argv[]){

	UPNPPortForwarder testInterface;
	UPNPCallbackInterface_Printf cb;

	testInterface.QueryUPNPSupport(&cb);
	RakSleep(15000);
	testInterface.OpenPortOnInterface(&cb,65534);
	RakSleep(15000);
	testInterface.OpenPortOnInterface(&cb,65535);
	RakSleep(15000);
	testInterface.OpenPortOnInterface(&cb,65533);
	RakSleep(15000);

	while (cbCount<4)
	{
		testInterface.CallCallbacks();
		RakSleep(50);
	}

	return 0;


}
