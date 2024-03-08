#include "SQLiteClientLogger_RNSLogger.h"
#include "RakNetTime.h"
#include "GetTime.h"
#include "RakNetStatistics.h"
#include "RakPeerInterface.h"
#include "SQLiteClientLoggerPlugin.h"

using namespace RakNet;

static const char *DEFAULT_RAKNET_STATISTICS_TABLE="RakNetStatistics";

SQLiteClientLogger_RakNetStatistics::SQLiteClientLogger_RakNetStatistics()
{
	lastUpdate=0;
}
SQLiteClientLogger_RakNetStatistics::~SQLiteClientLogger_RakNetStatistics()
{
}
void SQLiteClientLogger_RakNetStatistics::Update(void)
{
	RakNetTimeUS time = RakNet::GetTimeUS();
	if (time-lastUpdate>1000000)
	{
		lastUpdate=time;
		unsigned int i;
		char buff[32];
		RakNetStatistics rns;
		for (i=0; i < rakPeerInterface->GetMaximumNumberOfPeers(); i++)
		{
			if (rakPeerInterface->GetStatistics( i, &rns ))
			{
				rakPeerInterface->GetSystemAddressFromIndex(i).ToString(true,buff);

				rakSqlLog(DEFAULT_RAKNET_STATISTICS_TABLE,"SystemAddress,TotalBytesSent,TotalBytesReceived,SendBufferMessageCount,Packetloss,UnacknowledgedBytes,UnacknowledgedBytesCap,InSlowStart,LocalSendRate,localContinuousReceiveRate,remoteContinuousReceiveRate,estimatedLinkCapacityMBPS",\
					(\
					buff, \
					BITS_TO_BYTES( rns.totalBitsSent ), \
					BITS_TO_BYTES( rns.bitsReceived + rns.bitsWithBadCRCReceived ), \
					rns.messageSendBuffer[ SYSTEM_PRIORITY ] + rns.messageSendBuffer[ HIGH_PRIORITY ] + rns.messageSendBuffer[ MEDIUM_PRIORITY ] + rns.messageSendBuffer[ LOW_PRIORITY ], \
					100.0f * ( float ) rns.messagesTotalBitsResent / ( float ) (rns.totalBitsSent+rns.messagesTotalBitsResent), \
					rns.unacknowledgedBytes, \
					rns.CWNDLimit, \
					rns.isInSlowStart, \
					rns.localSendRate, \
					rns.localContinuousReceiveRate, \
					rns.remoteContinuousReceiveRate, \
					rns.estimatedLinkCapacityMBPS \
					) );
			}
		}
	}
}
