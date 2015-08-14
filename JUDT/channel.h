/*****************************************************************************
written by
   Yunhong Gu, last updated 05/23/2008
*****************************************************************************/

#ifndef __UDT_CHANNEL_H__
#define __UDT_CHANNEL_H__


#include "udt.h"
#include "packet.h"

#define JVN_TDATA_CONN       0x8F//分控向转发服务器发起连接(停用)
#define JVN_TDATA_NORMAL     0x90//分控/主控向转发服务器发送普通数据
#define JVN_TDATA_AOL        0x8E//主控向转发服务器上线(新)
#define JVN_TDATA_BCON       0x8D//分控向转发服务器发起连接(新)

#define JVN_CMD_TRYTOUCH     0x78//打洞包
#define JVN_REQ_EXCONNA      0xAD//分控请求主控地址
#define JVN_RSP_CONNA        0x87//服务器向分控返回主控地址
#define JVN_RSP_CONNAF       0x88//服务器向分控返回 主控未上线
#define YST_A_NEW_ADDRESS       0x100//主控的新地址
#define JVN_REQ_EXCONNA2       0xB0//分控请求主控地址 新带参数

#define JVN_CMD_YSTCHECK2     0xB1//查询及返回某号码是否在线以及号码主控SDK版本 返回分控NAT地址


class CChannel
{
	BOOL m_bAutoCloseSocket;//不需要自动关闭SOCKET套接字
public:
   CChannel();
   CChannel(const int& version);
   ~CChannel();

      // Functionality:
      //    Opne a UDP channel.
      // Parameters:
      //    0) [in] addr: The local address that UDP will use.
      // Returned value:
      //    None.

   void open(const sockaddr* addr = NULL);

      // Functionality:
      //    Opne a UDP channel based on an existing UDP socket.
      // Parameters:
      //    0) [in] udpsock: UDP socket descriptor.
      // Returned value:
      //    None.

   void open(UDPSOCKET udpsock);

      // Functionality:
      //    Disconnect and close the UDP entity.
      // Parameters:
      //    None.
      // Returned value:
      //    None.

   void close() const;

      // Functionality:
      //    Get the UDP sending buffer size.
      // Parameters:
      //    None.
      // Returned value:
      //    Current UDP sending buffer size.

   int getSndBufSize();

      // Functionality:
      //    Get the UDP receiving buffer size.
      // Parameters:
      //    None.
      // Returned value:
      //    Current UDP receiving buffer size.

   int getRcvBufSize();

      // Functionality:
      //    Set the UDP sending buffer size.
      // Parameters:
      //    0) [in] size: expected UDP sending buffer size.
      // Returned value:
      //    None.

   void setSndBufSize(const int& size);

      // Functionality:
      //    Set the UDP receiving buffer size.
      // Parameters:
      //    0) [in] size: expected UDP receiving buffer size.
      // Returned value:
      //    None.

   void setRcvBufSize(const int& size);

      // Functionality:
      //    Query the socket address that the channel is using.
      // Parameters:
      //    0) [out] addr: pointer to store the returned socket address.
      // Returned value:
      //    None.

   void getSockAddr(sockaddr* addr) const;

      // Functionality:
      //    Query the peer side socket address that the channel is connect to.
      // Parameters:
      //    0) [out] addr: pointer to store the returned socket address.
      // Returned value:
      //    None.

   void getPeerAddr(sockaddr* addr) const;

      // Functionality:
      //    Send a packet to the given address.
      // Parameters:
      //    0) [in] addr: pointer to the destination address.
      //    1) [in] packet: reference to a CPacket entity.
      // Returned value:
      //    Actual size of data sent.

   int sendto(const sockaddr* addr, CPacket& packet, const sockaddr* realaddr, const int nYSTNO, const char chGroup[4]) const;

      // Functionality:
      //    Receive a packet from the channel and record the source address.
      // Parameters:
      //    0) [in] addr: pointer to the source address.
      //    1) [in] packet: reference to a CPacket entity.
      // Returned value:
      //    Actual size of data received.

   int recvfrom(sockaddr* addr, CPacket& packet) const;

   #ifndef WIN32
   int sendtoclient(char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverSec);
   #else
   int sendtoclient(char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverSec);
   #endif 

private:
   void setUDPSockOpt();

private:
   int m_iIPversion;                    // IP version

   #ifndef WIN32
      int m_iSocket;                    // socket descriptor
   #else
      SOCKET m_iSocket;
   #endif

   int m_iSndBufSize;                   // UDP sending buffer size
   int m_iRcvBufSize;                   // UDP receiving buffer size

   char m_chbufnull[20];
   char m_chbuftmp[20];
};


#endif
