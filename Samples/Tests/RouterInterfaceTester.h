#pragma once

#include "RakNetTypes.h"
#include "PluginInterface2.h"
#include "DS_OrderedList.h"
#include "DS_WeightedGraph.h"
#include "PacketPriority.h"
#include "SystemAddressList.h"
#include "RouterInterface.h"
#include "Export.h"
#include "ConnectionGraph.h"
#include "RouterInterface.h"

class RouterInterfaceTester : public RouterInterface
{
public:
	RouterInterfaceTester(void);
	~RouterInterfaceTester(void);
	bool Send( const char *data, BitSize_t bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, SystemAddress systemAddress );
	void Reset ();
	bool wasTriggered();

private:
	bool recievedCall;
};
