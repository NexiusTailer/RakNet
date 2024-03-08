#include "RakNetDefines.h"
#ifndef _USE_RAKNET_FLOW_CONTROL

#ifdef _MSC_VER
#pragma warning( push )
#endif

#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4702)   // unreachable code

/*****************************************************************************
Copyright (c) 2001 - 2009, The Board of Trustees of the University of Illinois.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions
  and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the University of Illinois
  nor the names of its contributors may be used to
  endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

/*****************************************************************************
written by
   Yunhong Gu, last updated 01/15/2009
*****************************************************************************/

#ifndef WIN32
   #include <unistd.h>
   #include <netdb.h>
   #include <arpa/inet.h>
   #include <cerrno>
   #include <cstring>
   #include <cstdlib>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <cmath>
#include "queue.h"
#include "core.h"

using namespace std;


CUDTUnited CUDT::s_UDTUnited;

const UDTSOCKET CUDT::INVALID_SOCK = -1;
const int CUDT::ERROR = -1;

const UDTSOCKET UDT::INVALID_SOCK = CUDT::INVALID_SOCK;
const int UDT::ERROR = CUDT::ERROR;

const int32_t CSeqNo::m_iSeqNoTH = 0x3FFFFFFF;
const int32_t CSeqNo::m_iMaxSeqNo = 0x7FFFFFFF;
const int32_t CAckNo::m_iMaxAckSeqNo = 0x7FFFFFFF;
const int32_t CMsgNo::m_iMsgNoTH = 0xFFFFFFF;
const int32_t CMsgNo::m_iMaxMsgNo = 0x1FFFFFFF;

const int CUDT::m_iVersion = 4;
const int UDTCCWrapper::m_iSYNInterval = 10000;
const int UDTCCWrapper::m_iSelfClockInterval = 64;


CUDT::CUDT()
{
   udtCCWrapper.m_pSndBuffer = NULL;
   udtCCWrapper.m_pRcvBuffer = NULL;
   udtCCWrapper.m_pSndLossList = NULL;
   udtCCWrapper.m_pRcvLossList = NULL;
   udtCCWrapper.m_pACKWindow = NULL;
   udtCCWrapper.m_pSndTimeWindow = NULL;
   udtCCWrapper.m_pRcvTimeWindow = NULL;

   m_pSndQueue = NULL;
   m_pRcvQueue = NULL;
   m_pPeerAddr = NULL;
   m_pSNode = NULL;
   m_pRNode = NULL;

   // Initilize mutex and condition variables
   initSynch();

   // Default UDT configurations
   m_iMSS = 1500;
   m_bSynSending = true;
   m_bSynRecving = true;
   m_iFlightFlagSize = 25600;
   m_iSndBufSize = 8192;
   m_iRcvBufSize = 8192;
   m_Linger.l_onoff = 1;
   m_Linger.l_linger = 180;
   m_iUDPSndBufSize = 65536;
   m_iUDPRcvBufSize = m_iRcvBufSize * m_iMSS;
   m_iIPversion = AF_INET;
   m_bRendezvous = false;
   m_iSndTimeOut = -1;
   m_iRcvTimeOut = -1;
   m_bReuseAddr = true;
   m_llMaxBW = -1;

   m_pCCFactory = RakNet::OP_NEW< CCCFactory<CUDTCC> >(__FILE__,__LINE__);
   udtCCWrapper.m_pCC = NULL;
//   udtCCWrapper.m_pController = NULL;

   // Initial status
   m_bOpened = false;
   m_bListening = false;
   m_bConnected = false;
   m_bClosing = false;
   m_bShutdown = false;
   m_bBroken = false;
}

CUDT::CUDT(const CUDT& ancestor)
{
   udtCCWrapper.m_pSndBuffer = NULL;
   udtCCWrapper.m_pRcvBuffer = NULL;
   udtCCWrapper.m_pSndLossList = NULL;
   udtCCWrapper.m_pRcvLossList = NULL;
   udtCCWrapper.m_pACKWindow = NULL;
   udtCCWrapper.m_pSndTimeWindow = NULL;
   udtCCWrapper.m_pRcvTimeWindow = NULL;

   m_pSndQueue = NULL;
   m_pRcvQueue = NULL;
   m_pPeerAddr = NULL;
   m_pSNode = NULL;
   m_pRNode = NULL;

   // Initilize mutex and condition variables
   initSynch();

   // Default UDT configurations
   m_iMSS = ancestor.m_iMSS;
   m_bSynSending = ancestor.m_bSynSending;
   m_bSynRecving = ancestor.m_bSynRecving;
   m_iFlightFlagSize = ancestor.m_iFlightFlagSize;
   m_iSndBufSize = ancestor.m_iSndBufSize;
   m_iRcvBufSize = ancestor.m_iRcvBufSize;
   m_Linger = ancestor.m_Linger;
   m_iUDPSndBufSize = ancestor.m_iUDPSndBufSize;
   m_iUDPRcvBufSize = ancestor.m_iUDPRcvBufSize;
   m_iSockType = ancestor.m_iSockType;
   m_iIPversion = ancestor.m_iIPversion;
   m_bRendezvous = ancestor.m_bRendezvous;
   m_iSndTimeOut = ancestor.m_iSndTimeOut;
   m_iRcvTimeOut = ancestor.m_iRcvTimeOut;
   m_bReuseAddr = true;

   m_pCCFactory = ancestor.m_pCCFactory->clone();
   udtCCWrapper.m_pCC = NULL;
//   udtCCWrapper.m_pController = ancestor.udtCCWrapper.m_pController;

   // Initial status
   m_bOpened = false;
   m_bListening = false;
   m_bConnected = false;
   m_bClosing = false;
   m_bShutdown = false;
   m_bBroken = false;
}

CUDT::~CUDT()
{
   // release mutex/condtion variables
   destroySynch();

   // destroy the data structures
//   delete udtCCWrapper.m_pSndBuffer;
   //delete udtCCWrapper.m_pRcvBuffer;
   //delete udtCCWrapper.m_pSndLossList;
   //delete udtCCWrapper.m_pRcvLossList;
   //delete m_pACKWindow;
   //delete m_pSndTimeWindow;
   //delete udtCCWrapper.m_pRcvTimeWindow;
   //delete m_pCCFactory;
   //delete udtCCWrapper.m_pCC;
   //delete m_pPeerAddr;
   //delete m_pSNode;
   //delete m_pRNode;

   RakNet::OP_DELETE(udtCCWrapper.m_pSndBuffer,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pRcvBuffer,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pSndLossList,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pRcvLossList,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pACKWindow,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pSndTimeWindow,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pRcvTimeWindow,__FILE__,__LINE__);
   RakNet::OP_DELETE(m_pCCFactory,__FILE__,__LINE__);
   RakNet::OP_DELETE(udtCCWrapper.m_pCC,__FILE__,__LINE__);
   RakNet::OP_DELETE(m_pPeerAddr,__FILE__,__LINE__);
   RakNet::OP_DELETE(m_pSNode,__FILE__,__LINE__);
   RakNet::OP_DELETE(m_pRNode,__FILE__,__LINE__);
}

int CUDT::setOpt(UDTOpt optName, const void* optval, const int&)
{
   CGuard cg(m_ConnectionLock);
   CGuard sendguard(m_SendLock);
   CGuard recvguard(m_RecvLock);

   switch (optName)
   {
   case UDT_MSS:
      if (m_bOpened)
	  {
#if defined(UDT_DISABLE_EXCEPTIONS)
		  return -1;
#else
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);
#endif
	  }

      if (*(int*)optval < int(28 + sizeof(CHandShake)))
         THROW_CUDTEXCEPTION_RET(5, 3, 0, -1);

      m_iMSS = *(int*)optval;

      // Packet size cannot be greater than UDP buffer size
      if (m_iMSS > m_iUDPSndBufSize)
         m_iMSS = m_iUDPSndBufSize;
      if (m_iMSS > m_iUDPRcvBufSize)
         m_iMSS = m_iUDPRcvBufSize;

      break;

   case UDT_SNDSYN:
      m_bSynSending = *(bool *)optval;
      break;

   case UDT_RCVSYN:
      m_bSynRecving = *(bool *)optval;
      break;

   case UDT_CC:
      if (m_bConnected)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);
      if (NULL != m_pCCFactory)
    //     delete m_pCCFactory;
		RakNet::OP_DELETE(m_pCCFactory,__FILE__,__LINE__);
      m_pCCFactory = ((CCCVirtualFactory *)optval)->clone();

      break;

   case UDT_FC:
      if (m_bConnected)
         THROW_CUDTEXCEPTION_RET(5, 2, 0, -1);

      if (*(int*)optval < 1)
         THROW_CUDTEXCEPTION_RET(5, 3, -1, -1);

      m_iFlightFlagSize = *(int*)optval;

      break;

   case UDT_SNDBUF:
      if (m_bOpened)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);

      if (*(int*)optval <= 0)
         THROW_CUDTEXCEPTION_RET(5, 3, 0, -1);

      m_iSndBufSize = *(int*)optval / (m_iMSS - 28);

      break;

   case UDT_RCVBUF:
      if (m_bOpened)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);

      if (*(int*)optval <= 0)
         THROW_CUDTEXCEPTION_RET(5, 3, 0, -1);

      // Mimimum recv buffer size is 32 packets
      if (*(int*)optval > (m_iMSS - 28) * 32)
         m_iRcvBufSize = *(int*)optval / (m_iMSS - 28);
      else
         m_iRcvBufSize = 32;

      break;

   case UDT_LINGER:
      m_Linger = *(linger*)optval;
      break;

   case UDP_SNDBUF:
      if (m_bOpened)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);

      m_iUDPSndBufSize = *(int*)optval;

      if (m_iUDPSndBufSize < m_iMSS)
         m_iUDPSndBufSize = m_iMSS;

      break;

   case UDP_RCVBUF:
      if (m_bOpened)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);

      m_iUDPRcvBufSize = *(int*)optval;

      if (m_iUDPRcvBufSize < m_iMSS)
         m_iUDPRcvBufSize = m_iMSS;

      break;

   case UDT_RENDEZVOUS:
      if (m_bConnected)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);
      m_bRendezvous = *(bool *)optval;
      break;

   case UDT_SNDTIMEO: 
      m_iSndTimeOut = *(int*)optval; 
      break; 
    
   case UDT_RCVTIMEO: 
      m_iRcvTimeOut = *(int*)optval; 
      break; 

   case UDT_REUSEADDR:
      if (m_bOpened)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);
      m_bReuseAddr = *(bool*)optval;
      break;

   case UDT_MAXBW:
      if (m_bConnected)
         THROW_CUDTEXCEPTION_RET(5, 1, 0, -1);
      m_llMaxBW = *(int64_t*)optval;
      break;
    
   default:
      THROW_CUDTEXCEPTION_RET(5, 0, 0, -1);
   }

   return 0;
}

int CUDT::getOpt(UDTOpt optName, void* optval, int& optlen)
{
   CGuard cg(m_ConnectionLock);

   switch (optName)
   {
   case UDT_MSS:
      *(int*)optval = m_iMSS;
      optlen = sizeof(int);
      break;

   case UDT_SNDSYN:
      *(bool*)optval = m_bSynSending;
      optlen = sizeof(bool);
      break;

   case UDT_RCVSYN:
      *(bool*)optval = m_bSynRecving;
      optlen = sizeof(bool);
      break;

   case UDT_CC:
      if (!m_bOpened)
		  THROW_CUDTEXCEPTION_RET(5, 5, 0, -1);
      *(CCC**)optval = udtCCWrapper.m_pCC;
      optlen = sizeof(CCC*);

      break;

   case UDT_FC:
      *(int*)optval = m_iFlightFlagSize;
      optlen = sizeof(int);
      break;

   case UDT_SNDBUF:
	   // BUG: KevinJ
	   // m_iSndBufSize * (m_iMSS - 28) = 99999744
	   // Fails to send, check in CUDT::sendmsg is m_iSndBufSize * m_iPayloadSize = 98437248
      *(int*)optval = m_iSndBufSize * (m_iMSS - 28);
      optlen = sizeof(int);
      break;

   case UDT_RCVBUF:
      *(int*)optval = m_iRcvBufSize * (m_iMSS - 28);
      optlen = sizeof(int);
      break;

   case UDT_LINGER:
      if (optlen < (int)(sizeof(linger)))
         THROW_CUDTEXCEPTION_RET(5, 3, 0, -1);

      *(linger*)optval = m_Linger;
      optlen = sizeof(linger);
      break;

   case UDP_SNDBUF:
      *(int*)optval = m_iUDPSndBufSize;
      optlen = sizeof(int);
      break;

   case UDP_RCVBUF:
      *(int*)optval = m_iUDPRcvBufSize;
      optlen = sizeof(int);
      break;

   case UDT_RENDEZVOUS:
      *(bool *)optval = m_bRendezvous;
      optlen = sizeof(bool);
      break;

   case UDT_SNDTIMEO: 
      *(int*)optval = m_iSndTimeOut; 
      optlen = sizeof(int); 
      break; 
    
   case UDT_RCVTIMEO: 
      *(int*)optval = m_iRcvTimeOut; 
      optlen = sizeof(int); 
      break; 

   case UDT_REUSEADDR:
      *(bool *)optval = m_bReuseAddr;
      optlen = sizeof(bool);
      break;

   case UDT_MAXBW:
      *(int64_t*)optval = m_llMaxBW;
      break;

   default:
	   THROW_CUDTEXCEPTION_RET(5, 0, 0, -1);
   }
   return 0;
}

void CUDT::open()
{
   CGuard cg(m_ConnectionLock);

   // Initial sequence number, loss, acknowledgement, etc.
   m_iPktSize = m_iMSS - 28;
   m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;

   udtCCWrapper.m_iEXPCount = 1;
   udtCCWrapper.m_iBandwidth = 1;
   udtCCWrapper.m_iDeliveryRate = 16;
   udtCCWrapper.m_iAckSeqNo = 0;
   udtCCWrapper.m_ullLastAckTime = 0;

   // trace information
   m_StartTime = CTimer::getTime();
   m_llSentTotal = m_llRecvTotal = m_iSndLossTotal = m_iRcvLossTotal = m_iRetransTotal = m_iSentACKTotal = m_iRecvACKTotal = m_iSentNAKTotal = m_iRecvNAKTotal = 0;
   m_LastSampleTime = CTimer::getTime();
   m_llTraceSent = m_llTraceRecv = m_iTraceSndLoss = m_iTraceRcvLoss = m_iTraceRetrans = m_iSentACK = m_iRecvACK = m_iSentNAK = m_iRecvNAK = 0;

   // structures for queue
   if (NULL == m_pSNode)
   //   m_pSNode = new CSNode;
   m_pSNode = RakNet::OP_NEW<CSNode>(__FILE__, __LINE__);
   m_pSNode->m_pUDT = this;
   m_pSNode->m_llTimeStamp = 1;
   m_pSNode->m_iHeapLoc = -1;

   if (NULL == m_pRNode)
      //m_pRNode = new CRNode;
	  m_pRNode = RakNet::OP_NEW<CRNode>(__FILE__, __LINE__);
   m_pRNode->m_pUDT = this;
   m_pRNode->m_llTimeStamp = 1;
   m_pRNode->m_pPrev = m_pRNode->m_pNext = NULL;
   m_pRNode->m_bOnList = false;

   udtCCWrapper.m_iRTT = 10 * udtCCWrapper.m_iSYNInterval;
   udtCCWrapper.m_iRTTVar = udtCCWrapper.m_iRTT >> 1;
   udtCCWrapper.m_ullCPUFrequency = CTimer::getCPUFrequency();

   // set up the timers
   udtCCWrapper.m_ullSYNInt = udtCCWrapper.m_iSYNInterval * udtCCWrapper.m_ullCPUFrequency;
   
   udtCCWrapper.m_ullACKInt = udtCCWrapper.m_ullSYNInt;
   udtCCWrapper.m_ullNAKInt = (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) * udtCCWrapper.m_ullCPUFrequency;
   udtCCWrapper.m_ullEXPInt = (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) * udtCCWrapper.m_ullCPUFrequency + udtCCWrapper.m_ullSYNInt;

   CTimer::rdtsc(udtCCWrapper.m_ullNextACKTime);
   udtCCWrapper.m_ullNextACKTime += udtCCWrapper.m_ullSYNInt;
   CTimer::rdtsc(udtCCWrapper.m_ullNextNAKTime);
   udtCCWrapper.m_ullNextNAKTime += udtCCWrapper.m_ullNAKInt;
   CTimer::rdtsc(udtCCWrapper.m_ullNextEXPTime);
   udtCCWrapper.m_ullNextEXPTime += udtCCWrapper.m_ullEXPInt;

   udtCCWrapper.m_iPktCount = 0;
   udtCCWrapper.m_iLightACKCount = 1;

   udtCCWrapper.m_ullTargetTime = 0;
   udtCCWrapper.m_ullTimeDiff = 0;

   // Now UDT is opened.
   m_bOpened = true;
}

void CUDT::listen()
{
   CGuard cg(m_ConnectionLock);

   if (!m_bOpened)
      THROW_CUDTEXCEPTION(5, 0, 0);

   if (m_bConnected)
      THROW_CUDTEXCEPTION(5, 2, 0);

   // listen can be called more than once
   if (m_bListening)
      return;

   // if there is already another socket listening on the same port
   if (m_pRcvQueue->setListener(this) < 0)
      THROW_CUDTEXCEPTION(5, 11, 0);

   m_bListening = true;
}

int CUDT::connect(const sockaddr* serv_addr)
{
   CGuard cg(m_ConnectionLock);

   if (!m_bOpened)
	   THROW_CUDTEXCEPTION_RET(5, 0, 0, -1);

   if (m_bListening)
      THROW_CUDTEXCEPTION_RET(5, 2, 0, -1);

   if (m_bConnected)
      THROW_CUDTEXCEPTION_RET(5, 2, 0, UDT_ALREADY_CONNECTED);

   // register this socket in the rendezvous queue
   m_pRcvQueue->m_pRendezvousQueue->insert(m_SocketID, m_iIPversion, serv_addr);

   CPacket request;
   //char* reqdata = new char [m_iPayloadSize];
   char* reqdata = (char*) rakMalloc_Ex(m_iPayloadSize, __FILE__, __LINE__);
   CHandShake* req = (CHandShake *)reqdata;

   CPacket response;
   //char* resdata = new char [m_iPayloadSize];
   char* resdata = (char*) rakMalloc_Ex(m_iPayloadSize, __FILE__, __LINE__);
   CHandShake* res = (CHandShake *)resdata;

   // This is my current configurations.
   req->m_iVersion = m_iVersion;
   req->m_iType = m_iSockType;
   req->m_iMSS = m_iMSS;
   req->m_iFlightFlagSize = (m_iRcvBufSize < m_iFlightFlagSize)? m_iRcvBufSize : m_iFlightFlagSize;
   req->m_iReqType = (!m_bRendezvous) ? 1 : 0;
   req->m_iID = m_SocketID;

   // Random Initial Sequence Number
   uint64_t time64 = CTimer::getTime();
   uint32_t time32 = (uint32_t) (time64 & 0xFFFFFFFF);
   srand(time32);
   udtCCWrapper.m_iISN = req->m_iISN = (int32_t)(double(rand()) * CSeqNo::m_iMaxSeqNo / (RAND_MAX + 1.0));

   udtCCWrapper.m_iLastDecSeq = req->m_iISN - 1;
   udtCCWrapper.m_iSndLastAck = req->m_iISN;
   udtCCWrapper.m_iSndLastDataAck = req->m_iISN;
   udtCCWrapper.m_iSndCurrSeqNo = req->m_iISN - 1;
   udtCCWrapper.m_iSndLastAck2 = req->m_iISN;
   udtCCWrapper.m_ullSndLastAck2Time = CTimer::getTime();

   // Inform the server my configurations.
   request.pack(0, NULL, reqdata, sizeof(CHandShake));
   // ID = 0, connection request
   request.m_iID = 0;

   // Wait for the negotiated configurations from the peer side.
   response.pack(0, NULL, resdata, sizeof(CHandShake));

   uint64_t timeo = 3000000;
   if (m_bRendezvous)
      timeo *= 10;
   uint64_t entertime = CTimer::getTime();
   CUDTException e(0, 0);

   char* tmp = NULL;

   while (!m_bClosing)
   {
      m_pSndQueue->sendto(serv_addr, request);

      response.setLength(m_iPayloadSize);
      if (m_pRcvQueue->recvfrom(m_SocketID, response) > 0)
      {
         if (m_bRendezvous && (0 == response.getFlag()) && (NULL != tmp))
         {
            // a data packet comes, which means the peer side is already connected
            // in this situation, a previously recorded response (tmp) will be used
            memcpy(resdata, tmp, sizeof(CHandShake));
            //delete [] tmp;
			rakFree_Ex(tmp,__FILE__, __LINE__);
            break;
         }

         if ((1 != response.getFlag()) || (0 != response.getType()))
            response.setLength(-1);

         if (m_bRendezvous)
         {
            // regular connect should NOT communicate with rendezvous connect
            // rendezvous connect require 3-way handshake
            if (1 == res->m_iReqType)
               response.setLength(-1);
            else if ((0 == res->m_iReqType) || (0 == req->m_iReqType))
            {
               // tmp = new char [m_iPayloadSize];
			   tmp = (char*) rakMalloc_Ex(m_iPayloadSize, __FILE__, __LINE__);
               memcpy(tmp, resdata, sizeof(CHandShake));

               req->m_iReqType = -1;
               request.m_iID = res->m_iID;
               response.setLength(-1);
            }
         }
         else
         {
            // set cookie
            if (1 == res->m_iReqType)
            {
               req->m_iReqType = -1;
               req->m_iCookie = res->m_iCookie;
               response.setLength(-1);
            }
         }
      }

      if (response.getLength() > 0)
         break;

      if (CTimer::getTime() - entertime > timeo)
      {
         // timeout
         e = CUDTException(1, 1, 0);
         break;
      }
   }

   // delete [] reqdata;
   rakFree_Ex(reqdata,__FILE__, __LINE__);

   if (e.getErrorCode() == 0)
   {
      if (m_bClosing)						// if the socket is closed before connection...
         e = CUDTException(1);
      else if (1002 == res->m_iReqType)				// connection request rejected
         e = CUDTException(1, 2, 0);
      else if ((!m_bRendezvous) && (udtCCWrapper.m_iISN != res->m_iISN))	// secuity check
         e = CUDTException(1, 4, 0);
   }

   if (e.getErrorCode() != 0)
   {
      // connection failure, clean up and throw exception
	   // delete [] resdata;
	   rakFree_Ex(resdata,__FILE__, __LINE__);

      if (m_bRendezvous)
         m_pRcvQueue->m_pRendezvousQueue->remove(m_SocketID);
	THROW_CUDTEXCEPTION_E(-1);
   }

   // Got it. Re-configure according to the negotiated values.
   m_iMSS = res->m_iMSS;
   udtCCWrapper.m_iFlowWindowSize = res->m_iFlightFlagSize;
   m_iPktSize = m_iMSS - 28;
   m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;
   udtCCWrapper.m_iPeerISN = res->m_iISN;
   udtCCWrapper.m_iRcvLastAck = res->m_iISN;
   udtCCWrapper.m_iRcvLastAckAck = res->m_iISN;
   udtCCWrapper.m_iRcvCurrSeqNo = res->m_iISN - 1;
   m_PeerID = res->m_iID;

   // delete [] resdata;
   rakFree_Ex(resdata,__FILE__, __LINE__);

   // Prepare all data structures
   TRY_CUDTEXCEPTION
   {
#if defined(_USE_RAK_MEMORY_OVERRIDE)
	   
	   char *buff;
	   buff = (char *) (GetMalloc_Ex())(sizeof(CSndBuffer), __FILE__, __LINE__);
	   udtCCWrapper.m_pSndBuffer = new (buff) CSndBuffer(32, m_iPayloadSize);
	   buff = (char *) (GetMalloc_Ex())(sizeof(CRcvBuffer), __FILE__, __LINE__);
	   udtCCWrapper.m_pRcvBuffer = new (buff) CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
	   // after introducing lite ACK, the sndlosslist may not be cleared in time, so it requires twice space.
	   buff = (char *) (GetMalloc_Ex())(sizeof(CSndLossList), __FILE__, __LINE__);
	   udtCCWrapper.m_pSndLossList = new (buff) CSndLossList(udtCCWrapper.m_iFlowWindowSize * 2);
	   buff = (char *) (GetMalloc_Ex())(sizeof(CRcvLossList), __FILE__, __LINE__);
	   udtCCWrapper.m_pRcvLossList = new (buff) CRcvLossList(m_iFlightFlagSize);
	   buff = (char *) (GetMalloc_Ex())(sizeof(CACKWindow), __FILE__, __LINE__);
	   udtCCWrapper.m_pACKWindow = new (buff) CACKWindow(4096);
	   buff = (char *) (GetMalloc_Ex())(sizeof(CPktTimeWindow), __FILE__, __LINE__);
	   udtCCWrapper.m_pRcvTimeWindow = new (buff) CPktTimeWindow(16, 64);
	   buff = (char *) (GetMalloc_Ex())(sizeof(CPktTimeWindow), __FILE__, __LINE__);
	   udtCCWrapper.m_pSndTimeWindow = new (buff) CPktTimeWindow();
#else
      udtCCWrapper.m_pSndBuffer = new CSndBuffer(32, m_iPayloadSize);
      udtCCWrapper.m_pRcvBuffer = new CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
      // after introducing lite ACK, the sndlosslist may not be cleared in time, so it requires twice space.
      udtCCWrapper.m_pSndLossList = new CSndLossList(m_iFlowWindowSize * 2);
      udtCCWrapper.m_pRcvLossList = new CRcvLossList(m_iFlightFlagSize);
      m_pACKWindow = new CACKWindow(4096);
      udtCCWrapper.m_pRcvTimeWindow = new CPktTimeWindow(16, 64);
      m_pSndTimeWindow = new CPktTimeWindow();
#endif
   }
   CATCH_UDT_ELLIPSES
   {
	   THROW_CUDTEXCEPTION_RET(3, 2, 0, -1);
   }

   udtCCWrapper.m_pCC = m_pCCFactory->create();
   udtCCWrapper.m_pCC->m_UDT = m_SocketID;
   udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
   udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;

   //udtCCWrapper.m_pController->join(this, serv_addr, m_iIPversion, udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);
//   udtCCWrapper.m_pController->join(serv_addr, m_iIPversion, udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);
   udtCCWrapper.m_pCC->setMSS(m_iMSS);
   udtCCWrapper.m_pCC->setMaxCWndSize((int&)udtCCWrapper.m_iFlowWindowSize);
   udtCCWrapper.m_pCC->setSndCurrSeqNo(udtCCWrapper.m_iSndCurrSeqNo);
   udtCCWrapper.m_pCC->setRcvRate(udtCCWrapper.m_iDeliveryRate);
   udtCCWrapper.m_pCC->setRTT(udtCCWrapper.m_iRTT);
   udtCCWrapper.m_pCC->setBandwidth(udtCCWrapper.m_iBandwidth);
   udtCCWrapper.m_pCC->setUserParam((char*)&(m_llMaxBW), 8);
   udtCCWrapper.m_pCC->init();

   //m_pPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
   m_pPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)RakNet::OP_NEW<sockaddr_in>(__FILE__, __LINE__) : (sockaddr*)RakNet::OP_NEW<sockaddr_in6>(__FILE__, __LINE__);
   memcpy(m_pPeerAddr, serv_addr, (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

   // And, I am connected too.
   m_bConnected = true;

   // register this socket for receiving data packets
   m_pRcvQueue->setNewEntry(this);

   // remove from rendezvous queue
   m_pRcvQueue->m_pRendezvousQueue->remove(m_SocketID);

   return 0;
}

int CUDT::connect(const sockaddr* peer, CHandShake* hs)
{
   // Type 0 (handshake) control packet
   CPacket initpkt;
   CHandShake ci;
   memcpy(&ci, hs, sizeof(CHandShake));
   initpkt.pack(0, NULL, &ci, sizeof(CHandShake));

   // Uses the smaller MSS between the peers        
   if (ci.m_iMSS > m_iMSS)
      ci.m_iMSS = m_iMSS;
   else
      m_iMSS = ci.m_iMSS;

   // exchange info for maximum flow window size
   udtCCWrapper.m_iFlowWindowSize = ci.m_iFlightFlagSize;
   ci.m_iFlightFlagSize = (m_iRcvBufSize < m_iFlightFlagSize)? m_iRcvBufSize : m_iFlightFlagSize;

   udtCCWrapper.m_iPeerISN = ci.m_iISN;

   udtCCWrapper.m_iRcvLastAck = ci.m_iISN;
   udtCCWrapper.m_iRcvLastAckAck = ci.m_iISN;
   udtCCWrapper.m_iRcvCurrSeqNo = ci.m_iISN - 1;

   m_PeerID = ci.m_iID;
   ci.m_iID = m_SocketID;

   // use peer's ISN and send it back for security check
   udtCCWrapper.m_iISN = ci.m_iISN;

   udtCCWrapper.m_iLastDecSeq = udtCCWrapper.m_iISN - 1;
   udtCCWrapper.m_iSndLastAck = udtCCWrapper.m_iISN;
   udtCCWrapper.m_iSndLastDataAck = udtCCWrapper.m_iISN;
   udtCCWrapper.m_iSndCurrSeqNo = udtCCWrapper.m_iISN - 1;
   udtCCWrapper.m_iSndLastAck2 = udtCCWrapper.m_iISN;
   udtCCWrapper.m_ullSndLastAck2Time = CTimer::getTime();

   // this is a reponse handshake
   ci.m_iReqType = -1;

   // Save the negotiated configurations.
   memcpy(hs, &ci, sizeof(CHandShake));
  
   m_iPktSize = m_iMSS - 28;
   m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;

   // Prepare all structures
   TRY_CUDTEXCEPTION
   {
#if defined(_USE_RAK_MEMORY_OVERRIDE)
	  char *buff;
	  buff = (char *) (GetMalloc_Ex())(sizeof(CSndBuffer), __FILE__, __LINE__);
	  udtCCWrapper.m_pSndBuffer = new (buff) CSndBuffer(32, m_iPayloadSize);
	  buff = (char *) (GetMalloc_Ex())(sizeof(CRcvBuffer), __FILE__, __LINE__);
	  udtCCWrapper.m_pRcvBuffer = new (buff) CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
	  buff = (char *) (GetMalloc_Ex())(sizeof(CSndLossList), __FILE__, __LINE__);
	  udtCCWrapper.m_pSndLossList = new (buff) CSndLossList(udtCCWrapper.m_iFlowWindowSize * 2);
	  buff = (char *) (GetMalloc_Ex())(sizeof(CRcvLossList), __FILE__, __LINE__);
	  udtCCWrapper.m_pRcvLossList = new (buff) CRcvLossList(m_iFlightFlagSize);
	  buff = (char *) (GetMalloc_Ex())(sizeof(CACKWindow), __FILE__, __LINE__);
	  udtCCWrapper.m_pACKWindow = new (buff) CACKWindow(4096);
	  buff = (char *) (GetMalloc_Ex())(sizeof(CPktTimeWindow), __FILE__, __LINE__);
	  udtCCWrapper.m_pRcvTimeWindow = new (buff) CPktTimeWindow(16, 64);
	  buff = (char *) (GetMalloc_Ex())(sizeof(CPktTimeWindow), __FILE__, __LINE__);
	  udtCCWrapper.m_pSndTimeWindow = new (buff) CPktTimeWindow();
#else
	  udtCCWrapper.m_pSndBuffer = new CSndBuffer(32, m_iPayloadSize);
	  udtCCWrapper.m_pRcvBuffer = new CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
	  udtCCWrapper.m_pSndLossList = new CSndLossList(udtCCWrapper.m_iFlowWindowSize * 2);
	  udtCCWrapper.m_pRcvLossList = new CRcvLossList(m_iFlightFlagSize);
	  udtCCWrapper.m_pACKWindow = new CACKWindow(4096);
	  udtCCWrapper.m_pRcvTimeWindow = new CPktTimeWindow(16, 64);
	  udtCCWrapper.m_pSndTimeWindow = new CPktTimeWindow();
#endif
   }
   CATCH_UDT_ELLIPSES
   {
	   THROW_CUDTEXCEPTION_RET(3, 2, 0, -1);
   }

   udtCCWrapper.m_pCC = m_pCCFactory->create();
   udtCCWrapper.m_pCC->m_UDT = m_SocketID;
   udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
   udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;

   //udtCCWrapper.m_pController->join(this, peer, m_iIPversion, udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);
//   udtCCWrapper.m_pController->join(peer, m_iIPversion, udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);
   udtCCWrapper.m_pCC->setMSS(m_iMSS);
   udtCCWrapper.m_pCC->setMaxCWndSize((int&)udtCCWrapper.m_iFlowWindowSize);
   udtCCWrapper.m_pCC->setSndCurrSeqNo(udtCCWrapper.m_iSndCurrSeqNo);
   udtCCWrapper.m_pCC->setRcvRate(udtCCWrapper.m_iDeliveryRate);
   udtCCWrapper.m_pCC->setRTT(udtCCWrapper.m_iRTT);
   udtCCWrapper.m_pCC->setBandwidth(udtCCWrapper.m_iBandwidth);
   udtCCWrapper.m_pCC->init();

   m_pPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)RakNet::OP_NEW<sockaddr_in>(__FILE__, __LINE__) : (sockaddr*) RakNet::OP_NEW<sockaddr_in6>(__FILE__, __LINE__);
   memcpy(m_pPeerAddr, peer, (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

   // And of course, it is connected.
   m_bConnected = true;

   // register this socket for receiving data packets
   m_pRcvQueue->setNewEntry(this);

   return 0;
}

void CUDT::close()
{
   if (!m_bOpened)
      return;

   if (!m_bConnected)
      m_bClosing = true;

   if (0 != m_Linger.l_onoff)
   {
      uint64_t entertime = CTimer::getTime();

      while (!m_bBroken && m_bConnected && (udtCCWrapper.m_pSndBuffer->getCurrBufSize() > 0) && (CTimer::getTime() - entertime < m_Linger.l_linger * 1000000ULL))
      {
         #ifndef WIN32
            timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000;
            nanosleep(&ts, NULL);
         #else
            Sleep(1);
         #endif
      }
   }

   // remove this socket from the snd queue
   if (m_bConnected)
      m_pSndQueue->m_pSndUList->remove(this);

   CGuard cg(m_ConnectionLock);

   // Inform the threads handler to stop.
   m_bClosing = true;

   // Signal the sender and recver if they are waiting for data.
   releaseSynch();

   if (m_bListening)
   {
      m_bListening = false;
      m_pRcvQueue->removeListener(this);
   }
   if (m_bConnected)
   {
      if (!m_bShutdown)
         sendCtrl(5);

      udtCCWrapper.m_pCC->close();
      //udtCCWrapper.m_pController->leave(this, udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);
//	  udtCCWrapper.m_pController->leave(udtCCWrapper.m_iRTT, udtCCWrapper.m_iBandwidth);

      m_bConnected = false;
   }

   // waiting all send and recv calls to stop
   CGuard sendguard(m_SendLock);
   CGuard recvguard(m_RecvLock);

   // CLOSED.
   m_bOpened = false;
}

int CUDT::send(const char* data, const int& len)
{
   if (UDT_DGRAM == m_iSockType)
      THROW_CUDTEXCEPTION_RET(5, 10, 0, -1);

   CGuard sendguard(m_SendLock);

   // throw an exception if not connected
   if (m_bBroken || m_bClosing)
   {
#ifdef UDT_DISABLE_EXCEPTIONS
	   return UDT_LOST_CONNECTION;
#else
      THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
   }
   else if (!m_bConnected)
   {
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }

   if (len <= 0)
      return 0;

   if (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize())
   {
      if (!m_bSynSending)
	  {
         THROW_CUDTEXCEPTION_RET(6, 1, 0, -1);
	  }
      else
      {
         // wait here during a blocking sending
         #ifndef WIN32
            pthread_mutex_lock(&m_SendBlockLock);
            if (m_iSndTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize()))
                  pthread_cond_wait(&m_SendBlockCond, &m_SendBlockLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iSndTimeOut * 1000ULL;
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;
    
               pthread_cond_timedwait(&m_SendBlockCond, &m_SendBlockLock, &locktime);
            }
            pthread_mutex_unlock(&m_SendBlockLock);
         #else
            if (m_iSndTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize()))
                  WaitForSingleObject(m_SendBlockCond, INFINITE);
            }
            else 
               WaitForSingleObject(m_SendBlockCond, DWORD(m_iSndTimeOut)); 
         #endif

         // check the connection status
         if (m_bBroken || m_bClosing)
		 {
#ifdef UDT_DISABLE_EXCEPTIONS
			 return UDT_LOST_CONNECTION;
#else
            THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
		}
         else if (!m_bConnected)
		 {
            THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
			}
      }
   }

   if (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize())
      return 0; 

   int size = (m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize;
   if (size > len)
      size = len;

   // insert the user buffer into the sening list
   udtCCWrapper.m_pSndBuffer->addBuffer(data, size);

   // insert this socket to snd list if it is not on the list yet
   m_pSndQueue->m_pSndUList->update(this, false);

   return size;
}

int CUDT::recv(char* data, const int& len)
{
   if (UDT_DGRAM == m_iSockType)
      THROW_CUDTEXCEPTION_RET(5, 10, 0, -1);

   CGuard recvguard(m_RecvLock);

   // throw an exception if not connected
   if (!m_bConnected)
   {
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }
   else if ((m_bBroken || m_bClosing) && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
   {
      THROW_CUDTEXCEPTION_RET(2, 1, 0, -1);
   }

   if (len <= 0)
      return 0;

   if (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize())
   {
      if (!m_bSynRecving)
	  {
         THROW_CUDTEXCEPTION_RET(6, 2, 0, -1);
	  }
      else
      {
         #ifndef WIN32
            pthread_mutex_lock(&m_RecvDataLock);
            if (m_iRcvTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
                  pthread_cond_wait(&m_RecvDataCond, &m_RecvDataLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iRcvTimeOut * 1000ULL; 
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;
    
               pthread_cond_timedwait(&m_RecvDataCond, &m_RecvDataLock, &locktime); 
            }
            pthread_mutex_unlock(&m_RecvDataLock);
         #else
            if (m_iRcvTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
                  WaitForSingleObject(m_RecvDataCond, INFINITE);
            }
            else
               WaitForSingleObject(m_RecvDataCond, DWORD(m_iRcvTimeOut));
         #endif
      }
   }

   // throw an exception if not connected
   if (!m_bConnected)
   {
	   THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }
   else if ((m_bBroken || m_bClosing) && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
   {
      THROW_CUDTEXCEPTION_RET(2, 1, 0, -1);
   }

   return udtCCWrapper.m_pRcvBuffer->readBuffer(data, len);
}

int CUDT::sendmsg(const char* data, const int& len, const int& msttl, const bool& inorder)
{
   if (UDT_STREAM == m_iSockType)
	   THROW_CUDTEXCEPTION_RET(5, 9, 0, -1);

   CGuard sendguard(m_SendLock);

   // throw an exception if not connected
   if (m_bBroken || m_bClosing)
   {
#ifdef UDT_DISABLE_EXCEPTIONS
	   return UDT_LOST_CONNECTION;
#else
      THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
   }
   else if (!m_bConnected)
   {
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }

   if (len <= 0)
      return 0;

   if (len > m_iSndBufSize * m_iPayloadSize)
      THROW_CUDTEXCEPTION_RET(5, 12, 0, -1);

   if ((m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize < len)
   {
      if (!m_bSynSending)
	  {
#ifdef UDT_DISABLE_EXCEPTIONS
		  return 0;
#else
         THROW_CUDTEXCEPTION_RET(6, 1, 0, 0);
#endif
	  }
      else
      {
         // wait here during a blocking sending
         #ifndef WIN32
            pthread_mutex_lock(&m_SendBlockLock);
            if (m_iSndTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && ((m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize < len))
                  pthread_cond_wait(&m_SendBlockCond, &m_SendBlockLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iSndTimeOut * 1000ULL;
               timespec locktime;

               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;

               pthread_cond_timedwait(&m_SendBlockCond, &m_SendBlockLock, &locktime);
            }
            pthread_mutex_unlock(&m_SendBlockLock);
         #else
            if (m_iSndTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && ((m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize < len))
                  WaitForSingleObject(m_SendBlockCond, INFINITE);
            }
            else
               WaitForSingleObject(m_SendBlockCond, DWORD(m_iSndTimeOut));
         #endif

         // check the connection status
         if (m_bBroken || m_bClosing)
		 {
#ifdef UDT_DISABLE_EXCEPTIONS
			 return UDT_LOST_CONNECTION;
#else
            THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
		}
         else if (!m_bConnected)
		 {
            THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
		}
      }
   }

   if ((m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize < len)
      return 0;

   // insert the user buffer into the sening list
   udtCCWrapper.m_pSndBuffer->addBuffer(data, len, msttl, inorder);

   // insert this socket to the snd list if it is not on the list yet
   m_pSndQueue->m_pSndUList->update(this, false);

   return len;   
}

int CUDT::recvmsg(char* data, const int& len)
{
   if (UDT_STREAM == m_iSockType)
      THROW_CUDTEXCEPTION_RET(5, 9, 0, -1);

   CGuard recvguard(m_RecvLock);

   // throw an exception if not connected
   if (!m_bConnected)
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);

   if (len <= 0)
      return 0;

   if (m_bBroken || m_bClosing)
   {
      int res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len);
      if (0 == res)
	  {
#ifdef UDT_DISABLE_EXCEPTIONS
		  return UDT_LOST_CONNECTION;
#else
         THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
	  }
      else
         return res;
   }

   if (!m_bSynRecving)
   {
      int res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len);
      if (0 == res)
	  {
#ifdef UDT_DISABLE_EXCEPTIONS
		  return 0;
#else
         THROW_CUDTEXCEPTION_RET(6, 2, 0, 0);
#endif
	  }
      else
         return res;
   }

   int res = 0;
   bool timeout = false;

   do
   {
      #ifndef WIN32
         pthread_mutex_lock(&m_RecvDataLock);

         if (m_iRcvTimeOut < 0)
         {
            while (!m_bBroken && m_bConnected && !m_bClosing && (0 == (res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len))))
               pthread_cond_wait(&m_RecvDataCond, &m_RecvDataLock);
         }
         else
         {
            uint64_t exptime = CTimer::getTime() + m_iRcvTimeOut * 1000ULL;
            timespec locktime;

            locktime.tv_sec = exptime / 1000000;
            locktime.tv_nsec = (exptime % 1000000) * 1000;

            if (pthread_cond_timedwait(&m_RecvDataCond, &m_RecvDataLock, &locktime) == ETIMEDOUT)
               timeout = true;

            res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len);           
         }
         pthread_mutex_unlock(&m_RecvDataLock);
      #else
         if (m_iRcvTimeOut < 0)
         {
            while (!m_bBroken && m_bConnected && !m_bClosing && (0 == (res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len))))
               WaitForSingleObject(m_RecvDataCond, INFINITE);
         }
         else
         {
            if (WaitForSingleObject(m_RecvDataCond, DWORD(m_iRcvTimeOut)) == WAIT_TIMEOUT)
               timeout = true;

            res = udtCCWrapper.m_pRcvBuffer->readMsg(data, len);
         }
      #endif

      if (m_bBroken || m_bClosing)
	  {
#ifdef UDT_DISABLE_EXCEPTIONS
		  return UDT_LOST_CONNECTION;
#else
         THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
	  }
      else if (!m_bConnected)
	  {
		  THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
	  }
   } while ((0 == res) && !timeout);

   return res;
}
CDatagram* CUDT::recvDatagram(int udpSocket)
{
	return s_UDTUnited.recvDatagram(udpSocket);
}
int64_t CUDT::sendfile(ifstream& ifs, const int64_t& offset, const int64_t& size, const int& block)
{
   if (UDT_DGRAM == m_iSockType)
	   THROW_CUDTEXCEPTION_RET(5, 10, 0, -1);

   CGuard sendguard(m_SendLock);

   if (m_bBroken || m_bClosing)
   {
#ifdef UDT_DISABLE_EXCEPTIONS
	   return UDT_LOST_CONNECTION;
#else
      THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
   }
   else if (!m_bConnected)
   {
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }

   if (size <= 0)
      return 0;

   int64_t tosend = size;
   int unitsize;

   // positioning...
   TRY_CUDTEXCEPTION
   {
      ifs.seekg((streamoff)offset);
   }
   CATCH_UDT_ELLIPSES
   {
	   THROW_CUDTEXCEPTION_RET(4, 1, -1, -1);
   }

   // sending block by block
   while (tosend > 0)
   {
      if (ifs.bad() || ifs.fail() || ifs.eof())
         break;

      unitsize = int((tosend >= block) ? block : tosend);

      #ifndef WIN32
         pthread_mutex_lock(&m_SendBlockLock);
         while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize()))
            pthread_cond_wait(&m_SendBlockCond, &m_SendBlockLock);
         pthread_mutex_unlock(&m_SendBlockLock);
      #else
         while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= udtCCWrapper.m_pSndBuffer->getCurrBufSize()))
            WaitForSingleObject(m_SendBlockCond, INFINITE);
      #endif

      if (m_bBroken || m_bClosing)
	  {
#ifdef UDT_DISABLE_EXCEPTIONS
		  return UDT_LOST_CONNECTION;
#else
		  THROW_CUDTEXCEPTION_RET(2, 1, 0, UDT_LOST_CONNECTION);
#endif
	  }
      else if (!m_bConnected)
	  {
         THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
	  }

      tosend -= udtCCWrapper.m_pSndBuffer->addBufferFromFile(ifs, unitsize);

      // insert this socket to snd list if it is not on the list yet
      m_pSndQueue->m_pSndUList->update(this, false);
   }

   return size - tosend;
}

int64_t CUDT::recvfile(ofstream& ofs, const int64_t& offset, const int64_t& size, const int& block)
{
   if (UDT_DGRAM == m_iSockType)
	   THROW_CUDTEXCEPTION_RET(5, 10, 0, -1);

   CGuard recvguard(m_RecvLock);

   if (!m_bConnected)
   {
      THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
   }
   else if ((m_bBroken || m_bClosing) && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
   {
      THROW_CUDTEXCEPTION_RET(2, 1, 0, -1);
   }

   if (size <= 0)
      return 0;

   int64_t torecv = size;
   int unitsize = block;
   int recvsize;

   // positioning...
   TRY_CUDTEXCEPTION
   {
      ofs.seekp((streamoff)offset);
   }
   CATCH_UDT_ELLIPSES
   {
	   THROW_CUDTEXCEPTION_RET(4, 3, -1, -1);
   }

   // receiving... "recvfile" is always blocking
   while (torecv > 0)
   {
      if (ofs.bad() || ofs.fail())
         break;

      #ifndef WIN32
         pthread_mutex_lock(&m_RecvDataLock);
         while (!m_bBroken && m_bConnected && !m_bClosing && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
            pthread_cond_wait(&m_RecvDataCond, &m_RecvDataLock);
         pthread_mutex_unlock(&m_RecvDataLock);
      #else
         while (!m_bBroken && m_bConnected && !m_bClosing && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
            WaitForSingleObject(m_RecvDataCond, INFINITE);
      #endif

      if (!m_bConnected)
	  {
         THROW_CUDTEXCEPTION_RET(2, 2, 0, UDT_NOT_CONNECTED);
	  }
      else if ((m_bBroken || m_bClosing) && (0 == udtCCWrapper.m_pRcvBuffer->getRcvDataSize()))
	  {
		  THROW_CUDTEXCEPTION_RET(2, 1, 0, -1);
	  }

      unitsize = int((torecv >= block) ? block : torecv);
      recvsize = udtCCWrapper.m_pRcvBuffer->readBufferToFile(ofs, unitsize);

      torecv -= recvsize;
   }

   return size - torecv;
}

void CUDT::sample(CPerfMon* perf, bool clear)
{
   if (!m_bConnected)
      THROW_CUDTEXCEPTION(2, 2, 0);
   if (m_bBroken || m_bClosing)
   {
      THROW_CUDTEXCEPTION(2, 1, 0);
   }

   uint64_t currtime = CTimer::getTime();

   perf->msTimeStamp = (currtime - m_StartTime) / 1000;

   m_llSentTotal += m_llTraceSent;
   m_llRecvTotal += m_llTraceRecv;
   m_iSndLossTotal += m_iTraceSndLoss;
   m_iRcvLossTotal += m_iTraceRcvLoss;
   m_iRetransTotal += m_iTraceRetrans;
   m_iSentACKTotal += m_iSentACK;
   m_iRecvACKTotal += m_iRecvACK;
   m_iSentNAKTotal += m_iSentNAK;
   m_iRecvNAKTotal += m_iRecvNAK;

   perf->pktSentTotal = m_llSentTotal;
   perf->pktRecvTotal = m_llRecvTotal;
   perf->pktSndLossTotal = m_iSndLossTotal;
   perf->pktRcvLossTotal = m_iRcvLossTotal;
   perf->pktRetransTotal = m_iRetransTotal;
   perf->pktSentACKTotal = m_iSentACKTotal;
   perf->pktRecvACKTotal = m_iRecvACKTotal;
   perf->pktSentNAKTotal = m_iSentNAKTotal;
   perf->pktRecvNAKTotal = m_iRecvNAKTotal;

   perf->pktSent = m_llTraceSent;
   perf->pktRecv = m_llTraceRecv;
   perf->pktSndLoss = m_iTraceSndLoss;
   perf->pktRcvLoss = m_iTraceRcvLoss;
   perf->pktRetrans = m_iTraceRetrans;
   perf->pktSentACK = m_iSentACK;
   perf->pktRecvACK = m_iRecvACK;
   perf->pktSentNAK = m_iSentNAK;
   perf->pktRecvNAK = m_iRecvNAK;

   double interval = double(currtime - m_LastSampleTime);

   perf->mbpsSendRate = double(m_llTraceSent) * m_iPayloadSize * 8.0 / interval;
   perf->mbpsRecvRate = double(m_llTraceRecv) * m_iPayloadSize * 8.0 / interval;

   perf->usPktSndPeriod = udtCCWrapper.m_ullInterval / double(udtCCWrapper.m_ullCPUFrequency);
   perf->pktFlowWindow = udtCCWrapper.m_iFlowWindowSize;
   perf->pktCongestionWindow = (int)udtCCWrapper.m_dCongestionWindow;
   perf->pktFlightSize = CSeqNo::seqlen(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), udtCCWrapper.m_iSndCurrSeqNo);
   perf->msRTT = udtCCWrapper.m_iRTT/1000.0;
   perf->mbpsBandwidth = udtCCWrapper.m_iBandwidth * m_iPayloadSize * 8.0 / 1000000.0;

   #ifndef WIN32
      if (0 == pthread_mutex_trylock(&m_ConnectionLock))
   #else
      if (WAIT_OBJECT_0 == WaitForSingleObject(m_ConnectionLock, 0))
   #endif
   {
      perf->byteAvailSndBuf = (NULL == udtCCWrapper.m_pSndBuffer) ? 0 : m_iSndBufSize - udtCCWrapper.m_pSndBuffer->getCurrBufSize();
      perf->byteAvailRcvBuf = (NULL == udtCCWrapper.m_pRcvBuffer) ? 0 : udtCCWrapper.m_pRcvBuffer->getAvailBufSize();

      #ifndef WIN32
         pthread_mutex_unlock(&m_ConnectionLock);
      #else
         ReleaseMutex(m_ConnectionLock);
      #endif
   }
   else
   {
      perf->byteAvailSndBuf = 0;
      perf->byteAvailRcvBuf = 0;
   }

   if (clear)
   {
      m_llTraceSent = m_llTraceRecv = m_iTraceSndLoss = m_iTraceSndLoss = m_iTraceRetrans = m_iSentACK = m_iRecvACK = m_iSentNAK = m_iRecvNAK = 0;
      m_LastSampleTime = currtime;
   }
}

void CUDT::initSynch()
{
   #ifndef WIN32
      pthread_mutex_init(&m_SendBlockLock, NULL);
      pthread_cond_init(&m_SendBlockCond, NULL);
      pthread_mutex_init(&m_RecvDataLock, NULL);
      pthread_cond_init(&m_RecvDataCond, NULL);
      pthread_mutex_init(&m_SendLock, NULL);
      pthread_mutex_init(&m_RecvLock, NULL);
      pthread_mutex_init(&m_AckLock, NULL);
      pthread_mutex_init(&m_ConnectionLock, NULL);
   #else
      m_SendBlockLock = CreateMutex(NULL, false, NULL);
      m_SendBlockCond = CreateEvent(NULL, false, false, NULL);
      m_RecvDataLock = CreateMutex(NULL, false, NULL);
      m_RecvDataCond = CreateEvent(NULL, false, false, NULL);
      m_SendLock = CreateMutex(NULL, false, NULL);
      m_RecvLock = CreateMutex(NULL, false, NULL);
      m_AckLock = CreateMutex(NULL, false, NULL);
      m_ConnectionLock = CreateMutex(NULL, false, NULL);
   #endif
}

void CUDT::destroySynch()
{
   #ifndef WIN32
      pthread_mutex_destroy(&m_SendBlockLock);
      pthread_cond_destroy(&m_SendBlockCond);
      pthread_mutex_destroy(&m_RecvDataLock);
      pthread_cond_destroy(&m_RecvDataCond);
      pthread_mutex_destroy(&m_SendLock);
      pthread_mutex_destroy(&m_RecvLock);
      pthread_mutex_destroy(&m_AckLock);
      pthread_mutex_destroy(&m_ConnectionLock);
   #else
      CloseHandle(m_SendBlockLock);
      CloseHandle(m_SendBlockCond);
      CloseHandle(m_RecvDataLock);
      CloseHandle(m_RecvDataCond);
      CloseHandle(m_SendLock);
      CloseHandle(m_RecvLock);
      CloseHandle(m_AckLock);
      CloseHandle(m_ConnectionLock);
   #endif
}

void CUDT::releaseSynch()
{
   #ifndef WIN32
      // wake up user calls
      pthread_mutex_lock(&m_SendBlockLock);
      pthread_cond_signal(&m_SendBlockCond);
      pthread_mutex_unlock(&m_SendBlockLock);

      pthread_mutex_lock(&m_SendLock);
      pthread_mutex_unlock(&m_SendLock);

      pthread_mutex_lock(&m_RecvDataLock);
      pthread_cond_signal(&m_RecvDataCond);
      pthread_mutex_unlock(&m_RecvDataLock);

      pthread_mutex_lock(&m_RecvLock);
      pthread_mutex_unlock(&m_RecvLock);
   #else
      SetEvent(m_SendBlockCond);
      WaitForSingleObject(m_SendLock, INFINITE);
      ReleaseMutex(m_SendLock);
      SetEvent(m_RecvDataCond);
      WaitForSingleObject(m_RecvLock, INFINITE);
      ReleaseMutex(m_RecvLock);
   #endif
}
void CUDT::sendCtrl(const int& pkttype, void* lparam, void* rparam, const int& size)
{
	CPacket ctrlpkt;

	switch (pkttype)
	{
	case 2: //010 - Acknowledgement
		{
			int32_t ack;

			// If there is no loss, the ACK is the current largest sequence number plus 1;
			// Otherwise it is the smallest sequence number in the receiver loss list.
			if (0 == udtCCWrapper.m_pRcvLossList->getLossLength())
				ack = CSeqNo::incseq(udtCCWrapper.m_iRcvCurrSeqNo);
			else
				ack = udtCCWrapper.m_pRcvLossList->getFirstLostSeq();

			if (ack == udtCCWrapper.m_iRcvLastAckAck)
				break;

			// send out a lite ACK
			// to save time on buffer processing and bandwidth/AS measurement, a lite ACK only feeds back an ACK number
			if (4 == size)
			{
				ctrlpkt.pack(2, NULL, &ack, size);
				ctrlpkt.m_iID = m_PeerID;
				m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

				break;
			}

			uint64_t currtime;
			CTimer::rdtsc(currtime);

			// There are new received packets to acknowledge, update related information.
			if (CSeqNo::seqcmp(ack, udtCCWrapper.m_iRcvLastAck) > 0)
			{
				int acksize = CSeqNo::seqoff(udtCCWrapper.m_iRcvLastAck, ack);

				udtCCWrapper.m_iRcvLastAck = ack;

				udtCCWrapper.m_pRcvBuffer->ackData(acksize);

				// signal a waiting "recv" call if there is any data available
#ifndef WIN32
				pthread_mutex_lock(&m_RecvDataLock);
				if (m_bSynRecving)
					pthread_cond_signal(&m_RecvDataCond);
				pthread_mutex_unlock(&m_RecvDataLock);
#else
				if (m_bSynRecving)
					SetEvent(m_RecvDataCond);
#endif
			}
			else if (ack == udtCCWrapper.m_iRcvLastAck)
			{
				if ((currtime - udtCCWrapper.m_ullLastAckTime) < ((udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) * udtCCWrapper.m_ullCPUFrequency))
					break;
			}
			else
				break;

			// Send out the ACK only if has not been received by the sender before
			if (CSeqNo::seqcmp(udtCCWrapper.m_iRcvLastAck, udtCCWrapper.m_iRcvLastAckAck) > 0)
			{
				int32_t data[6];

				udtCCWrapper.m_iAckSeqNo = CAckNo::incack(udtCCWrapper.m_iAckSeqNo);
				data[0] = udtCCWrapper.m_iRcvLastAck;
				data[1] = udtCCWrapper.m_iRTT;
				data[2] = udtCCWrapper.m_iRTTVar;
				data[3] = udtCCWrapper.m_pRcvBuffer->getAvailBufSize();
				// a minimum flow window of 2 is used, even if buffer is full, to break potential deadlock
				if (data[3] < 2)
					data[3] = 2;

				if (currtime - udtCCWrapper.m_ullLastAckTime > udtCCWrapper.m_ullSYNInt)
				{
					data[4] = udtCCWrapper.m_pRcvTimeWindow->getPktRcvSpeed();
					data[5] = udtCCWrapper.m_pRcvTimeWindow->getBandwidth();
					ctrlpkt.pack(2, &udtCCWrapper.m_iAckSeqNo, data, 24);

					CTimer::rdtsc(udtCCWrapper.m_ullLastAckTime);
				}
				else
				{
					ctrlpkt.pack(2, &udtCCWrapper.m_iAckSeqNo, data, 16);
				}

				ctrlpkt.m_iID = m_PeerID;
				m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

				udtCCWrapper.m_pACKWindow->store(udtCCWrapper.m_iAckSeqNo, udtCCWrapper.m_iRcvLastAck);

				++ m_iSentACK;
			}

			break;
		}

	case 6: //110 - Acknowledgement of Acknowledgement
		ctrlpkt.pack(6, lparam);
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		break;

	case 3: //011 - Loss Report
		if (NULL != rparam)
		{
			if (1 == size)
			{
				// only 1 loss packet
				ctrlpkt.pack(3, NULL, (int32_t *)rparam + 1, 4);
			}
			else
			{
				// more than 1 loss packets
				ctrlpkt.pack(3, NULL, rparam, 8);
			}

			ctrlpkt.m_iID = m_PeerID;
			m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

			++ m_iSentNAK;
		}
		else if (udtCCWrapper.m_pRcvLossList->getLossLength() > 0)
		{
			// this is periodically NAK report

			// read loss list from the local receiver loss list
			// int32_t* data = new int32_t[m_iPayloadSize / 4];
			int32_t* data = RakNet::OP_NEW_ARRAY<int32_t>(m_iPayloadSize / 4, __FILE__, __LINE__ );
			int losslen;
			udtCCWrapper.m_pRcvLossList->getLossArray(data, losslen, m_iPayloadSize / 4, udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar);

			if (0 < losslen)
			{
				ctrlpkt.pack(3, NULL, data, losslen * 4);
				ctrlpkt.m_iID = m_PeerID;
				m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

				++ m_iSentNAK;
			}

			// delete [] data;
			RakNet::OP_DELETE_ARRAY(data,__FILE__, __LINE__);
		}

		break;

	case 4: //100 - Congestion Warning
		ctrlpkt.pack(4);
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		CTimer::rdtsc(udtCCWrapper.m_ullLastWarningTime);

		break;

	case 1: //001 - Keep-alive
		ctrlpkt.pack(1);
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		break;

	case 0: //000 - Handshake
		ctrlpkt.pack(0, NULL, rparam, sizeof(CHandShake));
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		break;

	case 5: //101 - Shutdown
		ctrlpkt.pack(5);
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		break;

	case 7: //111 - Msg drop request
		ctrlpkt.pack(7, lparam, rparam, 8);
		ctrlpkt.m_iID = m_PeerID;
		m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt);

		break;

	case 65535: //0x7FFF - Resevered for future use
		break;

	default:
		break;
	}
}
void CUDT::processCtrl(CPacket& ctrlpkt)
{
	// Just heard from the peer, reset the expiration count.
	udtCCWrapper.m_iEXPCount = 1;
	if ((CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo) == udtCCWrapper.m_iSndLastAck) || (2 == ctrlpkt.getType()) || (3 == ctrlpkt.getType()))
	{
		udtCCWrapper.m_ullEXPInt = udtCCWrapper.m_ullMinEXPInt;
		CTimer::rdtsc(udtCCWrapper.m_ullNextEXPTime);
		udtCCWrapper.m_ullNextEXPTime += udtCCWrapper.m_ullEXPInt;
	}

	switch (ctrlpkt.getType())
	{
	case 2: //010 - Acknowledgement
		{
			int32_t ack;

			// process a lite ACK
			if (4 == ctrlpkt.getLength())
			{
				ack = *(int32_t *)ctrlpkt.m_pcData;
				if (CSeqNo::seqcmp(ack, const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck)) >= 0)
				{
					udtCCWrapper.m_iFlowWindowSize -= CSeqNo::seqoff(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), ack);
					udtCCWrapper.m_iSndLastAck = ack;
				}

				break;
			}

			// read ACK seq. no.
			ack = ctrlpkt.getAckSeqNo();

			// send ACK acknowledgement
			// ACK2 can be much less than ACK
			uint64_t currtime = CTimer::getTime();
			if ((currtime - udtCCWrapper.m_ullSndLastAck2Time > (uint64_t)udtCCWrapper.m_iSYNInterval) || (ack == udtCCWrapper.m_iSndLastAck2))
			{
				sendCtrl(6, &ack);
				udtCCWrapper.m_iSndLastAck2 = ack;
				udtCCWrapper.m_ullSndLastAck2Time = currtime;
			}

			// Got data ACK
			ack = *(int32_t *)ctrlpkt.m_pcData;

			// check the validation of the ack
			if (CSeqNo::seqcmp(ack, CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo)) > 0)
			{
				//this should not happen: attack or bug
				m_bBroken = true;
				udtCCWrapper.m_iBrokenCounter = 0;
				break;
			}

			if (CSeqNo::seqcmp(ack, const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck)) >= 0)
			{
				// Update Flow Window Size, must update before and together with m_iSndLastAck
				udtCCWrapper.m_iFlowWindowSize = *((int32_t *)ctrlpkt.m_pcData + 3);
				udtCCWrapper.m_iSndLastAck = ack;
			}

			// protect packet retransmission
			CGuard::enterCS(m_AckLock);

			int offset = CSeqNo::seqoff(udtCCWrapper.m_iSndLastDataAck, ack);
			if (offset <= 0)
			{
				// discard it if it is a repeated ACK
				CGuard::leaveCS(m_AckLock);
				break;
			}

			// acknowledge the sending buffer
			udtCCWrapper.m_pSndBuffer->ackData(offset);

			// update sending variables
			udtCCWrapper.m_iSndLastDataAck = ack;
			udtCCWrapper.m_pSndLossList->remove(CSeqNo::decseq(udtCCWrapper.m_iSndLastDataAck));

			CGuard::leaveCS(m_AckLock);

#ifndef WIN32
			pthread_mutex_lock(&m_SendBlockLock);
			if (m_bSynSending)
				pthread_cond_signal(&m_SendBlockCond);
			pthread_mutex_unlock(&m_SendBlockLock);
#else
			if (m_bSynSending)
				SetEvent(m_SendBlockCond);
#endif

			// insert this socket to snd list if it is not on the list yet
			m_pSndQueue->m_pSndUList->update(this, false);

			// Update RTT
			//udtCCWrapper.m_iRTT = *((int32_t *)ctrlpkt.m_pcData + 1);
			//udtCCWrapper.m_iRTTVar = *((int32_t *)ctrlpkt.m_pcData + 2);
			int rtt = *((int32_t *)ctrlpkt.m_pcData + 1);
			udtCCWrapper.m_iRTTVar = (udtCCWrapper.m_iRTTVar * 3 + abs(rtt - udtCCWrapper.m_iRTT)) >> 2;
			udtCCWrapper.m_iRTT = (udtCCWrapper.m_iRTT * 7 + rtt) >> 3;

			udtCCWrapper.m_pCC->setRTT(udtCCWrapper.m_iRTT);

			udtCCWrapper.m_ullMinEXPInt = (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) * udtCCWrapper.m_ullCPUFrequency + udtCCWrapper.m_ullSYNInt;
			if (udtCCWrapper.m_ullMinEXPInt < 100000 * udtCCWrapper.m_ullCPUFrequency)
				udtCCWrapper.m_ullMinEXPInt = 100000 * udtCCWrapper.m_ullCPUFrequency;

			if (ctrlpkt.getLength() > 16)
			{
				// Update Estimated Bandwidth and packet delivery rate
				if (*((int32_t *)ctrlpkt.m_pcData + 4) > 0)
					udtCCWrapper.m_iDeliveryRate = (udtCCWrapper.m_iDeliveryRate * 7 + *((int32_t *)ctrlpkt.m_pcData + 4)) >> 3;

				if (*((int32_t *)ctrlpkt.m_pcData + 5) > 0)
					udtCCWrapper.m_iBandwidth = (udtCCWrapper.m_iBandwidth * 7 + *((int32_t *)ctrlpkt.m_pcData + 5)) >> 3;

				udtCCWrapper.m_pCC->setRcvRate(udtCCWrapper.m_iDeliveryRate);
				udtCCWrapper.m_pCC->setBandwidth(udtCCWrapper.m_iBandwidth);
			}

			udtCCWrapper.m_pCC->onACK(ack);
			// update CC parameters
			udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
			udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;

			++ m_iRecvACK;

			break;
		}

	case 6: //110 - Acknowledgement of Acknowledgement
		{
			int32_t ack;
			int rtt = -1;

			// update RTT
			rtt = udtCCWrapper.m_pACKWindow->acknowledge(ctrlpkt.getAckSeqNo(), ack);

			if (rtt <= 0)
				break;

			//if increasing delay detected...
			//   sendCtrl(4);

			// RTT EWMA
			udtCCWrapper.m_iRTTVar = (udtCCWrapper.m_iRTTVar * 3 + abs(rtt - udtCCWrapper.m_iRTT)) >> 2;
			udtCCWrapper.m_iRTT = (udtCCWrapper.m_iRTT * 7 + rtt) >> 3;

			udtCCWrapper.m_pCC->setRTT(udtCCWrapper.m_iRTT);

			udtCCWrapper.m_ullMinEXPInt = (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) * udtCCWrapper.m_ullCPUFrequency + udtCCWrapper.m_ullSYNInt;
			if (udtCCWrapper.m_ullMinEXPInt < 100000 * udtCCWrapper.m_ullCPUFrequency)
				udtCCWrapper.m_ullMinEXPInt = 100000 * udtCCWrapper.m_ullCPUFrequency;

			// update last ACK that has been received by the sender
			if (CSeqNo::seqcmp(ack, udtCCWrapper.m_iRcvLastAckAck) > 0)
				udtCCWrapper.m_iRcvLastAckAck = ack;

			break;
		}

	case 3: //011 - Loss Report
		{
			int32_t* losslist = (int32_t *)(ctrlpkt.m_pcData);

			udtCCWrapper.m_pCC->onLoss(losslist, ctrlpkt.getLength());
			// update CC parameters
			udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
			udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;

			bool secure = true;

			// decode loss list message and insert loss into the sender loss list
			for (int i = 0, n = (int)(ctrlpkt.getLength() / 4); i < n; ++ i)
			{
				if (0 != (losslist[i] & 0x80000000))
				{
					if ((CSeqNo::seqcmp(losslist[i] & 0x7FFFFFFF, losslist[i + 1]) > 0) || (CSeqNo::seqcmp(losslist[i + 1], udtCCWrapper.m_iSndCurrSeqNo) > 0))
					{
						// seq_a must not be greater than seq_b; seq_b must not be greater than the most recent sent seq
						secure = false;
						break;
					}

					if (CSeqNo::seqcmp(losslist[i] & 0x7FFFFFFF, const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck)) >= 0)
						m_iTraceSndLoss += udtCCWrapper.m_pSndLossList->insert(losslist[i] & 0x7FFFFFFF, losslist[i + 1]);
					else if (CSeqNo::seqcmp(losslist[i + 1], const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck)) >= 0)
						m_iTraceSndLoss += udtCCWrapper.m_pSndLossList->insert(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), losslist[i + 1]);

					++ i;
				}
				else if (CSeqNo::seqcmp(losslist[i], const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck)) >= 0)
				{
					if (CSeqNo::seqcmp(losslist[i], udtCCWrapper.m_iSndCurrSeqNo) > 0)
					{
						//seq_a must not be greater than the most recent sent seq
						secure = false;
						break;
					}

					m_iTraceSndLoss += udtCCWrapper.m_pSndLossList->insert(losslist[i], losslist[i]);
				}
			}

			if (!secure)
			{
				//this should not happen: attack or bug
				m_bBroken = true;
				udtCCWrapper.m_iBrokenCounter = 0;
				break;
			}

			// Wake up the waiting sender (avoiding deadlock on an infinite sleeping)
			udtCCWrapper.m_pSndLossList->insert(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck));

			// the lost packet (retransmission) should be sent out immediately
			m_pSndQueue->m_pSndUList->update(this);

			++ m_iRecvNAK;

			break;
		}

	case 4: //100 - Delay Warning
		// One way packet delay is increasing, so decrease the sending rate
		udtCCWrapper.m_ullInterval = (uint64_t)ceil(udtCCWrapper.m_ullInterval * 1.125);
		udtCCWrapper.m_iLastDecSeq = udtCCWrapper.m_iSndCurrSeqNo;

		break;

	case 1: //001 - Keep-alive
		// The only purpose of keep-alive packet is to tell that the peer is still alive
		// nothing needs to be done.

		break;

	case 0: //000 - Handshake
		if ((((CHandShake*)(ctrlpkt.m_pcData))->m_iReqType > 0) || (m_bRendezvous && (((CHandShake*)(ctrlpkt.m_pcData))->m_iReqType != -2)))
		{
			// The peer side has not received the handshake message, so it keeps querying
			// resend the handshake packet

			CHandShake initdata;
			initdata.m_iISN = udtCCWrapper.m_iISN;
			initdata.m_iMSS = m_iMSS;
			initdata.m_iFlightFlagSize = m_iFlightFlagSize;
			initdata.m_iReqType = (!m_bRendezvous) ? -1 : -2;
			initdata.m_iID = m_SocketID;
			sendCtrl(0, NULL, (char *)&initdata, sizeof(CHandShake));
		}

		break;

	case 5: //101 - Shutdown
		m_bShutdown = true;
		m_bClosing = true;
		m_bBroken = true;
		udtCCWrapper.m_iBrokenCounter = 60;

		// Signal the sender and recver if they are waiting for data.
		releaseSynch();

		CTimer::triggerEvent();

		break;

	case 7: //111 - Msg drop request
		udtCCWrapper.m_pRcvBuffer->dropMsg(ctrlpkt.getMsgSeq());
		udtCCWrapper.m_pRcvLossList->remove(*(int32_t*)ctrlpkt.m_pcData, *(int32_t*)(ctrlpkt.m_pcData + 4));

		break;

	case 65535: //0x7FFF - reserved and user defined messages
		udtCCWrapper.m_pCC->processCustomMsg(&ctrlpkt);
		// update CC parameters
		udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
		udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;

		break;

	default:
		break;
	}
}

int CUDT::packData(CPacket& packet, uint64_t& ts)
{
   int payload = 0;
   bool probe = false;

   uint64_t entertime;
   CTimer::rdtsc(entertime);

   if ((0 != udtCCWrapper.m_ullTargetTime) && (entertime > udtCCWrapper.m_ullTargetTime))
      udtCCWrapper.m_ullTimeDiff += entertime - udtCCWrapper.m_ullTargetTime;

   // Loss retransmission always has higher priority.
   if ((packet.m_iSeqNo = udtCCWrapper.m_pSndLossList->getLostSeq()) >= 0)
   {
      // protect m_iSndLastDataAck from updating by ACK processing
      CGuard ackguard(m_AckLock);

      int offset = CSeqNo::seqoff(udtCCWrapper.m_iSndLastDataAck, packet.m_iSeqNo);
      if (offset < 0)
         return 0;

      int msglen;

      payload = udtCCWrapper.m_pSndBuffer->readData(&(packet.m_pcData), offset, packet.m_iMsgNo, msglen);

      if (-1 == payload)
      {
         int32_t seqpair[2];
         seqpair[0] = packet.m_iSeqNo;
         seqpair[1] = CSeqNo::incseq(seqpair[0], msglen);
         sendCtrl(7, &packet.m_iMsgNo, seqpair, 8);

         // only one msg drop request is necessary
         udtCCWrapper.m_pSndLossList->remove(seqpair[1]);

         return 0;
      }
      else if (0 == payload)
         return 0;

      ++ m_iTraceRetrans;
   }
   else
   {
      // If no loss, pack a new packet.

      // check congestion/flow window limit
      int cwnd = (udtCCWrapper.m_iFlowWindowSize < (int)udtCCWrapper.m_dCongestionWindow) ? udtCCWrapper.m_iFlowWindowSize : (int)udtCCWrapper.m_dCongestionWindow;
      if (cwnd >= CSeqNo::seqlen(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo)))
      {
         if (0 != (payload = udtCCWrapper.m_pSndBuffer->readData(&(packet.m_pcData), packet.m_iMsgNo)))
         {
            udtCCWrapper.m_iSndCurrSeqNo = CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo);
            udtCCWrapper.m_pCC->setSndCurrSeqNo(udtCCWrapper.m_iSndCurrSeqNo);

            packet.m_iSeqNo = udtCCWrapper.m_iSndCurrSeqNo;

            // every 16 (0xF) packets, a packet pair is sent
            if (0 == (packet.m_iSeqNo & 0xF))
               probe = true;
         }
         else
         {
            udtCCWrapper.m_ullTargetTime = 0;
            udtCCWrapper.m_ullTimeDiff = 0;
            ts = 0;
            return 0;
         }
      }
      else
      {
         udtCCWrapper.m_ullTargetTime = 0;
         udtCCWrapper.m_ullTimeDiff = 0;
         ts = 0;
         return 0;
      }
   }

   packet.m_iTimeStamp = int(CTimer::getTime() - m_StartTime);
   udtCCWrapper.m_pSndTimeWindow->onPktSent(packet.m_iTimeStamp);

   packet.m_iID = m_PeerID;

   udtCCWrapper.m_pCC->onPktSent(&packet);

   ++ m_llTraceSent;

   if (probe)
   {
      // sends out probing packet pair
      ts = entertime;
      probe = false;
   }
   else
   {
      #ifndef NO_BUSY_WAITING
         ts = entertime + udtCCWrapper.m_ullInterval;
      #else
         if (udtCCWrapper.m_ullTimeDiff >= udtCCWrapper.m_ullInterval)
         {
            ts = entertime;
            udtCCWrapper.m_ullTimeDiff -= udtCCWrapper.m_ullInterval;
         }
         else
         {
            ts = entertime + udtCCWrapper.m_ullInterval - udtCCWrapper.m_ullTimeDiff;
            udtCCWrapper.m_ullTimeDiff = 0;
         }
      #endif
   }

   udtCCWrapper.m_ullTargetTime = ts;

   packet.m_iID = m_PeerID;
   packet.setLength(payload);

   return payload;
}
int CUDT::processData(CUnit* unit)
{
	CPacket& packet = unit->m_Packet;

	// Just heard from the peer, reset the expiration count.
	udtCCWrapper.m_iEXPCount = 1;
	udtCCWrapper.m_ullEXPInt = udtCCWrapper.m_ullMinEXPInt;

	if (CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo) == udtCCWrapper.m_iSndLastAck)
	{
		CTimer::rdtsc(udtCCWrapper.m_ullNextEXPTime);
		if (!udtCCWrapper.m_pCC->m_bUserDefinedRTO)
			udtCCWrapper.m_ullNextEXPTime += udtCCWrapper.m_ullEXPInt;
		else
			udtCCWrapper.m_ullNextEXPTime += udtCCWrapper.m_pCC->m_iRTO * udtCCWrapper.m_ullCPUFrequency;
	}

	udtCCWrapper.m_pCC->onPktReceived(&packet);

	++ udtCCWrapper.m_iPktCount;

	// update time information
	udtCCWrapper.m_pRcvTimeWindow->onPktArrival();

	// check if it is probing packet pair
	if (0 == (packet.m_iSeqNo & 0xF))
		udtCCWrapper.m_pRcvTimeWindow->probe1Arrival();
	else if (1 == (packet.m_iSeqNo & 0xF))
		udtCCWrapper.m_pRcvTimeWindow->probe2Arrival();

	++ m_llTraceRecv;

	int32_t offset = CSeqNo::seqoff(udtCCWrapper.m_iRcvLastAck, packet.m_iSeqNo);
	if ((offset < 0) || (offset >= udtCCWrapper.m_pRcvBuffer->getAvailBufSize()))
		return -1;

	if (udtCCWrapper.m_pRcvBuffer->addData(unit, offset) < 0)
		return -1;

	// Loss detection.
	if (CSeqNo::seqcmp(packet.m_iSeqNo, CSeqNo::incseq(udtCCWrapper.m_iRcvCurrSeqNo)) > 0)
	{
		// If loss found, insert them to the receiver loss list
		udtCCWrapper.m_pRcvLossList->insert(CSeqNo::incseq(udtCCWrapper.m_iRcvCurrSeqNo), CSeqNo::decseq(packet.m_iSeqNo));

		// pack loss list for NAK
		int32_t lossdata[2];
		lossdata[0] = CSeqNo::incseq(udtCCWrapper.m_iRcvCurrSeqNo) | 0x80000000;
		lossdata[1] = CSeqNo::decseq(packet.m_iSeqNo);

		// Generate loss report immediately.
		sendCtrl(3, NULL, lossdata, (CSeqNo::incseq(udtCCWrapper.m_iRcvCurrSeqNo) == CSeqNo::decseq(packet.m_iSeqNo)) ? 1 : 2);

		m_iTraceRcvLoss += CSeqNo::seqlen(udtCCWrapper.m_iRcvCurrSeqNo, packet.m_iSeqNo) - 2;
	}

	// This is not a regular fixed size packet...   
	//an irregular sized packet usually indicates the end of a message, so send an ACK immediately   
	if (packet.getLength() != m_iPayloadSize)   
		CTimer::rdtsc(udtCCWrapper.m_ullNextACKTime); 

	// Update the current largest sequence number that has been received.
	// Or it is a retransmitted packet, remove it from receiver loss list.
	if (CSeqNo::seqcmp(packet.m_iSeqNo, udtCCWrapper.m_iRcvCurrSeqNo) > 0)
		udtCCWrapper.m_iRcvCurrSeqNo = packet.m_iSeqNo;
	else
		udtCCWrapper.m_pRcvLossList->remove(packet.m_iSeqNo);

	return 0;
}
int CUDT::listen(sockaddr* addr, CPacket& packet)
{
   CGuard cg(m_ConnectionLock);
   if (m_bClosing)
      return 1002;

   CHandShake* hs = (CHandShake *)packet.m_pcData;

   // SYN cookie
   char clienthost[NI_MAXHOST];
   char clientport[NI_MAXSERV];
   getnameinfo(addr, (AF_INET == m_iVersion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6), clienthost, sizeof(clienthost), clientport, sizeof(clientport), NI_NUMERICHOST|NI_NUMERICSERV);
   int64_t timestamp = (CTimer::getTime() - m_StartTime) / 60000000; // secret changes every one minute
   char cookiestr[1024];
   sprintf(cookiestr, "%s:%s:%lld", clienthost, clientport, (long long int)timestamp);
   unsigned char cookie[16];
   CMD5::compute(cookiestr, cookie);

   if (1 == hs->m_iReqType)
   {
      hs->m_iCookie = *(int*)cookie;
      packet.m_iID = hs->m_iID;
      m_pSndQueue->sendto(addr, packet);

      return 0;
   }
   else
   {
      if (hs->m_iCookie != *(int*)cookie)
      {
         timestamp --;
         sprintf(cookiestr, "%s:%s:%lld", clienthost, clientport, (long long int)timestamp);
         CMD5::compute(cookiestr, cookie);

         if (hs->m_iCookie != *(int*)cookie)
            return -1;
      }
   }

   int32_t id = hs->m_iID;

   // When a peer side connects in...
   if ((1 == packet.getFlag()) && (0 == packet.getType()))
   {
      if ((hs->m_iVersion != m_iVersion) || (hs->m_iType != m_iSockType) || (-1 == s_UDTUnited.newConnection(m_SocketID, addr, hs)))
      {
         // couldn't create a new connection, reject the request
         hs->m_iReqType = 1002;
      }

      packet.m_iID = id;

      m_pSndQueue->sendto(addr, packet);
   }

   return hs->m_iReqType;
}
void CUDT::checkTimers()
{
   // update CC parameters
   udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
   udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;
   uint64_t minint = (uint64_t)(udtCCWrapper.m_ullCPUFrequency * udtCCWrapper.m_pSndTimeWindow->getMinPktSndInt() * 0.9);
   if (udtCCWrapper.m_ullInterval < minint)
      udtCCWrapper.m_ullInterval = minint;

   uint64_t currtime;
   CTimer::rdtsc(currtime);
   int32_t loss = udtCCWrapper.m_pRcvLossList->getFirstLostSeq();

   if ((currtime > udtCCWrapper.m_ullNextACKTime) || ((udtCCWrapper.m_pCC->m_iACKInterval > 0) && (udtCCWrapper.m_pCC->m_iACKInterval <= udtCCWrapper.m_iPktCount)))
   {
      // ACK timer expired or ACK interval reached

      sendCtrl(2);
      CTimer::rdtsc(currtime);
      if (udtCCWrapper.m_pCC->m_iACKPeriod > 0)
         udtCCWrapper.m_ullNextACKTime = currtime + udtCCWrapper.m_pCC->m_iACKPeriod * udtCCWrapper.m_ullCPUFrequency;
      else
         udtCCWrapper.m_ullNextACKTime = currtime + udtCCWrapper.m_ullACKInt;

      udtCCWrapper.m_iPktCount = 0;
      udtCCWrapper.m_iLightACKCount = 1;
   }
   else if (UDTCCWrapper::m_iSelfClockInterval * udtCCWrapper.m_iLightACKCount <= udtCCWrapper.m_iPktCount)
   {
      //send a "light" ACK
      sendCtrl(2, NULL, NULL, 4);
      ++ udtCCWrapper.m_iLightACKCount;
   }

   if ((loss >= 0) && (currtime > udtCCWrapper.m_ullNextNAKTime))
   {
      // NAK timer expired, and there is loss to be reported.
      sendCtrl(3);

      CTimer::rdtsc(currtime);
      udtCCWrapper.m_ullNextNAKTime = currtime + udtCCWrapper.m_ullNAKInt;
   }

   if (currtime > udtCCWrapper.m_ullNextEXPTime)
   {
      // Haven't receive any information from the peer, is it dead?!
      // timeout: at least 16 expirations and must be greater than 3 seconds and be less than 30 seconds
      if (((udtCCWrapper.m_iEXPCount > 16) && (udtCCWrapper.m_iEXPCount * ((udtCCWrapper.m_iEXPCount - 1) * (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) / 2 + UDTCCWrapper::m_iSYNInterval) > 3000000))
          || (udtCCWrapper.m_iEXPCount > 30)
		  || (udtCCWrapper.m_iEXPCount * ((udtCCWrapper.m_iEXPCount - 1) * (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) / 2 + UDTCCWrapper::m_iSYNInterval) > 30000000))
      {
         //
         // Connection is broken. 
         // UDT does not signal any information about this instead of to stop quietly.
         // Apllication will detect this when it calls any UDT methods next time.
         //
         m_bClosing = true;
         m_bBroken = true;
         udtCCWrapper.m_iBrokenCounter = 30;

         // update snd U list to remove this socket
         m_pSndQueue->m_pSndUList->update(this);

         releaseSynch();

         CTimer::triggerEvent();

         return;
      }

      // sender: Insert all the packets sent after last received acknowledgement into the sender loss list.
      // recver: Send out a keep-alive packet
      if (CSeqNo::incseq(udtCCWrapper.m_iSndCurrSeqNo) != udtCCWrapper.m_iSndLastAck)
      {
         int32_t csn = udtCCWrapper.m_iSndCurrSeqNo;
         udtCCWrapper.m_pSndLossList->insert(const_cast<int32_t&>(udtCCWrapper.m_iSndLastAck), csn);
      }
      else
         sendCtrl(1);

      if (udtCCWrapper.m_pSndBuffer->getCurrBufSize() > 0)
      {
         // immediately restart transmission
         m_pSndQueue->m_pSndUList->update(this);
      }

      ++ udtCCWrapper.m_iEXPCount;
	  udtCCWrapper.m_ullEXPInt = (udtCCWrapper.m_iEXPCount * (udtCCWrapper.m_iRTT + 4 * udtCCWrapper.m_iRTTVar) + UDTCCWrapper::m_iSYNInterval) * udtCCWrapper.m_ullCPUFrequency;
      if (udtCCWrapper.m_ullEXPInt < udtCCWrapper.m_iEXPCount * 100000 * udtCCWrapper.m_ullCPUFrequency)
         udtCCWrapper.m_ullEXPInt = udtCCWrapper.m_iEXPCount * 100000 * udtCCWrapper.m_ullCPUFrequency;
      CTimer::rdtsc(udtCCWrapper.m_ullNextEXPTime);
      udtCCWrapper.m_ullNextEXPTime += udtCCWrapper.m_ullEXPInt;

      udtCCWrapper.m_pCC->onTimeout();
      // update CC parameters
      udtCCWrapper.m_ullInterval = (uint64_t)(udtCCWrapper.m_pCC->m_dPktSndPeriod * udtCCWrapper.m_ullCPUFrequency);
      udtCCWrapper.m_dCongestionWindow = udtCCWrapper.m_pCC->m_dCWndSize;
   }
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // #ifndef _USE_RAKNET_FLOW_CONTROL
