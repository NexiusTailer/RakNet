#include "CCRakNetUDT.h"
#include "Rand.h"
#include "MTUSize.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
//#include <memory.h>
#include "RakAssert.h"

using namespace RakNet;

static const double UNSET_TIME_US=-1;
static const double CWND_MIN_THRESHOLD=2.0;
static const double UNDEFINED_TRANSFER_RATE=0.0;
/// Interval at which to update aspects of the system
/// 1. send acks
/// 2. update time interval between outgoing packets
/// 3, Yodate retransmit timeout
#if CC_TIME_TYPE_BYTES==4
static const CCTimeType SYN=10;
#else
static const CCTimeType SYN=10000;
#endif

double RTTVarMultiple=4.0;


// ****************************************************** PUBLIC METHODS ******************************************************

CCRakNetUDT::CCRakNetUDT()
{
}

// ----------------------------------------------------------------------------------------------------------------------------

CCRakNetUDT::~CCRakNetUDT()
{
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::Init(CCTimeType curTime, uint32_t maxDatagramPayload)
{
	(void) curTime;

	nextSYNUpdate=0;
	packetPairRecieptHistoryWriteIndex=0;
	packetArrivalHistoryWriteIndex=0;
	RTT=UNSET_TIME_US;
//	RTTVar=UNSET_TIME_US;
	isInSlowStart=true;
	LastDecSeq=0;
	NAKCount=1000;
	AvgNAKNum=1;
	DecInterval=1;
	DecCount=0;
	nextDatagramSequenceNumber=0;
	lastPacketPairPacketArrivalTime=0;
	lastPacketPairSequenceNumber=(DatagramSequenceNumberType)(const uint32_t)-1;
	lastPacketArrivalTime=0;
	CWND=CWND_MIN_THRESHOLD;
	lastUpdateWindowSizeAndAck=0;
	lastTransmitOfBAndAS=0;
	halveSNDOnNoDataTime=0;
	nextAllowedSend=0;
	ExpCount=1.0;
	totalUserDataBytesSent=0;
	oldestUnsentAck=0;
	MAXIMUM_MTU_INCLUDING_UDP_HEADER=maxDatagramPayload;
	CWND_MAX_THRESHOLD=25600;
#if CC_TIME_TYPE_BYTES==4
	const BytesPerMicrosecond DEFAULT_TRANSFER_RATE=(BytesPerMicrosecond) 3.6;
#else
	const BytesPerMicrosecond DEFAULT_TRANSFER_RATE=(BytesPerMicrosecond) .0036;
#endif
	B=DEFAULT_TRANSFER_RATE;
	AS=UNDEFINED_TRANSFER_RATE;
	const MicrosecondsPerByte DEFAULT_BYTE_INTERVAL=(MicrosecondsPerByte) (1.0/DEFAULT_TRANSFER_RATE);
	SND=DEFAULT_BYTE_INTERVAL;
	expectedNextSequenceNumber=0;
	sendBAndASCount=0;
	packetArrivalHistoryContinuousGapsIndex=0;
	packetPairRecipetHistoryGapsIndex=0;
	hasWrittenToPacketPairReceiptHistory=false;
	InitPacketArrivalHistory();
	InitPacketPairRecieptHistory();
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::SetMTU(uint32_t bytes)
{
	MAXIMUM_MTU_INCLUDING_UDP_HEADER=bytes;
	CWND_MAX_THRESHOLD=(1024*256)/MAXIMUM_MTU_INCLUDING_UDP_HEADER;
}
// ----------------------------------------------------------------------------------------------------------------------------
uint32_t CCRakNetUDT::GetMTU(void) const
{
	return MAXIMUM_MTU_INCLUDING_UDP_HEADER;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::Update(CCTimeType curTime, bool hasDataToSendOrResend)
{
	(void) hasDataToSendOrResend;
	(void) curTime;

	return;

	// I suspect this is causing major lag


	if (hasDataToSendOrResend==false)
		halveSNDOnNoDataTime=0;
	else if (halveSNDOnNoDataTime==0)
	{
		UpdateHalveSNDOnNoDataTime(curTime);
		ExpCount=1.0;
	}

	// If you send, and get no data at all from that time to RTO, then halve send rate7
	if (HasHalveSNDOnNoDataTimeElapsed(curTime))
	{
		/// 2000 bytes per second
		/// 0.0005 seconds per byte
		/// 0.5 milliseconds per byte
		/// 500 microseconds per byte
		// printf("No incoming data, halving send rate\n");
		SND*=2.0;
		CapMinSnd(__FILE__,__LINE__);
		ExpCount+=1.0;
		if (ExpCount>8.0)
			ExpCount=8.0;

		UpdateHalveSNDOnNoDataTime(curTime);
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
uint32_t CCRakNetUDT::GetRetransmissionBandwidth(CCTimeType curTime, CCTimeType estimatedTimeToNextTick)
{
	if (CCRakNetUDT::isInSlowStart)
		return 25600*MAXIMUM_MTU_SIZE;
	if (curTime+estimatedTimeToNextTick<nextAllowedSend)
		return 0;
	RakAssert(SND!=0);
	double availableTime = (double) estimatedTimeToNextTick;
#if CC_TIME_TYPE_BYTES==4
	if (availableTime > 100.0) // Sanity check
		availableTime=1000.0;
#else
	if (availableTime > 100000.0) // Sanity check
		availableTime=1000000.0;
#endif	
	uint32_t byteLimitDueToAvailableTime = (uint32_t) (availableTime/SND);
	return byteLimitDueToAvailableTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
uint32_t CCRakNetUDT::GetNewTransmissionBandwidth(CCTimeType curTime, CCTimeType estimatedTimeToNextTick, CCTimeType timeSinceLastContinualSend, uint32_t unacknowledgedBytes)
{
	(void) timeSinceLastContinualSend;
	if (unacknowledgedBytes>=CWND*MAXIMUM_MTU_INCLUDING_UDP_HEADER)
	{
		if (isInSlowStart==false)
		{
			CC_DEBUG_PRINTF_3("NOSEND_SLOWSTART: CWND Cap. unacknowledgedBytes=%i, CWND=%i\n", unacknowledgedBytes, (uint32_t) (CWND*MAXIMUM_MTU_INCLUDING_UDP_HEADER));
		}
		else
		{
			CC_DEBUG_PRINTF_3("NOSEND_SND: CWND Cap. unacknowledgedBytes=%i, CWND=%i\n", unacknowledgedBytes, (uint32_t) (CWND*MAXIMUM_MTU_INCLUDING_UDP_HEADER));
		}
		return 0;
	}

	uint32_t CWNDLimit = (uint32_t) (CWND*MAXIMUM_MTU_INCLUDING_UDP_HEADER-unacknowledgedBytes);
	if (isInSlowStart)
	{
		CC_DEBUG_PRINTF_3("SEND_SLOWSTART: %i allowed under CWND. CWND=%i\n", CWNDLimit, (uint32_t) (CWND*MAXIMUM_MTU_INCLUDING_UDP_HEADER));
		return CWNDLimit;
	}
	else
	{
		if (curTime+estimatedTimeToNextTick>=nextAllowedSend)
		{
			CCTimeType availableTime;
			uint32_t byteLimitDueToAvailableTime;
			if (nextAllowedSend==0)
			{
				availableTime=estimatedTimeToNextTick;
				byteLimitDueToAvailableTime = (uint32_t) (availableTime/SND);
				if (byteLimitDueToAvailableTime<GetMTU())
					return GetMTU();
			}
			else
			{
				availableTime=curTime+estimatedTimeToNextTick-nextAllowedSend;
				byteLimitDueToAvailableTime = (uint32_t) (availableTime/SND);
			}
			if (CWNDLimit < byteLimitDueToAvailableTime)
			{
				CC_DEBUG_PRINTF_2("SEND_SND: %i capped under CWND\n", CWNDLimit);
				return CWNDLimit;
			}
			else
			{
				CC_DEBUG_PRINTF_2("SEND_SND: %i capped under time\n", byteLimitDueToAvailableTime);
				return byteLimitDueToAvailableTime;
			}	
		}
		else
		{
			// Can't send until more time passes
#if CC_TIME_TYPE_BYTES==4
			CC_DEBUG_PRINTF_2("NOSEND_SND: SND Cap. %i MS delayed\n", (int)((nextAllowedSend-curTime)));
#else
			CC_DEBUG_PRINTF_2("NOSEND_SND: SND Cap. %i MS delayed\n", (int)((nextAllowedSend-curTime)/1000));
#endif
			return 0;
		}
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::ShouldSendACKs(CCTimeType curTime, CCTimeType estimatedTimeToNextTick)
{
	CCTimeType rto = GetSenderRTOForACK();

	// iphone crashes on comparison between double and int64 http://www.jenkinssoftware.com/forum/index.php?topic=2717.0
	if (rto==(CCTimeType) UNSET_TIME_US)
	{
		// Unknown how long until the remote system will retransmit, so better send right away
		return true;
	}

	
//	CCTimeType remoteRetransmitTime=oldestUnsentAck+rto-RTT*.5;
//	CCTimeType ackArrivalTimeIfWeDelay=RTT*.5+estimatedTimeToNextTick+curTime;
//	return ackArrivalTimeIfWeDelay<remoteRetransmitTime;

	// Simplified equation
	// GU: At least one ACK should be sent per SYN, otherwise your protocol will increase slower.
	return curTime >= oldestUnsentAck + SYN ||
		estimatedTimeToNextTick+curTime < oldestUnsentAck+rto-RTT;
}
// ----------------------------------------------------------------------------------------------------------------------------
DatagramSequenceNumberType CCRakNetUDT::GetNextDatagramSequenceNumber(void)
{
	DatagramSequenceNumberType dsnt=nextDatagramSequenceNumber;
	nextDatagramSequenceNumber++;
	return dsnt;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSendBytes(CCTimeType curTime, uint32_t numBytes)
{
	totalUserDataBytesSent+=numBytes;
	UpdateNextAllowedSend(curTime, numBytes);

	if (HasHalveSNDOnNoDataTimeElapsed(curTime))
	{
		// Set a timer for which if no acks or naks arrive, we halve the send rate
		UpdateHalveSNDOnNoDataTime(curTime);
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateNextAllowedSend(CCTimeType curTime, uint32_t numBytes)
{
	if (isInSlowStart)
	{
	}
	else
	{
		// Update the next time we can send again
		nextAllowedSend+=(CCTimeType)((double)numBytes*SND);

		if (nextAllowedSend+30000<curTime)
			nextAllowedSend=curTime+(CCTimeType)((double)numBytes*SND);
	}
}
// ****************************************************** UNIT TESTING ******************************************************
bool CCRakNetUDT::UnitTest(void)
{
	bool testCompleted;
	testCompleted=TestReceiverCalculateLinkCapacityMedian();
	if (!testCompleted)
	{
		RakAssert("TestReceiverCalculateLinkCapacityMedian unit test failed" && 0);
		return false;
	}

	DatagramSequenceNumberType a, b;
	a=0;
	b=1;
	RakAssert(GreaterThan(b,a));
	RakAssert(LessThan(a,b));
	a=(DatagramSequenceNumberType)(const uint32_t)-1;
	b=0;
	RakAssert(GreaterThan(b,a));
	RakAssert(LessThan(a,b));

	return true;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::TestReceiverCalculateLinkCapacityMedian(void)
{
	CCRakNetUDT ccRakNetUDT;
	int i;
	int lt=0,gt=0,eq=0;
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
		ccRakNetUDT.packetPairRecieptHistory[i]=frandomMT();
	BytesPerMicrosecond calculatedMedian=ccRakNetUDT.ReceiverCalculateLinkCapacityMedian();
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		if (ccRakNetUDT.packetPairRecieptHistory[i]<calculatedMedian)
			lt++;
		else if (ccRakNetUDT.packetPairRecieptHistory[i]>calculatedMedian)
			gt++;
		else
			eq++;
	}
	RakAssert(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH%2==0);

//	BytesPerMicrosecond averageOfMedianFilteredValues=ccRakNetUDT.ReceiverCalculateLinkCapacity();

	return lt==gt || lt==gt+1 || lt==gt-1 || eq!=1;
}






// ****************************************************** PROTECTED METHODS ******************************************************

void CCRakNetUDT::SetNextSYNUpdate(CCTimeType currentTime)
{
	nextSYNUpdate+=SYN;
	if (nextSYNUpdate < currentTime)
		nextSYNUpdate=currentTime+SYN;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateLinkCapacity(void) const
{
	BytesPerMicrosecond median = ReceiverCalculateLinkCapacityMedian();
	int i;
	const BytesPerMicrosecond oneEighthMedian=median*(1.0/8.0);
	const BytesPerMicrosecond eightTimesMedian=median*8.0f;
	BytesPerMicrosecond medianListLength=0.0;
	BytesPerMicrosecond sum=0.0;
	// Find average of acceptedMedianValues
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		if (packetPairRecieptHistory[i]>=oneEighthMedian &&
			packetPairRecieptHistory[i]<eightTimesMedian)
		{
			medianListLength=medianListLength+1.0;
			sum+=packetPairRecieptHistory[i];
		}
	}
	if (medianListLength==0.0)
		return UNDEFINED_TRANSFER_RATE;
	return sum/medianListLength;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateLinkCapacityMedian(void) const
{
	return CalculateListMedianRecursive(packetPairRecieptHistory, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateDataArrivalRate(CCTimeType curTime) const
{
	if (continuousBytesReceivedStartTime!=0 && curTime>continuousBytesReceivedStartTime)
		return (BytesPerMicrosecond) continuousBytesReceived/(BytesPerMicrosecond) (curTime-continuousBytesReceivedStartTime);

	BytesPerMicrosecond median = ReceiverCalculateDataArrivalRateMedian();
	int i;
	const BytesPerMicrosecond oneEighthMedian=median*(1.0/8.0);
	const BytesPerMicrosecond eightTimesMedian=median*8.0f;
	BytesPerMicrosecond medianListLength=0.0;
	BytesPerMicrosecond sum=0.0;
	// Find average of acceptedMedianValues
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		if (packetArrivalHistory[i]>=oneEighthMedian &&
			packetArrivalHistory[i]<eightTimesMedian)
		{
			medianListLength=medianListLength+1.0;
			sum+=packetArrivalHistory[i];
		}
	}
	if (medianListLength==0.0)
		return UNDEFINED_TRANSFER_RATE;
	return sum/medianListLength;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateDataArrivalRateMedian(void) const
{
	return CalculateListMedianRecursive(packetArrivalHistory, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::CalculateListMedianRecursive(const BytesPerMicrosecond inputList[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH], int inputListLength, int lessThanSum, int greaterThanSum)
{
	BytesPerMicrosecond lessThanMedian[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH], greaterThanMedian[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH];
	int lessThanMedianListLength=0, greaterThanMedianListLength=0;
	BytesPerMicrosecond median=inputList[0];
	int i;
	for (i=1; i < inputListLength; i++)
	{
		// If same value, spread among lists evenly
		if (inputList[i]<median || ((i&1)==0 && inputList[i]==median))
			lessThanMedian[lessThanMedianListLength++]=inputList[i];
		else
			greaterThanMedian[greaterThanMedianListLength++]=inputList[i];
	}
	RakAssert(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH%2==0);
	if (lessThanMedianListLength+lessThanSum==greaterThanMedianListLength+greaterThanSum+1 ||
		lessThanMedianListLength+lessThanSum==greaterThanMedianListLength+greaterThanSum-1)
		return median;
	
	if (lessThanMedianListLength+lessThanSum < greaterThanMedianListLength+greaterThanSum)
	{
		lessThanMedian[lessThanMedianListLength++]=median;
		return CalculateListMedianRecursive(greaterThanMedian, greaterThanMedianListLength, lessThanMedianListLength+lessThanSum, greaterThanSum);
	}
	else
	{
		greaterThanMedian[greaterThanMedianListLength++]=median;
		return CalculateListMedianRecursive(lessThanMedian, lessThanMedianListLength, lessThanSum, greaterThanMedianListLength+greaterThanSum);		
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::GreaterThan(DatagramSequenceNumberType a, DatagramSequenceNumberType b)
{
	 // a > b?
	const DatagramSequenceNumberType halfSpan =(DatagramSequenceNumberType) (((DatagramSequenceNumberType)(const uint32_t)-1)/(DatagramSequenceNumberType)2);
	return b!=a && b-a>halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::LessThan(DatagramSequenceNumberType a, DatagramSequenceNumberType b)
{
	// a < b?
	const DatagramSequenceNumberType halfSpan = ((DatagramSequenceNumberType)(const uint32_t)-1)/(DatagramSequenceNumberType)2;
	return b!=a && b-a<halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
CCTimeType CCRakNetUDT::GetSenderRTOForACK(void) const
{
	if (RTT==UNSET_TIME_US)
		return (CCTimeType) UNSET_TIME_US;
	double RTTVar = maxRTT-minRTT;
	return (CCTimeType)(RTT + RTTVarMultiple * RTTVar + SYN);
}
// ----------------------------------------------------------------------------------------------------------------------------
CCTimeType CCRakNetUDT::GetRTOForRetransmission(void) const
{
#if CC_TIME_TYPE_BYTES==4
	const CCTimeType maxThreshold=1000;
	const CCTimeType minThreshold=100;
#else
	const CCTimeType maxThreshold=1000000;
	const CCTimeType minThreshold=100000;
#endif

	if (RTT==UNSET_TIME_US)
	{
		return (CCTimeType) maxThreshold;
	}
	// Limit to reasonable values. This may hit due to bad systems
	double RTTVar = maxRTT-minRTT;
	CCTimeType time = (CCTimeType) ((ExpCount * (RTT + RTTVarMultiple * RTTVar) + (double) SYN));
	if (time < minThreshold)
		return minThreshold;
	if (time > maxThreshold)
		return maxThreshold;
	return time;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnNAK(CCTimeType curTime, DatagramSequenceNumberType nakSequenceNumber)
{
	ResetOnDataArrivalHalveSNDOnNoDataTime(curTime);

	if (isInSlowStart)
	{
		if (AS!=UNDEFINED_TRANSFER_RATE)
			EndSlowStart();
		return;
	}

	if (nakSequenceNumber>=LastDecSeq)
	{
		// Slow down sends
		SND*=1.125;

		// Test
//		printf("NEW_CONGESTION_PERIOD,start=%i,stop=%i,SND*=1.125=%f\n",nakSequenceNumber.val,nextDatagramSequenceNumber.val,1.0/SND);

		CapMinSnd(__FILE__,__LINE__);
		// Update average number of NAKs per congestion period
		AvgNAKNum=NAKCount;
		// Restart number of NAKs this congestion period
		NAKCount=1;
		// Every interval number of NAKs, we slow the send rate (in addition to the first)
		DecInterval=1+(randomMT()%AvgNAKNum);
		// Decremented send rate 1 time this congestion period
		DecCount=1;
		// Sequence number that was most recently sent this congestion period
		LastDecSeq=nextDatagramSequenceNumber-(DatagramSequenceNumberType)1;

		return;
	}

	++NAKCount;

	if (DecCount<=5 && (NAKCount%DecInterval)==0)
	{
		// Slow down sends more
		SND*=1.125;

		// Test
//		printf("DecInterval,SND*=1.125=%f ",1.0/SND);

		CapMinSnd(__FILE__,__LINE__);
		// Decremented again
		DecCount++;
	}

	// Record continuing congestion period
	LastDecSeq=nextDatagramSequenceNumber-(DatagramSequenceNumberType)1;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::EndSlowStart(void)
{
	RakAssert(isInSlowStart==true);
	RakAssert(AS!=UNDEFINED_TRANSFER_RATE);

	isInSlowStart=false;
	SND=1.0/AS;
	CapMinSnd(__FILE__,__LINE__);

	printf("ENDING SLOW START\n");
#if CC_TIME_TYPE_BYTES==4
	printf("Initial SND=%f Kilobytes per second\n", 1.0/SND);
#else
	printf("Initial SND=%f Megabytes per second\n", 1.0/SND);
#endif
	if (SND > .1)
		PrintLowBandwidthWarning();	
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnGotPacketPair(DatagramSequenceNumberType datagramSequenceNumber, uint32_t sizeInBytes, CCTimeType curTime)
{
	if (datagramSequenceNumber-(DatagramSequenceNumberType)1==lastPacketPairSequenceNumber && lastPacketPairPacketArrivalTime!=0)
	{
		// Is a pair
		if (curTime>lastPacketPairPacketArrivalTime)
		{
		//	if ((BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)(curTime-lastPacketPairPacketArrivalTime)<1.0)
		//	{
		//		int a=5;
		//	}
			// TODO - on perm slowdown, packet pairs never sent
//			printf("Got PP %f MGBPSEC\n", (BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)(curTime-lastPacketPairPacketArrivalTime));

//			printf("Packet Pair gap is %I64u\n", (curTime-lastPacketPairPacketArrivalTime));

			BytesPerMicrosecond val = (BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)(curTime-lastPacketPairPacketArrivalTime);
			// .0036 is 28.8K modem. Anything less is considered an error due to timing issues
			//RakAssert(val >=  0.0035);
			if (val < 0.0035)
				return;

			mostRecentPacketPairValue=val;
			if (hasWrittenToPacketPairReceiptHistory==false)
			{
				hasWrittenToPacketPairReceiptHistory=true;
				for (int j=0; j < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; j++)
					packetPairRecieptHistory[j]=mostRecentPacketPairValue;

				packetPairRecieptHistoryWriteIndex=1;
			}
			else
			{
				packetPairRecieptHistory[packetPairRecieptHistoryWriteIndex++]=mostRecentPacketPairValue;
				packetPairRecieptHistoryGaps[packetPairRecipetHistoryGapsIndex++]=(int)(curTime-lastPacketPairPacketArrivalTime);
				packetPairRecipetHistoryGapsIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);
				// Wrap to 0 at the end of the range
				// Assumes power of 2 for CC_RAKNET_UDT_PACKET_HISTORY_LENGTH
				packetPairRecieptHistoryWriteIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);
			}
			
		
		}
	}
	else
	{
		lastPacketPairSequenceNumber=datagramSequenceNumber;
		lastPacketPairPacketArrivalTime=curTime;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::OnGotPacket(DatagramSequenceNumberType datagramSequenceNumber, bool isContinuousSend, CCTimeType curTime, uint32_t sizeInBytes, uint32_t *skippedMessageCount)
{	
	CC_DEBUG_PRINTF_2("R%i ",datagramSequenceNumber.val);


	if (datagramSequenceNumber==expectedNextSequenceNumber)
	{
		*skippedMessageCount=0;
		expectedNextSequenceNumber=datagramSequenceNumber+(DatagramSequenceNumberType)1;
	}
	else if (GreaterThan(datagramSequenceNumber, expectedNextSequenceNumber))
	{
		*skippedMessageCount=datagramSequenceNumber-expectedNextSequenceNumber;
		// Sanity check, just use timeout resend if this was really valid
		if (*skippedMessageCount>1000)
		{
			// During testing, the nat punchthrough server got 51200 on the first packet. I have no idea where this comes from, but has happened twice
			if (*skippedMessageCount>(uint32_t)50000)
				return false;
			*skippedMessageCount=1000;
		}
		expectedNextSequenceNumber=datagramSequenceNumber+(DatagramSequenceNumberType)1;
	}
	else
	{
		*skippedMessageCount=0;
	}

	ResetOnDataArrivalHalveSNDOnNoDataTime(curTime);
	
	if (curTime>lastPacketArrivalTime)
	{
		CCTimeType interval = curTime-lastPacketArrivalTime;

//		printf("Packet arrival gap is %I64u\n", (interval));

		if (isContinuousSend)
		{
			continuousBytesReceived+=sizeInBytes;
			if (continuousBytesReceivedStartTime==0)
				continuousBytesReceivedStartTime=lastPacketArrivalTime;


			mostRecentPacketArrivalHistory=(BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)interval;

	//		if (mostRecentPacketArrivalHistory < (BytesPerMicrosecond)0.0035)
	//		{
	//			printf("%s:%i LIKELY BUG: Calculated packetArrivalHistory is below 28.8 Kbps modem\nReport to rakkar@jenkinssoftware.com with file and line number\n", __FILE__, __LINE__);
	//		}

			packetArrivalHistoryContinuousGaps[packetArrivalHistoryContinuousGapsIndex++]=(int) interval;
			packetArrivalHistoryContinuousGapsIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);

			packetArrivalHistory[packetArrivalHistoryWriteIndex++]=mostRecentPacketArrivalHistory;
			// Wrap to 0 at the end of the range
			// Assumes power of 2 for CC_RAKNET_UDT_PACKET_HISTORY_LENGTH
			packetArrivalHistoryWriteIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);
		}
		else
		{
			continuousBytesReceivedStartTime=0;
			continuousBytesReceived=0;
		}

		lastPacketArrivalTime=curTime;
	}
	return true;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnAck(CCTimeType curTime, CCTimeType rtt, bool hasBAndAS, BytesPerMicrosecond _B, BytesPerMicrosecond _AS, double totalUserDataBytesAcked )
{
#if CC_TIME_TYPE_BYTES==4
	RakAssert(rtt < 10000);
#else
	RakAssert(rtt < 10000000);
#endif
	if (hasBAndAS)
	{
		RakAssert(_B!=UNDEFINED_TRANSFER_RATE && _AS!=UNDEFINED_TRANSFER_RATE);
		B=B * .875 + _B * .125;
		// AS is packet arrival rate
		AS=_AS;
		CC_DEBUG_PRINTF_4("ArrivalRate=%f linkCap=%f incomingLinkCap=%f\n", _AS,B,_B);
	}
	UpdateRTT(rtt);
	ResetOnDataArrivalHalveSNDOnNoDataTime(curTime);

	if (oldestUnsentAck==0)
		oldestUnsentAck=curTime;

	if (lastUpdateWindowSizeAndAck+SYN > curTime)
		return;

	if (isInSlowStart==true)
		UpdateWindowSizeAndAckOnAckPreSlowStart(totalUserDataBytesAcked);
	else
		UpdateWindowSizeAndAckOnAckPerSyn(curTime);

	lastUpdateWindowSizeAndAck=curTime;

}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSendAckGetBAndAS(CCTimeType curTime, bool *hasBAndAS, BytesPerMicrosecond *_B, BytesPerMicrosecond *_AS)
{
	if (curTime>lastTransmitOfBAndAS+SYN)
	{		
		// Median doesn't work, too high
		*_B=ReceiverCalculateLinkCapacity();

		/*
		// Lowest value doesn't work, too low
		BytesPerMicrosecond lowestValue=packetPairRecieptHistory[0];
		for (int i=1; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
		{
			if (packetPairRecieptHistory[i]<lowestValue)
				lowestValue=packetPairRecieptHistory[i];
		}
		*_B=lowestValue;
		*/

		*_AS=ReceiverCalculateDataArrivalRate(curTime);

		
		if (*_B==UNDEFINED_TRANSFER_RATE || *_AS==UNDEFINED_TRANSFER_RATE)
		{
			*hasBAndAS=false;
		}
		else
		{
			if (packetArrivalHistoryContinuousGaps[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1]!=0)
			{
				// BytesPerMicrosecond packetPairGapValue = CalculateListMedianRecursive(packetPairRecieptHistoryGaps, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);

				BytesPerMicrosecond packetPairGapLow=packetPairRecieptHistoryGaps[0];
				for (int i=1; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
				{
					if (packetPairRecieptHistoryGaps[i]<packetPairGapLow)
						packetPairGapLow=packetPairRecieptHistoryGaps[i];
				}
				BytesPerMicrosecond packetPairGapValue;
				BytesPerMicrosecond sum;
				sum=0;
				for (int i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
				{
					if (packetPairRecieptHistoryGaps[i]<packetPairGapLow)
						sum+=packetPairRecieptHistoryGaps[i];
				}
				sum/=CC_RAKNET_UDT_PACKET_HISTORY_LENGTH;
				packetPairGapValue=sum;

			//	BytesPerMicrosecond arrivalHistoryVal = CalculateListMedianRecursive(packetArrivalHistoryContinuousGaps, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);
				sum=0;
				for (int i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
				{
					sum+=packetArrivalHistoryContinuousGaps[i];
				}
				BytesPerMicrosecond arrivalHistoryVal=sum/(BytesPerMicrosecond)CC_RAKNET_UDT_PACKET_HISTORY_LENGTH;
				if (arrivalHistoryVal > packetPairGapValue)
				{
					double avgMicrosecondsLostDueToCompetingBandwidth = arrivalHistoryVal-packetPairGapValue;
					// TODO - should be actual bytes sent, not max
					double interval=(double)GetMTU()/ *_B;
					interval+=avgMicrosecondsLostDueToCompetingBandwidth;
//					printf("Modifying _B from %f to ", *_B);
					*_B = (double)GetMTU()/interval;
//					printf("%f\n", *_B);
				}
			}

			sendBAndASCount++;
			lastTransmitOfBAndAS=curTime;
			*hasBAndAS=true;
			CC_DEBUG_PRINTF_3("DataArrivalRate=%f,LinkCapacity=%f  ", *_AS,*_B);
		}
	}
	else
	{
		*hasBAndAS=false;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSendAck(CCTimeType curTime, uint32_t numBytes)
{
	(void) numBytes;
	(void) curTime;

	// This is not accounted for on the remote system, and thus causes bandwidth to be underutilized
	//UpdateNextAllowedSend(curTime, numBytes+UDP_HEADER_SIZE);
	
	oldestUnsentAck=0;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSendNACK(CCTimeType curTime, uint32_t numBytes)
{
	(void) numBytes;
	(void) curTime;

	// This is not accounted for on the remote system, and thus causes bandwidth to be underutilized
//	UpdateNextAllowedSend(curTime, numBytes+UDP_HEADER_SIZE);
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateRTT(CCTimeType rtt)
{
	if (RTT==UNSET_TIME_US)
	{
#if CC_TIME_TYPE_BYTES==4
		RakAssert(rtt < 10000);
#else
		RakAssert(rtt < 10000000);
#endif
		RTT=(double) rtt;
		// RTTVar=(double) rtt;
		minRTT=maxRTT=(double)rtt;
	}
	else
	{
		// Ignore massively out of range values, such as from debugging
		if (rtt > RTT * 4 + 100000)
			return;

		RTT = RTT * 0.875 + (double) rtt * 0.125;
		// RTTVar = RTTVar * 0.875 + fabs(RTT - (double) rtt) * 0.125;
		if (rtt < minRTT)
		{
			minRTT=(double)rtt;
		}
		else if (rtt > maxRTT)
		{
			maxRTT=(double)rtt;
		}
		else
		{
			// gradually close min/max while not updated
			double delta = maxRTT-minRTT;
			if (delta>0)
			{
				minRTT+=delta*.05;
				maxRTT-=delta*.05;
			}
		}
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateWindowSizeAndAckOnAckPreSlowStart(double totalUserDataBytesAcked)
{
	// During slow start, max window size is the number of full packets that have been sent out
	// CWND=(double) ((double)totalUserDataBytesSent/(double)MAXIMUM_MTU_INCLUDING_UDP_HEADER);
	CC_DEBUG_PRINTF_3("CWND increasing from %f to %f\n", CWND, (double) ((double)totalUserDataBytesAcked/(double)MAXIMUM_MTU_INCLUDING_UDP_HEADER));
	CWND=(double) ((double)totalUserDataBytesAcked/(double)MAXIMUM_MTU_INCLUDING_UDP_HEADER);
	if (CWND>=CWND_MAX_THRESHOLD)
	{
		CWND=CWND_MAX_THRESHOLD;

		if (AS!=UNDEFINED_TRANSFER_RATE)
			EndSlowStart();
	}
	if (CWND<CWND_MIN_THRESHOLD)
		CWND=CWND_MIN_THRESHOLD;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateWindowSizeAndAckOnAckPerSyn(CCTimeType curTime)
{
	(void) curTime;

	if (isInSlowStart==true)
		return;

	RakAssert(RTT!=UNSET_TIME_US);
	//RakAssert(RTTVar!=UNSET_TIME_US);

	CWND=AS/(BytesPerMicrosecond)MAXIMUM_MTU_INCLUDING_UDP_HEADER * (RTT + SYN) + 16.0;
	if (CWND>CWND_MAX_THRESHOLD)
		CWND=CWND_MAX_THRESHOLD;

	// C is the current sending rate, which can be computed by SND (C = 1/SND).
	BytesPerMicrosecond C = 1.0 / SND;
	// inc is the amount to increase by
	MicrosecondsPerByte inc;
	// Fixed packet size
	// B = estimated link capacity
//	const double PS=MAXIMUM_MTU_INCLUDING_UDP_HEADER;
	const double PS_Inverse=1.0/MAXIMUM_MTU_INCLUDING_UDP_HEADER;
	const double Beta = 0.0015;
	//const double Beta = 0.0000015;
	if (B <= C)
	{
		//inc = 1/PS;
		inc = PS_Inverse;
	}
	else
	{
		// Inside log10 should be bits per second
		// 1000000 is to convert bytes per microsecond to bytes per second
#if CC_TIME_TYPE_BYTES==4
		double a =    pow(10.0, ceil(log10( (B-C) * 1000.0 * 8.0))) * Beta * PS_Inverse;
#else
		double a =    pow(10.0, ceil(log10( (B-C) * 1000000.0 * 8.0))) * Beta * PS_Inverse;
#endif
		double b = PS_Inverse;
		if (a>b)
			inc = a;
		else
			inc = b;
	}
//	printf("SND=%f to ", 1.0/SND);
	SND = (SND * SYN) / (SND * inc + SYN);
//	printf("%f\n", 1.0/SND);
	CapMinSnd(__FILE__,__LINE__);

	if (isInSlowStart==false && SND > 1.0)
		PrintLowBandwidthWarning();
}
// ----------------------------------------------------------------------------------------------------------------------------
double CCRakNetUDT::BytesPerMicrosecondToPacketsPerMillisecond(BytesPerMicrosecond in)
{
#if CC_TIME_TYPE_BYTES==4
	const BytesPerMicrosecond factor = 1.0 / (BytesPerMicrosecond) MAXIMUM_MTU_INCLUDING_UDP_HEADER;
#else
	const BytesPerMicrosecond factor = 1000.0 / (BytesPerMicrosecond) MAXIMUM_MTU_INCLUDING_UDP_HEADER;
#endif
	return in * factor;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::HasHalveSNDOnNoDataTimeElapsed(CCTimeType curTime)
{
	// halveSNDOnNoDataTime remains 0 until we know the RTO
	return halveSNDOnNoDataTime!=0 &&
		curTime>halveSNDOnNoDataTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateHalveSNDOnNoDataTime(CCTimeType curTime)
{
	if (RTT==UNSET_TIME_US)
		return;
	halveSNDOnNoDataTime=GetRTOForRetransmission()+curTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::ResetOnDataArrivalHalveSNDOnNoDataTime(CCTimeType curTime)
{
	if (halveSNDOnNoDataTime!=0)
		UpdateHalveSNDOnNoDataTime(curTime);
	ExpCount=1.0;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::InitPacketPairRecieptHistory(void)
{
	// Initializing with low-end default values works very poorly on bad networks
	// Early NAKs cause it to use the default values, and it takes a very long time to recover
	unsigned int i;
	mostRecentPacketPairValue=UNDEFINED_TRANSFER_RATE;
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		packetPairRecieptHistory[i]=UNDEFINED_TRANSFER_RATE;
		packetPairRecieptHistoryGaps[i]=0;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::InitPacketArrivalHistory(void)
{
	unsigned int i;
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		packetArrivalHistory[i]=UNDEFINED_TRANSFER_RATE;
		packetArrivalHistoryContinuousGaps[i]=0;
	}

	continuousBytesReceived=0;
	continuousBytesReceivedStartTime=0;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::PrintLowBandwidthWarning(void)
{
	
	/*
		printf("\n-------LOW BANDWIDTH -----\n");
		if (isInSlowStart==false)
			printf("SND=%f Megabytes per second\n", 1.0/SND);
		printf("Window size=%f\n", CWND);
		printf("Pipe from packet pair = %f megabytes per second\n", B);
		printf("RTT=%f milliseconds\n", RTT/1000.0);
		printf("RTT Variance=%f milliseconds\n", RTTVar/1000.0);
		printf("Retransmission=%i milliseconds\n", GetRTOForRetransmission()/1000);
		printf("Packet arrival rate on the remote system=%f megabytes per second\n", AS);
		printf("Packet arrival rate on our system=%f megabytes per second\n", ReceiverCalculateDataArrivalRate());
		printf("isInSlowStart=%i\n", isInSlowStart);
		printf("---------------\n");
	*/
}
BytesPerMicrosecond CCRakNetUDT::GetLocalReceiveRate(CCTimeType currentTime) const
{
	return ReceiverCalculateDataArrivalRate(currentTime);
}
double CCRakNetUDT::GetRTT(void) const
{
	if (RTT==UNSET_TIME_US)
		return 0.0;
	return RTT;
}
void CCRakNetUDT::CapMinSnd(const char *file, int line)
{
	(void) file;
	(void) line;

	if (SND > 500)
	{
		SND=500;
		CC_DEBUG_PRINTF_3("%s:%i LIKELY BUG: SND has gotten below 500 microseconds between messages (28.8 modem)\nReport to rakkar@jenkinssoftware.com with file and line number\n", file, line);
	}
}
