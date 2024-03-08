#include "RakNetDefines.h"
#ifndef _USE_RAKNET_FLOW_CONTROL

/*****************************************************************************
Copyright (c) 2001 - 2008, The Board of Trustees of the University of Illinois.
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
   Yunhong Gu, last updated 12/28/2008
*****************************************************************************/

#include <cstring>
#include <cmath>
#include "buffer.h"

using namespace std;

CSndBuffer::CSndBuffer(const int& size, const int& mss):
m_pBlock(NULL),
m_pFirstBlock(NULL),
m_pCurrBlock(NULL),
m_pLastBlock(NULL),
m_iNextMsgNo(0),
m_iSize(size),
m_iMSS(mss),
m_iCount(0)
{
   // initial physical buffer of "size"
   // m_pBuffer = new Buffer;
	m_pBuffer = RakNet::OP_NEW<Buffer>(__FILE__, __LINE__);
   // m_pBuffer->m_pcData = new char [m_iSize * m_iMSS];
   m_pBuffer->m_pcData = (char*) rakMalloc_Ex(m_iSize * m_iMSS, __FILE__, __LINE__);
   m_pBuffer->m_iSize = m_iSize;
   m_pBuffer->m_pNext = NULL;

   // circular linked list for out bound packets
   //m_pBlock = new Block;
   m_pBlock = RakNet::OP_NEW<Block>(__FILE__, __LINE__);
   Block* pb = m_pBlock;
   for (int i = 1; i < m_iSize; ++ i)
   {
      //pb->m_pNext = new Block;
	   pb->m_pNext = RakNet::OP_NEW<Block>(__FILE__, __LINE__);
      pb = pb->m_pNext;
   }
   pb->m_pNext = m_pBlock;

   pb = m_pBlock;
   char* pc = m_pBuffer->m_pcData;
   for (int i = 0; i < m_iSize; ++ i)
   {
      pb->m_pcData = pc;
      pb = pb->m_pNext;
      pc += m_iMSS;
   }

   m_pFirstBlock = m_pCurrBlock = m_pLastBlock = m_pBlock;

   #ifndef WIN32
      pthread_mutex_init(&m_BufLock, NULL);
   #else
      m_BufLock = CreateMutex(NULL, false, NULL);
   #endif
}

CSndBuffer::~CSndBuffer()
{
   Block* pb = m_pBlock->m_pNext;
   while (pb != m_pBlock)
   {
      Block* temp = pb;
      pb = pb->m_pNext;
     // delete temp;
	  RakNet::OP_DELETE(temp,__FILE__,__LINE__);
   }
  //  delete m_pBlock;
   RakNet::OP_DELETE(m_pBlock,__FILE__,__LINE__);

   while (m_pBuffer != NULL)
   {
      Buffer* temp = m_pBuffer;
      m_pBuffer = m_pBuffer->m_pNext;
      //delete [] temp->m_pcData;
	  rakFree_Ex(temp->m_pcData,__FILE__,__LINE__);
      //delete temp;
	  RakNet::OP_DELETE(temp,__FILE__,__LINE__);
   }

   #ifndef WIN32
      pthread_mutex_destroy(&m_BufLock);
   #else
      CloseHandle(m_BufLock);
   #endif
}

void CSndBuffer::addBuffer(const char* data, const int& len, const int& ttl, const bool& order)
{
   int size = len / m_iMSS;
   if ((len % m_iMSS) != 0)
      size ++;

   // dynamically increase sender buffer
   while (size + m_iCount >= m_iSize)
      increase();

   uint64_t time = CTimer::getTime();
   int32_t inorder = order;
   inorder <<= 29;

   Block* s = m_pLastBlock;
   for (int i = 0; i < size; ++ i)
   {
      int pktlen = len - i * m_iMSS;
      if (pktlen > m_iMSS)
         pktlen = m_iMSS;

      memcpy(s->m_pcData, data + i * m_iMSS, pktlen);
      s->m_iLength = pktlen;

      s->m_iMsgNo = m_iNextMsgNo | inorder;
      if (i == 0)
         s->m_iMsgNo |= 0x80000000;
      if (i == size - 1)
         s->m_iMsgNo |= 0x40000000;

      s->m_OriginTime = time;
      s->m_iTTL = ttl;

      s = s->m_pNext;
   }
   m_pLastBlock = s;

   CGuard::enterCS(m_BufLock);
   m_iCount += size;
   CGuard::leaveCS(m_BufLock);

   m_iNextMsgNo ++;
}

int CSndBuffer::addBufferFromFile(ifstream& ifs, const int& len)
{
   int size = len / m_iMSS;
   if ((len % m_iMSS) != 0)
      size ++;

   // dynamically increase sender buffer
   while (size + m_iCount >= m_iSize)
      increase();

   Block* s = m_pLastBlock;
   int total = 0;
   for (int i = 0; i < size; ++ i)
   {
      if (ifs.bad() || ifs.fail() || ifs.eof())
         break;

      int pktlen = len - i * m_iMSS;
      if (pktlen > m_iMSS)
         pktlen = m_iMSS;

      ifs.read(s->m_pcData, pktlen);
      if ((pktlen = ifs.gcount()) <= 0)
         break;

      s->m_iLength = pktlen;
      s->m_iTTL = -1;
      s = s->m_pNext;

      total += pktlen;
   }
   m_pLastBlock = s;

   CGuard::enterCS(m_BufLock);
   m_iCount += size;
   CGuard::leaveCS(m_BufLock);

   return total;
}

int CSndBuffer::readData(char** data, int32_t& msgno)
{
   // No data to read
   if (m_pCurrBlock == m_pLastBlock)
      return 0;

   *data = m_pCurrBlock->m_pcData;
   int readlen = m_pCurrBlock->m_iLength;
   msgno = m_pCurrBlock->m_iMsgNo;

   m_pCurrBlock = m_pCurrBlock->m_pNext;

   return readlen;
}

int CSndBuffer::readData(char** data, const int offset, int32_t& msgno, int& msglen)
{
   CGuard bufferguard(m_BufLock);

   Block* p = m_pFirstBlock;

   for (int i = 0; i < offset; ++ i)
      p = p->m_pNext;

   if ((p->m_iTTL > 0) && ((CTimer::getTime() - p->m_OriginTime) / 1000 > (uint64_t)p->m_iTTL))
   {
      msgno = p->m_iMsgNo & 0x1FFFFFFF;

      msglen = 1;
      p = p->m_pNext;
      while (msgno == (p->m_iMsgNo & 0x1FFFFFFF))
      {
         p = p->m_pNext;
         msglen ++;
      }

      return -1;
   }

   *data = p->m_pcData;
   int readlen = p->m_iLength;
   msgno = p->m_iMsgNo;

   return readlen;
}

void CSndBuffer::ackData(const int& offset)
{
   CGuard bufferguard(m_BufLock);

   for (int i = 0; i < offset; ++ i)
      m_pFirstBlock = m_pFirstBlock->m_pNext;

   m_iCount -= offset;

   CTimer::triggerEvent();
}

int CSndBuffer::getCurrBufSize() const
{
   return m_iCount;
}

int CSndBuffer::increase()
{
   int unitsize = m_pBuffer->m_iSize;

   // new physical buffer
   Buffer* nbuf = NULL;
   TRY_CUDTEXCEPTION
   {
      //nbuf  = new Buffer;
	   nbuf  = RakNet::OP_NEW<Buffer>(__FILE__, __LINE__);
      // nbuf->m_pcData = new char [unitsize * m_iMSS];
	   nbuf->m_pcData = (char*) rakMalloc_Ex(unitsize * m_iMSS, __FILE__, __LINE__);

	  if (nbuf->m_pcData==0)
	  {
		  return -1;
	  }
   }
   CATCH_UDT_ELLIPSES
   {
      //delete nbuf;
	   RakNet::OP_DELETE(nbuf,__FILE__,__LINE__);
	  THROW_CUDTEXCEPTION_RET(3, 2, 0, -1);
   }
   nbuf->m_iSize = unitsize;
   nbuf->m_pNext = NULL;

   // insert the buffer at the end of the buffer list
   Buffer* p = m_pBuffer;
   while (NULL != p->m_pNext)
      p = p->m_pNext;
   p->m_pNext = nbuf;

   // new packet blocks
   Block* nblk = NULL;
   TRY_CUDTEXCEPTION
   {
    //  nblk = new Block;
	   nblk = RakNet::OP_NEW<Block>(__FILE__, __LINE__);
   }
   CATCH_UDT_ELLIPSES
   {
    //  delete nblk;
	  RakNet::OP_DELETE(nblk,__FILE__,__LINE__);
	  THROW_CUDTEXCEPTION_RET(3, 2, 0, -1);
   }
   Block* pb = nblk;
   for (int i = 1; i < unitsize; ++ i)
   {
      //pb->m_pNext = new Block;
	   pb->m_pNext = RakNet::OP_NEW<Block>(__FILE__, __LINE__);;
      pb = pb->m_pNext;
   }

   // insert the new blocks onto the existing one
   pb->m_pNext = m_pLastBlock->m_pNext;
   m_pLastBlock->m_pNext = nblk;

   pb = nblk;
   char* pc = nbuf->m_pcData;
   for (int i = 0; i < unitsize; ++ i)
   {
      pb->m_pcData = pc;
      pb = pb->m_pNext;
      pc += m_iMSS;
   }

   m_iSize += unitsize;

   return 0;
}

////////////////////////////////////////////////////////////////////////////////

CRcvBuffer::CRcvBuffer(CUnitQueue* queue):
m_pUnit(NULL),
m_iSize(65536),
m_pUnitQueue(queue),
m_iStartPos(0),
m_iLastAckPos(0),
m_iMaxPos(-1),
m_iNotch(0)
{
   m_pUnit = RakNet::OP_NEW_ARRAY<CUnit*>(m_iSize, __FILE__, __LINE__ );
}

CRcvBuffer::CRcvBuffer(const int& bufsize, CUnitQueue* queue):
m_pUnit(NULL),
m_iSize(bufsize),
m_pUnitQueue(queue),
m_iStartPos(0),
m_iLastAckPos(0),
m_iMaxPos(-1),
m_iNotch(0)
{
	//m_pUnit = new CUnit* [m_iSize];
   m_pUnit = RakNet::OP_NEW_ARRAY<CUnit*>(m_iSize, __FILE__, __LINE__ );
   for (int i = 0; i < m_iSize; ++ i)
      m_pUnit[i] = NULL;
}

CRcvBuffer::~CRcvBuffer()
{
   for (int i = 0; i < m_iSize; ++ i)
   {
      if (NULL != m_pUnit[i])
      {
         m_pUnit[i]->m_iFlag = 0;
         -- m_pUnitQueue->m_iCount;
      }
   }

   //delete [] m_pUnit;
   RakNet::OP_DELETE_ARRAY(m_pUnit,__FILE__, __LINE__);
}

int CRcvBuffer::addData(CUnit* unit, int offset)
{
   int pos = (m_iLastAckPos + offset) % m_iSize;
   if (offset > m_iMaxPos)
      m_iMaxPos = offset;

   if (NULL != m_pUnit[pos])
      return -1;
   
   m_pUnit[pos] = unit;

   unit->m_iFlag = 1;
   ++ m_pUnitQueue->m_iCount;

   return 0;
}

int CRcvBuffer::readBuffer(char* data, const int& len)
{
   int p = m_iStartPos;
   int lastack = m_iLastAckPos;
   int rs = len;

   while ((p != lastack) && (rs > 0))
   {
      int unitsize = m_pUnit[p]->m_Packet.getLength() - m_iNotch;
      if (unitsize > rs)
         unitsize = rs;

      memcpy(data, m_pUnit[p]->m_Packet.m_pcData + m_iNotch, unitsize);
      data += unitsize;

      if ((rs > unitsize) || (rs == m_pUnit[p]->m_Packet.getLength() - m_iNotch))
      {
         CUnit* tmp = m_pUnit[p];
         m_pUnit[p] = NULL;
         tmp->m_iFlag = 0;
         -- m_pUnitQueue->m_iCount;

         if (++ p == m_iSize)
            p = 0;

         m_iNotch = 0;
      }
      else
         m_iNotch += rs;

      rs -= unitsize;
   }

   m_iStartPos = p;
   return len - rs;
}

int CRcvBuffer::readBufferToFile(ofstream& file, const int& len)
{
   int p = m_iStartPos;
   int lastack = m_iLastAckPos;
   int rs = len;

   while ((p != lastack) && (rs > 0))
   {
      int unitsize = m_pUnit[p]->m_Packet.getLength() - m_iNotch;
      if (unitsize > rs)
         unitsize = rs;

      file.write(m_pUnit[p]->m_Packet.m_pcData + m_iNotch, unitsize);

      if ((rs > unitsize) || (rs == m_pUnit[p]->m_Packet.getLength() - m_iNotch))
      {
         CUnit* tmp = m_pUnit[p];
         m_pUnit[p] = NULL;
         tmp->m_iFlag = 0;
         -- m_pUnitQueue->m_iCount;

         if (++ p == m_iSize)
            p = 0;

         m_iNotch = 0;
      }
      else
         m_iNotch += rs;

      rs -= unitsize;
   }

   m_iStartPos = p;
   return len - rs;
}

void CRcvBuffer::ackData(const int& len)
{
   m_iLastAckPos = (m_iLastAckPos + len) % m_iSize;
   m_iMaxPos -= len;

   CTimer::triggerEvent();
}

int CRcvBuffer::getAvailBufSize() const
{
   // One slot must be empty in order to tell the different between "empty buffer" and "full buffer"
   return m_iSize - getRcvDataSize() - 1;
}

int CRcvBuffer::getRcvDataSize() const
{
   if (m_iLastAckPos >= m_iStartPos)
      return m_iLastAckPos - m_iStartPos;

   return m_iSize + m_iLastAckPos - m_iStartPos;
}

void CRcvBuffer::dropMsg(const int32_t& msgno)
{
   for (int i = m_iStartPos, n = (m_iLastAckPos + m_iMaxPos) % m_iSize; i != n; i = (i + 1) % m_iSize)
      if ((NULL != m_pUnit[i]) && (msgno == m_pUnit[i]->m_Packet.m_iMsgNo))
         m_pUnit[i]->m_iFlag = 3;
}

int CRcvBuffer::readMsg(char* data, const int& len)
{
   int p, q;
   bool passack;
   if (!scanMsg(p, q, passack))
      return 0;

   int rs = len;
   while (p != (q + 1) % m_iSize)
   {
      int unitsize = m_pUnit[p]->m_Packet.getLength();
      if ((rs >= 0) && (unitsize > rs))
         unitsize = rs;

      if (unitsize > 0)
      {
         memcpy(data, m_pUnit[p]->m_Packet.m_pcData, unitsize);
         data += unitsize;
         rs -= unitsize;
      }

      if (!passack)
      {
         CUnit* tmp = m_pUnit[p];
         m_pUnit[p] = NULL;
         tmp->m_iFlag = 0;
         -- m_pUnitQueue->m_iCount;
      }
      else
         m_pUnit[p]->m_iFlag = 2;

      if (++ p == m_iSize)
         p = 0;
   }

   if (!passack)
      m_iStartPos = (q + 1) % m_iSize;

   return len - rs;
}

int CRcvBuffer::getRcvMsgNum()
{
   int p, q;
   bool passack;
   return scanMsg(p, q, passack) ? 1 : 0;
}

bool CRcvBuffer::scanMsg(int& p, int& q, bool& passack)
{
   // empty buffer
   if ((m_iStartPos == m_iLastAckPos) && (0 == m_iMaxPos))
      return false;

   //skip all bad msgs at the beginning
   while (m_iStartPos != m_iLastAckPos)
   {
      if ((1 == m_pUnit[m_iStartPos]->m_iFlag) && (m_pUnit[m_iStartPos]->m_Packet.getMsgBoundary() > 1))
         break;

      CUnit* tmp = m_pUnit[m_iStartPos];
      m_pUnit[m_iStartPos] = NULL;
      tmp->m_iFlag = 0;
      -- m_pUnitQueue->m_iCount;

      if (++ m_iStartPos == m_iSize)
         m_iStartPos = 0;
   }

   p = -1;                  // message head
   q = m_iStartPos;         // message tail
   passack = m_iStartPos == m_iLastAckPos;
   bool found = false;

   // looking for the first message
   for (int i = 0, n = m_iMaxPos + getRcvDataSize(); i <= n; ++ i)
   {
      if ((NULL != m_pUnit[q]) && (1 == m_pUnit[q]->m_iFlag))
      {
         switch (m_pUnit[q]->m_Packet.getMsgBoundary())
         {
         case 3: // 11
            p = q;
            found = true;
            break;

         case 2: // 10
            p = q;
            break;

         case 1: // 01
            if (p != -1)
               found = true;
         }
      }
      else
      {
         // a hole in this message, not valid, restart search
         p = -1;
      }

      if (found)
      {
         // the msg has to be ack'ed or it is allowed to read out of order, and was not read before
         if (!passack || !m_pUnit[q]->m_Packet.getMsgOrderFlag())
            break;

         found = false;
      }

      if (++ q == m_iSize)
         q = 0;

      if (q == m_iLastAckPos)
         passack = true;
   }

   // no msg found
   if (!found)
   {
      // if the message is larger than the receiver buffer, return part of the message
      if ((p != -1) && ((q + 1) % m_iSize == p))
         found = true;
   }

   return found;
}

#endif // #ifndef _USE_RAKNET_FLOW_CONTROL
