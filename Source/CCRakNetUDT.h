#ifndef __CONGESTION_CONTROL_UDT_H
#define __CONGESTION_CONTROL_UDT_H

#include "NativeTypes.h"
#include "RakNetTime.h"

typedef uint32_t DatagramSequenceNumberType;
typedef double BytesPerMicrosecond;
typedef double MicrosecondsPerByte;

/// CC_RAKNET_UDT_PACKET_HISTORY_LENGTH should be a power of 2 for the writeIndex variables to wrap properly
#define CC_RAKNET_UDT_PACKET_HISTORY_LENGTH 64

namespace RakNet
{

/// \brief Encapsulates UDT congestion control, as used by RakNet
/// Requirements:
/// <OL>
/// <LI>Each datagram is no more than MAXIMUM_MTU_SIZE, after accounting for the UDP header
/// <LI>Each datagram containing a user message has a sequence number which is set after calling OnSend(). Set it by calling GetNextDatagramSequenceNumber()
/// <LI>System is designed to be used from a single thread.
/// <LI>Each packet should have a timeout time based on GetSenderRTOForACK(). If this time elapses, add the packet to the head of the send list for retransmission.
/// </OL>
///
/// Recommended:
/// <OL>
/// <LI>Call sendto in its own thread. This takes a significant amount of time in high speed networks.
/// </OL>
///
/// Algorithm:
/// <OL>
/// <LI>On a new connection, call Init()
/// <LI>On a periodic interval (SYN time is the best) call Update(). Also call ShouldSendACKs(), and send buffered ACKS if it returns true.
/// <LI>Call OnSendAck() when sending acks.
/// <LI>When you want to send or resend data, call GetNumberOfBytesToSend(). It will return you enough bytes to keep you busy for \a estimatedTimeToNextTick. You can send more than this to fill out a datagram, or to send packet pairs
/// <LI>Call OnSend() when sending datagrams.
/// <LI>When data arrives, record the sequence number and buffer an ACK for it, to be sent from Update() if ShouldSendACKs() returns true
/// <LI>Every 16 packets that you send, send two of them back to back (a packet pair) as long as both packets are the same size. If you don't have two packets the same size, it is fine to defer this until you do.
/// <LI>When you get a packet, call OnGotPacket(). If the packet is also either of a packet pair, call OnGotPacketPair()
/// <LI>If you get a packet, and the sequence number is not 1 + the last sequence number, send a NAK. On the remote system, call OnNAK() and resend that message.
/// <LI>If you get an ACK, remove that message from retransmission. Call OnAck().
/// </OL>
class CCRakNetUDT
{
	public:
	
	CCRakNetUDT();
	~CCRakNetUDT();

	/// Reset all variables to their initial states, for a new connection
	void Init(RakNetTimeUS curTime);

	/// Update over time
	void Update(RakNetTimeUS curTime, bool hasDataToSendOrResend);

	/// How many bytes can we send, pursuant to congestion control, at any given time?
	/// You can send more than this amount to fill out a datagram, or to send two messages as a packet pair
	/// Just keep it under control so as to not overflow the network or significantly affect congestion control
	uint32_t GetNumberOfBytesToSend(RakNetTimeUS curTime, RakNetTimeUS estimatedTimeToNextTick, uint32_t unacknowledgedBytes);

	/// Acks do not have to be sent immediately. Instead, they can be buffered up such that groups of acks are sent at a time
	/// This reduces overall bandwidth usage
	/// How long they can be buffered depends on the retransmit time of the sender
	/// Should call once per update tick, and send if needed
	bool ShouldSendACKs(RakNetTimeUS curTime, RakNetTimeUS estimatedTimeToNextTick, RakNetTimeUS timeOldestACKGenerated);

	/// Every data packet sent must contain a sequence number
	/// Call this function to get it. The sequence number is passed into OnGotPacketPair()
	DatagramSequenceNumberType GetNextDatagramSequenceNumber(void);

	/// Call this when you send packets
	/// Every 15th and 16th packets should be sent as a packet pair if possible
	/// When packets marked as a packet pair arrive, pass to OnGotPacketPair()
	/// When any packets arrive, (additionally) pass to OnGotPacket
	/// Packets should contain our system time, so we can pass rtt to OnAck()
	void OnSend(RakNetTimeUS curTime, uint32_t numBytes);

	/// Call this when you get a packet pair
	void OnGotPacketPair(DatagramSequenceNumberType datagramSequenceNumber, uint32_t sizeInBytes, RakNetTimeUS curTime);

	/// Call this when you get a packet (including packet pairs)
	/// If the DatagramSequenceNumberType is out of order, and the message is reliable, you should send a NAK
	void OnGotPacket(RakNetTimeUS curTime, uint32_t sizeInBytes);

	/// Call when you get a NAK, with the sequence number of the lost message
	/// Affects the congestion control
	void OnNAK(RakNetTimeUS curTime, DatagramSequenceNumberType nakSequenceNumber);

	/// Call this when an ACK arrives.
	/// hasBAndAS are possibly written with the ack, see OnSendAck()
	/// B and AS are used in the calculations in UpdateWindowSizeAndAckOnAckPostSlowStart
	/// B and AS are updated at most once per SYN 
	void OnAck(RakNetTimeUS curTime, RakNetTimeUS rtt, bool hasBAndAS, BytesPerMicrosecond _B, BytesPerMicrosecond _AS );

	/// Call when we send an ack, to write B and AS if needed
	/// B and AS are only written once per SYN, to prevent slow calculations
	void OnSendAck(RakNetTimeUS curTime, bool *hasBAndAS, BytesPerMicrosecond *_B, BytesPerMicrosecond *_AS);


	/// Same as GetRTOForRetransmission, but does not factor in ExpCount
	/// This is because the receiver does not know ExpCount for the sender, and even if it did, acks shouldn't be delayed for this reason
	RakNetTimeUS GetSenderRTOForACK(void) const;


	// --------------------------- UNIT TESTING ---------------------------
	static bool UnitTest(void);
	static bool TestReceiverCalculateLinkCapacityMedian(void);

	protected:
	// --------------------------- PROTECTED VARIABLES ---------------------------
	/// time interval between outgoing packets, in milliseconds
	/// Only used when slowStart==false
	/// Increased over time as we continually get messages
	/// Decreased on NAK and timeout
	/// Starts at 0 (invalid)
	MicrosecondsPerByte SND;
	
	/// Supportive window mechanism, controlling the maximum number of in-flight packets
	/// Used both during and after slow-start, but primarily during slow-start
	/// Starts at 2, which is also the low threshhold
	/// Max is the socket receive buffer / MTU
	/// CWND = AS * (RTT + SYN) + 16
	double CWND;
	
	/// Interval at which to update aspects of the system
	/// 1. send acks
	/// 2. update time interval between outgoing packets
	/// 3, Yodate retransmit timeout
	const RakNetTimeUS SYN;
	
	/// When we do an update process on the SYN interval, nextSYNUpdate is set to the next time we should update
	/// Normally this is nextSYNUpdate+=SYN, in order to update on a consistent schedule
	/// However, if this would result in an immediate update yet again, it is set to SYN microseconds past the current time (in case the thread did not update for a long time)
	RakNetTimeUS nextSYNUpdate;
		
	/// Tracks PacketPairHistoryNode
	/// History is initialized with 1 second intervals for all packets (slow start)
	/// All values greater than 8x of the median or less than 1/8 of the median should be ignored when calculating the average
	/// Used to calculate link capacity (via ReceiverCalculateLinkCapacity())
	/// B = averageAmongElements(numBytes/intervalBetweenPair)
	/// B is calculated on the receiver, and send to the sender back in acks
	/// B is only sent to the sender every 10 acks, to avoid unnecessary calculation
	/// Each node represents numBytes in one of the packets in the pair / intervalBetweenPair
	/// Value is in bytes per microsecond
	BytesPerMicrosecond packetPairRecieptHistory[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH];
	
	/// Index into packetPairRecieptHistory where we will next write
	/// The history is always full (starting with default values) so no read index is needed
	int packetPairRecieptHistoryWriteIndex;

	/// Sent to the sender by the receiver from packetPairRecieptHistory whenever a back to back packet arrives on the receiver
	/// Updated by B = B * .875 + incomingB * .125
	BytesPerMicrosecond B;
	
	/// Running round trip time (ping*2)
	/// Only sender needs to know this
	/// Initialized to UNSET
	/// Set to rtt on first calculation
	/// Updated gradually by RTT = RTT * 0.875 + rtt * 0.125
	double RTT;
	
	/// Round trip time variance
	/// Only sender needs to know this
	/// Initialized to UNSET
	/// Set to rtt on first calculation
	/// Updated gradually by RTTVar = RTTVar * 0.875 + abs(RTT - rtt)) * 0.125
	double RTTVar;	
		
	/// Used to calculate packet arrival rate (in UDT) but data arrival rate (in RakNet, where not all datagrams are the same size)
	/// Filter is used to cull lowest half of values for bytesPerMicrosecond, to discount spikes and inactivity
	/// Referred to in the documentation as AS, data arrival rate
	/// AS is sent to the sender and calculated every 10th ack
	/// Each node represents (curTime-lastPacketArrivalTime)/bytes
	/// Used with ReceiverCalculateDataArrivalRate();
	BytesPerMicrosecond packetArrivalHistory[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH];

	/// Index into packetArrivalHistory where we will next write
	/// The history is always full (starting with default values) so no read index is needed
	int packetArrivalHistoryWriteIndex;

	/// Tracks the time the last packet that arrived, so BytesPerMicrosecond can be calculated for packetArrivalHistory when a new packet arrives
	RakNetTimeUS lastPacketArrivalTime;

	/// Data arrival rate from the sender to the receiver, as told to us by the receiver
	/// Used to calculate initial sending rate when slow start stops
	BytesPerMicrosecond AS;

	/// When the receiver last calculated and send B and AS, from packetArrivalHistory and packetPairRecieptHistory
	/// Used to prevent it from being calculated and send too frequently, as they are slow operations
	RakNetTimeUS lastTransmitOfBAndAS;
	
	/// New connections start in slow start
	/// During slow start, SND is not used, only CWND
	/// Slow start ends when we get a NAK, or the maximum size of CWND is reached
	/// SND is initialized to the inverse of the receiver's packet arrival rate when slow start ends
	bool isInSlowStart;
	
	/// Largest sequence number sent so far when a NAK arrives
	/// Initialized to 0
	/// If a NAK arrives with a sequence number larger than LastDecSeq, then this is defined as a new congestion period, in which case we do various things to slow our send rate.
	DatagramSequenceNumberType LastDecSeq;
	
	/// How many NAKs arrived this congestion period
	/// Initialized to 1 when the congestion period starts
	uint32_t NAKCount;
	
	/// How many NAKs do you get on average during a congestion period?
	/// Starts at 1
	/// Used to generate a random number, DecRandom, between 1 and AvgNAKNum
	uint32_t AvgNAKNum;
	
	/// How many times we have decremented SND this congestion period. Used to limit the number of decrements to 5
	uint32_t DecCount;

	/// Every DecInterval NAKs per congestion period, we decrease the send rate 
	uint32_t DecInterval;

	/// Every outgoing datagram is assigned a sequence number, which increments by 1 every assignment
	DatagramSequenceNumberType nextDatagramSequenceNumber;

	/// If a packet is marked as a packet pair, lastPacketPairPacketArrivalTime is set to the time it arrives
	/// This is used so when the 2nd packet of the pair arrives, we can calculate the time interval between the two
	RakNetTimeUS lastPacketPairPacketArrivalTime;

	/// If a packet is marked as a packet pair, lastPacketPairSequenceNumber is checked to see if the last packet we got
	/// was the packet immediately before the one that arrived
	/// If so, we can use lastPacketPairPacketArrivalTime to get the time between the two packets, and thus estimate the link capacity
	/// Initialized to -1, so the first packet of a packet pair won't be treated as the second
	DatagramSequenceNumberType lastPacketPairSequenceNumber;

	/// Used to cap UpdateWindowSizeAndAckOnAckPostSlowStart() to once speed increase per SYN
	/// This is to prevent speeding up faster than congestion control can compensate for
	RakNetTimeUS lastUpdateWindowSizeAndAck;

	/// If you send, and get no data at all from that time to RTO, then halve send rate
	/// Used to backoff for low-bandwidth networks (as in UDT 4.5)
	/// Set to:
	/// 0 initially
	/// curTime+RTO when elapsed during per-update tick, and data is waiting to be sent, or resent. (And halves send rate at that point)
	/// curTime+RTO when data is sent if already elapsed
	RakNetTimeUS halveSNDOnNoDataTime;

	/// Every time SND is halved due to timeout, the RTO is increased
	/// This is to prevent massive retransmissions to an unresponsive system
	/// Reset on any data arriving
	double ExpCount;

	/// Every time we send, nextAllowedSend is set to curTime + the number of bytes sent * SND
	/// We can then not send again until that time elapses
	/// The best amount of time to wait before sending again is exactly up until that time
	RakNetTimeUS nextAllowedSend;

	/// Total number of user data bytes sent
	/// Used to adjust the window size, on ACK, during slow start
	uint64_t totalUserDataBytesSent;

	// --------------------------- PROTECTED METHODS ---------------------------
	/// Update nextSYNUpdate by SYN, or the same amount past the current time if no updates have occurred for a long time
	void SetNextSYNUpdate(RakNetTimeUS currentTime);

	/// Init corresponding list to use 1 MTU sized packet per second default
	void InitPacketPairRecieptHistory(void);

	/// Init corresponding list to use 1 MTU sized packet per second default
	void InitPacketArrivalHistory(void);
	
	/// Returns the average link capacity as based on the packet pair technique, after doing median filtering
	BytesPerMicrosecond ReceiverCalculateLinkCapacity(void);
	/// Returns the median of the link capacity
	BytesPerMicrosecond ReceiverCalculateLinkCapacityMedian(void);

	/// Returns the rate of data arrival, based on packets arriving on the sender.
	BytesPerMicrosecond ReceiverCalculateDataArrivalRate(void);
	/// Returns the median of the data arrival rate
	BytesPerMicrosecond ReceiverCalculateDataArrivalRateMedian(void);

	/// Calculates the median an array of BytesPerMicrosecond
	static BytesPerMicrosecond CalculateListMedianRecursive(BytesPerMicrosecond inputList[CC_RAKNET_UDT_PACKET_HISTORY_LENGTH], int inputListLength, int lessThanSum, int greaterThanSum);
	
	/// Is a > b, accounting for variable overflow?
	static bool GreaterThan(DatagramSequenceNumberType a, DatagramSequenceNumberType b);
	/// Is a < b, accounting for variable overflow?
	static bool LessThan(DatagramSequenceNumberType a, DatagramSequenceNumberType b);

	/// Retransmission time out for the sender
	/// If the time difference between when a message was last transmitted, and the current time is greater than RTO then packet is eligible for retransmission, pending congestion control
	/// RTO = (RTT + 4 * RTTVar) + SYN
	/// If we have been continuously sending for the last RTO, and no ACK or NAK at all, SND*=2;
	/// This is per message, which is different from UDT, but RakNet supports packetloss with continuing data where UDT is only RELIABLE_ORDERED
	/// Minimum value is 100 milliseconds
	RakNetTimeUS GetRTOForRetransmission(void) const;

	/// Stop slow start, and enter normal transfer rate
	void EndSlowStart(void);

	/// Does the named conversion
	inline double BytesPerMicrosecondToPacketsPerMillisecond(BytesPerMicrosecond in);

	/// Update the round trip time, from ACK or ACK2
	void UpdateRTT(RakNetTimeUS rtt);

	/// Update the corresponding variables pre-slow start
	void UpdateWindowSizeAndAckOnAckPreSlowStart(void);

	/// Update the corresponding variables post-slow start
	void UpdateWindowSizeAndAckOnAckPostSlowStart(RakNetTimeUS curTime);

	/// Returns true on elapsed, false otherwise
	bool HasHalveSNDOnNoDataTimeElapsed(RakNetTimeUS curTime);

	/// Sets halveSNDOnNoDataTime to the future, unless we don't know the RTO (ping*2) yet
	void UpdateHalveSNDOnNoDataTime(RakNetTimeUS curTime);

	/// Sets halveSNDOnNoDataTime to the future, and also resets ExpCount, which is used to multiple the RTO on no data arriving at all
	void ResetOnDataArrivalHalveSNDOnNoDataTime(RakNetTimeUS curTime);


};

}

#endif
