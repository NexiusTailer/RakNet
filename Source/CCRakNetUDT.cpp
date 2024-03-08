#include "CCRakNetUDT.h"
#include "Rand.h"
#include "MTUSize.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "RakAssert.h"

using namespace RakNet;

static const BytesPerMicrosecond DEFAULT_TRANSFER_RATE=(BytesPerMicrosecond)MAXIMUM_MTU_SIZE/1000000.0;
static const MicrosecondsPerByte DEFAULT_BYTE_INTERVAL=(MicrosecondsPerByte) (1.0/DEFAULT_TRANSFER_RATE);
static const double UNSET_TIME_US=-1.0;
static const double CWND_MIN_THRESHOLD=2.0;
static const double CWND_MAX_THRESHOLD=(1024*256)/MAXIMUM_MTU_SIZE;

// ****************************************************** PUBLIC METHODS ******************************************************

CCRakNetUDT::CCRakNetUDT() : SYN(10000)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

CCRakNetUDT::~CCRakNetUDT()
{
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::Init(RakNetTimeUS curTime)
{
	InitPacketPairRecieptHistory();
	InitPacketArrivalHistory();
	SND=DEFAULT_BYTE_INTERVAL;
	nextSYNUpdate=0;
	packetPairRecieptHistoryWriteIndex=0;
	packetArrivalHistoryWriteIndex=0;
	B=DEFAULT_TRANSFER_RATE;
	RTT=UNSET_TIME_US;
	RTTVar=UNSET_TIME_US;
	AS=DEFAULT_TRANSFER_RATE;
	isInSlowStart=true;
	LastDecSeq=0;
	NAKCount=1;
	AvgNAKNum=1;
	DecInterval=1;
	DecCount=0;
	nextDatagramSequenceNumber=0;
	lastPacketPairPacketArrivalTime=0;
	lastPacketPairSequenceNumber=(DatagramSequenceNumberType)-1;
	lastPacketArrivalTime=curTime;
	CWND=CWND_MIN_THRESHOLD;
	lastUpdateWindowSizeAndAck=0;
	lastTransmitOfBAndAS=0;
	halveSNDOnNoDataTime=0;
	nextAllowedSend=0;
	ExpCount=1.0;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::Update(RakNetTimeUS curTime, bool hasDataToSendOrResend)
{
	// If you send, and get no data at all from that time to RTO, then halve send rate7
	if (hasDataToSendOrResend && HasHalveSNDOnNoDataTimeElapsed(curTime))
	{
		UpdateHalveSNDOnNoDataTime(curTime);
		SND*=2.0;
		ExpCount+=1.0;
	}

	// Send waiting acks, if the oldest ack has exceeded the threshhold (which may be 0)
}
// ----------------------------------------------------------------------------------------------------------------------------
uint32_t CCRakNetUDT::GetNumberOfBytesToSend(RakNetTimeUS curTime, RakNetTimeUS estimatedTimeToNextTick, uint32_t unacknowledgedBytes)
{
	uint32_t CWNDLimit = (uint32_t) (CWND*MAXIMUM_MTU_SIZE-unacknowledgedBytes);
	if (isInSlowStart)
	{
		return CWNDLimit;
	}
	else
	{
		if (curTime+estimatedTimeToNextTick<nextAllowedSend)
			return 0;
		RakAssert(SYN!=0);
		RakNetTimeUS availableTime = curTime+estimatedTimeToNextTick-nextAllowedSend;
		uint32_t SYNLimit = (uint32_t) (availableTime/SYN);
		return CWNDLimit < SYNLimit ? CWNDLimit : SYNLimit;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::ShouldSendACKs(RakNetTimeUS curTime, RakNetTimeUS estimatedTimeToNextTick, RakNetTimeUS timeOldestACKGenerated)
{
	RakNetTimeUS rto = GetSenderRTOForACK();

	if (rto==UNSET_TIME_US)
	{
		// Unknown how long until the remote system will retransmit, so better send right away
		return true;
	}

//	RakNetTimeUS remoteRetransmitTime=timeOldestACKGenerated+rto-RTT*.5;
//	RakNetTimeUS ackArrivalTimeIfWeDelay=RTT*.5+estimatedTimeToNextTick+curTime;
//	return ackArrivalTimeIfWeDelay<remoteRetransmitTime;

	// Simplified equation
	// GU: At least one ACK should be sent per SYN, otherwise your protocol will increase slower.
	return curTime >= timeOldestACKGenerated + SYN ||
		estimatedTimeToNextTick+curTime < timeOldestACKGenerated+rto-RTT;
}
// ----------------------------------------------------------------------------------------------------------------------------
DatagramSequenceNumberType CCRakNetUDT::GetNextDatagramSequenceNumber(void)
{
	DatagramSequenceNumberType dsnt=nextDatagramSequenceNumber;
	nextDatagramSequenceNumber++;
	return dsnt;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSend(RakNetTimeUS curTime, uint32_t numBytes)
{
	totalUserDataBytesSent+=numBytes;
	if (isInSlowStart)
	{
	}
	else
	{
		// Update the next time we can send again
		nextAllowedSend=(RakNetTimeUS)(curTime+(double)numBytes*SND);
	}

	if (HasHalveSNDOnNoDataTimeElapsed(curTime))
	{
		// Set a timer for which if no acks or naks arrive, we halve the send rate
		UpdateHalveSNDOnNoDataTime(curTime);
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
	a=(DatagramSequenceNumberType)-1;
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
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
		ccRakNetUDT.packetPairRecieptHistory[i]=frandomMT();
	BytesPerMicrosecond calculatedMedian=ccRakNetUDT.ReceiverCalculateLinkCapacityMedian();
	int lt=0,gt=0;
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
	{
		if (ccRakNetUDT.packetPairRecieptHistory[i]<calculatedMedian)
			lt++;
		else if (ccRakNetUDT.packetPairRecieptHistory[i]>calculatedMedian)
			gt++;
	}
	RakAssert(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH%2==0);

//	BytesPerMicrosecond averageOfMedianFilteredValues=ccRakNetUDT.ReceiverCalculateLinkCapacity();

	return lt==gt || lt==gt+1 || lt==gt-1;
}	






// ****************************************************** PROTECTED METHODS ******************************************************

void CCRakNetUDT::SetNextSYNUpdate(RakNetTimeUS currentTime)
{
	nextSYNUpdate+=SYN;
	if (nextSYNUpdate < currentTime)
		nextSYNUpdate=currentTime+SYN;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::InitPacketPairRecieptHistory(void)
{
	unsigned int i;
	// One second between MTU sized packets
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
		packetPairRecieptHistory[i]=DEFAULT_TRANSFER_RATE;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::InitPacketArrivalHistory(void)
{
	unsigned int i;
	// One second between MTU sized packets
	for (i=0; i < CC_RAKNET_UDT_PACKET_HISTORY_LENGTH; i++)
		packetArrivalHistory[i]=DEFAULT_TRANSFER_RATE;

}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateLinkCapacity(void)
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
	return sum/medianListLength;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateLinkCapacityMedian(void)
{
	return CalculateListMedianRecursive(packetPairRecieptHistory, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateDataArrivalRate(void)
{
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
	return sum/medianListLength;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::ReceiverCalculateDataArrivalRateMedian(void)
{
	return CalculateListMedianRecursive(packetArrivalHistory, CC_RAKNET_UDT_PACKET_HISTORY_LENGTH, 0, 0);
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetUDT::CalculateListMedianRecursive(BytesPerMicrosecond inputList[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH], int inputListLength, int lessThanSum, int greaterThanSum)
{
	BytesPerMicrosecond lessThanMedian[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH], greaterThanMedian[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH];
	int lessThanMedianListLength=0, greaterThanMedianListLength=0;
	BytesPerMicrosecond median=inputList[0];
	int i;
	for (i=1; i < inputListLength; i++)
	{
		if (inputList[i]<median)
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
	const DatagramSequenceNumberType halfSpan = ((DatagramSequenceNumberType)-1)/2;
	return b-a>halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::LessThan(DatagramSequenceNumberType a, DatagramSequenceNumberType b)
{
	// a < b?
	const DatagramSequenceNumberType halfSpan = ((DatagramSequenceNumberType)-1)/2;
	return b-a<halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
RakNetTimeUS CCRakNetUDT::GetSenderRTOForACK(void) const
{
	if (RTT==UNSET_TIME_US)
		return (RakNetTimeUS) UNSET_TIME_US;
	return (RakNetTimeUS)(RTT + 4.0 * RTTVar + SYN);
}
// ----------------------------------------------------------------------------------------------------------------------------
RakNetTimeUS CCRakNetUDT::GetRTOForRetransmission(void) const
{
	if (RTT==UNSET_TIME_US)
		return (RakNetTimeUS) UNSET_TIME_US;
	return (RakNetTimeUS)(ExpCount * (RTT + 4.0 * RTTVar) + SYN);
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnNAK(RakNetTimeUS curTime, DatagramSequenceNumberType nakSequenceNumber)
{
	ResetOnDataArrivalHalveSNDOnNoDataTime(curTime);

	if (isInSlowStart)
	{
		EndSlowStart();
		return;
	}

	if (nakSequenceNumber>=LastDecSeq)
	{
		// Slow down sends
		SND*=1.125;
		// Update average number of NAKs per congestion period
		AvgNAKNum=NAKCount;
		// Restart number of NAKs this congestion period
		NAKCount=1;
		// Every interval number of NAKs, we slow the send rate (in addition to the first)
		DecInterval=1+(randomMT()%AvgNAKNum);
		// Decremented send rate 1 time this congestion period
		DecCount=1;
		// Sequence number that was most recently sent this congestion period
		LastDecSeq=nextDatagramSequenceNumber-1;

		return;
	}

	++NAKCount;

	if (DecCount<=5 && (NAKCount%DecInterval)==0)
	{
		// Slow down sends more
		SND*=1.125;
		// Decremented again
		DecCount++;
	}

	// Record continuing congestion period
	LastDecSeq=nextDatagramSequenceNumber-1;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::EndSlowStart(void)
{
	RakAssert(isInSlowStart==true);

	isInSlowStart=false;
	SND=1.0/AS;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnGotPacketPair(DatagramSequenceNumberType datagramSequenceNumber, uint32_t sizeInBytes, RakNetTimeUS curTime)
{
	if (datagramSequenceNumber-1==lastPacketPairSequenceNumber)
	{
		// Is a pair
		if (curTime>lastPacketPairPacketArrivalTime)
		{
			// Has a meaningful interval
			packetPairRecieptHistory[packetPairRecieptHistoryWriteIndex++]=(BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)(curTime-lastPacketPairPacketArrivalTime);
			// Wrap to 0 at the end of the range
			// Assumes power of 2 for CC_RAKNET_UDT_PACKET_HISTORY_LENGTH
			packetPairRecieptHistoryWriteIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);
		}
	}
	else
	{
		lastPacketPairSequenceNumber=datagramSequenceNumber;
		lastPacketPairPacketArrivalTime=curTime;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnGotPacket(RakNetTimeUS curTime, uint32_t sizeInBytes)
{
	if (curTime>lastPacketArrivalTime)
	{
		RakNetTimeUS interval = curTime-lastPacketArrivalTime;
		lastPacketArrivalTime=curTime;
		packetArrivalHistory[packetArrivalHistoryWriteIndex++]=(BytesPerMicrosecond)sizeInBytes/(BytesPerMicrosecond)interval;
		// Wrap to 0 at the end of the range
		// Assumes power of 2 for CC_RAKNET_UDT_PACKET_HISTORY_LENGTH
		packetArrivalHistoryWriteIndex&=(CC_RAKNET_UDT_PACKET_HISTORY_LENGTH-1);
	}		
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnAck(RakNetTimeUS curTime, RakNetTimeUS rtt, bool hasBAndAS, BytesPerMicrosecond _B, BytesPerMicrosecond _AS )
{
	if (hasBAndAS)
	{
		B=_B;
		AS=_AS;
	}

	UpdateRTT(rtt);

	ResetOnDataArrivalHalveSNDOnNoDataTime(curTime);

	if (isInSlowStart==false)
		UpdateWindowSizeAndAckOnAckPostSlowStart(curTime);
	else
		UpdateWindowSizeAndAckOnAckPreSlowStart();
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::OnSendAck(RakNetTimeUS curTime, bool *hasBAndAS, BytesPerMicrosecond *_B, BytesPerMicrosecond *_AS)
{
	if (curTime>lastTransmitOfBAndAS+SYN)
	{
		*hasBAndAS=true;
		*_B=ReceiverCalculateLinkCapacity();
		*_AS=ReceiverCalculateDataArrivalRate();
		lastTransmitOfBAndAS=curTime;
	}
	else
	{
		*hasBAndAS=false;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateRTT(RakNetTimeUS rtt)
{
	if (RTT==UNSET_TIME_US)
	{
		RTT=(double) rtt;
		RTTVar=(double) rtt;
	}
	else
	{
		RTT = RTT * 0.875 + (double) rtt * 0.125;
		RTTVar = RTTVar * 0.875 + abs(RTT - (double) rtt) * 0.125;
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateWindowSizeAndAckOnAckPreSlowStart(void)
{
	// During slow start, max window size is the number of full packets that have been sent out
	CWND=(double) (totalUserDataBytesSent/MAXIMUM_MTU_SIZE);
	if (CWND>=CWND_MAX_THRESHOLD)
	{
		CWND=CWND_MAX_THRESHOLD;
		EndSlowStart();
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateWindowSizeAndAckOnAckPostSlowStart(RakNetTimeUS curTime)
{
	if (lastUpdateWindowSizeAndAck+SYN < curTime)
		return;

	RakAssert(isInSlowStart==false);
	RakAssert(RTT!=UNSET_TIME_US);
	RakAssert(RTTVar!=UNSET_TIME_US);

	CWND=BytesPerMicrosecondToPacketsPerMillisecond(AS) * (RTT + SYN) + 16.0;
	if (CWND>CWND_MAX_THRESHOLD)
		CWND=CWND_MAX_THRESHOLD;

	// C is the current sending rate, which can be computed by SND (C = 1/SND).
	BytesPerMicrosecond C = 1.0 / SND;
	// inc is the amount to increase by
	MicrosecondsPerByte inc;
	// Fixed packet size
	const double PS=MAXIMUM_MTU_SIZE;
	const double PS_Inverse=1.0/MAXIMUM_MTU_SIZE;
	const double Beta = 0.0000015;
	if (B <= C)
	{
		//inc = 1/PS;
		inc = PS_Inverse;
	}
	else
	{
		//inc = max(10^(ceil(log10((B-C)*PS*8))) * Beta/PS, 1/PS);
		double a = pow(10,(ceil(log10((B-C)*PS*8)))) * Beta*PS_Inverse;
		double b = PS_Inverse;
		if (a>b)
			inc = a;
		else
			inc = b;
	}
	SND = (SND * SYN) / (SND * inc + SYN);

	lastUpdateWindowSizeAndAck=curTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
double CCRakNetUDT::BytesPerMicrosecondToPacketsPerMillisecond(BytesPerMicrosecond in)
{
	const BytesPerMicrosecond factor = 1000.0 / (BytesPerMicrosecond) MAXIMUM_MTU_SIZE;
	return in * factor;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetUDT::HasHalveSNDOnNoDataTimeElapsed(RakNetTimeUS curTime)
{
	// halveSNDOnNoDataTime remains 0 until we know the RTO
	return halveSNDOnNoDataTime!=0 &&
		curTime>halveSNDOnNoDataTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::UpdateHalveSNDOnNoDataTime(RakNetTimeUS curTime)
{
	RakNetTimeUS rto = GetRTOForRetransmission();
	if (rto==UNSET_TIME_US)
		return;
	halveSNDOnNoDataTime=rto+curTime;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetUDT::ResetOnDataArrivalHalveSNDOnNoDataTime(RakNetTimeUS curTime)
{
	UpdateHalveSNDOnNoDataTime(curTime);
	ExpCount=1.0;
}
