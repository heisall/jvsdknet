/****************************************************************************
written by
   Yunhong Gu, last updated 05/05/2008
*****************************************************************************/

#ifndef WIN32
   #include <netdb.h>
   #include <arpa/inet.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <cstring>
   #include <cstdio>
   #include <cerrno>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #ifdef LEGACY_WIN32
      #include <wspiapi.h>
   #endif
#endif
#include "channel.h"
#include "packet.h"

#include "common.h"

#ifdef WIN32
   #define socklen_t int
#endif

#ifndef WIN32
   #define NET_ERROR errno
	#define  TRUE true
	#define  FALSE false
#else
   #define NET_ERROR WSAGetLastError()
#endif

FUNC_UDT_RECV_CALLBACK g_pfRecv = NULL;

CChannel::CChannel():
m_iIPversion(AF_INET),
m_iSocket(),
m_iSndBufSize(65536),
m_iRcvBufSize(65536)
{
	memset(m_chbuftmp, 0, 20);
	memset(m_chbufnull, 0, 20);
	m_bAutoCloseSocket = TRUE;
	m_iSocket = 0;
}

CChannel::CChannel(const int& version):
m_iIPversion(version),
m_iSocket(),
m_iSndBufSize(65536),
m_iRcvBufSize(65536)
{
	memset(m_chbuftmp, 0, 20);
	memset(m_chbufnull, 0, 20);
	m_bAutoCloseSocket = TRUE;
	m_iSocket = 0;
}

CChannel::~CChannel()
{
}

void CChannel::open(const sockaddr* addr)
{
   // construct an socket
	m_bAutoCloseSocket = TRUE;
   m_iSocket = socket(m_iIPversion, SOCK_DGRAM, 0);

   #ifdef WIN32
      if (INVALID_SOCKET == m_iSocket)
   #else
      if (m_iSocket < 0)
   #endif
      throw CUDTException(1, 0, NET_ERROR);

   if (NULL != addr)
   {
      socklen_t namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

      if (0 != bind(m_iSocket, addr, namelen))
         throw CUDTException(1, 3, NET_ERROR);
   }
/*   else
   {
      //sendto or WSASendTo will also automatically bind the socket
      addrinfo hints;
      addrinfo* res;

      memset(&hints, 0, sizeof(struct addrinfo));

//      hints.ai_flags = AI_PASSIVE;
      hints.ai_family = m_iIPversion;
      hints.ai_socktype = SOCK_DGRAM;

      if (0 != getaddrinfo(NULL, "0", &hints, &res))
         throw CUDTException(1, 3, NET_ERROR);

      if (0 != bind(m_iSocket, res->ai_addr, res->ai_addrlen))
         throw CUDTException(1, 3, NET_ERROR);

      freeaddrinfo(res);
   }
*/
   setUDPSockOpt();
}

void CChannel::open(UDPSOCKET udpsock)
{
	m_bAutoCloseSocket = FALSE;
   m_iSocket = udpsock;
   setUDPSockOpt();
}

void CChannel::setUDPSockOpt()
{
   #if defined(BSD) || defined(OSX)
      // BSD system will fail setsockopt if the requested buffer size exceeds system maximum value
      int maxsize = 64000;
      if (0 != setsockopt(m_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&m_iRcvBufSize, sizeof(int)))
         setsockopt(m_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&maxsize, sizeof(int));
      if (0 != setsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (char*)&m_iSndBufSize, sizeof(int)))
         setsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (char*)&maxsize, sizeof(int));
   #else
      // for other systems, if requested is greated than maximum, the maximum value will be automactally used
      if ((0 != setsockopt(m_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&m_iRcvBufSize, sizeof(int))) ||
          (0 != setsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (char*)&m_iSndBufSize, sizeof(int))))
         throw CUDTException(1, 3, NET_ERROR);
   #endif

   timeval tv;
   tv.tv_sec = 0;
   #if defined (BSD) || defined (OSX)
      // Known BSD bug as the day I wrote this code.
      // A small time out value will cause the socket to block forever.
      tv.tv_usec = 10000;
   #else
      tv.tv_usec = 100;
   #endif

   #ifdef UNIX
      // Set non-blocking I/O
      // UNIX does not support SO_RCVTIMEO
      int opts = fcntl(m_iSocket, F_GETFL);
      if (-1 == fcntl(m_iSocket, F_SETFL, opts | O_NONBLOCK))
         throw CUDTException(1, 3, NET_ERROR);
   #elif WIN32
      DWORD ot = 1; //milliseconds
      if (0 != setsockopt(m_iSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&ot, sizeof(DWORD)))
         throw CUDTException(1, 3, NET_ERROR);
   #else
      // Set receiving time-out value
      if (0 != setsockopt(m_iSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(timeval)))
         throw CUDTException(1, 3, NET_ERROR);
   #endif
}

void CChannel::close() const
{
	if(m_bAutoCloseSocket && m_iSocket > 0)
	{
  #ifndef WIN32
      ::close(m_iSocket);
   #else
      closesocket(m_iSocket);
   #endif
	}

 }

int CChannel::getSndBufSize()
{
   socklen_t size = sizeof(socklen_t);

   getsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (char *)&m_iSndBufSize, &size);

   return m_iSndBufSize;
}

int CChannel::getRcvBufSize()
{
   socklen_t size = sizeof(socklen_t);

   getsockopt(m_iSocket, SOL_SOCKET, SO_RCVBUF, (char *)&m_iRcvBufSize, &size);

   return m_iRcvBufSize;
}

void CChannel::setSndBufSize(const int& size)
{
   m_iSndBufSize = size;
}

void CChannel::setRcvBufSize(const int& size)
{
   m_iRcvBufSize = size;
}

void CChannel::getSockAddr(sockaddr* addr) const
{
   socklen_t namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

   getsockname(m_iSocket, addr, &namelen);
}

void CChannel::getPeerAddr(sockaddr* addr) const
{
   socklen_t namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

   getpeername(m_iSocket, addr, &namelen);
}
//void OutputDebug(char* format, ...);

int CChannel::sendto(const sockaddr* addr, CPacket& packet, const sockaddr* realaddr, const int nYSTNO, const char chGroup[4]) const
{
   // convert control information into network order
   if (packet.getFlag())
      for (int i = 0, n = packet.getLength() / 4; i < n; ++ i)
         *((uint32_t *)packet.m_pcData + i) = htonl(*((uint32_t *)packet.m_pcData + i));

   // convert packet header into network order
   //for (int j = 0; j < 4; ++ j)
   //   packet.m_nHeader[j] = htonl(packet.m_nHeader[j]);
   uint32_t* p = packet.m_nHeader;
//   for (int j = 0; j < 4; ++ j)
   for (int j = 0; j < 3; ++ j)
   {
      *p = htonl(*p);
      ++ p;
   }

   int res = 0;

   if(realaddr == NULL && nYSTNO <= 0)
   {//普通模式
	#ifndef WIN32
	   msghdr mh;
	   mh.msg_name = (sockaddr*)addr;
	   mh.msg_namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
	   mh.msg_iov = (iovec*)&(packet.m_PacketVector[1]);
	   mh.msg_iovlen = 2;
	   mh.msg_control = NULL;
	   mh.msg_controllen = 0;
	   mh.msg_flags = 0;
	   
	   res = sendmsg(m_iSocket, &mh, 0);
	#else
	   DWORD size = CPacket::m_iPktHdrSize + packet.getLength();
	   int addrsize = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
	   res = WSASendTo(m_iSocket, (LPWSABUF)&packet.m_PacketVector[1], 2, &size, 0, addr, addrsize, NULL, NULL);
	   res = (0 == res) ? size : -1;
	#endif
   }
   else
   {//转发模式
	   /*加头*/
	   if(realaddr != NULL)
	   {//type+realaddr
		   int nType = JVN_TDATA_NORMAL;
		   BYTE exhead[4+sizeof(sockaddr)] = {0};
		   memcpy(exhead, &nType, 4);
		   memcpy(&exhead[4], (BYTE*)realaddr, sizeof(sockaddr));
		   packet.m_PacketVector[0].iov_base = (char*)exhead;
		   packet.m_PacketVector[0].iov_len = 4+sizeof(sockaddr);

		#ifndef WIN32
		   msghdr mh;
		   mh.msg_name = (sockaddr*)addr;
		   mh.msg_namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
		   mh.msg_iov = (iovec*)packet.m_PacketVector;
		   mh.msg_iovlen = 3;
		   mh.msg_control = NULL;
		   mh.msg_controllen = 0;
		   mh.msg_flags = 0;
		   
		   res = sendmsg(m_iSocket, &mh, 0);
		#else
		   DWORD size = CPacket::m_iPktHdrSize + packet.getLength() + packet.m_PacketVector[0].iov_len;
		   int addrsize = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
		   res = WSASendTo(m_iSocket, (LPWSABUF)packet.m_PacketVector, 3, &size, 0, addr, addrsize, NULL, NULL);
		   res = (0 == res) ? size : -1;
		#endif
	   }
	   else
	   {//type+nYSTNO+chgroup
		   int nType = JVN_TDATA_BCON;
		   BYTE exhead[12] = {0};
		   memcpy(exhead, &nType, 4);
		   memcpy(&exhead[4], &nYSTNO, 4);
		   memcpy(&exhead[8],chGroup,4);
		   packet.m_PacketVector[0].iov_base = (char*)exhead;
		   packet.m_PacketVector[0].iov_len = 12;
		#ifndef WIN32
		   msghdr mh;
		   mh.msg_name = (sockaddr*)addr;
		   mh.msg_namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
		   mh.msg_iov = (iovec*)packet.m_PacketVector;
		   mh.msg_iovlen = 3;
		   mh.msg_control = NULL;
		   mh.msg_controllen = 0;
		   mh.msg_flags = 0;
		   
		   res = sendmsg(m_iSocket, &mh, 0);
		#else
		   DWORD size = CPacket::m_iPktHdrSize + packet.getLength() + packet.m_PacketVector[0].iov_len;
		   int addrsize = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
		   res = WSASendTo(m_iSocket, (LPWSABUF)packet.m_PacketVector, 3, &size, 0, addr, addrsize, NULL, NULL);
		   res = (0 == res) ? size : -1;
		#endif
	   }
   }
 //	if(res == 144)
 //	{
 	//	SOCKADDR_IN srv = {0};
 	//	memcpy(&srv,addr,sizeof(SOCKADDR_IN));
 		
// 		OutputDebug("send 144 to %s : %d ",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port));
//	}
//    // convert back into local host order
   //for (int k = 0; k < 4; ++ k)
   //   packet.m_nHeader[k] = ntohl(packet.m_nHeader[k]);
   p = packet.m_nHeader;
//   for (int k = 0; k < 4; ++ k)
   for (int k = 0; k < 3; ++ k)
   {
      *p = ntohl(*p);
       ++ p;
   }

   if (packet.getFlag())
      for (int l = 0, n = packet.getLength() / 4; l < n; ++ l)
         *((uint32_t *)packet.m_pcData + l) = ntohl(*((uint32_t *)packet.m_pcData + l));

   return res;
}

int CChannel::recvfrom(sockaddr* addr, CPacket& packet) const
{
#ifndef WIN32
	msghdr mh;   
	mh.msg_name = addr;
	mh.msg_namelen = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
	mh.msg_iov = (iovec*)&(packet.m_PacketVector[1]);
	mh.msg_iovlen = 2;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;
	
    //#ifdef UNIX
	    fd_set set;
	    timeval tv;
	    FD_ZERO(&set);
	    FD_SET(m_iSocket, &set);
	    tv.tv_sec = 0;
	    tv.tv_usec = 10000;
	    int iRet = select(m_iSocket+1, &set, NULL, &set, &tv);
		if (iRet < 1)
		{
			packet.setLength(-1);
			return -1;
		}
   // #endif
	int res = recvmsg(m_iSocket, &mh, 0);
#else
      DWORD size = CPacket::m_iPktHdrSize + packet.getLength();
      DWORD flag = 0;
      int addrsize = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

      int res = WSARecvFrom(m_iSocket, (LPWSABUF)&packet.m_PacketVector[1], 2, &size, &flag, addr, &addrsize, NULL, NULL);
      res = (0 == res) ? size : -1;
#endif

   if (res <= 0)
   {
      packet.setLength(-1);
      return -1;
   }

   BOOL bNewUDP = FALSE;
   if(res > 8)//纯UDP判断
   {
	   unsigned char uchtmp[8]={0};
	   unsigned int uHead = 0;
//	   memcpy(&uHead,packet.m_PacketVector[1].iov_base,4);
	   int nLen = 0;
//	   memcpy(&nLen,packet.m_PacketVector[1].iov_base + 4,4);
	   memcpy(uchtmp, packet.m_PacketVector[1].iov_base, 8);
	   memcpy(&uHead, uchtmp, 4);
	   memcpy(&nLen, &uchtmp[4], 4);
	   if(nLen == res && UDP_HEAD_VALUE == uHead)
	   {
		   bNewUDP = TRUE;
	   }

   }
   if(res == 8 || res == 13 || res == 20 || res == 22 || res == 36 || res == 37 || res == 40 || res == 41 || bNewUDP)//8,13,20可能是打洞包 22,36,40可能是服务器返回包
   {
	   char buff[2048] = {0};
	   memcpy((void *)&buff[0], packet.m_PacketVector[1].iov_base, packet.m_PacketVector[1].iov_len);
	   if(res > 12 && res <= 2000)
	   {
		   memcpy((void *)&buff[12], packet.m_PacketVector[2].iov_base, res - 12);
	   }
	   struct sockaddr_in sin;
	   int len = sizeof(sin);
	   
#ifdef WIN32
	   if(getsockname(m_iSocket, (struct sockaddr *)&sin, &len) != 0)
#else
	   if(getsockname(m_iSocket, (struct sockaddr *)&sin, (socklen_t* )&len) != 0)
#endif
	   {
		   printf("getsockname() error:%s\n", strerror(errno));
	   }
// 	   int nT = 0;
// 	   memcpy(&nT,buff,4);
// 	   if(nT < 0x200 && nT > 		
	   g_pfRecv(m_iSocket,ntohs(sin.sin_port),addr,buff,res,bNewUDP);
	   if(bNewUDP)
			return -1;
   }
// 	   SOCKADDR_IN srv = {0};
// 	   memcpy(&srv,addr,sizeof(SOCKADDR_IN));
// 	   OutputDebug("recv =========== %d   %s : %d",res,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port)   }
  //  if(res == 144)
  //  {
 //	   SOCKADDR_IN srv = {0};
 //	   memcpy(&srv,addr,sizeof(SOCKADDR_IN));
 
 //	   OutputDebug("recv 144 from %s : %d ",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port));
//}
  	packet.setLength(res - CPacket::m_iPktHdrSize);

   // convert back into local host order
   //for (int i = 0; i < 4; ++ i)
   //   packet.m_nHeader[i] = ntohl(packet.m_nHeader[i]);
   uint32_t* p = packet.m_nHeader;
//   for (int i = 0; i < 4; ++ i)
   for (int i = 0; i < 3; ++ i)
   {
      *p = ntohl(*p);
      ++ p;
   }

   if (packet.getFlag())
      for (int j = 0, n = packet.getLength() / 4; j < n; ++ j)
         *((uint32_t *)packet.m_pcData + j) = ntohl(*((uint32_t *)packet.m_pcData + j));

   return packet.getLength();
}

#ifdef WIN32
int CChannel::sendtoclient(char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverSec)
#else
int CChannel::sendtoclient(char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverSec)
#endif
{
#ifdef WIN32
	__try
	{
#endif
		int   status,nbytesreceived;   
		if(m_iSocket ==-1)     
		{
			return -1;   
		}
		struct   timeval   tv={ntimeoverSec,0};   
		fd_set   fd;   
		FD_ZERO(&fd);     
		FD_SET(m_iSocket,&fd);     
		if(ntimeoverSec==0)   
		{
			status=select(m_iSocket+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);     
		}
		else
		{
			status=select(m_iSocket+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);     
		}
		switch(status)     
		{   
		case   -1:     
			//
			return -1;   
		case   0:                                    
			//
			return 0;   
		default:                   
			if(FD_ISSET(m_iSocket,&fd))     
			{   
				if((nbytesreceived=::sendto(m_iSocket,pchbuf,nlen,nflags,to,ntolen))==-1)   
				{
					return -1;   
				}
				else   
				{
					return nbytesreceived;   
				}
			}   
		}   
		return -1;
#ifdef WIN32
	}
	__except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
	{
		return -1;
	}   
#endif
}

