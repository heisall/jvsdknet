/*****************************************************************************
written by
   Yunhong Gu, last updated 05/05/2009
*****************************************************************************/

#ifdef WIN32
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #ifdef LEGACY_WIN32
      #include <wspiapi.h>
   #endif
#endif

#include <cstring>
#include "common.h"
#include "queue.h"
#include "core.h"

using namespace std;

CUnitQueue::CUnitQueue():
m_pQEntry(NULL),
m_pCurrQueue(NULL),
m_pLastQueue(NULL),
m_iSize(0),
m_iCount(0),
m_iMSS(),
m_iIPversion(),
m_iMAXRBUF(32)//256
{
}

CUnitQueue::~CUnitQueue()
{
   CQEntry* p = m_pQEntry;

   while (p != NULL)
   {
	   if(p->m_pUnit != NULL)
	   {
		   delete[] p->m_pUnit;
		   p->m_pUnit = NULL;
	   }
	   if(p->m_pBuffer != NULL)
	   {
		   delete[] p->m_pBuffer;
		   p->m_pBuffer = NULL;
	   }
	   
	   CQEntry* q = p;
	   if (p == m_pLastQueue)
	   {
		   p = NULL;
	   }
	   else
	   {
		   p = p->m_pNext;
	   }
	   if(q != NULL)
	   {
		   delete q;
		   q = NULL;
	   }
   }
}

int CUnitQueue::init(const int& size, const int& mss, const int& version)
{
   CQEntry* tempq = NULL;
   CUnit* tempu = NULL;
   char* tempb = NULL;

   try
   {
      tempq = new CQEntry;
      tempu = new CUnit [size];
      tempb = new char [size * mss];
   }
   catch (...)
   {
      delete tempq;
	  tempq = NULL;
      delete[] tempu;
	  tempu = NULL;
      delete[] tempb;
	  tempb = NULL;

      return -1;
   }

   for (int i = 0; i < size; ++ i)
   {
      tempu[i].m_iFlag = 0;
      tempu[i].m_Packet.m_pcData = tempb + i * mss;
   }
   tempq->m_pUnit = tempu;
   tempq->m_pBuffer = tempb;
   tempq->m_iSize = size;

   m_pQEntry = m_pCurrQueue = m_pLastQueue = tempq;
   m_pQEntry->m_pNext = m_pQEntry;

   m_pAvailUnit = m_pCurrQueue->m_pUnit;

   m_iSize = size;
   m_iMSS = mss;
   m_iIPversion = version;

   return 0;
}

int CUnitQueue::increase()
{
//#ifndef WIN32
//	if(m_iSize >= m_iMAXRBUF)
//	{
//		return -1;
//	}
//#endif
   // adjust/correct m_iCount
   int real_count = 0;
   CQEntry* p = m_pQEntry;
   while (p != NULL)
   {
      CUnit* u = p->m_pUnit;
      for (CUnit* end = u + p->m_iSize; u != end; ++ u)
         if (u->m_iFlag != 0)
            ++ real_count;

      if (p == m_pLastQueue)
         p = NULL;
      else
         p = p->m_pNext;
   }
   m_iCount = real_count;
   if (double(m_iCount) / m_iSize < 0.9)
      return -1;

   CQEntry* tempq = NULL;
   CUnit* tempu = NULL;
   char* tempb = NULL;

   // all queues have the same size
   int size = m_pQEntry->m_iSize;

   try
   {
      tempq = new CQEntry;
      tempu = new CUnit [size];
      tempb = new char [size * m_iMSS];
   }
   catch (...)
   {
      delete tempq;
	  tempq = NULL;
      delete[] tempu;
	  tempu = NULL;
      delete[] tempb;
	  tempb = NULL;

      return -1;
   }

   for (int i = 0; i < size; ++ i)
   {
      tempu[i].m_iFlag = 0;
      tempu[i].m_Packet.m_pcData = tempb + i * m_iMSS;
   }
   tempq->m_pUnit = tempu;
   tempq->m_pBuffer = tempb;
   tempq->m_iSize = size;

   m_pLastQueue->m_pNext = tempq;
   m_pLastQueue = tempq;
   m_pLastQueue->m_pNext = m_pQEntry;

   m_iSize += size;

   return 0;
}

int CUnitQueue::shrink()
{
	CQEntry* p = m_pQEntry->m_pNext;
	if(p == m_pQEntry)
	{
		return 0;
	}
	
	//判断运行时申请的CQEntry是否被占用
	p = m_pQEntry;
	int flag=0;
	while (p != NULL)
	{
		CUnit* end;
		CUnit* u = p->m_pUnit;
		//flag=0;
		for (end = u + p->m_iSize; u != end; ++ u)
		{
			if (u->m_iFlag != 0)
			{
				flag=1;
				break;
			}
		}
		
		if (p == m_pLastQueue)
		{
			break;
		}
		else
		{
			p = p->m_pNext;
		}
	}
	if(flag)
	{
		//printf("shrink fail\n");
		return 0;
	}
	
	//运行时申请的CQEntry空闲，可以释放
	//Printf("shrink\n");
	p = m_pQEntry->m_pNext;
	while (p != NULL)
	{
		if(p->m_pUnit != NULL)
		{
			delete[] p->m_pUnit;
			p->m_pUnit = NULL;
		}
		if(p->m_pBuffer != NULL)
		{
			delete[] p->m_pBuffer;
			p->m_pBuffer = NULL;
		}
		
		CQEntry* q = p;
		if (p == m_pLastQueue)
		{
			p = NULL;
		}
		else
		{
			p = p->m_pNext;
		}
		if(q != NULL)
		{
			delete q;
			q = NULL;
		}
	}
	
	//重置
	//printf("shrink,before reset\n");
	m_pCurrQueue = m_pLastQueue = m_pQEntry;
	m_pQEntry->m_pNext = m_pQEntry;
	m_pAvailUnit = m_pQEntry->m_pUnit;
	m_iSize = m_pQEntry->m_iSize;
	m_iCount = 0;
	
	//printf("shrink,ok\n");
	return 0;
   // currently queue cannot be shrunk.
//   return -1;
}
int CUnitQueue::setmaxrecvbuf(int nmax)
{
	if(nmax == 32)//256
	{//复位值
		m_iMAXRBUF = nmax;
	}
	else if(nmax > m_iMAXRBUF)
	{//非复位值 则以最大为有效
		m_iMAXRBUF = nmax;
	}

	return m_iMAXRBUF;
}

CUnit* CUnitQueue::getNextAvailUnit()
{
   if (m_iCount * 10 > m_iSize * 9)
      increase();

   if (m_iCount >= m_iSize)
      return NULL;

   CQEntry* entrance = m_pCurrQueue;

   do
   {
      for (CUnit* sentinel = m_pCurrQueue->m_pUnit + m_pCurrQueue->m_iSize - 1; m_pAvailUnit != sentinel; ++ m_pAvailUnit)
         if (m_pAvailUnit->m_iFlag == 0)
            return m_pAvailUnit;

      if (m_pCurrQueue->m_pUnit->m_iFlag == 0)
      {
         m_pAvailUnit = m_pCurrQueue->m_pUnit;
         return m_pAvailUnit;
      }

      m_pCurrQueue = m_pCurrQueue->m_pNext;
      m_pAvailUnit = m_pCurrQueue->m_pUnit;
   } while (m_pCurrQueue != entrance);

   increase();

   return NULL;
}


//dvr中使用原结构理论上更周密，但实际性能不太理想
//dvr中使用新的环形结构，简单高效，但对于时间排序的处理有明显的缺陷，导致cc中的码流控制不再起作用，
//但实测并未发现问题,考虑潜在风险，暂只应用于dvr部分
#ifdef WIN32
CSndUList::CSndUList():
m_pHeap(NULL),
m_iArrayLength(4096),
m_iLastEntry(-1),
m_ListLock(),
m_pWindowLock(NULL),
m_pWindowCond(NULL),
m_pTimer(NULL),
m_bWaitCont(0)
{
   m_pHeap = new CSNode*[m_iArrayLength];

   #ifndef WIN32
      pthread_mutex_init(&m_ListLock, NULL);
   #else
      m_ListLock = CreateMutex(NULL, false, NULL);
   #endif
}

CSndUList::~CSndUList()
{
	if(m_pHeap != NULL)
	{
		delete[] m_pHeap;
		m_pHeap = NULL;
	}

   #ifndef WIN32
      pthread_mutex_destroy(&m_ListLock);
   #else
      CloseHandle(m_ListLock);
   #endif
}

//该函数没有调用 因此此处不会自动增加队列
void CSndUList::insert(const int64_t& ts, const CUDT* u)
{
   CGuard listguard(m_ListLock);

   // increase the heap array size if necessary
   if (m_iLastEntry == m_iArrayLength - 1)
   {
      CSNode** temp = NULL;

      try
      {
         temp = new CSNode*[m_iArrayLength * 2];
      }
      catch(...)
      {
         return;
      }

      memcpy(temp, m_pHeap, sizeof(CSNode*) * m_iArrayLength);
      m_iArrayLength *= 2;
      delete[] m_pHeap;
      m_pHeap = temp;
   }

   insert_(ts, u);
}

void CSndUList::update(const CUDT* u, const bool& reschedule)
{
   CGuard listguard(m_ListLock);

   CSNode* n = u->m_pSNode;

   if (n->m_iHeapLoc >= 0)
   {
      if (!reschedule)
         return;

      if (n->m_iHeapLoc == 0)
      {
         n->m_llTimeStamp = 1;
         m_pTimer->interrupt();
         return;
      }

      remove_(u);
   }

   insert_(1, u);
}

int CSndUList::pop(sockaddr*& addr, CPacket& pkt, sockaddr*& realaddr)
{
   //CGuard listguard(m_ListLock);
	CGuard::enterCS(m_ListLock);
   if (-1 == m_iLastEntry)
   {
	   CGuard::leaveCS(m_ListLock);
	   return -1;
   }
   CUDT* u = m_pHeap[0]->m_pUDT;
   remove_(u);

   if (!u->m_bConnected || u->m_bBroken)
   {
	   CGuard::leaveCS(m_ListLock);
       return -1;
   }
   CGuard::leaveCS(m_ListLock);

   // pack a packet from the socket
   uint64_t ts;
   if (u->packData(pkt, ts) <= 0)
      return -1;

   CGuard::enterCS(m_ListLock);
   addr = u->m_pPeerAddr;
   realaddr = u->m_pRealPeerAddr;
   // insert a new entry, ts is the next processing time
   if (ts > 0)
      insert_(ts, u);

   CGuard::leaveCS(m_ListLock);
   return 1;
}

void CSndUList::remove(const CUDT* u)
{
   CGuard listguard(m_ListLock);

   remove_(u);
}

uint64_t CSndUList::getNextProcTime()
{
   CGuard listguard(m_ListLock);

   if (-1 == m_iLastEntry)
      return 0;

   return m_pHeap[0]->m_llTimeStamp;
}

void CSndUList::insert_(const int64_t& ts, const CUDT* u)
{
   CSNode* n = u->m_pSNode;

   // do not insert repeated node
   if (n->m_iHeapLoc >= 0)
      return;

   m_iLastEntry ++;
   m_pHeap[m_iLastEntry] = n;
   n->m_llTimeStamp = ts;

   int q = m_iLastEntry;
   int p = q;
   while (p != 0)
   {
      p = (q - 1) >> 1;
      if (m_pHeap[p]->m_llTimeStamp > m_pHeap[q]->m_llTimeStamp)
      {
         CSNode* t = m_pHeap[p];
         m_pHeap[p] = m_pHeap[q];
         m_pHeap[q] = t;
         t->m_iHeapLoc = q;
         q = p;
      }
      else
         break;
   }

   n->m_iHeapLoc = q;

   // first entry, activate the sending queue
   if (0 == m_iLastEntry || (m_iLastEntry > 0 && m_bWaitCont == 1))
   {
	     m_bWaitCont = 0;
      #ifndef WIN32
         pthread_mutex_lock(m_pWindowLock);
         pthread_cond_signal(m_pWindowCond);
         pthread_mutex_unlock(m_pWindowLock);
      #else
         SetEvent(*m_pWindowCond);
      #endif
   }
}

void CSndUList::remove_(const CUDT* u)
{
   CSNode* n = u->m_pSNode;

   if (n->m_iHeapLoc >= 0)
   {
      // remove the node from heap
      m_pHeap[n->m_iHeapLoc] = m_pHeap[m_iLastEntry];
      m_iLastEntry --;
      m_pHeap[n->m_iHeapLoc]->m_iHeapLoc = n->m_iHeapLoc;

      int q = n->m_iHeapLoc;
      int p = q * 2 + 1;
      while (p <= m_iLastEntry)
      {
         if ((p + 1 <= m_iLastEntry) && (m_pHeap[p]->m_llTimeStamp > m_pHeap[p + 1]->m_llTimeStamp))
            p ++;

         if (m_pHeap[q]->m_llTimeStamp > m_pHeap[p]->m_llTimeStamp)
         {
            CSNode* t = m_pHeap[p];
            m_pHeap[p] = m_pHeap[q];
            m_pHeap[p]->m_iHeapLoc = p;
            m_pHeap[q] = t;
            m_pHeap[q]->m_iHeapLoc = q;

            q = p;
            p = q * 2 + 1;
         }
         else
            break;
      }

      n->m_iHeapLoc = -1;
   }
}
#else//////////////////////////////////////////////////////////////////////////
CSndUList::CSndUList():
m_pHeap(NULL),
m_iArrayLength(4096),
m_iLastEntry(-1),
m_iRead(0),
m_iWrite(0),
m_pWindowLock(NULL),
m_pWindowCond(NULL),
m_pTimer(NULL),
m_bWaitCont(0)
{
   m_pHeap = new CSNode*[m_iArrayLength];

   #ifndef WIN32
       pthread_mutex_init(&m_ReadLock, NULL);
      pthread_mutex_init(&m_WriteLock, NULL);
   #else
      m_ReadLock = CreateMutex(NULL, false, NULL);
      m_WriteLock = CreateMutex(NULL, false, NULL);
   #endif
}

CSndUList::~CSndUList()
{
	if(m_pHeap != NULL)
	{
		delete[] m_pHeap;
		m_pHeap = NULL;
	}

   #ifndef WIN32
      pthread_mutex_destroy(&m_ReadLock);
      pthread_mutex_destroy(&m_WriteLock);
   #else
      CloseHandle(m_ReadLock);
      CloseHandle(m_WriteLock);
   #endif
}

void CSndUList::insert(const int64_t& ts, const CUDT* u)
{
}

void CSndUList::update(const CUDT* u, const bool& reschedule, const int64_t& ts)
{
	CSNode* n = u->m_pSNode;

	//printf("update,m_iHeapLoc=%d,reschedule=%d,ts=%x\n",n->m_iHeapLoc,reschedule, (long)ts);

	//检测合法
	/*if( ULIST_EMPTY(m_iRead, m_iWrite) ||
		(m_iRead < m_iWrite && (n->m_iHeapLoc < m_iRead || n->m_iHeapLoc >= m_iWrite)) ||
		(m_iRead > m_iWrite && (n->m_iHeapLoc < m_iRead && n->m_iHeapLoc >= m_iWrite))   )
	{
		//printf("Update err,n->m_iHeapLoc=%d,m_iRead=%d,m_iWrite=%d\n",n->m_iHeapLoc,m_iRead,m_iWrite);
		pthread_mutex_lock(&m_ReadLock);
		n->m_iHeapLoc = -1;
		pthread_mutex_unlock(&m_ReadLock);
	}*/
	
	//在发送队列上
	if (n->m_iHeapLoc >= 0)
	{
		if (!reschedule)
		{
			try_wakeup_worker_();
			return;
		}
		
		if (n->m_iHeapLoc == m_iRead)
		{
			pthread_mutex_lock(&m_ReadLock);
			n->m_llTimeStamp = 1;
			pthread_mutex_unlock(&m_ReadLock);
			m_pTimer->interrupt();
			//printf("update, send now,m_iArrayLength=%d\n",m_iArrayLength);
			return;
		}

		pthread_mutex_lock(&m_ReadLock);
		pthread_mutex_lock(&m_WriteLock);
		remove_(u);
		add_head_(1, u);
		pthread_mutex_unlock(&m_WriteLock);
		pthread_mutex_unlock(&m_ReadLock);
		return;
	}

	//不在发送队列上
   if(!reschedule && ts!=0)
   {
   	  //printf("update,add_tail_\n");
	  pthread_mutex_lock(&m_WriteLock);
      add_tail_(ts, u);
	  pthread_mutex_unlock(&m_WriteLock);
   }
   else
   {
   	  //printf("update,add_head_\n");
	  pthread_mutex_lock(&m_ReadLock);
	  pthread_mutex_lock(&m_WriteLock);
      add_head_(1, u);
	  pthread_mutex_unlock(&m_WriteLock);
	  pthread_mutex_unlock(&m_ReadLock);
   }

}
int CSndUList::pop(sockaddr*& addr, CPacket& pkt, sockaddr*& realaddr)
{
   //printf("pop,in\n");
   
   //popup from the list
   pthread_mutex_lock(&m_ReadLock);
   if ( ULIST_EMPTY(m_iRead, m_iWrite) )
   {
//   	  Printf("pop,list empty\n");
      pthread_mutex_unlock(&m_ReadLock);
      return -1;
   }
   
   CUDT* u = m_pHeap[m_iRead]->m_pUDT;
   remove_head_();
   pthread_mutex_unlock(&m_ReadLock);		
   

   if (!u->m_bConnected || u->m_bBroken)
      return -1;

   // pack a packet from the socket
   uint64_t ts;
   if (u->packData(pkt, ts) <= 0)
      return -1;

   addr = u->m_pPeerAddr;
   realaddr = u->m_pRealPeerAddr;
   
   // insert a new entry, ts is the next processing time
   if (ts > 0)
   {
   	  pthread_mutex_lock(&m_WriteLock);
      add_tail_no_wakeup_worker_(ts, u);	//在CSndQueue::worker线程中调用，所以不需要唤醒CSndQueue::worker线程
	  pthread_mutex_unlock(&m_WriteLock);
   }
   return 1;
}

void CSndUList::remove(const CUDT* u)
{
	pthread_mutex_lock(&m_ReadLock);
	pthread_mutex_lock(&m_WriteLock);
	remove_(u);
	pthread_mutex_unlock(&m_WriteLock);
	pthread_mutex_unlock(&m_ReadLock);
}

uint64_t CSndUList::getNextProcTime()
{
   uint64_t i;
   pthread_mutex_lock(&m_ReadLock);
   if( ULIST_EMPTY(m_iRead, m_iWrite) )
   {
      pthread_mutex_unlock(&m_ReadLock);
      return 0;
   }
   i = m_pHeap[m_iRead]->m_llTimeStamp;
   pthread_mutex_unlock(&m_ReadLock);
   return i;
}

int CSndUList::IsEmpty()
{
   /*int i = 0;
   pthread_mutex_lock(&m_ReadLock);
   pthread_mutex_lock(&m_WriteLock);
   if( ULIST_EMPTY(m_iRead, m_iWrite) )
   {
      i = 1;
   }
   pthread_mutex_unlock(&m_WriteLock);
   pthread_mutex_unlock(&m_ReadLock);
   return i;
   */
   return ULIST_EMPTY(m_iRead, m_iWrite);
}

void CSndUList::insert_(const int64_t& ts, const CUDT* u)
{
}

void CSndUList::remove_(const CUDT* u)
{
	CSNode* n = u->m_pSNode;
	int i;
	
	//检测合法
	if(n->m_iHeapLoc < 0 || ULIST_EMPTY(m_iRead, m_iWrite))
	{
//		Printf("Remove_ err1,n->m_iHeapLoc=%d,m_iRead=%d,m_iWrite=%d\n",n->m_iHeapLoc,m_iRead,m_iWrite);
		n->m_iHeapLoc = -1;		//非法值，改成-1
		return;
	}
	if( (m_iRead < m_iWrite && (n->m_iHeapLoc < m_iRead || n->m_iHeapLoc >= m_iWrite)) ||
		(m_iRead > m_iWrite && (n->m_iHeapLoc < m_iRead && n->m_iHeapLoc >= m_iWrite))   )
	{
//		printf("Remove_ err2,n->m_iHeapLoc=%d,m_iRead=%d,m_iWrite=%d\n",n->m_iHeapLoc,m_iRead,m_iWrite);
		n->m_iHeapLoc = -1;		//非法值，改成-1
		return;
	}

	if(m_iRead < m_iWrite)
	{
		for(i=n->m_iHeapLoc; i<m_iWrite-1; i++)
		{
			m_pHeap[i] = m_pHeap[i+1];
			m_pHeap[i]->m_iHeapLoc = i;
		}
	}
	else
	{
		int loc = (n->m_iHeapLoc < m_iRead) ? (n->m_iHeapLoc + m_iArrayLength) : n->m_iHeapLoc;
		int w = m_iWrite + m_iArrayLength;
		int t,t1;
		for(i=loc; i<w-1; i++)
		{
			t = i%m_iArrayLength;
			t1 = (i+1)%m_iArrayLength;
			m_pHeap[t] = m_pHeap[t1];
			m_pHeap[t]->m_iHeapLoc = t;
		}
	}
	n->m_iHeapLoc = -1;

	UL_PTR_DEC(m_iWrite);
}

inline void CSndUList::remove_head_()
{
	if( !ULIST_EMPTY(m_iRead, m_iWrite) )
	{
		m_pHeap[m_iRead]->m_iHeapLoc = -1;
		//m_pHeap[m_iRead] = NULL;	//remove后没有把指针写为NULL
		
		UL_PTR_INC(m_iRead);
	}
}

inline void CSndUList::add_head_(const int64_t& ts, const CUDT* u)
{
	int wr = m_iWrite;
	int rd = m_iRead;
	CSNode* n = u->m_pSNode;
	
	if( ULIST_FULL(rd, wr) )
	{
		//没有提供队列增长的机制，如果发现这条打印，需要增大队列
//		printf("Add_head_ full!!!!!!,r=%d,w=%d,size=%d\n",m_iRead, m_iWrite, m_iArrayLength);
		return;
	}

	UL_PTR_DEC(rd);
	
	m_pHeap[rd] = n;
	n->m_iHeapLoc = rd;
	n->m_llTimeStamp = ts;

	m_iRead = rd;
	//printf("add_head_,m_iRead=%d,m_iWrite=%d,pos=%d\n",m_iRead, m_iWrite,pos);

	//激活CSndQueue::worker线程
	if(1 == m_bWaitCont)
	{
#ifndef WIN32
		pthread_mutex_lock(m_pWindowLock);
		if(1 == m_bWaitCont)
		{
			//printf("add_head_,wake cont\n");
			m_bWaitCont = 0;
			pthread_cond_signal(m_pWindowCond);
		}
		pthread_mutex_unlock(m_pWindowLock);
#else
		SetEvent(*m_pWindowCond);
#endif
	}
}

inline void CSndUList::add_tail_(const int64_t& ts, const CUDT* u)
{
	int rd = m_iRead;
	CSNode* n = u->m_pSNode;
	
	if( ULIST_FULL(rd, m_iWrite) )
	{
		//没有提供队列增长的机制，如果发现这条打印，需要增大队列
//		printf("Add_tail_ full!!!!!!,r=%d,w=%d,size=%d\n",m_iRead, m_iWrite, m_iArrayLength);
		return;
	}	
	m_pHeap[m_iWrite] = n;
	n->m_iHeapLoc = m_iWrite;
	n->m_llTimeStamp = ts;

	UL_PTR_INC(m_iWrite);
	//printf("add_tail_,m_iRead=%d,m_iWrite=%d\n",m_iRead, m_iWrite);

	//激活CSndQueue::worker线程
	if(1 == m_bWaitCont)
	{
#ifndef WIN32
		pthread_mutex_lock(m_pWindowLock);
		if(1 == m_bWaitCont)
		{
			//printf("add_tail_,wake cont\n");
			m_bWaitCont = 0;
			pthread_cond_signal(m_pWindowCond);
		}
		pthread_mutex_unlock(m_pWindowLock);
#else
		SetEvent(*m_pWindowCond);
#endif
	}
}

inline void CSndUList::add_tail_no_wakeup_worker_(const int64_t& ts, const CUDT* u)
{
	int rd = m_iRead;
	CSNode* n = u->m_pSNode;
	
	if( ULIST_FULL(rd, m_iWrite) )
	{
		//没有提供队列增长的机制，如果发现这条打印，需要增大队列
//		printf("Add_tail_ full!!!!!!,r=%d,w=%d,size=%d\n",m_iRead, m_iWrite, m_iArrayLength);
		return;
	}	
	m_pHeap[m_iWrite] = n;
	n->m_iHeapLoc = m_iWrite;
	n->m_llTimeStamp = ts;

	UL_PTR_INC(m_iWrite);
	//printf("add_tail_,m_iRead=%d,m_iWrite=%d\n",m_iRead, m_iWrite);
}

inline void CSndUList::try_wakeup_worker_()
{
	//激活CSndQueue::worker线程
#ifndef WIN32
	if(1 == m_bWaitCont)
	{
		pthread_mutex_lock(m_pWindowLock);
		if(1 == m_bWaitCont)
		{
			//printf("add_tail_,wake cont\n");
			m_bWaitCont = 0;
			pthread_cond_signal(m_pWindowCond);
		}
		pthread_mutex_unlock(m_pWindowLock);
	}
#else
	if (1 == m_bWaitCont)
	{
		SetEvent(*m_pWindowCond);
	}
#endif
}

#endif

//
CSndQueue::CSndQueue():
m_WorkerThread(),
m_pSndUList(NULL),
m_pChannel(NULL),
m_pTimer(NULL),
m_WindowLock(),
m_WindowCond(),
m_bClosing(false),
m_ExitCond(),
m_bIFJVP2P(true),
m_bIFWAIT(true)
{
   #ifndef WIN32
      pthread_cond_init(&m_WindowCond, NULL);
      pthread_mutex_init(&m_WindowLock, NULL);
   #else
      m_WindowLock = CreateMutex(NULL, false, NULL);
      m_WindowCond = CreateEvent(NULL, false, false, NULL);
      m_ExitCond = CreateEvent(NULL, false, false, NULL);
   #endif
}

CSndQueue::~CSndQueue()
{
   m_bClosing = true;

   #ifndef WIN32
      pthread_mutex_lock(&m_WindowLock);
      pthread_cond_signal(&m_WindowCond);
      pthread_mutex_unlock(&m_WindowLock);
      if (0 != m_WorkerThread)
         pthread_join(m_WorkerThread, NULL);
      pthread_cond_destroy(&m_WindowCond);
      pthread_mutex_destroy(&m_WindowLock);
   #else
      SetEvent(m_WindowCond);
      if (NULL != m_WorkerThread)
	  {
         WaitForSingleObject(m_ExitCond, 1000);//INFINITE);
		 WaitThreadExit(m_WorkerThread);
	  }
      CloseHandle(m_WorkerThread);
      CloseHandle(m_WindowLock);
      CloseHandle(m_WindowCond);
      CloseHandle(m_ExitCond);
   #endif

    if(m_pSndUList != NULL)
	{
		delete m_pSndUList;
		m_pSndUList = NULL;
	}
}

void CSndQueue::init(const CChannel* c, const CTimer* t)
{
   m_pChannel = (CChannel*)c;
   m_pTimer = (CTimer*)t;
   m_pSndUList = new CSndUList;
   m_pSndUList->m_pWindowLock = &m_WindowLock;
   m_pSndUList->m_pWindowCond = &m_WindowCond;
   m_pSndUList->m_pTimer = m_pTimer; 

   #ifndef WIN32
      if (0 != pthread_create(&m_WorkerThread, NULL, CSndQueue::worker, this))
      {
         m_WorkerThread = 0;
		 if(m_pSndUList != NULL)
		 {
			 delete m_pSndUList;
			 m_pSndUList = NULL;
		 }
         throw CUDTException(3, 1);
      }
   #else
      DWORD threadID;
      m_WorkerThread = CreateThread(NULL, 0, CSndQueue::worker, this, 0, &threadID);
      if (NULL == m_WorkerThread)
      {
		  if(m_pSndUList != NULL)
		  {
			  delete m_pSndUList;
			  m_pSndUList = NULL;
		  }
		  throw CUDTException(3, 1);
	  }
   #endif
}

void CSndQueue::setifjvp2p(bool bifjvp2p)
{
	m_bIFJVP2P = bifjvp2p;
}
void CSndQueue::setifwait(bool bifwait)
{
	m_bIFWAIT = bifwait;
}

#ifndef WIN32
   void* CSndQueue::worker(void* param)
#else
   DWORD WINAPI CSndQueue::worker(LPVOID param)
#endif
{
   CSndQueue* self = (CSndQueue*)param;

   CPacket pkt;
   unsigned long ulsp = 0;
   ulsp = CTimer::getCPUFrequency() * 5000;//20000;// && (ts-currtime)/CTimer::getCPUFrequency() <= 70000
   
   while (!self->m_bClosing)
   {
      uint64_t ts = self->m_pSndUList->getNextProcTime();
	  
      if (ts > 0)
      {		  
         // wait until next processing time of the first socket on the list
    	  if(self->m_bIFJVP2P || self->m_bIFWAIT)
		  {//多播模式运行时需要严格控制流量，普通模式尽量发送即可
			  uint64_t currtime;
			  CTimer::rdtsc(currtime);
			  //del by wm
			  //         if (currtime < ts && ts-currtime<3000)//5000000)//50000000)
			  //		 if (currtime < ts && ts-currtime<30000000)//50000000)
			  //		 if (currtime < ts && (ts-currtime)/(CTimer::getCPUFrequency()*1000) <= 70)
			  //		 {//未到达发送时间 且时间差之不太大(严格按照理论实践会造成查网络环境时传输量太少)
			  //            self->m_pTimer->sleepto(ts);
			  //		 }
			  
			  if (currtime < ts)
			  {//未到达发送时间 且时间差之不太大(严格按照理论实践会造成查网络环境时传输量太少)
				  if(ts-currtime < ulsp)
				  {//间隔小于70ms
					  self->m_pTimer->sleepto(ts);
				  }
				  else
				  {//间隔最大允许30ms
					  self->m_pTimer->sleepto(currtime+(ulsp));
				  }
			  }
		  }

         // it is time to process it, pop it out/remove from the list
         sockaddr* addr;
         CPacket pkt;
		 sockaddr* realaddr=NULL;
         if (self->m_pSndUList->pop(addr, pkt, realaddr) < 0)
		 {        
			 continue;
		 }

         self->m_pChannel->sendto(addr, pkt, realaddr, 0, "");
      }
      else
      {
         // wait here if there is no sockets with data to be sent
         #ifndef WIN32
            pthread_mutex_lock(&self->m_WindowLock);
            if (!self->m_bClosing && (self->m_pSndUList->m_iLastEntry < 0))
				self->m_pSndUList->m_bWaitCont = 1;
               pthread_cond_wait(&self->m_WindowCond, &self->m_WindowLock);
            pthread_mutex_unlock(&self->m_WindowLock);
         #else
            WaitForSingleObject(self->m_WindowCond, INFINITE);
         #endif
      }
   }

   #ifndef WIN32
      return NULL;
   #else
      SetEvent(self->m_ExitCond);
      return 0;
   #endif
}

int CSndQueue::sendto(const sockaddr* addr, CPacket& packet, const sockaddr* realaddr, const int nYSTNO, const char chGroup[4])
{
   // send out the packet immediately (high priority), this is a control packet
   m_pChannel->sendto(addr, packet, realaddr, nYSTNO, chGroup);

   return packet.getLength();
}

/*****************************************************************************
*名称  : WaitThreadExit
*功能  : 等待指定的线程关闭，若在300毫秒内没有主动退出，则强制关闭该线程
*参数  : [IN] HANDEL hThread	 需要结束的线程
*返回值:
*其他  :s
****************************************************************************/
#ifdef WIN32
void CSndQueue::WaitThreadExit(HANDLE &hThread)
{
	DWORD dwExitCode;
	unsigned int iWaitMilliSecond = 0;
	if (hThread > 0) 
	{
		for ( ;; ) 
		{
			if ( ::GetExitCodeThread(hThread, &dwExitCode) )  
			{
				if (dwExitCode != STILL_ACTIVE) 
				{
					break;
				}
				else
				{
					Sleep(1);
					iWaitMilliSecond += 1;
					if (iWaitMilliSecond > 300) //等待300毫秒后强行退出线程
					{
						if ( TerminateThread(hThread, 1) )
						{
							//"线程强制结束.
							break;
						}
						else
						{
							//"线程强制结束失败."
							iWaitMilliSecond = 0;
						}
					}
				}
			} 
			else
			{
				break;
			}
		}
	}
}
#endif


//
CRcvUList::CRcvUList():
m_pUList(NULL),
m_pLast(NULL)
{
}

CRcvUList::~CRcvUList()
{
}

void CRcvUList::insert(const CUDT* u)
{
   CRNode* n = u->m_pRNode;
   CTimer::rdtsc(n->m_llTimeStamp);

   if (NULL == m_pUList)
   {
      // empty list, insert as the single node
      n->m_pPrev = n->m_pNext = NULL;
      m_pLast = m_pUList = n;

      return;
   }

   // always insert at the end for RcvUList
   n->m_pPrev = m_pLast;
   n->m_pNext = NULL;
   m_pLast->m_pNext = n;
   m_pLast = n;
}

void CRcvUList::remove(const CUDT* u)
{
   CRNode* n = u->m_pRNode;
   if(n == NULL)
   {
	   return;
   }

   if (!n->m_bOnList)
      return;

   if (NULL == n->m_pPrev)
   {
      // n is the first node
      m_pUList = n->m_pNext;
      if (NULL == m_pUList)
         m_pLast = NULL;
      else
         m_pUList->m_pPrev = NULL;
   }
   else
   {
      n->m_pPrev->m_pNext = n->m_pNext;
      if (NULL == n->m_pNext)
      {
         // n is the last node
         m_pLast = n->m_pPrev;
      }
      else
         n->m_pNext->m_pPrev = n->m_pPrev;
   }

   n->m_pNext = n->m_pPrev = NULL;
}

void CRcvUList::update(const CUDT* u)
{
   CRNode* n = u->m_pRNode;

   if (!n->m_bOnList)
      return;

   CTimer::rdtsc(n->m_llTimeStamp);

   // if n is the last node, do not need to change
   if (NULL == n->m_pNext)
      return;

   if (NULL == n->m_pPrev)
   {
      m_pUList = n->m_pNext;
      m_pUList->m_pPrev = NULL;
   }
   else
   {
      n->m_pPrev->m_pNext = n->m_pNext;
      n->m_pNext->m_pPrev = n->m_pPrev;
   }

   n->m_pPrev = m_pLast;
   n->m_pNext = NULL;
   m_pLast->m_pNext = n;
   m_pLast = n;
}

//
CHash::CHash():
m_pBucket(NULL),
m_iHashSize(0)
{
}

CHash::~CHash()
{
   for (int i = 0; i < m_iHashSize; ++ i)
   {
      CBucket* b = m_pBucket[i];
      while (NULL != b)
      {
         CBucket* n = b->m_pNext;
         delete b;
         b = n;
      }
   }

   if(m_pBucket != NULL)
   {
	   delete[] m_pBucket;
	   m_pBucket = NULL;
   }
}

void CHash::init(const int& size)
{
   m_pBucket = new CBucket* [size];

   for (int i = 0; i < size; ++ i)
      m_pBucket[i] = NULL;

   m_iHashSize = size;
}

CUDT* CHash::lookup(const int32_t& id)
{
   // simple hash function (% hash table size); suitable for socket descriptors
   CBucket* b = m_pBucket[id % m_iHashSize];

   while (NULL != b)
   {
      if (id == b->m_iID)
         return b->m_pUDT;
      b = b->m_pNext;
   }

   return NULL;
}

void CHash::insert(const int32_t& id, const CUDT* u)
{
   CBucket* b = m_pBucket[id % m_iHashSize];

   CBucket* n = new CBucket;
   n->m_iID = id;
   n->m_pUDT = (CUDT*)u;
   n->m_pNext = b;

   m_pBucket[id % m_iHashSize] = n;
}

void CHash::remove(const int32_t& id)
{
   CBucket* b = m_pBucket[id % m_iHashSize];
   CBucket* p = NULL;

   while (NULL != b)
   {
      if (id == b->m_iID)
      {
         if (NULL == p)
            m_pBucket[id % m_iHashSize] = b->m_pNext;
         else
            p->m_pNext = b->m_pNext;

         delete b;
		 b = NULL;

         return;
      }

      p = b;
      b = b->m_pNext;
   }
}


//
CRendezvousQueue::CRendezvousQueue():
m_vRendezvousID(),
m_RIDVectorLock()
{
   #ifndef WIN32
      pthread_mutex_init(&m_RIDVectorLock, NULL);
   #else
      m_RIDVectorLock = CreateMutex(NULL, false, NULL);
   #endif
}

CRendezvousQueue::~CRendezvousQueue()
{
   #ifndef WIN32
      pthread_mutex_destroy(&m_RIDVectorLock);
   #else
      CloseHandle(m_RIDVectorLock);
   #endif

   for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
   {
      if (AF_INET == i->m_iIPversion)
	  {
		  delete (sockaddr_in*)i->m_pPeerAddr;
		  i->m_pPeerAddr = NULL;
	  }
      else
	  {
		  delete (sockaddr_in6*)i->m_pPeerAddr;
		  i->m_pPeerAddr = NULL;
	  }
   }

   m_vRendezvousID.clear();

//   std::vector<CRL> tmp = m_vRendezvousID;
//   m_vRendezvousID.swap(tmp);
}

void CRendezvousQueue::insert(const UDTSOCKET& id, const int& ipv, const sockaddr* addr, int nYSTNO)
{
   CGuard vg(m_RIDVectorLock);

   CRL r;
   r.m_iID = id;
   r.m_iIPversion = ipv;
   r.m_pPeerAddr = (AF_INET == ipv) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
   r.m_nYSTNO = nYSTNO;
   memcpy(r.m_pPeerAddr, addr, (AF_INET == ipv) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

   m_vRendezvousID.insert(m_vRendezvousID.end(), r);
}

void CRendezvousQueue::remove(const UDTSOCKET& id)
{
   CGuard vg(m_RIDVectorLock);

   for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
      if (i->m_iID == id)
      {
         if (AF_INET == i->m_iIPversion)
		 {
            delete (sockaddr_in*)i->m_pPeerAddr;
			i->m_pPeerAddr = NULL;
		 }
         else
		 {
            delete (sockaddr_in6*)i->m_pPeerAddr;
			i->m_pPeerAddr = NULL;
		 }
         m_vRendezvousID.erase(i);

         return;
      }
}

bool CRendezvousQueue::retrieve(const sockaddr* addr, UDTSOCKET& id)
{
   CGuard vg(m_RIDVectorLock);

   for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
   {
      if (CIPAddress::ipcmp(addr, i->m_pPeerAddr, i->m_iIPversion) && ((0 == id) || (id == i->m_iID)))
      {
         id = i->m_iID;
         return true;
      }
   }

   return false;
}

bool CRendezvousQueue::retrieveyst(const sockaddr* addr)
{
	CGuard vg(m_RIDVectorLock);
	
	for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
	{
		if (CIPAddress::ipcmp(addr, i->m_pPeerAddr, i->m_iIPversion))
		{
			return true;
		}
	}
	
	return false;
}
bool CRendezvousQueue::retrieveseryst(const sockaddr* addr, UDTSOCKET& id, int nYSTNO)
{
	CGuard vg(m_RIDVectorLock);
	for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
	{
		if(nYSTNO == i->m_nYSTNO)
		{
			id = i->m_iID;
			return true;
		}
	}
	return false;
}
bool CRendezvousQueue::retrievepunchyst(const sockaddr* addr, UDTSOCKET& id, int nYSTNO)
{
	CGuard vg(m_RIDVectorLock);
	for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
	{
		if(nYSTNO == i->m_nYSTNO)
		{
			id = i->m_iID;
			memcpy(i->m_pPeerAddr,addr, (AF_INET == i->m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
			return true;
		}
	}
	return false;
}
bool CRendezvousQueue::getysttouchaddr(sockaddr* addr, UDTSOCKET& id)
{
	CGuard vg(m_RIDVectorLock);
	
	for (vector<CRL>::iterator i = m_vRendezvousID.begin(); i != m_vRendezvousID.end(); ++ i)
	{
		if (id == i->m_iID)
		{
			memcpy(addr, i->m_pPeerAddr, (AF_INET == i->m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
			return true;
		}
	}
	
	return false;
}



//
CRcvQueue::CRcvQueue():
m_WorkerThread(),
m_UnitQueue(),
m_pRcvUList(NULL),
m_pHash(NULL),
m_pChannel(NULL),
m_pTimer(NULL),
m_iPayloadSize(),
m_bClosing(false),
m_ExitCond(),
m_LSLock(),
m_pListener(NULL),
m_pRendezvousQueue(NULL),
m_vNewEntry(),
m_IDLock(),
m_mBuffer(),
m_PassLock(),
m_PassCond(),
m_NatListLock()
{
   #ifndef WIN32
      pthread_mutex_init(&m_PassLock, NULL);
      pthread_cond_init(&m_PassCond, NULL);
      pthread_mutex_init(&m_LSLock, NULL);
      pthread_mutex_init(&m_IDLock, NULL);
	  pthread_mutex_init(&m_UQLock, NULL);

	  pthread_mutex_init(&m_NatListLock, NULL);
 #else
      m_PassLock = CreateMutex(NULL, false, NULL);
      m_PassCond = CreateEvent(NULL, false, false, NULL);
      m_LSLock = CreateMutex(NULL, false, NULL);
      m_IDLock = CreateMutex(NULL, false, NULL);
      m_ExitCond = CreateEvent(NULL, false, false, NULL);
	  m_UQLock = CreateMutex(NULL, false, NULL);

	  m_NatListLock = CreateMutex(NULL, false, NULL);
  #endif

   m_vNewEntry.reserve(32);
   m_vNewEntry.clear();
}

CRcvQueue::~CRcvQueue()
{
   m_bClosing = true;

   #ifndef WIN32
      if (0 != m_WorkerThread)
         pthread_join(m_WorkerThread, NULL);
      pthread_mutex_destroy(&m_PassLock);
      pthread_cond_destroy(&m_PassCond);
      pthread_mutex_destroy(&m_LSLock);
      pthread_mutex_destroy(&m_IDLock);
	  pthread_mutex_destroy(&m_UQLock);

	  pthread_mutex_destroy(&m_NatListLock);
  #else
      if (NULL != m_WorkerThread)
	  {
         WaitForSingleObject(m_ExitCond, 1000);//INFINITE);
		 WaitThreadExit(m_WorkerThread);
	  }
      CloseHandle(m_WorkerThread);
      CloseHandle(m_PassLock);
      CloseHandle(m_PassCond);
      CloseHandle(m_LSLock);
      CloseHandle(m_IDLock);
      CloseHandle(m_ExitCond);
	  CloseHandle(m_UQLock);

	  CloseHandle(m_NatListLock);
  #endif

    if(m_pRcvUList != NULL)
	{
		delete m_pRcvUList;
		m_pRcvUList = NULL;
	}
	if(m_pHash != NULL)
	{
		delete m_pHash;
		m_pHash = NULL;
	}
	if(m_pRendezvousQueue != NULL)
	{
		delete m_pRendezvousQueue;
		m_pRendezvousQueue = NULL;
	}
	
	
	for (map<int32_t, CPacket*>::iterator i = m_mBuffer.begin(); i != m_mBuffer.end(); ++ i)
	{
		if(i->second->m_pcData != NULL)
		{
			delete[] i->second->m_pcData;
			i->second->m_pcData = NULL;
		}
		if(i->second != NULL)
		{
			delete i->second;
			i->second = NULL;
		}
	}

	m_mBuffer.clear();
	m_vNewEntry.clear();

//	map<int32_t, CPacket*> tmp1 = m_mBuffer;
//	m_mBuffer.swap(tmp1);
//	std::vector<CUDT*> tmp2 = m_vNewEntry;
//	m_vNewEntry.swap(tmp2);
}

void CRcvQueue::init(const int& qsize, const int& payload, const int& version, const int& hsize, const CChannel* cc, const CTimer* t)
{
   m_iPayloadSize = payload;

   m_UnitQueue.init(qsize, payload, version);

   m_pHash = new CHash;
   m_pHash->init(hsize);

   m_pChannel = (CChannel*)cc;
   m_pTimer = (CTimer*)t;

   m_pRcvUList = new CRcvUList;
   m_pRendezvousQueue = new CRendezvousQueue;

   #ifndef WIN32
      if (0 != pthread_create(&m_WorkerThread, NULL, CRcvQueue::worker, this))
      {
         m_WorkerThread = 0;
		 if(m_pHash != NULL)
		 {
			 delete m_pHash;
			 m_pHash = NULL;
		 }
		 if(m_pRcvUList != NULL)
		 {
			 delete m_pRcvUList;
			 m_pRcvUList = NULL;
		 }
		 if(m_pRendezvousQueue != NULL)
		 {
			 delete m_pRendezvousQueue;
			 m_pRendezvousQueue = NULL;
		 }
         throw CUDTException(3, 1);
      }
   #else
      DWORD threadID;
      m_WorkerThread = CreateThread(NULL, 0, CRcvQueue::worker, this, 0, &threadID);
      if (NULL == m_WorkerThread)
	  {
		  if(m_pHash != NULL)
		  {
			  delete m_pHash;
			  m_pHash = NULL;
		  }
		  if(m_pRcvUList != NULL)
		  {
			  delete m_pRcvUList;
			  m_pRcvUList = NULL;
		  }
		  if(m_pRendezvousQueue != NULL)
		  {
			  delete m_pRendezvousQueue;
			  m_pRendezvousQueue = NULL;
		  }

		  throw CUDTException(3, 1);
	  }
         
   #endif
}

int CRcvQueue::setmaxrecvbuf(int nmax)
{
	return m_UnitQueue.setmaxrecvbuf(nmax);
}
//void OutputDebug(char* format, ...);

#ifndef WIN32
   void* CRcvQueue::worker(void* param)
#else
   DWORD WINAPI CRcvQueue::worker(LPVOID param)
#endif
{
   CRcvQueue* self = (CRcvQueue*)param;

   sockaddr* addr = (AF_INET == self->m_UnitQueue.m_iIPversion) ? (sockaddr*) new sockaddr_in : (sockaddr*) new sockaddr_in6;
   CUDT* u = NULL;
   //int32_t id;
   int id;
#ifndef WIN32
   struct timeval start;
#endif

   int ncount=0;

   while (!self->m_bClosing)
   {
      #ifdef NO_BUSY_WAITING
         self->m_pTimer->tick();
      #endif

      // check waiting list, if new socket, insert it to the list
      while(self->ifNewEntry())
      {
         CUDT* ne = self->getNewEntry();
         if (NULL != ne)
         {
            self->m_pRcvUList->insert(ne);
            self->m_pHash->insert(ne->m_SocketID, ne);
         }
      }

	  CGuard::enterCS(self->m_UQLock);
      // find next available slot for incoming packet
      CUnit* unit = self->m_UnitQueue.getNextAvailUnit();
      if (NULL == unit)
      {
         // no space, skip this packet
         CPacket temp;
         temp.m_pcData = new char[self->m_iPayloadSize];
         temp.setLength(self->m_iPayloadSize);
         self->m_pChannel->recvfrom(addr, temp);
         delete[] temp.m_pcData;
		 temp.m_pcData = NULL;
         goto TIMER_CHECK;
      }

      unit->m_Packet.setLength(self->m_iPayloadSize);

      // reading next incoming packet
      if (self->m_pChannel->recvfrom(addr, unit->m_Packet) <= 0)
      {
		  goto TIMER_CHECK;
	  }

      id = unit->m_Packet.m_iID;
//		OutputDebug("id = %d",id);
      // ID 0 is for connection request, which should be passed to the listening socket or rendezvous sockets
      if (0 == id)
      {
         if (NULL != self->m_pListener)
			 ((CUDT*)self->m_pListener)->listen(addr, unit->m_Packet, unit->m_Packet.m_PacketVector[2].iov_len);
         else if (self->m_pRendezvousQueue->retrieve(addr, id))
            self->storePkt(id, unit->m_Packet.clone());
      }
      else if (id > 0)
      {
         if (NULL != (u = self->m_pHash->lookup(id)))
         {
            if (CIPAddress::ipcmp(addr, u->m_pPeerAddr, u->m_iIPversion))
            {
               if (u->m_bConnected && !u->m_bBroken && !u->m_bClosing)
               {
                  if (0 == unit->m_Packet.getFlag())
                     u->processData(unit);
                  else
                     u->processCtrl(unit->m_Packet);

                  u->checkTimers();
                  self->m_pRcvUList->update(u);
               }
            }
         }
         else if (self->m_pRendezvousQueue->retrieve(addr, id))
		 {
			 self->storePkt(id, unit->m_Packet.clone());
		 }

      }	  

TIMER_CHECK:
      // take care of the timing event for all UDT sockets

	  CGuard::leaveCS(self->m_UQLock);
	  
      CRNode* ul = self->m_pRcvUList->m_pUList;
      uint64_t currtime;
      CTimer::rdtsc(currtime);
      uint64_t ctime = currtime - 100000 * CTimer::getCPUFrequency();

	  if(ul == NULL)
	  {
		  if(ncount > 50)
		  {
			  ncount=0;
			#ifndef WIN32
			  start.tv_sec = 0;
			  start.tv_usec = 1000;
			  select(0,NULL,NULL,NULL,&start);
			#else
			  Sleep(1);
			#endif
		  }
		  else
		  {
			  ncount++;
		  }
	  }

      while ((NULL != ul) && (ul->m_llTimeStamp < ctime))
      {
         CUDT* u = ul->m_pUDT;

         if (u->m_bConnected && !u->m_bBroken && !u->m_bClosing)
         {
            u->checkTimers();
            self->m_pRcvUList->update(u);
         }
         else
         {
            // the socket must be removed from Hash table first, then RcvUList
            self->m_pHash->remove(u->m_SocketID);
            self->m_pRcvUList->remove(u);
			u->m_pRNode->m_bOnList = false;
         }

         ul = self->m_pRcvUList->m_pUList;
      }

   }

   if (AF_INET == self->m_UnitQueue.m_iIPversion)
   {
	   delete (sockaddr_in*)addr;
	   addr = NULL;
   }
   else
   {
	   delete (sockaddr_in6*)addr;
	   addr = NULL;
   }

   #ifndef WIN32
      return NULL;
   #else
      SetEvent(self->m_ExitCond);
      return 0;
   #endif
}

int CRcvQueue::recvfrom(const int32_t& id, CPacket& packet)
{
   CGuard bufferlock(m_PassLock);

   map<int32_t, CPacket*>::iterator i = m_mBuffer.find(id);

   if (i == m_mBuffer.end())
   {
      #ifndef WIN32

#ifdef MOBILE_CLIENT
	   struct timespec abstime;
	   struct timeval now;
	   int timeout_ms = 400;
	   gettimeofday(&now, NULL);
	   int nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
	   abstime.tv_nsec = nsec % 1000000000;
	   abstime.tv_sec = now.tv_sec + nsec / 1000000000 + timeout_ms / 1000;
	   
            int result=  pthread_cond_timedwait(&m_PassCond, &m_PassLock, (const struct timespec *)&abstime);
#else

         uint64_t now = CTimer::getTime();
         timespec timeout;

         timeout.tv_sec = now / 1000000;//+1
         timeout.tv_nsec = (now % 1000000 + 400) * 1000;//(now % 1000000) * 1000;

         pthread_cond_timedwait(&m_PassCond, &m_PassLock, &timeout);
#endif
      #else
         ReleaseMutex(m_PassLock);
         WaitForSingleObject(m_PassCond, 400);//1000
         WaitForSingleObject(m_PassLock, INFINITE);
      #endif

      i = m_mBuffer.find(id);
      if (i == m_mBuffer.end())
      {
         packet.setLength(-1);

		 //////////////////////////////////////////////////////////////////////////
		 //没找到对应的消息，很可能是误收了其他连接的服务器反馈，复位下信号，便于其他再次接收
//	#ifndef WIN32
//		 pthread_cond_signal(&m_PassCond);
//	#else
//		 SetEvent(m_PassCond);
//	#endif
		 //////////////////////////////////////////////////////////////////////////
         return -1;
      }
   }

   if (packet.getLength() < i->second->getLength())
   {
      packet.setLength(-1);
      return -1;
   }

   memcpy(packet.m_nHeader, i->second->m_nHeader, CPacket::m_iPktHdrSize);
   memcpy(packet.m_pcData, i->second->m_pcData, i->second->getLength());
   packet.setLength(i->second->getLength());

   delete[] i->second->m_pcData;
   i->second->m_pcData = NULL;
   delete i->second;
   i->second = NULL;
   m_mBuffer.erase(i);

   return packet.getLength();
}

int CRcvQueue::setListener(const CUDT* u)
{
  
    CGuard lslock(m_LSLock);

   if (NULL != m_pListener)
      return -1;

   m_pListener = (CUDT*)u;
   return 1;
}

void CRcvQueue::removeListener(const CUDT* u)
{
   CGuard lslock(m_LSLock);

   if (u == m_pListener)
      m_pListener = NULL;
}

void CRcvQueue::setNewEntry(CUDT* u)
{
   CGuard listguard(m_IDLock);
   m_vNewEntry.insert(m_vNewEntry.end(), u);
}

bool CRcvQueue::ifNewEntry()
{
   return !(m_vNewEntry.empty());
}

CUDT* CRcvQueue::getNewEntry()
{
   CGuard listguard(m_IDLock);

   if (m_vNewEntry.empty())
   {
//	   std::vector<CUDT*> tmp = m_vNewEntry;
//	   m_vNewEntry.swap(tmp);
      return NULL;
   }

   CUDT* u = (CUDT*)*(m_vNewEntry.begin());
   m_vNewEntry.erase(m_vNewEntry.begin());

   return u;
}
void CRcvQueue::storePkt(const int32_t& id, CPacket* pkt)
{
   #ifndef WIN32
      pthread_mutex_lock(&m_PassLock);
   #else
      WaitForSingleObject(m_PassLock, INFINITE);
   #endif

   map<int32_t, CPacket*>::iterator i = m_mBuffer.find(id);

   if (i == m_mBuffer.end())
      m_mBuffer[id] = pkt;
   else
   {
      delete[] i->second->m_pcData;
	  i->second->m_pcData = NULL;
      delete i->second;
      i->second = pkt;
   }

   #ifndef WIN32
      pthread_mutex_unlock(&m_PassLock);
      pthread_cond_signal(&m_PassCond);
   #else
      ReleaseMutex(m_PassLock);
      SetEvent(m_PassCond);
   #endif
}


/*****************************************************************************
*名称  : WaitThreadExit
*功能  : 等待指定的线程关闭，若在300毫秒内没有主动退出，则强制关闭该线程
*参数  : [IN] HANDEL hThread	 需要结束的线程
*返回值:
*其他  :s
****************************************************************************/
#ifdef WIN32
void CRcvQueue::WaitThreadExit(HANDLE &hThread)
{
	DWORD dwExitCode;
	unsigned int iWaitMilliSecond = 0;
	if (hThread > 0) 
	{
		for ( ;; ) 
		{
			if ( ::GetExitCodeThread(hThread, &dwExitCode) )  
			{
				if (dwExitCode != STILL_ACTIVE) 
				{
					break;
				}
				else
				{
					Sleep(1);
					iWaitMilliSecond += 1;
					if (iWaitMilliSecond > 300) //等待300毫秒后强行退出线程
					{
						if ( TerminateThread(hThread, 1) )
						{
							//"线程强制结束.
							break;
						}
						else
						{
							//"线程强制结束失败."
							iWaitMilliSecond = 0;
						}
					}
				}
			} 
			else
			{
				break;
			}
		}
	}
}
#endif



















