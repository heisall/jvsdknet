/*****************************************************************************
written by
   Yunhong Gu, last updated 07/10/2009
*****************************************************************************/
//////////////////////////////????
#ifdef WIN32
#define socklen_t int
#endif
////////////////////////////
#ifdef WIN32
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #pragma comment(lib,"Ws2_32.lib")
   #ifdef LEGACY_WIN32
      #include <wspiapi.h>
   #endif
#else
	#include "udt.h"
   #include <unistd.h>
   #include <sys/wait.h>

#ifndef MOBILE_CLIENT
   #include <sys/sysinfo.h>
#endif

#endif
#include <cstring>
#include "api.h"
#include "core.h"

using namespace std;

CUDTSocket::CUDTSocket():
m_Status(INIT),
m_TimeStamp(0),
m_iIPversion(0),
m_pSelfAddr(NULL),
m_pPeerAddr(NULL),
m_pRealSelfAddr(NULL),
m_pRealPeerAddr(NULL),
m_SocketID(0),
m_ListenSocket(0),
m_PeerID(0),
m_iISN(0),
m_pUDT(NULL),
m_pQueuedSockets(NULL),
m_pAcceptSockets(NULL),
m_AcceptCond(),
m_AcceptLock(),
m_uiBackLog(0),
m_nChannelID(0),
m_nPTLinkID(0),
m_nPTYSTNO(0),
m_nPTYSTADDR(0),
m_nMPort(0),
m_nYSTLV(0),
m_nYSTFV(0),
m_uchVirtual(0),
m_nVChannelID(0),
m_nPeerTCP(0),
m_nYSTLV_2(0),
m_nYSTFV_2(0)
{
   #ifndef WIN32
      pthread_mutex_init(&m_AcceptLock, NULL);
      pthread_cond_init(&m_AcceptCond, NULL);
   #else
      m_AcceptLock = CreateMutex(NULL, false, NULL);
      m_AcceptCond = CreateEvent(NULL, false, false, NULL);
   #endif
}

CUDTSocket::~CUDTSocket()
{
   if (AF_INET == m_iIPversion)
   {
      if(m_pSelfAddr != NULL)
	  {
		  delete (sockaddr_in*)m_pSelfAddr;
		  m_pSelfAddr = NULL;
	  }
      if(m_pPeerAddr != NULL) 
	  {
		  delete (sockaddr_in*)m_pPeerAddr;
		  m_pPeerAddr = NULL;
	  }

	  if(m_pRealSelfAddr != NULL)
	  {
		  delete (sockaddr_in*)m_pRealSelfAddr;
		  m_pRealSelfAddr = NULL;
	  }
      if(m_pRealPeerAddr != NULL) 
	  {
		  delete (sockaddr_in*)m_pRealPeerAddr;
		  m_pRealPeerAddr = NULL;
	  }
   }
   else
   {
	   if(m_pSelfAddr != NULL)
	   {
		   delete (sockaddr_in6*)m_pSelfAddr;
		   m_pSelfAddr = NULL;
	   }
	   if(m_pPeerAddr != NULL)
	   {
		   delete (sockaddr_in6*)m_pPeerAddr;
		   m_pPeerAddr = NULL;
	   }

	   if(m_pRealSelfAddr != NULL)
	   {
		   delete (sockaddr_in6*)m_pRealSelfAddr;
		   m_pRealSelfAddr = NULL;
	   }
	   if(m_pRealPeerAddr != NULL)
	   {
		   delete (sockaddr_in6*)m_pRealPeerAddr;
		   m_pRealPeerAddr = NULL;
	   }
   }

   if(m_pUDT != NULL)
   {
	   delete m_pUDT;
	   m_pUDT = NULL;
   }

   if(m_pQueuedSockets != NULL)
   {
	   m_pQueuedSockets->clear();
//	   std::set<UDTSOCKET> tmp = *m_pQueuedSockets;
//	   m_pQueuedSockets->swap(tmp);
	   delete m_pQueuedSockets;
	   m_pQueuedSockets = NULL;
   }
  
   if(m_pAcceptSockets != NULL)
   {
	   m_pAcceptSockets->clear();
//	   std::set<UDTSOCKET> tmp = *m_pAcceptSockets;
//	   m_pAcceptSockets->swap(tmp);
	   delete m_pAcceptSockets;
	   m_pAcceptSockets = NULL;
   }

   #ifndef WIN32
      pthread_mutex_destroy(&m_AcceptLock);
      pthread_cond_destroy(&m_AcceptCond);
   #else
      CloseHandle(m_AcceptLock);
      CloseHandle(m_AcceptCond);
   #endif
}

////////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
CUDTUnited::CUDTUnited():
m_Sockets(),
m_ControlLock(),
m_IDLock(),
m_SocketID(0),
m_vMultiplexer(),
m_MultiplexerLock(),
m_pCache(NULL),
m_bClosing(false),
m_GCStopLock(),
m_GCStopCond(),
m_InitLock(),
m_bGCStatus(false),
m_GCThread(),
m_ClosedSockets()
#else
CUDTUnited::CUDTUnited():
m_Sockets(),
m_ControlLock(),
m_IDLock(),
m_SocketID(0),
m_TLSError(),
m_vMultiplexer(),
m_MultiplexerLock(),
m_pCache(NULL),
m_bClosing(false),
m_GCStopLock(),
m_GCStopCond(),
m_InitLock(),
m_bGCStatus(false),
m_GCThread(),
m_ClosedSockets(),
m_curConNum(0)
#endif
{
   srand((unsigned int)CTimer::getTime());
   m_SocketID = 1 + (int)((1 << 30) * (double(rand()) / RAND_MAX));

   #ifndef WIN32
      pthread_mutex_init(&m_ControlLock, NULL);
      pthread_mutex_init(&m_IDLock, NULL);
      pthread_mutex_init(&m_InitLock, NULL);
   #else
      m_ControlLock = CreateMutex(NULL, false, NULL);
      m_IDLock = CreateMutex(NULL, false, NULL);
      m_InitLock = CreateMutex(NULL, false, NULL);
   #endif

   #ifndef WIN32
//      pthread_key_create(&m_TLSError, TLSDestroy);
   #else
      m_TLSError = TlsAlloc();
      m_TLSLock = CreateMutex(NULL, false, NULL);
   #endif

   m_pCache = new CCache;
}

CUDTUnited::~CUDTUnited()
{
   #ifndef WIN32
      pthread_mutex_destroy(&m_ControlLock);
      pthread_mutex_destroy(&m_IDLock);
      pthread_mutex_destroy(&m_InitLock);
   #else
      CloseHandle(m_ControlLock);
      CloseHandle(m_IDLock);
      CloseHandle(m_InitLock);
   #endif

   #ifndef WIN32
//      pthread_key_delete(m_TLSError);
   #else
      TlsFree(m_TLSError);
      CloseHandle(m_TLSLock);
   #endif

    if(m_pCache != NULL)
	{
		delete m_pCache;
		m_pCache = NULL;
	}

	m_vMultiplexer.clear();
	m_ClosedSockets.clear();
	m_Sockets.clear();

//	std::vector<CMultiplexer> tmp1 = m_vMultiplexer;
//	m_vMultiplexer.swap(tmp1);
//	std::map<UDTSOCKET, CUDTSocket*> tmp2 = m_ClosedSockets;
//	m_ClosedSockets.swap(tmp2);
//	std::map<UDTSOCKET, CUDTSocket*> tmp3 = m_Sockets;
//	m_Sockets.swap(tmp3);
}

int CUDTUnited::startup()
{
   CGuard gcinit(m_InitLock);

   // Global initialization code
   #ifdef WIN32
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(2, 2);
   
		if (0 != WSAStartup(wVersionRequested, &wsaData))
			throw CUDTException(1, 0,  WSAGetLastError());
   #endif

   if (m_bGCStatus)
      return true;

   m_bClosing = false;
   #ifndef WIN32
      pthread_mutex_init(&m_GCStopLock, NULL);
      pthread_cond_init(&m_GCStopCond, NULL);
	  //////////////////////////////////////////////////////////////////////////
//	  pthread_condattr_t condattr;
//	  int ret = pthread_condattr_init(&condattr);
//	  if (ret == 0) 
//	  {
//		  pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
//	  }
//	  pthread_cond_init(&m_GCStopCond, &condattr);
	  //////////////////////////////////////////////////////////////////////////
      pthread_create(&m_GCThread, NULL, garbageCollect, this);
   #else
      m_GCStopLock = CreateMutex(NULL, false, NULL);
      m_GCStopCond = CreateEvent(NULL, false, false, NULL);
      DWORD ThreadID;
      m_GCThread = CreateThread(NULL, 0, garbageCollect, this, NULL, &ThreadID);
   #endif

   m_bGCStatus = true;

   return 0;
}

int CUDTUnited::cleanup()
{
   CGuard gcinit(m_InitLock);

   if (!m_bGCStatus)
      return 0;

   m_bClosing = true;
   #ifndef WIN32
      pthread_cond_signal(&m_GCStopCond);
      pthread_join(m_GCThread, NULL);
      pthread_mutex_destroy(&m_GCStopLock);
      pthread_cond_destroy(&m_GCStopCond);
   #else
      SetEvent(m_GCStopCond);
      WaitForSingleObject(m_GCThread, 1000);//INFINITE);
	  WaitThreadExit(m_GCThread);//wm
      CloseHandle(m_GCThread);
      CloseHandle(m_GCStopLock);
      CloseHandle(m_GCStopCond);
   #endif

   m_bGCStatus = false;

   // Global destruction code
   #ifdef WIN32
      WSACleanup();
   #endif

   return 0;
}

UDTSOCKET CUDTUnited::newSocket(const int& af, const int& type)
{
   if ((type != SOCK_STREAM) && (type != SOCK_DGRAM))
      throw CUDTException(5, 3, 0);

   CUDTSocket* ns = NULL;

   try
   {
      ns = new CUDTSocket;
      ns->m_pUDT = new CUDT;
      if (AF_INET == af)
      {
         ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
         ((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
      }
      else
      {
         ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
         ((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
      }
   }
   catch (...)
   {
      delete ns;
	  ns = NULL;
      throw CUDTException(3, 2, 0);
   }

   CGuard::enterCS(m_IDLock);
   ns->m_SocketID = -- m_SocketID;
   CGuard::leaveCS(m_IDLock);

   ns->m_Status = CUDTSocket::INIT;
   ns->m_ListenSocket = 0;
   ns->m_pUDT->m_SocketID = ns->m_SocketID;
   ns->m_pUDT->m_iSockType = (SOCK_STREAM == type) ? UDT_STREAM : UDT_DGRAM;
   ns->m_pUDT->m_iIPversion = ns->m_iIPversion = af;
   ns->m_pUDT->m_pCache = m_pCache;

   // protect the m_Sockets structure.
   CGuard::enterCS(m_ControlLock);
   try
   {
      m_Sockets[ns->m_SocketID] = ns;
   }
   catch (...)
   {
      //failure and rollback
      delete ns;
      ns = NULL;
   }
   CGuard::leaveCS(m_ControlLock);

   if (NULL == ns)
      throw CUDTException(3, 2, 0);

   return ns->m_SocketID;
}

static unsigned long get_mem(void)
{
#ifndef WIN32
	char name[128] = {0};
	unsigned long freeSize = 0;
	unsigned long cacheSize = 0;
	char name2[128] = {0};
	char buf[256] = {0};
	FILE* fd = NULL;
	fd = fopen("/proc/meminfo","r");
	if(fd != NULL)
	{
		fgets(buf,sizeof(buf),fd);//toal
		fgets(buf,sizeof(buf),fd);//free
		sscanf(buf,"%s %u %s",name,&freeSize,name2);
		//printf("name:%s, size:%d, name2:%s\n",name,freeSize,name2);
		
		fgets(buf,sizeof(buf),fd);//buff
		fgets(buf,sizeof(buf),fd);//cache
		sscanf(buf,"%s %u %s",name,&cacheSize,name2);
		//printf("name:%s, size:%d, name2:%s\n",name,cacheSize,name2);
		fclose(fd);
	}
	
	return (freeSize+cacheSize);
#else
	
#endif
}

int CUDTUnited::newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs)
{
   CUDTSocket* ns = NULL;
   CUDTSocket* ls = locate(listen);

   if (NULL == ls)
      return -1;

   // if this connection has already been processed
   if (NULL != (ns = locate(listen, peer, hs->m_iID, hs->m_iISN)))
   {
      if (ns->m_pUDT->m_bBroken)
      {
         // last connection from the "peer" address has been broken
         ns->m_Status = CUDTSocket::CLOSED;
         ns->m_TimeStamp = CTimer::getTime();

         CGuard::enterCS(ls->m_AcceptLock);
         ls->m_pQueuedSockets->erase(ns->m_SocketID);
         ls->m_pAcceptSockets->erase(ns->m_SocketID);
         CGuard::leaveCS(ls->m_AcceptLock);
      }
      else
      {
         // connection already exist, this is a repeated connection request
         // respond with existing HS information

         hs->m_iISN = ns->m_pUDT->m_iISN;
         hs->m_iMSS = ns->m_pUDT->m_iMSS;
         hs->m_iFlightFlagSize = ns->m_pUDT->m_iFlightFlagSize;
         hs->m_iReqType = -1;
         hs->m_iID = ns->m_SocketID;

         return 0;

         //except for this situation a new connection should be started
      }
   }

   // exceeding backlog, refuse the connection request
   if (ls->m_pQueuedSockets->size() >= ls->m_uiBackLog)
      return -1;

//////////////////////////////////////////////////////////////////////////
   //获取系统内存是否充足
#ifndef WIN32
#ifndef MOBILE_CLIENT
//   struct sysinfo sysInf;
//   sysinfo(&sysInf);
//   int nFreeMemAmount = sysInf.freeram;//+sysInf.bufferram;
//   printf("freemem:%d\n", nFreeMemAmount);
   //unsigned long nFreeMemAmount = get_mem();
   unsigned long nFreeMemAmount = get_mem();

   int nlimit = 4;
   if(!ls->m_pUDT->m_bIFConnLimit)
   {
	   nlimit = 1;
   }
   if(nFreeMemAmount<nlimit*1024)//6000
   {
       printf("no enough memory(%dM) for newconnect!!!!!!!\n",nlimit);
       return -1;
   }

   if(hs->m_uchVirtual == 1 && hs->m_nChannelID == -2 && nFreeMemAmount<15*1024)//15000
   {
	   printf("no enough memory(15M) for newvirtualconnect!!!!!!!\n");
       return -1;
   }
#endif
#endif
//////////////////////////////////////////////////////////////////////////

   try
   {
      ns = new CUDTSocket;
      ns->m_pUDT = new CUDT(*(ls->m_pUDT));
      if (AF_INET == ls->m_iIPversion)
      {
         ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
         ((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
         ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
         memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in));

		 if(hs->m_piRealSelfIP[0] != 0 || hs->m_piRealSelfIP[1] != 0 || hs->m_piRealSelfIP[2] != 0 || hs->m_piRealSelfIP[3] != 0 && hs->m_iRealSelfPort > 0)
		 {
			 //..
			 ns->m_pRealSelfAddr = (sockaddr*)(new sockaddr_in);
			 ns->m_pRealPeerAddr = (sockaddr*)(new sockaddr_in);
			 CIPAddress::pton(ns->m_pRealPeerAddr, hs->m_piRealSelfIP, ls->m_iIPversion);
			 CIPAddress::pton(ns->m_pRealSelfAddr, hs->m_piRealPeerIP, ls->m_iIPversion);
			 ((sockaddr_in*)ns->m_pRealPeerAddr)->sin_port = htons(hs->m_iRealSelfPort);
			 ((sockaddr_in*)ns->m_pRealSelfAddr)->sin_port = htons(hs->m_iRealPeerPort);
		 }
      }
      else
      {
         ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
         ((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
         ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
         memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in6));

		 if(hs->m_piRealSelfIP[0] != 0 || hs->m_piRealSelfIP[1] != 0 || hs->m_piRealSelfIP[2] != 0 || hs->m_piRealSelfIP[3] != 0 && hs->m_iRealSelfPort > 0)
		 {
			 //..
			 ns->m_pRealSelfAddr = (sockaddr*)(new sockaddr_in6);
			 ns->m_pRealPeerAddr = (sockaddr*)(new sockaddr_in6);
			 CIPAddress::pton(ns->m_pRealPeerAddr, hs->m_piRealSelfIP, ls->m_iIPversion);
			 CIPAddress::pton(ns->m_pRealSelfAddr, hs->m_piRealPeerIP, ls->m_iIPversion);
			 ((sockaddr_in*)ns->m_pRealPeerAddr)->sin_port = htons(hs->m_iRealSelfPort);
			 ((sockaddr_in*)ns->m_pRealSelfAddr)->sin_port = htons(hs->m_iRealPeerPort);
		 }
      }
   }
   catch (...)
   {
      delete ns;
	  ns = NULL;
      return -1;
   }

   CGuard::enterCS(m_IDLock);
   ns->m_SocketID = -- m_SocketID;
   CGuard::leaveCS(m_IDLock);

   ns->m_ListenSocket = listen;
   ns->m_iIPversion = ls->m_iIPversion;
   ns->m_pUDT->m_SocketID = ns->m_SocketID;
   ns->m_PeerID = hs->m_iID;
   ns->m_iISN = hs->m_iISN;

   ns->m_nChannelID = hs->m_nChannelID;//...
   ns->m_nPTLinkID = hs->m_nPTLinkID;
   ns->m_nPTYSTNO = hs->m_nPTYSTNO;
   ns->m_nPTYSTADDR = hs->m_nPTYSTADDR;

   ns->m_nYSTFV = hs->m_nYSTLV;
   ns->m_nYSTLV = hs->m_nYSTFV;
   ns->m_nYSTFV_2 = hs->m_nYSTLV_2;
   ns->m_nYSTLV_2 = hs->m_nYSTFV_2;

   ns->m_uchVirtual = hs->m_uchVirtual;
   ns->m_nVChannelID = hs->m_nVChannelID;
   
   ns->m_nPeerTCP = hs->m_uchLTCP;

   int error = 0;

   try
   {
      // bind to the same addr of listening socket
      ns->m_pUDT->open();
      updateMux(ns->m_pUDT, ns, ls);
      ns->m_pUDT->connect(peer, hs);
   }
   catch (...)
   {
      error = 1;
      goto ERR_ROLLBACK;
   }

   ns->m_Status = CUDTSocket::CONNECTED;

   // copy address information of local node
   ns->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(ns->m_pSelfAddr);
   CIPAddress::pton(ns->m_pSelfAddr, ns->m_pUDT->m_piSelfIP, ns->m_iIPversion);

   // protect the m_Sockets structure.
   CGuard::enterCS(m_ControlLock);
   try
   {
      m_Sockets[ns->m_SocketID] = ns;
	  m_curConNum++;
   }
   catch (...)
   {
      error = 2;
   }
   CGuard::leaveCS(m_ControlLock);

   CGuard::enterCS(ls->m_AcceptLock);
   try
   {
      ls->m_pQueuedSockets->insert(ns->m_SocketID);
   }
   catch (...)
   {
      error = 3;
   }
   CGuard::leaveCS(ls->m_AcceptLock);

   CTimer::triggerEvent();

   ERR_ROLLBACK:
   if (error > 0)
   {
      ns->m_pUDT->close();
      ns->m_Status = CUDTSocket::CLOSED;
      ns->m_TimeStamp = CTimer::getTime();

      return -1;
   }

   // wake up a waiting accept() call
   #ifndef WIN32
      pthread_mutex_lock(&(ls->m_AcceptLock));
      pthread_cond_signal(&(ls->m_AcceptCond));
      pthread_mutex_unlock(&(ls->m_AcceptLock));
   #else
      SetEvent(ls->m_AcceptCond);
   #endif

   return 1;
}

CUDT* CUDTUnited::lookup(const UDTSOCKET u)
{
   // protects the m_Sockets structure
   CGuard cg(m_ControlLock);

   map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

   if ((i == m_Sockets.end()) || (i->second->m_Status == CUDTSocket::CLOSED))
   {
	#ifdef WIN32
	   throw CUDTException(5, 4, 0);
	#else
	   return NULL;
	#endif
   }

   return i->second->m_pUDT;
}

CUDTSocket::UDTSTATUS CUDTUnited::getStatus(const UDTSOCKET u)
{
   // protects the m_Sockets structure
   CGuard cg(m_ControlLock);

   map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

   if (i == m_Sockets.end())
      return CUDTSocket::INIT;

   if (i->second->m_pUDT->m_bBroken)
      return CUDTSocket::BROKEN;

   return i->second->m_Status;   
}

int CUDTUnited::bind(const UDTSOCKET u, const sockaddr* name, const int& namelen)
{
   CUDTSocket* s = locate(u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   // cannot bind a socket more than once
   if (CUDTSocket::INIT != s->m_Status)
      throw CUDTException(5, 0, 0);

   // check the size of SOCKADDR structure
   if (AF_INET == s->m_iIPversion)
   {
      if (namelen != sizeof(sockaddr_in))
         throw CUDTException(5, 3, 0);
   }
   else
   {
      if (namelen != sizeof(sockaddr_in6))
         throw CUDTException(5, 3, 0);
   }

   s->m_pUDT->open();
   updateMux(s->m_pUDT, s, name);
   s->m_Status = CUDTSocket::OPENED;

   // copy address information of local node
   s->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(s->m_pSelfAddr);

   return 0;
}

int CUDTUnited::bind(UDTSOCKET u, UDPSOCKET udpsock)
{
   CUDTSocket* s = locate(u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   // cannot bind a socket more than once
   if (CUDTSocket::INIT != s->m_Status)
      throw CUDTException(5, 0, 0);

   sockaddr_in name4;
   sockaddr_in6 name6;
   sockaddr* name;
   socklen_t namelen;

   if (AF_INET == s->m_iIPversion)
   {
      namelen = sizeof(sockaddr_in);
      name = (sockaddr*)&name4;
   }
   else
   {
      namelen = sizeof(sockaddr_in6);
      name = (sockaddr*)&name6;
   }

   if (-1 == ::getsockname(udpsock, name, &namelen))
      throw CUDTException(5, 3);

   s->m_pUDT->open();
   updateMux(s->m_pUDT, s, name, &udpsock);
   s->m_Status = CUDTSocket::OPENED;

   // copy address information of local node
   s->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(s->m_pSelfAddr);

   return 0;
}

int CUDTUnited::listen(const UDTSOCKET u, const int& backlog)
{
   CUDTSocket* s = locate(u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   // do nothing if the socket is already listening
   if (CUDTSocket::LISTENING == s->m_Status)
      return 0;

   // a socket can listen only if is in OPENED status
   if (CUDTSocket::OPENED != s->m_Status)
      throw CUDTException(5, 5, 0);

   // listen is not supported in rendezvous connection setup
   if (s->m_pUDT->m_bRendezvous)
      throw CUDTException(5, 7, 0);

   if (backlog <= 0)
      throw CUDTException(5, 3, 0);

   s->m_uiBackLog = backlog;

   try
   {
      s->m_pQueuedSockets = new set<UDTSOCKET>;
      s->m_pAcceptSockets = new set<UDTSOCKET>;
   }
   catch (...)
   {
	   s->m_pQueuedSockets->clear();
	   
//	   std::set<UDTSOCKET> tmp = *(s->m_pQueuedSockets);
//	   s->m_pQueuedSockets->swap(tmp);
      delete s->m_pQueuedSockets;
	  s->m_pQueuedSockets = NULL;

	  s->m_pAcceptSockets->clear();//
	  delete s->m_pAcceptSockets;//
	  s->m_pAcceptSockets = NULL;//
      throw CUDTException(3, 2, 0);
   }

   s->m_pUDT->listen();

   s->m_Status = CUDTSocket::LISTENING;

   return 0;
}

UDTSOCKET CUDTUnited::accept(const UDTSOCKET listen, sockaddr* addr, int* addrlen)
{
   if ((NULL != addr) && (NULL == addrlen))
      throw CUDTException(5, 3, 0);

   CUDTSocket* ls = locate(listen);

   if (ls == NULL)
      throw CUDTException(5, 4, 0);

   // the "listen" socket must be in LISTENING status
   if (CUDTSocket::LISTENING != ls->m_Status)
      throw CUDTException(5, 6, 0);

   // no "accept" in rendezvous connection setup
   if (ls->m_pUDT->m_bRendezvous)
      throw CUDTException(5, 7, 0);

   UDTSOCKET u = CUDT::INVALID_SOCK;
   bool accepted = false;

   // !!only one conection can be set up each time!!
   #ifndef WIN32
      while (!accepted)
      {
         pthread_mutex_lock(&(ls->m_AcceptLock));

         if (ls->m_pQueuedSockets->size() > 0)
         {
            u = *(ls->m_pQueuedSockets->begin());
            ls->m_pAcceptSockets->insert(ls->m_pAcceptSockets->end(), u);
            ls->m_pQueuedSockets->erase(ls->m_pQueuedSockets->begin());

            accepted = true;
         }
         else if (!ls->m_pUDT->m_bSynRecving)
            accepted = true;
         else if (CUDTSocket::LISTENING == ls->m_Status)
            pthread_cond_wait(&(ls->m_AcceptCond), &(ls->m_AcceptLock));

         if (CUDTSocket::LISTENING != ls->m_Status)
            accepted = true;

         pthread_mutex_unlock(&(ls->m_AcceptLock));
      }
   #else
      while (!accepted)
      {
         WaitForSingleObject(ls->m_AcceptLock, INFINITE);

         if (ls->m_pQueuedSockets->size() > 0)
         {
            u = *(ls->m_pQueuedSockets->begin());
            ls->m_pAcceptSockets->insert(ls->m_pAcceptSockets->end(), u);
            ls->m_pQueuedSockets->erase(ls->m_pQueuedSockets->begin());

            accepted = true;
         }
         else if (!ls->m_pUDT->m_bSynRecving)
            accepted = true;

         ReleaseMutex(ls->m_AcceptLock);

         if  (!accepted & (CUDTSocket::LISTENING == ls->m_Status))
            WaitForSingleObject(ls->m_AcceptCond, INFINITE);

         if (CUDTSocket::LISTENING != ls->m_Status)
         {
            SetEvent(ls->m_AcceptCond);
            accepted = true;
         }
      }
   #endif

   if (u == CUDT::INVALID_SOCK)
   {
      // non-blocking receiving, no connection available
      if (!ls->m_pUDT->m_bSynRecving)
	  {
		  //throw CUDTException(6, 2, 0);
		  return u;
	  }
	  else
	  {
		  // listening socket is closed
		  throw CUDTException(5, 6, 0);
	  }
   }

   if (AF_INET == locate(u)->m_iIPversion)
      *addrlen = sizeof(sockaddr_in);
   else
      *addrlen = sizeof(sockaddr_in6);

   // copy address information of peer node
   memcpy(addr, locate(u)->m_pPeerAddr, *addrlen);

   return u;
}

int CUDTUnited::connect(STJUDTCONN stJUDTCONN)
{
   CUDTSocket* s = locate(stJUDTCONN.u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   // check the size of SOCKADDR structure
   if (AF_INET == s->m_iIPversion)
   {
      if (stJUDTCONN.namelen != sizeof(sockaddr_in))
         throw CUDTException(5, 3, 0);
   }
   else
   {
      if (stJUDTCONN.namelen != sizeof(sockaddr_in6))
         throw CUDTException(5, 3, 0);
   }

   // a socket can "connect" only if it is in INIT or OPENED status
   if (CUDTSocket::INIT == s->m_Status)
   {
      if (!s->m_pUDT->m_bRendezvous)
      {
         s->m_pUDT->open();
         updateMux(s->m_pUDT, s);
         s->m_Status = CUDTSocket::OPENED;
      }
      else
         throw CUDTException(5, 8, 0);
   }
   else if (CUDTSocket::OPENED != s->m_Status)
      throw CUDTException(5, 2, 0);

   // copy address information of local node
   s->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(s->m_pSelfAddr);
   CIPAddress::pton(s->m_pSelfAddr, s->m_pUDT->m_piSelfIP, s->m_iIPversion);

   s->m_pUDT->connect(stJUDTCONN.name, 
	                  stJUDTCONN.nChannelID, 
					  stJUDTCONN.chGroup, 
					  stJUDTCONN.nYSTNO, 
					  stJUDTCONN.nPTLinkID, 
					  stJUDTCONN.nPTYSTNO, 
					  stJUDTCONN.nPTYSTADDR, 
					  stJUDTCONN.chCheckGroup,
					  stJUDTCONN.nCheckYSTNO,
					  stJUDTCONN.nLVer_new,
					  stJUDTCONN.nLVer_old,
					  stJUDTCONN.nMinTime,
					  stJUDTCONN.uchVirtual,
					  stJUDTCONN.nVChannelID,
                      stJUDTCONN.uchLLTCP,
                      stJUDTCONN.pbQuickExit);
   s->m_Status = CUDTSocket::CONNECTED;
   
   s->m_nYSTFV = s->m_pUDT->m_nYSTFV;
   s->m_nYSTLV = s->m_pUDT->m_nYSTLV;
   s->m_nYSTFV_2 = s->m_pUDT->m_nYSTFV_2;
   s->m_nYSTLV_2 = s->m_pUDT->m_nYSTLV_2;

   s->m_nPeerTCP = (s->m_pUDT->m_bFTCP?1:0);
   // record peer address
   if (AF_INET == s->m_iIPversion)
   {
      s->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
      memcpy(s->m_pPeerAddr, stJUDTCONN.name, sizeof(sockaddr_in));

	  if(s->m_pUDT->m_piRealPeerIP[0] != 0 || s->m_pUDT->m_piRealPeerIP[1] != 0 || s->m_pUDT->m_piRealPeerIP[2] != 0 || s->m_pUDT->m_piRealPeerIP[3] != 0 && s->m_pUDT->m_iRealPeerPort > 0)
	  {
		  s->m_pRealPeerAddr = (sockaddr*)(new sockaddr_in);
		  s->m_pRealSelfAddr = (sockaddr*)(new sockaddr_in);
		  CIPAddress::pton(s->m_pRealSelfAddr, s->m_pUDT->m_piRealSelfIP, s->m_iIPversion);//..
		  CIPAddress::pton(s->m_pRealPeerAddr, s->m_pUDT->m_piRealPeerIP, s->m_iIPversion);//..
		  ((sockaddr_in*)s->m_pRealPeerAddr)->sin_port = htons(s->m_pUDT->m_iRealPeerPort);
		  ((sockaddr_in*)s->m_pRealSelfAddr)->sin_port = htons(s->m_pUDT->m_iRealSelfPort);
	  }
   }
   else
   {
      s->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
      memcpy(s->m_pPeerAddr, stJUDTCONN.name, sizeof(sockaddr_in6));

	  if(s->m_pUDT->m_piRealPeerIP[0] != 0 || s->m_pUDT->m_piRealPeerIP[1] != 0 || s->m_pUDT->m_piRealPeerIP[2] != 0 || s->m_pUDT->m_piRealPeerIP[3] != 0 && s->m_pUDT->m_iRealPeerPort > 0)
	  {
		  s->m_pRealPeerAddr = (sockaddr*)(new sockaddr_in6);
		  s->m_pRealSelfAddr = (sockaddr*)(new sockaddr_in6);
		  CIPAddress::pton(s->m_pRealSelfAddr, s->m_pUDT->m_piRealSelfIP, s->m_iIPversion);//..
		  CIPAddress::pton(s->m_pRealPeerAddr, s->m_pUDT->m_piRealPeerIP, s->m_iIPversion);//..
		  ((sockaddr_in*)s->m_pRealPeerAddr)->sin_port = htons(s->m_pUDT->m_iRealPeerPort);
		  ((sockaddr_in*)s->m_pRealSelfAddr)->sin_port = htons(s->m_pUDT->m_iRealSelfPort);
	  }
   }

   return 0;
}

int CUDTUnited::close(const UDTSOCKET u)
{
   CUDTSocket* s = locate(u);

   // silently drop a request to close an invalid ID, rather than return error   
   if (NULL == s)
      return 0;

   s->m_pUDT->close();

   // a socket will not be immediated removed when it is closed
   // in order to prevent other methods from accessing invalid address
   // a timer is started and the socket will be removed after approximately 1 second
   s->m_TimeStamp = CTimer::getTime();

   CUDTSocket::UDTSTATUS os = s->m_Status;

   // synchronize with garbage collection.
   CGuard::enterCS(m_ControlLock);

   s->m_Status = CUDTSocket::CLOSED;

   m_Sockets.erase(s->m_SocketID);
   m_ClosedSockets[s->m_SocketID] = s;

   if (0 != s->m_ListenSocket)
   {
	   // if it is an accepted socket, remove it from the listener's queue
	   map<UDTSOCKET, CUDTSocket*>::iterator ls = m_Sockets.find(s->m_ListenSocket);
	   if (ls != m_Sockets.end())
	   {
		   CGuard::enterCS(ls->second->m_AcceptLock);
		   ls->second->m_pAcceptSockets->erase(s->m_SocketID);
		   CGuard::leaveCS(ls->second->m_AcceptLock);
	   }
   }

   CGuard::leaveCS(m_ControlLock);

   // broadcast all "accept" waiting
   if (CUDTSocket::LISTENING == os)
   {
      #ifndef WIN32
         pthread_mutex_lock(&(s->m_AcceptLock));
         pthread_mutex_unlock(&(s->m_AcceptLock));
         pthread_cond_broadcast(&(s->m_AcceptCond));
      #else
         SetEvent(s->m_AcceptCond);
      #endif
   }

   CTimer::triggerEvent();

   return 0;
}

int CUDTUnited::getpeername(const UDTSOCKET u, sockaddr* name, int* namelen)
{
   if (CUDTSocket::CONNECTED != getStatus(u))
      throw CUDTException(2, 2, 0);

   CUDTSocket* s = locate(u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
      throw CUDTException(2, 2, 0);

   if (AF_INET == s->m_iIPversion)
      *namelen = sizeof(sockaddr_in);
   else
      *namelen = sizeof(sockaddr_in6);

   // copy address information of peer node
   memcpy(name, s->m_pPeerAddr, *namelen);

   return 0;
}

int CUDTUnited::getrealpeername(const UDTSOCKET u, sockaddr* name, int* namelen)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	if (AF_INET == s->m_iIPversion)
		*namelen = sizeof(sockaddr_in);
	else
		*namelen = sizeof(sockaddr_in6);
	
	// copy address information of peer node
	memcpy(name, s->m_pRealPeerAddr, *namelen);
	
	return 0;
}

int CUDTUnited::getsockname(const UDTSOCKET u, sockaddr* name, int* namelen)
{
   CUDTSocket* s = locate(u);

   if (NULL == s)
      throw CUDTException(5, 4, 0);

   if (CUDTSocket::INIT == s->m_Status)
      throw CUDTException(2, 2, 0);

   if (AF_INET == s->m_iIPversion)
      *namelen = sizeof(sockaddr_in);
   else
      *namelen = sizeof(sockaddr_in6);

   // copy address information of local node
   memcpy(name, s->m_pSelfAddr, *namelen);

   return 0;
}

int CUDTUnited::getrealsockname(const UDTSOCKET u, sockaddr* name, int* namelen)
{
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (CUDTSocket::INIT == s->m_Status)
		throw CUDTException(2, 2, 0);
	
	if (AF_INET == s->m_iIPversion)
		*namelen = sizeof(sockaddr_in);
	else
		*namelen = sizeof(sockaddr_in6);
	
	// copy address information of local node
	memcpy(name, s->m_pRealSelfAddr, *namelen);
	
	return 0;
}

int CUDTUnited::getschannel(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nChannelID;
}

int CUDTUnited::getvchannel(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nVChannelID;
}

unsigned char CUDTUnited::getvirtual(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_uchVirtual;
}

int CUDTUnited::getptystno(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nPTYSTNO;
}

int CUDTUnited::getptystaddr(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nPTYSTADDR;
}

int CUDTUnited::getptystid(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nPTLinkID;
}

int CUDTUnited::gettranstype(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	if(s->m_pRealPeerAddr != NULL)
	{
		return 1;
	}
	return 0;
}

int CUDTUnited::getystverF(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	if(s->m_nYSTFV_2 > 0 && s->m_nYSTFV_2 < 20501212)
	{//新版本号有效，则优先使用新版本。因为新产品肯定都是这种。
		return s->m_nYSTFV_2;
	}
	return s->m_nYSTFV;
}

int CUDTUnited::getpeertcp(const UDTSOCKET u)
{
	if (CUDTSocket::CONNECTED != getStatus(u))
		throw CUDTException(2, 2, 0);
	
	CUDTSocket* s = locate(u);
	
	if (NULL == s)
		throw CUDTException(5, 4, 0);
	
	if (!s->m_pUDT->m_bConnected || s->m_pUDT->m_bBroken)
		throw CUDTException(2, 2, 0);
	
	return s->m_nPeerTCP;
}

int CUDTUnited::select(ud_set* readfds, ud_set* writefds, ud_set* exceptfds, const timeval* timeout)
{
   uint64_t entertime = CTimer::getTime();

   uint64_t to;
   if (NULL == timeout)
      to = (__int64)1 << 62;//0xFFFFFFFFFFFFFFFF;//ULL;//0xFFFFFFFF;//
   else
      to = timeout->tv_sec * 1000000 + timeout->tv_usec;

   // initialize results
   int count = 0;
   set<UDTSOCKET> rs, ws, es;

   // retrieve related UDT sockets
   vector<CUDTSocket*> ru, wu, eu;
   CUDTSocket* s;
   if (NULL != readfds)
      for (set<UDTSOCKET>::iterator i1 = readfds->begin(); i1 != readfds->end(); ++ i1)
      {
         if (CUDTSocket::BROKEN == getStatus(*i1))
         {
            rs.insert(*i1);
            ++ count;
         }
         else if (NULL == (s = locate(*i1)))
            throw CUDTException(5, 4, 0);
         else
            ru.insert(ru.end(), s);
      }
   if (NULL != writefds)
      for (set<UDTSOCKET>::iterator i2 = writefds->begin(); i2 != writefds->end(); ++ i2)
      {
         if (CUDTSocket::BROKEN == getStatus(*i2))
         {
            ws.insert(*i2);
            ++ count;
         }
         else if (NULL == (s = locate(*i2)))
            throw CUDTException(5, 4, 0);
         else
            wu.insert(wu.end(), s);
      }
   if (NULL != exceptfds)
      for (set<UDTSOCKET>::iterator i3 = exceptfds->begin(); i3 != exceptfds->end(); ++ i3)
      {
         if (CUDTSocket::BROKEN == getStatus(*i3))
         {
            es.insert(*i3);
            ++ count;
         }
         else if (NULL == (s = locate(*i3)))
            throw CUDTException(5, 4, 0);
         else
            eu.insert(eu.end(), s);
      }

   do
   {
      // query read sockets
      for (vector<CUDTSocket*>::iterator j1 = ru.begin(); j1 != ru.end(); ++ j1)
      {
         s = *j1;

         if ((s->m_pUDT->m_bConnected && (s->m_pUDT->m_pRcvBuffer->getRcvDataSize() > 0) && ((s->m_pUDT->m_iSockType == UDT_STREAM) || (s->m_pUDT->m_pRcvBuffer->getRcvMsgNum() > 0)))
            || (!s->m_pUDT->m_bListening && (s->m_pUDT->m_bBroken || !s->m_pUDT->m_bConnected))
            || (s->m_pUDT->m_bListening && (s->m_pQueuedSockets->size() > 0))
            || (s->m_Status == CUDTSocket::CLOSED))
         {
            rs.insert(s->m_SocketID);
            ++ count;
         }
      }

      // query write sockets
      for (vector<CUDTSocket*>::iterator j2 = wu.begin(); j2 != wu.end(); ++ j2)
      {
         s = *j2;

         if ((s->m_pUDT->m_bConnected && (s->m_pUDT->m_pSndBuffer->getCurrBufSize() < s->m_pUDT->m_iSndBufSize))
            || s->m_pUDT->m_bBroken || !s->m_pUDT->m_bConnected || (s->m_Status == CUDTSocket::CLOSED))
         {
            ws.insert(s->m_SocketID);
            ++ count;
         }
      }

      // query exceptions on sockets
      for (vector<CUDTSocket*>::iterator j3 = eu.begin(); j3 != eu.end(); ++ j3)
      {
         // check connection request status, not supported now
      }

      if (0 < count)
         break;

      CTimer::waitForEvent();
   } while (to > CTimer::getTime() - entertime);

   if (NULL != readfds)
      *readfds = rs;

   if (NULL != writefds)
      *writefds = ws;

   if (NULL != exceptfds)
      *exceptfds = es;

   return count;
}

int CUDTUnited::selectEx(const vector<UDTSOCKET>& fds, vector<UDTSOCKET>* readfds, vector<UDTSOCKET>* writefds, vector<UDTSOCKET>* exceptfds, int64_t msTimeOut)
{
   uint64_t entertime = CTimer::getTime();

   uint64_t to;
   if (msTimeOut >= 0)
      to = msTimeOut * 1000;
   else
      to = (__int64)1 << 62;//0xFFFFFFFFFFFFFFFF;//ULL;//0xFFFFFFFF;//

   // initialize results
   int count = 0;
   if (NULL != readfds)
      readfds->clear();
   if (NULL != writefds)
      writefds->clear();
   if (NULL != exceptfds)
      exceptfds->clear();

   do
   {
      for (vector<UDTSOCKET>::const_iterator i = fds.begin(); i != fds.end(); ++ i)
      {
         CUDTSocket* s = locate(*i);

         if ((NULL == s) || s->m_pUDT->m_bBroken || (s->m_Status == CUDTSocket::CLOSED))
         {
            if (NULL != exceptfds)
            {
               exceptfds->push_back(*i);
               ++ count;
            }
            continue;
         }

         if (NULL != readfds)
         {
            if ((s->m_pUDT->m_bConnected && (s->m_pUDT->m_pRcvBuffer->getRcvDataSize() > 0) && ((s->m_pUDT->m_iSockType == UDT_STREAM) || (s->m_pUDT->m_pRcvBuffer->getRcvMsgNum() > 0)))
               || (s->m_pUDT->m_bListening && (s->m_pQueuedSockets->size() > 0)))
            {
               readfds->push_back(s->m_SocketID);
               ++ count;
            }
         }

         if (NULL != writefds)
         {
            if (s->m_pUDT->m_bConnected && (s->m_pUDT->m_pSndBuffer->getCurrBufSize() < s->m_pUDT->m_iSndBufSize))
            {
               writefds->push_back(s->m_SocketID);
               ++ count;
            }
         }
      }

      if (count > 0)
         break;

      CTimer::waitForEvent();
   } while (to > CTimer::getTime() - entertime);

   return count;
}

CUDTSocket* CUDTUnited::locate(const UDTSOCKET u)
{
   CGuard cg(m_ControlLock);

   map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

   if ( (i == m_Sockets.end()) || (i->second->m_Status == CUDTSocket::CLOSED))
      return NULL;

   return i->second;
}

CUDTSocket* CUDTUnited::locate(const UDTSOCKET u, const sockaddr* peer, const UDTSOCKET& id, const int32_t& isn)
{
   CGuard cg(m_ControlLock);

   map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

   CGuard ag(i->second->m_AcceptLock);

   // look up the "peer" address in queued sockets set
   for (set<UDTSOCKET>::iterator j1 = i->second->m_pQueuedSockets->begin(); j1 != i->second->m_pQueuedSockets->end(); ++ j1)
   {
      map<UDTSOCKET, CUDTSocket*>::iterator k1 = m_Sockets.find(*j1);
      // this socket might have been closed and moved m_ClosedSockets
      if (k1 == m_Sockets.end())
         continue;

      if (CIPAddress::ipcmp(peer, k1->second->m_pPeerAddr, i->second->m_iIPversion))
      {
         if ((id == k1->second->m_PeerID) && (isn == k1->second->m_iISN))
            return k1->second;
      }
   }

   // look up the "peer" address in accept sockets set
   for (set<UDTSOCKET>::iterator j2 = i->second->m_pAcceptSockets->begin(); j2 != i->second->m_pAcceptSockets->end(); ++ j2)
   {
      map<UDTSOCKET, CUDTSocket*>::iterator k2 = m_Sockets.find(*j2);
      // this socket might have been closed and moved m_ClosedSockets
      if (k2 == m_Sockets.end())
         continue;

      if (CIPAddress::ipcmp(peer, k2->second->m_pPeerAddr, i->second->m_iIPversion))
      {
         if ((id == k2->second->m_PeerID) && (isn == k2->second->m_iISN))
            return k2->second;
      }
   }

   return NULL;
}

void CUDTUnited::checkBrokenSockets()
{
   CGuard cg(m_ControlLock);

   // set of sockets To Be Closed and To Be Removed
   set<UDTSOCKET> tbc;
   set<UDTSOCKET> tbr;

   for (map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.begin(); i != m_Sockets.end(); ++ i)
   {
      // check broken connection
      if (i->second->m_pUDT->m_bBroken)
      {
         // if there is still data in the receiver buffer, wait longer
//         if ((i->second->m_pUDT->m_pRcvBuffer->getRcvDataSize() > 0) && (i->second->m_pUDT->m_iBrokenCounter -- > 0))
//            continue;

		 i->second->m_pUDT->close();//wm
         //close broken connections and start removal timer
         i->second->m_Status = CUDTSocket::CLOSED;
         i->second->m_TimeStamp = CTimer::getTime();
         tbc.insert(i->first);
         m_ClosedSockets[i->first] = i->second;

         // remove from listener's queue
         map<UDTSOCKET, CUDTSocket*>::iterator ls = m_Sockets.find(i->second->m_ListenSocket);
         if (ls == m_Sockets.end())
         {
			 ls = m_ClosedSockets.find(i->second->m_ListenSocket);
			 if (ls == m_ClosedSockets.end())
				 continue;
         }
		 
         CGuard::enterCS(ls->second->m_AcceptLock);
         ls->second->m_pQueuedSockets->erase(i->second->m_SocketID);
         ls->second->m_pAcceptSockets->erase(i->second->m_SocketID);
         CGuard::leaveCS(ls->second->m_AcceptLock);
      }
   }

   for (map<UDTSOCKET, CUDTSocket*>::iterator j = m_ClosedSockets.begin(); j != m_ClosedSockets.end(); ++ j)
   {
	   //wm
   #ifdef WIN32
	   if(j==NULL || j->second==NULL)
	   {
		   continue;
	   }
   #endif
	   //wm
      // timeout 1 second to destroy a socket AND it has been removed from RcvUList
      if ((CTimer::getTime() - j->second->m_TimeStamp > 1000000) && ((NULL == j->second->m_pUDT->m_pRNode) || !j->second->m_pUDT->m_pRNode->m_bOnList))
         tbr.insert(j->first);

      // sockets cannot be removed here because it will invalidate the map iterator
   }

   // move closed sockets to the ClosedSockets structure
   for (set<UDTSOCKET>::iterator k = tbc.begin(); k != tbc.end(); ++ k)
      m_Sockets.erase(*k);

//   if(m_Sockets.empty())
//   {
//	   std::map<UDTSOCKET, CUDTSocket*> tmp = m_Sockets;
//	   m_Sockets.swap(tmp);
//   }
   // remove those timeout sockets
   for (set<UDTSOCKET>::iterator l = tbr.begin(); l != tbr.end(); ++ l)
      removeSocket(*l);

   tbc.clear();
   tbr.clear();

//   std::set<UDTSOCKET> tmp = tbc;
//   tbc.swap(tmp);
//   tbr.swap(tmp);
}

void CUDTUnited::removeSocket(const UDTSOCKET u)
{
   map<UDTSOCKET, CUDTSocket*>::iterator i = m_ClosedSockets.find(u);

   // invalid socket ID
   if (i == m_ClosedSockets.end())
      return;

   // decrease multiplexer reference count, and remove it if necessary
   int port;
   if (AF_INET == i->second->m_iIPversion)
      port = ntohs(((sockaddr_in*)(i->second->m_pSelfAddr))->sin_port);
   else
      port = ntohs(((sockaddr_in6*)(i->second->m_pSelfAddr))->sin6_port);

   if(port <= 0)
   {
	   port = i->second->m_nMPort;//清理重复绑定第二次连接又未建立的特殊情况
   }
   vector<CMultiplexer>::iterator m;
   for (m = m_vMultiplexer.begin(); m != m_vMultiplexer.end(); ++ m)
      if (port == m->m_iPort)
         break;

   if (NULL != i->second->m_pQueuedSockets)
   {
      CGuard::enterCS(i->second->m_AcceptLock);

      // if it is a listener, close all un-accepted sockets in its queue and remove them later
      set<UDTSOCKET> tbc;
      for (set<UDTSOCKET>::iterator q = i->second->m_pQueuedSockets->begin(); q != i->second->m_pQueuedSockets->end(); ++ q)
      {
//		 m_Sockets[*q]->m_pUDT->m_bBroken = true;
         m_Sockets[*q]->m_pUDT->close();
         m_Sockets[*q]->m_TimeStamp = CTimer::getTime();
         m_Sockets[*q]->m_Status = CUDTSocket::CLOSED;
         tbc.insert(*q);
         m_ClosedSockets[*q] = m_Sockets[*q];
      }
      for (set<UDTSOCKET>::iterator c = tbc.begin(); c != tbc.end(); ++ c)
	  {
		  m_Sockets.erase(*c);
		  i->second->m_pQueuedSockets->erase(*c);
	  }

	  tbc.clear();

//	  std::set<UDTSOCKET> tmp = tbc;
//	  tbc.swap(tmp);
      CGuard::leaveCS(i->second->m_AcceptLock);
   }

   // delete this one
   i->second->m_pUDT->close();
   if(m_ClosedSockets[u] != NULL)
   {
	   delete m_ClosedSockets[u];
	   m_curConNum--;
   }
   m_ClosedSockets[u] = NULL;
   m_ClosedSockets.erase(u);

   if (m == m_vMultiplexer.end())
      return;

   m->m_iRefCount --;
   if (0 == m->m_iRefCount)
   {
      m->m_pChannel->close();
      delete m->m_pSndQueue;
	  m->m_pSndQueue = NULL;
      delete m->m_pRcvQueue;
	  m->m_pRcvQueue = NULL;
      delete m->m_pTimer;
	  m->m_pTimer = NULL;
      delete m->m_pChannel;
	  m->m_pChannel = NULL;
      m_vMultiplexer.erase(m);
   }
}

/*
void CUDTUnited::setError(CUDTException* e)
{
   #ifndef WIN32
      delete (CUDTException*)pthread_getspecific(m_TLSError);
	  pthread_setspecific(m_TLSError, e);
   #else
      CGuard tg(m_TLSLock);
      delete (CUDTException*)TlsGetValue(m_TLSError);
      TlsSetValue(m_TLSError, e);
      m_mTLSRecord[GetCurrentThreadId()] = e;
   #endif
}
*/
#ifndef WIN32
void CUDTUnited::setError(const CUDTException& e)
{
	m_e.reset(e);
//	pthread_setspecific(m_TLSError, &m_e);
}
void CUDTUnited::setError(int major, int minor, int err)
{
	m_e.reset(major, minor, err);
//	pthread_setspecific(m_TLSError, &m_e);
}
#else
void CUDTUnited::setError(CUDTException* e)
{
	CGuard tg(m_TLSLock);
	delete (CUDTException*)TlsGetValue(m_TLSError);
	TlsSetValue(m_TLSError, e);
	m_mTLSRecord[GetCurrentThreadId()] = e;
}
#endif

CUDTException* CUDTUnited::getError()
{
   #ifndef WIN32
//      if(NULL == pthread_getspecific(m_TLSError))
//         pthread_setspecific(m_TLSError, new CUDTException);
//      return (CUDTException*)pthread_getspecific(m_TLSError);
	    return &m_e;
   #else
      CGuard tg(m_TLSLock);
      if(NULL == TlsGetValue(m_TLSError))
      {
         CUDTException* e = new CUDTException;
         TlsSetValue(m_TLSError, e);
         m_mTLSRecord[GetCurrentThreadId()] = e;
      }
      return (CUDTException*)TlsGetValue(m_TLSError);
   #endif
}

#ifdef WIN32
typedef HANDLE (WINAPI*OPENTHREAD)(DWORD dwFlag, bool bInheritHandle, DWORD dwThreadId);//add 20091022
OPENTHREAD OpenThread=(OPENTHREAD)GetProcAddress(GetModuleHandle("Kernel32"), "OpenThread");//add 20091022
void CUDTUnited::checkTLSValue()
{
   CGuard tg(m_TLSLock);


   
   vector<DWORD> tbr;
   for (map<DWORD, CUDTException*>::iterator i = m_mTLSRecord.begin(); i != m_mTLSRecord.end(); ++i)
   {
       HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, i->first);
	   if (NULL == h)
	   {
		   tbr.insert(tbr.end(), i->first);
		   break;
	   }
	   if (WAIT_OBJECT_0 == WaitForSingleObject(h, 0))
	   {
		   delete i->second;
		   i->second = NULL;
		   tbr.insert(tbr.end(), i->first);
	   }
	   if(h > 0)
	   {
		   CloseHandle(h);
	   }

   }
   for (vector<DWORD>::iterator j = tbr.begin(); j != tbr.end(); ++ j)
      m_mTLSRecord.erase(*j);

   tbr.clear();

//   std::vector<DWORD> tmp = tbr;
//   tbr.swap(tmp);
}
#endif

void CUDTUnited::updateMux(CUDT* u, CUDTSocket *us, const sockaddr* addr, const UDPSOCKET* udpsock)
{
   CGuard cg(m_ControlLock);

   if ((u->m_bReuseAddr) && (NULL != addr))
   {
      int port = (AF_INET == u->m_iIPversion) ? ntohs(((sockaddr_in*)addr)->sin_port) : ntohs(((sockaddr_in6*)addr)->sin6_port);

      // find a reusable address
      for (vector<CMultiplexer>::iterator i = m_vMultiplexer.begin(); i != m_vMultiplexer.end(); ++ i)
      {
         if ((i->m_iIPversion == u->m_iIPversion) && (i->m_iMSS == u->m_iMSS) && i->m_bReusable)
         {
            if (i->m_iPort == port)
            {
				i->m_pRcvQueue->setmaxrecvbuf(u->m_iTRcvBufSize);//
				i->m_pSndQueue->setifjvp2p(u->m_bIFJVP2P);
				i->m_pSndQueue->setifwait(u->m_bIFWAIT);
               // reuse the existing multiplexer
               ++ i->m_iRefCount;
               u->m_pSndQueue = i->m_pSndQueue;
               u->m_pRcvQueue = i->m_pRcvQueue;
			   us->m_nMPort = port;
               return;
            }
         }
      }
   }

   // a new multiplexer is needed
   CMultiplexer m;
   m.m_iMSS = u->m_iMSS;
   m.m_iIPversion = u->m_iIPversion;
   m.m_iRefCount = 1;
   m.m_bReusable = u->m_bReuseAddr;

   m.m_pChannel = new CChannel(u->m_iIPversion);
   m.m_pChannel->setSndBufSize(u->m_iUDPSndBufSize);
   m.m_pChannel->setRcvBufSize(u->m_iUDPRcvBufSize);

   try
   {
      if (NULL != udpsock)
         m.m_pChannel->open(*udpsock);
      else
         m.m_pChannel->open(addr);
   }
   catch (CUDTException& e)
   {
      m.m_pChannel->close();
      delete m.m_pChannel;
	  m.m_pChannel = NULL;
      throw e;
   }

   sockaddr_in sd4;
   sockaddr_in6 sd6;
   sockaddr* sa = (AF_INET == u->m_iIPversion) ? (sockaddr*)&sd4 : (sockaddr*)&sd6;//(AF_INET == u->m_iIPversion) ? (sockaddr*) new sockaddr_in : (sockaddr*) new sockaddr_in6;
   m.m_pChannel->getSockAddr(sa);
   m.m_iPort = (AF_INET == u->m_iIPversion) ? ntohs(((sockaddr_in*)sa)->sin_port) : ntohs(((sockaddr_in6*)sa)->sin6_port);
   if (AF_INET == u->m_iIPversion)
   {
//	   delete (sockaddr_in*)sa;
	   sa = NULL;
   }
   else
   {
//	   delete (sockaddr_in6*)sa;
	   sa = NULL;
   }
   m.m_pTimer = new CTimer;

   m.m_pSndQueue = new CSndQueue;
   m.m_pSndQueue->init(m.m_pChannel, m.m_pTimer);
   m.m_pRcvQueue = new CRcvQueue;
   m.m_pRcvQueue->init(32, u->m_iPayloadSize, m.m_iIPversion, 1024, m.m_pChannel, m.m_pTimer);

   m.m_pSndQueue->setifjvp2p(u->m_bIFJVP2P);
   m.m_pSndQueue->setifwait(u->m_bIFWAIT);

   m_vMultiplexer.insert(m_vMultiplexer.end(), m);

   u->m_pSndQueue = m.m_pSndQueue;
   u->m_pRcvQueue = m.m_pRcvQueue;
}

void CUDTUnited::updateMux(CUDT* u, CUDTSocket *us, const CUDTSocket* ls)
{
   CGuard cg(m_ControlLock);

   int port = (AF_INET == ls->m_iIPversion) ? ntohs(((sockaddr_in*)ls->m_pSelfAddr)->sin_port) : ntohs(((sockaddr_in6*)ls->m_pSelfAddr)->sin6_port);

   // find the listener's address
   for (vector<CMultiplexer>::iterator i = m_vMultiplexer.begin(); i != m_vMultiplexer.end(); ++ i)
   {
      if (i->m_iPort == port)
      {
		  i->m_pRcvQueue->setmaxrecvbuf(u->m_iTRcvBufSize);//
		  i->m_pSndQueue->setifjvp2p(u->m_bIFJVP2P);
		  i->m_pSndQueue->setifwait(u->m_bIFWAIT);
         // reuse the existing multiplexer
         ++ i->m_iRefCount;
         u->m_pSndQueue = i->m_pSndQueue;
         u->m_pRcvQueue = i->m_pRcvQueue;
		 us->m_nMPort = port;
         return;
      }
   }
}

#ifndef WIN32
   void* CUDTUnited::garbageCollect(void* p)
#else
   DWORD WINAPI CUDTUnited::garbageCollect(LPVOID p)
#endif
{
   CUDTUnited* self = (CUDTUnited*)p;

   CGuard gcguard(self->m_GCStopLock);

  int bEmptyOld=0, bEmptyNew=0;
   while (!self->m_bClosing)
   {
      self->checkBrokenSockets();

      #ifdef WIN32
         self->checkTLSValue();
      #else
		 bEmptyNew = self->m_ClosedSockets.empty();
		 if ( bEmptyNew && (!bEmptyOld) )
		 {
/**/			
			 //////////////////////////////////////////////////////////////////////////
			 if(1 == self->m_Sockets.size())
			 {
				 vector<CMultiplexer>::iterator m = self->m_vMultiplexer.begin();
				 if (m != self->m_vMultiplexer.end())
				 {
					 pthread_mutex_lock(&m->m_pRcvQueue->m_UQLock);
					 m->m_pRcvQueue->m_UnitQueue.shrink();
					 pthread_mutex_unlock(&m->m_pRcvQueue->m_UQLock);
				 }
			 }
			 //////////////////////////////////////////////////////////////////////////
			 
			 //合并碎片
			 fragment_free();
		 }
		 bEmptyOld = bEmptyNew;
      #endif

      #ifndef WIN32
         timeval now;
         timespec timeout;
         gettimeofday(&now, 0);
         timeout.tv_sec = now.tv_sec + 1;
         timeout.tv_nsec = now.tv_usec * 1000;
         pthread_cond_timedwait(&self->m_GCStopCond, &self->m_GCStopLock, &timeout);
		 //////////////////////////////////////////////////////////////////////////
//		 struct timespec tv;
//		 clock_gettime(CLOCK_MONOTONIC, &tv);
//		 tv.tv_sec += 1;//这里设置1秒后没收到事件超时返回
//		 pthread_cond_timedwait(&self->m_GCStopCond, &self->m_GCStopLock, &tv);
		 //////////////////////////////////////////////////////////////////////////
      #else
         WaitForSingleObject(self->m_GCStopCond, 100);//1000);
      #endif
   }

   // remove all sockets and multiplexers
   CGuard::enterCS(self->m_ControlLock);
   for (map<UDTSOCKET, CUDTSocket*>::iterator i = self->m_Sockets.begin(); i != self->m_Sockets.end(); ++ i)
   {
//	  i->second->m_pUDT->m_bBroken = true;
      i->second->m_pUDT->close();
      i->second->m_Status = CUDTSocket::CLOSED;
      i->second->m_TimeStamp = CTimer::getTime();
      self->m_ClosedSockets[i->first] = i->second;

	  // remove from listener's queue
      map<UDTSOCKET, CUDTSocket*>::iterator ls = self->m_Sockets.find(i->second->m_ListenSocket);
      if (ls == self->m_Sockets.end())
      {
		  ls = self->m_ClosedSockets.find(i->second->m_ListenSocket);
		  if (ls == self->m_ClosedSockets.end())
			  continue;
      }
	  
      CGuard::enterCS(ls->second->m_AcceptLock);
      ls->second->m_pQueuedSockets->erase(i->second->m_SocketID);
      ls->second->m_pAcceptSockets->erase(i->second->m_SocketID);
      CGuard::leaveCS(ls->second->m_AcceptLock);
   }
   self->m_Sockets.clear();

//   std::map<UDTSOCKET, CUDTSocket*> tmp = self->m_Sockets;
//   self->m_Sockets.swap(tmp);

   for (map<UDTSOCKET, CUDTSocket*>::iterator j = self->m_ClosedSockets.begin(); j != self->m_ClosedSockets.end(); ++ j)
   {
	   j->second->m_TimeStamp = 0;
   }
   CGuard::leaveCS(self->m_ControlLock);

   while (true)
   {
	   self->checkBrokenSockets();
	   
	   CGuard::enterCS(self->m_ControlLock);
	   bool empty = self->m_ClosedSockets.empty();
	   CGuard::leaveCS(self->m_ControlLock);
	   
	   if (empty)
		   break;

      #ifndef WIN32
         usleep(1000);
      #else
         Sleep(1);
      #endif
   }

   #ifndef WIN32
      return NULL;
   #else
      return 0;
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
void CUDTUnited::WaitThreadExit(HANDLE &hThread)
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

////////////////////////////////////////////////////////////////////////////////

int CUDT::startup()
{
   return s_UDTUnited.startup();
}

int CUDT::cleanup()
{
   return s_UDTUnited.cleanup();
}

UDTSOCKET CUDT::socket(int af, int type, int)
{
   if (!s_UDTUnited.m_bGCStatus)
      s_UDTUnited.startup();

   try
   {
      return s_UDTUnited.newSocket(af, type);
   }
   catch (CUDTException& e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
   
      return INVALID_SOCK;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return INVALID_SOCK;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return INVALID_SOCK;
   }
}

int CUDT::bind(UDTSOCKET u, const sockaddr* name, int namelen)
{
   try
   {
      return s_UDTUnited.bind(u, name, namelen);
   }
   catch (CUDTException& e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::bind(UDTSOCKET u, UDPSOCKET udpsock)
{
   try
   {
      return s_UDTUnited.bind(u, udpsock);
   }
   catch (CUDTException& e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::listen(UDTSOCKET u, int backlog)
{
   try
   {
      return s_UDTUnited.listen(u, backlog);
   }
   catch (CUDTException& e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

UDTSOCKET CUDT::accept(UDTSOCKET u, sockaddr* addr, int* addrlen)
{
   try
   {
      return s_UDTUnited.accept(u, addr, addrlen);
   }
   catch (CUDTException& e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return INVALID_SOCK;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return INVALID_SOCK;
   }
}

int CUDT::connect(STJUDTCONN stJUDTCONN)
{
   try
   {
      return s_UDTUnited.connect(stJUDTCONN);
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::close(UDTSOCKET u)
{
   try
   {
      return s_UDTUnited.close(u);
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}
 

int CUDT::getpeername(UDTSOCKET u, sockaddr* name, int* namelen)
{
   try
   {
      return s_UDTUnited.getpeername(u, name, namelen);
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::getrealpeername(UDTSOCKET u, sockaddr* name, int* namelen)
{
	try
	{
		return s_UDTUnited.getrealpeername(u, name, namelen);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getsockname(UDTSOCKET u, sockaddr* name, int* namelen)
{
   try
   {
      return s_UDTUnited.getsockname(u, name, namelen);;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::getrealsockname(UDTSOCKET u, sockaddr* name, int* namelen)
{
	try
	{
		return s_UDTUnited.getrealsockname(u, name, namelen);;
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getschannel(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getschannel(u);
	}
	catch (CUDTException e)
	{
	#ifndef WIN32
		s_UDTUnited.setError(e);
	#else
		s_UDTUnited.setError(new CUDTException(e));
	#endif
		
		return ERROR;
	}
	catch (...)
	{
	#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
	#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
		
		return ERROR;
	}
}

int CUDT::getvchannel(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getvchannel(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

unsigned char CUDT::getvirtual(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getvirtual(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getptystno(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getptystno(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getptystaddr(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getptystaddr(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getptystid(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getptystid(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::gettranstype(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.gettranstype(u);
	}
	catch (CUDTException e)
	{
	#ifndef WIN32
		s_UDTUnited.setError(e);
	#else
		s_UDTUnited.setError(new CUDTException(e));
	#endif
		
		return ERROR;
	}
	catch (...)
	{
	#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
	#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
		
		return ERROR;
	}
}

int CUDT::getystverF(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getystverF(u);
	}
	catch (CUDTException e)
	{
	#ifndef WIN32
		s_UDTUnited.setError(e);
	#else
		s_UDTUnited.setError(new CUDTException(e));
	#endif
		
		return ERROR;
	}
	catch (...)
	{
	#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
	#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
		
		return ERROR;
	}
}

int CUDT::getpeertcp(UDTSOCKET u)
{
	try
	{
		return s_UDTUnited.getpeertcp(u);
	}
	catch (CUDTException e)
	{
#ifndef WIN32
		s_UDTUnited.setError(e);
#else
		s_UDTUnited.setError(new CUDTException(e));
#endif
		
		return ERROR;
	}
	catch (...)
	{
#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
#endif
		
		return ERROR;
	}
}

int CUDT::getsockopt(UDTSOCKET u, int, UDTOpt optname, void* optval, int* optlen)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  udt->getOpt(optname, optval, *optlen);
	  }
      
      return 0;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::setsockopt(UDTSOCKET u, int, UDTOpt optname, const void* optval, int optlen)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  udt->setOpt(optname, optval, optlen);
	  }
      
      return 0;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::send(UDTSOCKET u, const char* buf, int len, int flags, bool bonce)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  return udt->send((char*)buf, len, bonce);
	  }
      return ERROR;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::recv(UDTSOCKET u, char* buf, int len, int)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  return udt->recv(buf, len);
	  }
      return ERROR;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
		
		return ERROR;
	}
	catch (...)
	{
	#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
	#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
		
		return ERROR;
	}
}

int CUDT::sendmsg(UDTSOCKET u, const char* buf, int len, int nttl, bool border, unsigned int unregion)
{
	try
	{
		CUDT* udt = s_UDTUnited.lookup(u);
		if(udt != NULL)
		{
			return udt->sendmsg((char*)buf, len, nttl, border, unregion);
		}
		return ERROR;
	}
	catch (CUDTException e)
	{
	#ifndef WIN32
		s_UDTUnited.setError(e);
	#else
		s_UDTUnited.setError(new CUDTException(e));
	#endif
		
		return ERROR;
	}
	catch (bad_alloc&)
	{
	#ifndef WIN32
		s_UDTUnited.setError(3, 2, 0);
	#else
		s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
		
		return ERROR;
	}
	catch (...)
	{
	#ifndef WIN32
		s_UDTUnited.setError(-1, 0, 0);
	#else
		s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
		
		return ERROR;
	}
}

int CUDT::recvmsg(UDTSOCKET u, char* buf, int len)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  return udt->recvmsg(buf, len);
	  }
      return ERROR;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::select(int, ud_set* readfds, ud_set* writefds, ud_set* exceptfds, const timeval* timeout)
{
   if ((NULL == readfds) && (NULL == writefds) && (NULL == exceptfds))
   {
	#ifndef WIN32
	   s_UDTUnited.setError(5, 3, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(5, 3, 0));
	#endif
      
      return ERROR;
   }

   try
   {
      return s_UDTUnited.select(readfds, writefds, exceptfds, timeout);
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	    s_UDTUnited.setError(3, 2, 0);
	#else
	    s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
     
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

int CUDT::selectEx(const vector<UDTSOCKET>& fds, vector<UDTSOCKET>* readfds, vector<UDTSOCKET>* writefds, vector<UDTSOCKET>* exceptfds, int64_t msTimeOut)
{
   if ((NULL == readfds) && (NULL == writefds) && (NULL == exceptfds))
   {
	#ifndef WIN32
	   s_UDTUnited.setError(5, 3, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(5, 3, 0));
	#endif
      
      return ERROR;
   }

   try
   {
      return s_UDTUnited.selectEx(fds, readfds, writefds, exceptfds, msTimeOut);
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (bad_alloc&)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(3, 2, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(3, 2, 0));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

CUDTException& CUDT::getlasterror()
{
   return *s_UDTUnited.getError();
}

int CUDT::perfmon(UDTSOCKET u, CPerfMon* perf, bool clear)
{
   try
   {
      CUDT* udt = s_UDTUnited.lookup(u);
	  if(udt != NULL)
	  {
		  udt->sample(perf, clear);
	  }
	  else
	  {
		  return ERROR;
	  }
      
      return 0;
   }
   catch (CUDTException e)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(e);
	#else
	   s_UDTUnited.setError(new CUDTException(e));
	#endif
      
      return ERROR;
   }
   catch (...)
   {
	#ifndef WIN32
	   s_UDTUnited.setError(-1, 0, 0);
	#else
	   s_UDTUnited.setError(new CUDTException(-1, 0, 0));
	#endif
      
      return ERROR;
   }
}

CUDT* CUDT::getUDTHandle(UDTSOCKET u)
{
   try
   {
      return s_UDTUnited.lookup(u);
   }
   catch (...)
   {
      return NULL;
   }
}

////////////////////////////////////////////////////////////////////////////////

namespace UDT
{

int startup()
{
   return CUDT::startup();
}

int cleanup()
{
   return CUDT::cleanup();
}

UDTSOCKET socket(int af, int type, int protocol)
{
   return CUDT::socket(af, type, protocol);
}

int bind(UDTSOCKET u, const struct sockaddr* name, int namelen)
{
   return CUDT::bind(u, name, namelen);
}

int bind(UDTSOCKET u, UDPSOCKET udpsock)
{
   return CUDT::bind(u, udpsock);
}

int listen(UDTSOCKET u, int backlog)
{
   return CUDT::listen(u, backlog);
}

UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen)
{
   return CUDT::accept(u, addr, addrlen);
}

int connect(STJUDTCONN stJUDTCONN)
{
   return CUDT::connect(stJUDTCONN);
}

int close(UDTSOCKET u)
{
   return CUDT::close(u);
}


int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
   return CUDT::getpeername(u, name, namelen);
}

int getrealpeername(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
	return CUDT::getrealpeername(u, name, namelen);
}

int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
   return CUDT::getsockname(u, name, namelen);
}

int getrealsockname(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
	return CUDT::getrealsockname(u, name, namelen);
}

int getschannel(UDTSOCKET u)
{
	return CUDT::getschannel(u);
}

int getvchannel(UDTSOCKET u)
{
	return CUDT::getvchannel(u);
}

unsigned char getvirtual(UDTSOCKET u)
{
	return CUDT::getvirtual(u);
}

int getptystno(UDTSOCKET u)
{
	return CUDT::getptystno(u);
}

int getptystaddr(UDTSOCKET u)
{
	return CUDT::getptystaddr(u);
}

int getptystid(UDTSOCKET u)
{
	return CUDT::getptystid(u);
}

int gettranstype(UDTSOCKET u)
{
	return CUDT::gettranstype(u);
}

int getystverF(UDTSOCKET u)
{
	return CUDT::getystverF(u);
}

int getpeertcp(UDTSOCKET u)
{
	return CUDT::getpeertcp(u);
}

int getsockopt(UDTSOCKET u, int level, SOCKOPT optname, void* optval, int* optlen)
{
   return CUDT::getsockopt(u, level, optname, optval, optlen);
}

int setsockopt(UDTSOCKET u, int level, SOCKOPT optname, const void* optval, int optlen)
{
   return CUDT::setsockopt(u, level, optname, optval, optlen);
}

int send(UDTSOCKET u, const char* buf, int len, int flags, bool bonce)
{
   return CUDT::send(u, buf, len, flags, bonce);
}

int recv(UDTSOCKET u, char* buf, int len, int flags)
{
   return CUDT::recv(u, buf, len, flags);
}

int sendmsg(UDTSOCKET u, const char* buf, int len, int nttl, bool border, unsigned int unregion)
{
	return CUDT::sendmsg(u, buf, len, nttl, border, unregion);
}

int recvmsg(UDTSOCKET u, char* buf, int len)
{
   return CUDT::recvmsg(u, buf, len);
}

int select(int nfds, UDSET* readfds, UDSET* writefds, UDSET* exceptfds, const struct timeval* timeout)
{
   return CUDT::select(nfds, readfds, writefds, exceptfds, timeout);
}

int selectEx(const vector<UDTSOCKET>& fds, vector<UDTSOCKET>* readfds, vector<UDTSOCKET>* writefds, vector<UDTSOCKET>* exceptfds, int64_t msTimeOut)
{
   return CUDT::selectEx(fds, readfds, writefds, exceptfds, msTimeOut);
}

ERRORINFO& getlasterror()
{
   return CUDT::getlasterror();
}

int perfmon(UDTSOCKET u, TRACEINFO* perf, bool clear)
{
   return CUDT::perfmon(u, perf, clear);
}


}
