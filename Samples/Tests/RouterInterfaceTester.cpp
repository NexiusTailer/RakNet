#include "RouterInterfaceTester.h"

RouterInterfaceTester::RouterInterfaceTester(void)
{

	Reset();
}

RouterInterfaceTester::~RouterInterfaceTester(void)
{
}

bool RouterInterfaceTester::Send( const char *data, BitSize_t bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, SystemAddress systemAddress )
{

	recievedCall=true;
	return false;
}

void RouterInterfaceTester::Reset ()
{
	recievedCall=false;
}

bool RouterInterfaceTester::wasTriggered()
{

	return recievedCall;

}