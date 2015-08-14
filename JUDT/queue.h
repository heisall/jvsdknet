/*****************************************************************************
written by
   Yunhong Gu, last updated 06/02/2008
*****************************************************************************/


#ifndef __UDT_QUEUE_H__
#define __UDT_QUEUE_H__

#include "common.h"
#include "packet.h"
#include "channel.h"
#include <vector>
#include <map>

class CUDT;

struct CUnit
{
   CPacket m_Packet;		// packet
   int m_iFlag;			// 0: free, 1: occupied, 2: msg read but not freed (out-of-order), 3: msg dropped
};

class CUnitQueue
{
friend class CRcvQueue;
friend class CRcvBuffer;

public:
   CUnitQueue();
   ~CUnitQueue();

public:

      // Functionality:
      //    Initialize the unit queue.
      // Parameters:
      //    1) [in] size: queue size
      //    2) [in] mss: maximum segament size
      //    3) [in] version: IP version
      // Returned value:
      //    0: success, -1: failure.

   int init(const int& size, const int& mss, const int& version);

      // Functionality:
      //    Increase (double) the unit queue size.
      // Parameters:
      //    None.
      // Returned value:
      //    0: success, -1: failure.

   int increase();

      // Functionality:
      //    Decrease (halve) the unit queue size.
      // Parameters:
      //    None.
      // Returned value:
      //    0: success, -1: failure.

   int shrink();

      // Functionality:
      //    find an available unit for incoming packet.
      // Parameters:
      //    None.
      // Returned value:
      //    Pointer to the available unit, NULL if not found.

   CUnit* getNextAvailUnit();

   int setmaxrecvbuf(int nmax);
private:
   struct CQEntry
   {
      CUnit* m_pUnit;		// unit queue
      char* m_pBuffer;		// data buffer
      int m_iSize;		// size of each queue

      CQEntry* m_pNext;
   }
   *m_pQEntry,			// pointer to the first unit queue
   *m_pCurrQueue,		// pointer to the current available queue
   *m_pLastQueue;		// pointer to the last unit queue

   CUnit* m_pAvailUnit;         // recent available unit

   int m_iSize;			// total size of the unit queue, in number of packets
   int m_iCount;		// total number of valid packets in the queue

   int m_iMSS;			// unit buffer size
   int m_iIPversion;		// IP version
   
public:
	int m_iMAXRBUF;//每个端口共用一个接收容器，容器的最大容积

private:
   CUnitQueue(const CUnitQueue&);
   CUnitQueue& operator=(const CUnitQueue&);
};

struct CSNode
{
   CUDT* m_pUDT;		// Pointer to the instance of CUDT socket
   uint64_t m_llTimeStamp;      // Time Stamp

   int m_iHeapLoc;		// location on the heap, -1 means not on the heap
};

#ifdef WIN32
class CSndUList
{
friend class CSndQueue;

public:
   CSndUList();
   ~CSndUList();

public:

      // Functionality:
      //    Insert a new UDT instance into the list.
      // Parameters:
      //    1) [in] ts: time stamp: next processing time
      //    2) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void insert(const int64_t& ts, const CUDT* u);

      // Functionality:
      //    Update the timestamp of the UDT instance on the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      //    2) [in] resechedule: if the timestampe shoudl be rescheduled
      // Returned value:
      //    None.

   void update(const CUDT* u, const bool& reschedule = true);

      // Functionality:
      //    Retrieve the next packet and peer address from the first entry, and reschedule it in the queue.
      // Parameters:
      //    0) [out] addr: destination address of the next packet
      //    1) [out] pkt: the next packet to be sent
      // Returned value:
      //    1 if successfully retrieved, -1 if no packet found.

   int pop(sockaddr*& addr, CPacket& pkt, sockaddr*& realaddr);

      // Functionality:
      //    Remove UDT instance from the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void remove(const CUDT* u);

      // Functionality:
      //    Retrieve the next scheduled processing time.
      // Parameters:
      //    None.
      // Returned value:
      //    Scheduled processing time of the first UDT socket in the list.

   uint64_t getNextProcTime();

private:
   void insert_(const int64_t& ts, const CUDT* u);
   void remove_(const CUDT* u);

private:
   CSNode** m_pHeap;			// The heap array
   int m_iArrayLength;			// physical length of the array
   int m_iLastEntry;			// position of last entry on the heap array

   pthread_mutex_t m_ListLock;

   pthread_mutex_t* m_pWindowLock;
   pthread_cond_t* m_pWindowCond;

   CTimer* m_pTimer;
   
public:
	int m_bWaitCont;				//是否正在等m_pWindowCond

private:
   CSndUList(const CSndUList&);
   CSndUList& operator=(const CSndUList&);
};
#else//////////////////////////////////////////////////////////////////////////
class CSndUList
{
friend class CSndQueue;

public:
   CSndUList();
   ~CSndUList();

public:

      // Functionality:
      //    Insert a new UDT instance into the list.
      // Parameters:
      //    1) [in] ts: time stamp: next processing time
      //    2) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void insert(const int64_t& ts, const CUDT* u);

      // Functionality:
      //    Update the timestamp of the UDT instance on the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      //    2) [in] resechedule: if the timestampe shoudl be rescheduled
      // Returned value:
      //    None.

	
   void update(const CUDT* u, const bool& reschedule = true, const int64_t& ts = 0);//如果ts不为0，只在写指针处插入

      // Functionality:
      //    Retrieve the next packet and peer address from the first entry, and reschedule it in the queue.
      // Parameters:
      //    0) [out] addr: destination address of the next packet
      //    1) [out] pkt: the next packet to be sent
      // Returned value:
      //    1 if successfully retrieved, -1 if no packet found.

   int pop(sockaddr*& addr, CPacket& pkt, sockaddr*& realaddr);

      // Functionality:
      //    Remove UDT instance from the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void remove(const CUDT* u);

      // Functionality:
      //    Retrieve the next scheduled processing time.
      // Parameters:
      //    None.
      // Returned value:
      //    Scheduled processing time of the first UDT socket in the list.

   uint64_t getNextProcTime();

   //返回1:队列为空，0:
   int IsEmpty();
private:
   void insert_(const int64_t& ts, const CUDT* u);
   void remove_(const CUDT* u);

   inline void add_head_(const int64_t& ts, const CUDT* u);	//读指针处增加
   inline void add_tail_(const int64_t& ts, const CUDT* u);	//写指针处增加
   inline void add_tail_no_wakeup_worker_(const int64_t& ts, const CUDT* u);//写指针处增加,不唤醒CSndQueue发送线程
   inline void remove_head_();								//读指针处删除
   inline void try_wakeup_worker_();						//唤醒CSndQueue发送线程
private:
   CSNode** m_pHeap;			// The heap array
   int m_iArrayLength;			// physical length of the array
   int m_iLastEntry;			// position of last entry on the heap array,已经不用了

//当(m_iRead == m_iWrite)时，缓冲区中的数据为空。
//当((m_iWrite+1) == (m_iRead) || (m_iWrite+1) == (m_iRead+m_iArrayLength)) 时，表示缓冲区满
//缓冲区中数据长度:(m_iWrite - m_iRead + m_iArrayLength) % m_iArrayLength
   int m_iRead;					// 读位置(还未读取),取值范围0~(m_iLastEntry-1)
   int m_iWrite;				// 写位置(还未写入),取值范围0~(m_iLastEntry-1)

//使用环形缓冲区,防止有两个线程同时访问读锁或者写锁,加了两个锁。
//读取m_iRead指向的元素只需要申请读锁，可直接访问m_iWrite，多次使用m_iWrite时最好先保存到局部变量。
//在m_iWrite指向的位置插入一个元素只需要申请写锁，可直接访问m_iRead，多次使用m_iRead时最好先保存到局部变量。
//如果在队列中间插入或者删除或修改元素，需要先申请读锁再申请写锁
   pthread_mutex_t m_ReadLock;	//读锁
   pthread_mutex_t m_WriteLock;	//写锁

   //pthread_mutex_t m_ListLock;	//原先的锁，不用了

   pthread_mutex_t* m_pWindowLock;
   pthread_cond_t* m_pWindowCond;

   CTimer* m_pTimer;
   
public:
   int m_bWaitCont;				//是否正在等m_pWindowCond

private:
   CSndUList(const CSndUList&);
   CSndUList& operator=(const CSndUList&);
   
public:
	#define ULIST_SIZE(r,w)  (((w) + m_iArrayLength - (r)) % m_iArrayLength)
	#define ULIST_EMPTY(r,w) ((r) == (w))
	#define ULIST_FULL(r,w)  ((r) == (w)+1 || ((w)+1) == ((r)+m_iArrayLength))
	#define UL_PTR_INC(p) \
		do{\
			if((p) >= m_iArrayLength-1)\
			{\
				(p) = 0;\
			}\
			else\
			{\
				(p)++;\
			}\
		}while(0)
	#define UL_PTR_DEC(p) \
		do{\
			if((p) <= 0)\
			{\
				(p) = m_iArrayLength-1;\
			}\
			else\
			{\
				(p)--;\
			}\
		}while(0)

	
};
#endif

struct CRNode
{
   CUDT* m_pUDT;                // Pointer to the instance of CUDT socket
   uint64_t m_llTimeStamp;      // Time Stamp

   CRNode* m_pPrev;             // previous link
   CRNode* m_pNext;             // next link

   bool m_bOnList;              // if the node is already on the list
};

class CRcvUList
{
public:
   CRcvUList();
   ~CRcvUList();

public:

      // Functionality:
      //    Insert a new UDT instance to the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void insert(const CUDT* u);

      // Functionality:
      //    Remove the UDT instance from the list.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void remove(const CUDT* u);

      // Functionality:
      //    Move the UDT instance to the end of the list, if it already exists; otherwise, do nothing.
      // Parameters:
      //    1) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void update(const CUDT* u);

public:
   CRNode* m_pUList;		// the head node

private:
   CRNode* m_pLast;		// the last node

private:
   CRcvUList(const CRcvUList&);
   CRcvUList& operator=(const CRcvUList&);
};

class CHash
{
public:
   CHash();
   ~CHash();

public:

      // Functionality:
      //    Initialize the hash table.
      // Parameters:
      //    1) [in] size: hash table size
      // Returned value:
      //    None.

   void init(const int& size);

      // Functionality:
      //    Look for a UDT instance from the hash table.
      // Parameters:
      //    1) [in] id: socket ID
      // Returned value:
      //    Pointer to a UDT instance, or NULL if not found.

   CUDT* lookup(const int32_t& id);

      // Functionality:
      //    Insert an entry to the hash table.
      // Parameters:
      //    1) [in] id: socket ID
      //    2) [in] u: pointer to the UDT instance
      // Returned value:
      //    None.

   void insert(const int32_t& id, const CUDT* u);

      // Functionality:
      //    Remove an entry from the hash table.
      // Parameters:
      //    1) [in] id: socket ID
      // Returned value:
      //    None.

   void remove(const int32_t& id);

private:
   struct CBucket
   {
      int32_t m_iID;		// Socket ID
      CUDT* m_pUDT;		// Socket instance

      CBucket* m_pNext;		// next bucket
   } **m_pBucket;		// list of buckets (the hash table)

   int m_iHashSize;		// size of hash table

private:
   CHash(const CHash&);
   CHash& operator=(const CHash&);
};

class CRendezvousQueue
{
public:
   CRendezvousQueue();
   ~CRendezvousQueue();

public:
   void insert(const UDTSOCKET& id, const int& ipv, const sockaddr* addr, int nYSTNO=0);
   void remove(const UDTSOCKET& id);
   bool retrieve(const sockaddr* addr, UDTSOCKET& id);
   bool retrieveyst(const sockaddr* addr);
   bool retrieveseryst(const sockaddr* addr, UDTSOCKET& id, int nYSTNO);
   bool retrievepunchyst(const sockaddr* addr, UDTSOCKET& id, int nYSTNO);
   bool getysttouchaddr(sockaddr* addr, UDTSOCKET& id);

private:
   struct CRL
   {
      UDTSOCKET m_iID;
      int m_iIPversion;
      sockaddr* m_pPeerAddr;
	  int m_nYSTNO;
   };
   std::vector<CRL> m_vRendezvousID;         // The sockets currently in rendezvous mode

   pthread_mutex_t m_RIDVectorLock;
};

class CSndQueue
{
friend class CUDT;
friend class CUDTUnited;

public:
   CSndQueue();
   ~CSndQueue();

public:

      // Functionality:
      //    Initialize the sending queue.
      // Parameters:
      //    1) [in] c: UDP channel to be associated to the queue
      //    2) [in] t: Timer
      // Returned value:
      //    None.

   void init(const CChannel* c, const CTimer* t);

      // Functionality:
      //    Send out a packet to a given address.
      // Parameters:
      //    1) [in] addr: destination address
      //    2) [in] packet: packet to be sent out
      // Returned value:
      //    Size of data sent out.

   int sendto(const sockaddr* addr, CPacket& packet, const sockaddr* realaddr, const int nYSTNO, const char chGroup[4]);

   void setifjvp2p(bool bifjvp2p);
   void setifwait(bool bifwait);
private:
#ifndef WIN32
   static void* worker(void* param);
#else
   static DWORD WINAPI worker(LPVOID param);
   void WaitThreadExit(HANDLE &hThread);//强制退出线程
#endif

   pthread_t m_WorkerThread;

private:
   CSndUList* m_pSndUList;		// List of UDT instances for data sending
   CChannel* m_pChannel;                // The UDP channel for data sending
   CTimer* m_pTimer;			// Timing facility

   pthread_mutex_t m_WindowLock;
   pthread_cond_t m_WindowCond;

   volatile bool m_bClosing;		// closing the worker
   pthread_cond_t m_ExitCond;

   volatile bool m_bIFJVP2P;
   volatile bool m_bIFWAIT;

private:
   CSndQueue(const CSndQueue&);
   CSndQueue& operator=(const CSndQueue&);
};


struct SERVER_INFO
{
	UDTSOCKET m_iID;//UDT 编号
	int nSVer;//服务器版本号
	
	unsigned int unAddr;
	unsigned short unPort;

	unsigned int unSerAddr;
	SERVER_INFO()
	{
		m_iID = 0;
		nSVer = 0;
		unAddr = 0;
		unPort = 0;
		unSerAddr = 0;
	}
};//服务器返回的数据保存


class CRcvQueue
{
friend class CUDT;
friend class CUDTUnited;

public:
   CRcvQueue();
   ~CRcvQueue();

public:

      // Functionality:
      //    Initialize the receiving queue.
      // Parameters:
      //    1) [in] size: queue size
      //    2) [in] mss: maximum packet size
      //    3) [in] version: IP version
      //    4) [in] hsize: hash table size
      //    5) [in] c: UDP channel to be associated to the queue
      //    6) [in] t: timer
      // Returned value:
      //    None.

   void init(const int& size, const int& payload, const int& version, const int& hsize, const CChannel* c, const CTimer* t);

      // Functionality:
      //    Read a packet for a specific UDT socket id.
      // Parameters:
      //    1) [in] id: Socket ID
      //    2) [out] packet: received packet
      // Returned value:
      //    Data size of the packet

   int recvfrom(const int32_t& id, CPacket& packet);

   int setmaxrecvbuf(int nmax);
private:
#ifndef WIN32
   static void* worker(void* param);
#else
   static DWORD WINAPI worker(LPVOID param);
   void WaitThreadExit(HANDLE &hThread);//强制退出线程
#endif

   pthread_t m_WorkerThread;

private:
   CUnitQueue m_UnitQueue;		// The received packet queue

   CRcvUList* m_pRcvUList;		// List of UDT instances that will read packets from the queue
   CHash* m_pHash;			// Hash table for UDT socket looking up
   CChannel* m_pChannel;		// UDP channel for receving packets
   CTimer* m_pTimer;			// shared timer with the snd queue

   int m_iPayloadSize;                  // packet payload size

   volatile bool m_bClosing;            // closing the workder
   pthread_cond_t m_ExitCond;

private:
   int setListener(const CUDT* u);
   void removeListener(const CUDT* u);

   void setNewEntry(CUDT* u);
   bool ifNewEntry();
   CUDT* getNewEntry();

   void storePkt(const int32_t& id, CPacket* pkt);

private:
   pthread_mutex_t m_LSLock;
//   volatile CUDT* m_pListener;			// pointer to the (unique, if any) listening UDT entity
   CUDT* m_pListener;			// pointer to the (unique, if any) listening UDT entity
   CRendezvousQueue* m_pRendezvousQueue;	// The list of sockets in rendezvous mode

   std::vector<CUDT*> m_vNewEntry;              // newly added entries, to be inserted
   pthread_mutex_t m_IDLock;

   std::map<int32_t, CPacket*> m_mBuffer;	// temporary buffer for rendezvous connection request
   pthread_mutex_t m_PassLock;
   pthread_cond_t m_PassCond;

   pthread_mutex_t m_NatListLock;

public:
	pthread_mutex_t m_UQLock;	//CRcvQueue::worker()和shrink()的互斥

private:
   CRcvQueue(const CRcvQueue&);
   CRcvQueue& operator=(const CRcvQueue&);
};

struct CMultiplexer
{
   CSndQueue* m_pSndQueue;	// The sending queue
   CRcvQueue* m_pRcvQueue;	// The receiving queue
   CChannel* m_pChannel;	// The UDP channel for sending and receiving
   CTimer* m_pTimer;		// The timer

   int m_iPort;			// The UDP port number of this multiplexer
   int m_iIPversion;		// IP version
   int m_iMSS;			// Maximum Segment Size
   int m_iRefCount;		// number of UDT instances that are associated with this multiplexer
   bool m_bReusable;		// if this one can be shared with others
};

#endif
