/*****************************************************************************
written by
   Yunhong Gu, last updated 05/05/2009
*****************************************************************************/

#ifndef __UDT_API_H__
#define __UDT_API_H__


#include <map>
#include <vector>
#include "udt.h"
#include "packet.h"
#include "queue.h"
#include "cache.h"


class CUDT;

class CUDTSocket
{
public:
   CUDTSocket();
   ~CUDTSocket();

   enum UDTSTATUS {INIT = 1, OPENED, LISTENING, CONNECTED, BROKEN, CLOSED};
   UDTSTATUS m_Status;                       // current socket state

   uint64_t m_TimeStamp;                     // time when the socket is closed

   int m_iIPversion;                         // IP version
   sockaddr* m_pSelfAddr;                    // pointer to the local address of the socket
   sockaddr* m_pPeerAddr;                    // pointer to the peer address of the socket
   sockaddr* m_pRealSelfAddr;                // 转发时实际本身地址
   sockaddr* m_pRealPeerAddr;                // 转发时实际目的地址

   UDTSOCKET m_SocketID;                     // socket ID
   UDTSOCKET m_ListenSocket;                 // ID of the listener socket; 0 means this is an independent socket

   UDTSOCKET m_PeerID;                       // peer socket ID
   int32_t m_iISN;                           // initial sequence number, used to tell different connection from same IP:port

   CUDT* m_pUDT;                             // pointer to the UDT entity

   std::set<UDTSOCKET>* m_pQueuedSockets;    // set of connections waiting for accept()
   std::set<UDTSOCKET>* m_pAcceptSockets;    // set of accept()ed connections

   pthread_cond_t m_AcceptCond;              // used to block "accept" call
   pthread_mutex_t m_AcceptLock;             // mutex associated to m_AcceptCond

   unsigned int m_uiBackLog;                 // maximum number of connections in queue

   int m_nChannelID;                         //对应服务通道,用在主控端
   int m_nPTLinkID;                           //目的ID
   int m_nPTYSTNO;                             //对应服务号码
   int m_nPTYSTADDR;                           //对应服务地址
   int m_nMPort;                             //本地绑定端口 用于垃圾清理中
   int m_nYSTLV;//本地云视通版本
   int m_nYSTFV;//远端云视通版本
   int m_nYSTLV_2;//本地云视通版本,注：以前的版本被用死了，只能重新定义，后续小心使用
   int m_nYSTFV_2;//远端云视通版本,注：以前的版本被用死了，只能重新定义，后续小心使用

   unsigned char m_uchVirtual;//连接是否是虚连接
   int m_nVChannelID;//虚连接时对应的实际通道

   int m_nPeerTCP;//远端是否支持内网tcp连接(早期版本没有该功能)
private:
   CUDTSocket(const CUDTSocket&);
   CUDTSocket& operator=(const CUDTSocket&);
};

////////////////////////////////////////////////////////////////////////////////

class CUDTUnited
{
friend class CUDT;

public:
   CUDTUnited();
   ~CUDTUnited();

public:

      // Functionality:
      //    initialize the UDT library.
      // Parameters:
      //    None.
      // Returned value:
      //    0 if success, otherwise -1 is returned.

   int startup();

      // Functionality:
      //    release the UDT library.
      // Parameters:
      //    None.
      // Returned value:
      //    0 if success, otherwise -1 is returned.

   int cleanup();

      // Functionality:
      //    Create a new UDT socket.
      // Parameters:
      //    0) [in] af: IP version, IPv4 (AF_INET) or IPv6 (AF_INET6).
      //    1) [in] type: socket type, SOCK_STREAM or SOCK_DGRAM
      // Returned value:
      //    The new UDT socket ID, or INVALID_SOCK.

   UDTSOCKET newSocket(const int& af, const int& type);

      // Functionality:
      //    Create a new UDT connection.
      // Parameters:
      //    0) [in] listen: the listening UDT socket;
      //    1) [in] peer: peer address.
      //    2) [in/out] hs: handshake information from peer side (in), negotiated value (out);
      // Returned value:
      //    If the new connection is successfully created: 1 success, 0 already exist, -1 error.

   int newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs);

      // Functionality:
      //    look up the UDT entity according to its ID.
      // Parameters:
      //    0) [in] u: the UDT socket ID.
      // Returned value:
      //    Pointer to the UDT entity.

   CUDT* lookup(const UDTSOCKET u);

      // Functionality:
      //    Check the status of the UDT socket.
      // Parameters:
      //    0) [in] u: the UDT socket ID.
      // Returned value:
      //    UDT socket status, or INIT if not found.

   CUDTSocket::UDTSTATUS getStatus(const UDTSOCKET u);

      // socket APIs

   int bind(const UDTSOCKET u, const sockaddr* name, const int& namelen);
   int bind(const UDTSOCKET u, UDPSOCKET udpsock);
   int listen(const UDTSOCKET u, const int& backlog);
   UDTSOCKET accept(const UDTSOCKET listen, sockaddr* addr, int* addrlen);
   int connect(STJUDTCONN stJUDTCONN);
   int close(const UDTSOCKET u);
   int getpeername(const UDTSOCKET u, sockaddr* name, int* namelen);
   int getrealpeername(const UDTSOCKET u, sockaddr* name, int* namelen);
   int getsockname(const UDTSOCKET u, sockaddr* name, int* namelen);
   int getrealsockname(const UDTSOCKET u, sockaddr* name, int* namelen);
   int getschannel(const UDTSOCKET u);
   int getvchannel(const UDTSOCKET u);
   unsigned char getvirtual(const UDTSOCKET u);
   int getptystno(const UDTSOCKET u);
   int getptystaddr(const UDTSOCKET u);
   int getptystid(const UDTSOCKET u);
   int gettranstype(UDTSOCKET u);
   int getystverF(UDTSOCKET u);
   int getpeertcp(UDTSOCKET u);
   int select(ud_set* readfds, ud_set* writefds, ud_set* exceptfds, const timeval* timeout);
   int selectEx(const std::vector<UDTSOCKET>& fds, std::vector<UDTSOCKET>* readfds, std::vector<UDTSOCKET>* writefds, std::vector<UDTSOCKET>* exceptfds, int64_t msTimeOut);

      // Functionality:
      //    record the UDT exception.
      // Parameters:
      //    0) [in] e: pointer to a UDT exception instance.
      // Returned value:
      //    None.

#ifdef WIN32
   void setError(CUDTException* e);
#else
   void setError(const CUDTException& e);
   void setError(int major = 0, int minor = 0, int err = -1);
#endif

      // Functionality:
      //    look up the most recent UDT exception.
      // Parameters:
      //    None.
      // Returned value:
      //    pointer to a UDT exception instance.

   CUDTException* getError();

private:
   std::map<UDTSOCKET, CUDTSocket*> m_Sockets;       // stores all the socket structures

   pthread_mutex_t m_ControlLock;                    // used to synchronize UDT API

   pthread_mutex_t m_IDLock;                         // used to synchronize ID generation
   UDTSOCKET m_SocketID;                             // seed to generate a new unique socket ID

   CUDTException m_e;
   int m_curConNum;         //
private:
	
   #ifndef WIN32
      static void TLSDestroy(void* e) {if (NULL != e) delete (CUDTException*)e;}
   #else
	  pthread_key_t m_TLSError;                         // thread local error record (last error)
      std::map<DWORD, CUDTException*> m_mTLSRecord;
      void checkTLSValue();
      pthread_mutex_t m_TLSLock;
   #endif

private:
   CUDTSocket* locate(const UDTSOCKET u);
   CUDTSocket* locate(const UDTSOCKET u, const sockaddr* peer, const UDTSOCKET& id, const int32_t& isn);
   void updateMux(CUDT* u, CUDTSocket *us, const sockaddr* addr = NULL, const UDPSOCKET* = NULL);
   void updateMux(CUDT* u, CUDTSocket *us, const CUDTSocket* ls);

   #ifdef WIN32
	   void WaitThreadExit(HANDLE &hThread);//强制退出线程
   #endif

private:
   std::vector<CMultiplexer> m_vMultiplexer;		// UDP multiplexer
   pthread_mutex_t m_MultiplexerLock;

private:
   CCache* m_pCache;					// UDT network information cache

private:
   volatile bool m_bClosing;
   pthread_mutex_t m_GCStopLock;
   pthread_cond_t m_GCStopCond;

   pthread_mutex_t m_InitLock;
   bool m_bGCStatus;					// if the GC thread is working (true)

   pthread_t m_GCThread;
   #ifndef WIN32
      static void* garbageCollect(void*);
   #else
      static DWORD WINAPI garbageCollect(LPVOID);
   #endif

   std::map<UDTSOCKET, CUDTSocket*> m_ClosedSockets;   // temporarily store closed sockets

   void checkBrokenSockets();
   void removeSocket(const UDTSOCKET u);

private:
   CUDTUnited(const CUDTUnited&);
   CUDTUnited& operator=(const CUDTUnited&);
};

#endif
