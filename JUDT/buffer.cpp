/*****************************************************************************
written by
   Yunhong Gu, last updated 05/05/2009
*****************************************************************************/

#include <cstring>
#include <cmath>
#include "buffer.h"

using namespace std;

CSndBuffer::CSndBuffer(const int& size, const int& mss):
m_BufLock(),
m_pBlock(NULL),
m_pFirstBlock(NULL),
m_pCurrBlock(NULL),
m_pLastBlock(NULL),
m_pBuffer(NULL),
m_iNextMsgNo(1),
m_iSize(size),
m_iMSS(mss),
m_iCount(0)
{
   // initial physical buffer of "size"
   m_pBuffer = new Buffer;
   m_pBuffer->m_pcData = new char [m_iSize * m_iMSS];
#ifndef WIN32
   m_pBuffer->m_pBufBlock = new Block [m_iSize];
#endif
   m_pBuffer->m_iSize = m_iSize;
   m_pBuffer->m_pNext = NULL;

#ifdef WIN32//windows下仍采用小块分配
   // circular linked list for out bound packets
   m_pBlock = new Block;
   Block* pb = m_pBlock;
   int i = 1;
   for (i = 1; i < m_iSize; ++ i)
   {
      pb->m_pNext = new Block;
	  pb->m_iMsgNo = 0;
      pb = pb->m_pNext;
	  pb->m_unRegion = 0;
	  pb->m_iMsgCount = 0;
   }
   pb->m_pNext = m_pBlock;

   pb = m_pBlock;
   char* pc = m_pBuffer->m_pcData;
   for (i = 0; i < m_iSize; ++ i)
   {
      pb->m_pcData = pc;
      pb = pb->m_pNext;
      pc += m_iMSS;
   }

   m_pFirstBlock = m_pCurrBlock = m_pLastBlock = m_pBlock;

   m_BufLock = CreateMutex(NULL, false, NULL);
#else//linux下为减少new次数，集中分配
   Block* pb = m_pBuffer->m_pBufBlock;
   char* pc = m_pBuffer->m_pcData;
   int i;
   for (i = 0; i < m_iSize-1; ++ i)
   {
	   pb->m_pcData = pc;
	   pb->m_iMsgNo = 0;
	   pb->m_unRegion = 0;
	   pb->m_iMsgCount = 0;
	   pb->m_pNext = &m_pBuffer->m_pBufBlock[i+1];
	   pb = pb->m_pNext;
	   pc += m_iMSS;
   }
   pb->m_pcData = pc;
   pb->m_iMsgNo = 0;
   pb->m_unRegion = 0;
   pb->m_iMsgCount = 0;
   pb->m_pNext = m_pBuffer->m_pBufBlock;
   
   m_pFirstBlock = m_pCurrBlock = m_pLastBlock = m_pBlock = m_pBuffer->m_pBufBlock;

   pthread_mutex_init(&m_BufLock, NULL);
#endif
}

CSndBuffer::~CSndBuffer()
{
   #ifndef WIN32
	  while (m_pBuffer != NULL)
	  {
		  Buffer* temp = m_pBuffer;
		  m_pBuffer = m_pBuffer->m_pNext;
		  delete[] temp->m_pBufBlock;
		  temp->m_pBufBlock = NULL;
		  delete[] temp->m_pcData;
		  temp->m_pcData = NULL;
		  delete temp;
		  temp = NULL;
	  }

      pthread_mutex_destroy(&m_BufLock);
   #else
	  Block* pb = m_pBlock->m_pNext;
	  while (pb != m_pBlock)
	  {
		  Block* temp = pb;
		  pb = pb->m_pNext;
		  delete temp;
		  temp = NULL;
	  }
	  if(m_pBlock != NULL)
	  {
		  delete m_pBlock;
		  m_pBlock = NULL;
	  }
	  
	  while (m_pBuffer != NULL)
	  {
		  Buffer* temp = m_pBuffer;
		  m_pBuffer = m_pBuffer->m_pNext;
		  delete[] temp->m_pcData;
		  temp->m_pcData = NULL;
		  delete temp;
		  temp = NULL;
	  }
	  
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
	  s->m_unRegion = 0;
	  s->m_iMsgCount = 0;

      s = s->m_pNext;
   }
   m_pLastBlock = s;

   CGuard::enterCS(m_BufLock);
   m_iCount += size;
   CGuard::leaveCS(m_BufLock);

   m_iNextMsgNo ++;
   if (m_iNextMsgNo == CMsgNo::m_iMaxMsgNo)
   {
	   m_iNextMsgNo = 1;
   }
}

void CSndBuffer::addBuffer(const char* data, const int& len, int nttl, bool border, unsigned int unregion)//unregion 还没用
{
	int size = len / m_iMSS;
	if ((len % m_iMSS) != 0)
		size ++;
	
	// dynamically increase sender buffer
	while (size + m_iCount >= m_iSize)
		increase();
	
	uint64_t time = CTimer::getTime();
	int32_t inorder = border?1:0;
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

		if(i != 0 && i!= size-1)
		{
			s->m_iMsgNo &= 0x1FFFFFFF;
		}
		
		s->m_OriginTime = time;
		s->m_iTTL = nttl;
		s->m_unRegion = unregion;
		s->m_iMsgCount = size;
		s = s->m_pNext;
	}
	m_pLastBlock = s;
	
	CGuard::enterCS(m_BufLock);
	m_iCount += size;
	CGuard::leaveCS(m_BufLock);
	
	m_iNextMsgNo ++;
	if (m_iNextMsgNo == CMsgNo::m_iMaxMsgNo)
	{
		m_iNextMsgNo = 1;
	}
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

   if ((p->m_iTTL > 0) && ((CTimer::getTime() - p->m_OriginTime) / 1000 > (uint64_t)p->m_iTTL))//p->m_iTTL >= 0
   {
      msgno = p->m_iMsgNo & 0x1FFFFFFF;
	  unsigned int unregion = p->m_unRegion;

      msglen = 1;
      p = p->m_pNext;
      bool move = false;
      while (msgno == (p->m_iMsgNo & 0x1FFFFFFF) || ( p->m_unRegion > 0 && p->m_unRegion == unregion && msglen <= (m_iSize-offset)))
//	  while (msgno == (p->m_iMsgNo & 0x1FFFFFFF))
      {
		  if (p == m_pCurrBlock)
			  move = true;
		  p = p->m_pNext;
		  if (move)
			  m_pCurrBlock = p;
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

void CSndBuffer::increase()
{
   int unitsize = m_pBuffer->m_iSize;

   // new physical buffer
   Buffer* nbuf = NULL;
   try
   {
      nbuf  = new Buffer;
      nbuf->m_pcData = new char [unitsize * m_iMSS];
#ifndef WIN32
      nbuf->m_pBufBlock = new Block [m_iSize];
#endif
   }
   catch (...)
   {
	   if(nbuf->m_pcData != NULL)
	   {
		   delete[] nbuf->m_pcData;
		   nbuf->m_pcData = NULL;
	   }
#ifndef WIN32
	   if(nbuf->m_pBufBlock != NULL)
	   {
		   delete[] nbuf->m_pBufBlock;
		   nbuf->m_pBufBlock = NULL;
	   }
#endif
      delete nbuf;
	  nbuf = NULL;
      throw CUDTException(3, 2, 0);
   }
   nbuf->m_iSize = unitsize;
   nbuf->m_pNext = NULL;

   // insert the buffer at the end of the buffer list
   Buffer* p = m_pBuffer;
   while (NULL != p->m_pNext)
      p = p->m_pNext;
   p->m_pNext = nbuf;

#ifdef WIN32
   // new packet blocks
   Block* nblk = NULL;
   try
   {
      nblk = new Block;
   }
   catch (...)
   {
	   if(nbuf->m_pcData != NULL)
	   {
		   delete[] nbuf->m_pcData;
		   nbuf->m_pcData = NULL;
	   }

	   delete nbuf;
	   nbuf = NULL;
	   
      delete nblk;
	  nblk = NULL;
      throw CUDTException(3, 2, 0);
   }
   Block* pb = nblk;
   int i = 1;
   for (i = 1; i < unitsize; ++ i)
   {
      pb->m_pNext = new Block;
      pb = pb->m_pNext;
   }

   // insert the new blocks onto the existing one
   pb->m_pNext = m_pLastBlock->m_pNext;
   m_pLastBlock->m_pNext = nblk;

   pb = nblk;
   char* pc = nbuf->m_pcData;
   for (i = 0; i < unitsize; ++ i)
   {
      pb->m_pcData = pc;
      pb = pb->m_pNext;
      pc += m_iMSS;
   }
#else
   //初始化Block数组
   Block* pb = nbuf->m_pBufBlock;
   char* pc = nbuf->m_pcData;
   int i;
   for (i = 0; i < unitsize-1; ++ i)
   {
	   pb->m_pcData = pc;
	   pb->m_iMsgNo = 0;
	   pb->m_unRegion = 0;
	   pb->m_iMsgCount = 0;
	   pb->m_pNext = &nbuf->m_pBufBlock[i+1];
	   pb = pb->m_pNext;
	   pc += m_iMSS;
   }
   pb->m_pcData = pc;
   pb->m_iMsgNo = 0;
   pb->m_unRegion = 0;
   pb->m_iMsgCount = 0;
   
   //Block数组加入队列
   pb->m_pNext = m_pLastBlock->m_pNext;
   m_pLastBlock->m_pNext = nbuf->m_pBufBlock;
#endif
   m_iSize += unitsize;
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
   m_pUnit = new CUnit* [m_iSize];
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
   m_pUnit = new CUnit* [m_iSize];
   for (int i = 0; i < m_iSize; ++ i)
   {
	   m_pUnit[i] = NULL;
   }
}

CRcvBuffer::~CRcvBuffer()
{
   for (int i = 0; i < m_iSize; ++ i)
   {
      if (NULL != m_pUnit[i])
      {
         m_pUnit[i]->m_iFlag = 0; //???
         -- m_pUnitQueue->m_iCount;
      }
   }

   delete[] m_pUnit;
   m_pUnit = NULL;
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

void CRcvBuffer::ackData(const int& len)
{
   m_iLastAckPos = (m_iLastAckPos + len) % m_iSize;
   m_iMaxPos -= len;
   if (m_iMaxPos < 0)
	   m_iMaxPos = 0;

   CTimer::triggerEvent();
}

int CRcvBuffer::getAvailBufSize() const
{
   // One slot must be empty in order to tell the difference between "empty buffer" and "full buffer"
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
   {
      if ((NULL != m_pUnit[i]) && (msgno == m_pUnit[i]->m_Packet.m_iMsgNo))
	  {
         m_pUnit[i]->m_iFlag = 3;
	  }
   }
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
		
		if (unitsize > 0 && rs > 0)
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
   if ((m_iStartPos == m_iLastAckPos) && (0 >= m_iMaxPos))
      return false;

   //skip all bad msgs at the beginning
   while (m_iStartPos != m_iLastAckPos)
   {
      if (NULL == m_pUnit[m_iStartPos])
      {
         if (++ m_iStartPos == m_iSize)
            m_iStartPos = 0;
         continue;
      }

      if ((1 == m_pUnit[m_iStartPos]->m_iFlag) && (m_pUnit[m_iStartPos]->m_Packet.getMsgBoundary() > 1))
	  {
		  bool good = true;
		  
		  // look ahead for the whole message
		  for (int i = m_iStartPos; i != m_iLastAckPos;)
		  {
			  if ((NULL == m_pUnit[i]) || (1 != m_pUnit[i]->m_iFlag))
			  {
				  good = false;
				  break;
			  }
			  //wm 中间夹杂了其他数据，丢弃
			  if(m_pUnit[m_iStartPos]->m_Packet.getMsgSeq() != m_pUnit[i]->m_Packet.getMsgSeq())
			  {
				  good = false;
				  break;
			  }
			   
			  if ((m_pUnit[i]->m_Packet.getMsgBoundary() == 1) || (m_pUnit[i]->m_Packet.getMsgBoundary() == 3))
				  break;
			  
			  if (++ i == m_iSize)
				  i = 0;
		  }
		  
		  if (good)
			  break;
      }

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
