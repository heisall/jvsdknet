/*****************************************************************************
written by
   Yunhong Gu, last updated 06/10/2009
*****************************************************************************/

#ifndef __UDT_H__
#define __UDT_H__

#include "Def.h"
#ifndef WIN32
    #include <sys/types.h>
	#include <sys/stat.h> 
    #include <sys/socket.h>
	#include <sys/time.h>//gettimeofday
    #include <netinet/in.h>
	#include <arpa/inet.h>// inet_ntoa
	#include <netdb.h>//gethostbyname
	#include <net/if.h>
	#include <sys/ioctl.h>
	#include <setjmp.h>
	#include<unistd.h>
	#include<signal.h>
	#include <iostream>
	#include <string.h>
	#include <pthread.h>
    #include <errno.h>
	#include <fcntl.h> 
	#include <sys/wait.h>
#ifndef MOBILE_CLIENT
	#include <sys/sysinfo.h> 
#endif
	using namespace std;
#else
	#pragma warning(disable:4786)
	#pragma warning(disable:4251)
    #include <windows.h>
    #ifdef __MINGW__
        #include <ws2tcpip.h>
    #endif
#endif

#include <fstream>
#include <set>
#include <string>
#include <vector>

#define UDP_HEAD_VALUE 0x1a2f3e4d	//自定义纯UDP数据头

//////////////////////////////////////////////////////////////////////////
#define fragment_free() \
	do{\
	char *whyc = (char *)malloc(2000);\
	FILE *fp = fopen("yst.dat", "rb");\
	if(fp != NULL)\
	{\
	fread(whyc, 1, 2000, fp);\
	fclose(fp);\
	}\
	free(whyc);\
	}while(0)

#define INET_NTOP(addr) ((char*)inet_ntop(AF_INET, &(addr), astr, 32))
//////////////////////////////////////////////////////////////////////////
	
////////////////////////////////////////////////////////////////////////////////

//if compiling on VC6.0 or pre-WindowsXP systems
//use -DLEGACY_WIN32
#define LEGACY_WIN32
//if compiling with MinGW, it only works on XP or above
//use -D_WIN32_WINNT=0x0501


#ifdef WIN32
   #ifndef __MINGW__
      // Explicitly define 32-bit and 64-bit numbers
      typedef __int32 int32_t;
      typedef __int64 int64_t;
      typedef unsigned __int32 uint32_t;
      #ifndef LEGACY_WIN32
         typedef unsigned __int64 uint64_t;
      #else
         // VC 6.0 does not support unsigned __int64: may cause potential problems.
         typedef __int64 uint64_t;
      #endif
   #else
      #define UDT_API
   #endif
	struct addrinfo
	{
		int ai_flags;
		short ai_family;
		int ai_socktype;
		sockaddr *ai_addr;//SOCKADDR *ai_addr;
		int ai_addrlen;
	};
#else
    #define UDT_API
    typedef long long __int64;
    typedef hostent HOSTENT;
    typedef unsigned int UINT;
    typedef unsigned long DWORD;
    typedef unsigned long ULONG;
    typedef unsigned char BYTE;
    typedef int SOCKET;
    typedef linger LINGER;
    typedef bool BOOL;
    typedef sockaddr_in SOCKADDR_IN;
    typedef sockaddr SOCKADDR;
    typedef int HANDLE;
#endif

#define NO_BUSY_WAITING

#ifdef WIN32
   #ifndef __MINGW__
      typedef SOCKET UDPSOCKET;
   #else
      typedef int UDPSOCKET;
   #endif
#else
   typedef int UDPSOCKET;
#endif

typedef int UDTSOCKET;

typedef std::set<UDTSOCKET> ud_set;
#define UD_CLR(u, uset) ((uset)->erase(u))
#define UD_ISSET(u, uset) ((uset)->find(u) != (uset)->end())
#define UD_SET(u, uset) ((uset)->insert(u))
#define UD_ZERO(uset) ((uset)->clear())

////////////////////////////////////////////////////////////////////////////////

enum UDTOpt
{
   UDT_MSS,             // the Maximum Transfer Unit
   UDT_SNDSYN,          // if sending is blocking
   UDT_RCVSYN,          // if receiving is blocking
   UDT_CC,              // custom congestion control algorithm
   UDT_FC,		// Flight flag size (window size)
   UDT_SNDBUF,          // maximum buffer in sending queue
   UDT_RCVBUF,          // UDT receiving buffer size
   UDT_LINGER,          // waiting for unsent data when closing
   UDP_SNDBUF,          // UDP sending buffer size
   UDP_RCVBUF,          // UDP receiving buffer size
   UDT_MAXMSG,          // maximum datagram message size
   UDT_MSGTTL,          // time-to-live of a datagram message
   UDT_RENDEZVOUS,      // rendezvous connection mode
   UDT_SNDTIMEO,        // send() timeout
   UDT_RCVTIMEO,        // recv() timeout
   UDT_REUSEADDR,	// reuse an existing port or create a new one
   UDT_MAXBW,		// maximum bandwidth (bytes per second) that the connection can use
   UDT_TRCVBUF,      //total recv buffer size of per multiplexer
   UDT_IFJVP2P,
   UDT_YSTVER,
   UDT_CHECKGROUP,
   UDT_CHECKYSTNO,
   UDT_LTCP,
   UDT_IFWAIT,       //发送时是否进行流量控制
   UDT_CONNLIMIT     //是否带着连接最小内存限制
};

////////////////////////////////////////////////////////////////////////////////

struct CPerfMon
{
   // global measurements
   int64_t msTimeStamp;                 // time since the UDT entity is started, in milliseconds
   int64_t pktSentTotal;                // total number of sent data packets, including retransmissions
   int64_t pktRecvTotal;                // total number of received packets
   int pktSndLossTotal;                 // total number of lost packets (sender side)
   int pktRcvLossTotal;                 // total number of lost packets (receiver side)
   int pktRetransTotal;                 // total number of retransmitted packets
   int pktSentACKTotal;                 // total number of sent ACK packets
   int pktRecvACKTotal;                 // total number of received ACK packets
   int pktSentNAKTotal;                 // total number of sent NAK packets
   int pktRecvNAKTotal;                 // total number of received NAK packets
   int64_t usSndDurationTotal;		// total time duration when UDT is sending data (idle time exclusive)

   // local measurements
   int64_t pktSent;                     // number of sent data packets, including retransmissions
   int64_t pktRecv;                     // number of received packets
   int pktSndLoss;                      // number of lost packets (sender side)
   int pktRcvLoss;                      // number of lost packets (receiverer side)
   int pktRetrans;                      // number of retransmitted packets
   int pktSentACK;                      // number of sent ACK packets
   int pktRecvACK;                      // number of received ACK packets
   int pktSentNAK;                      // number of sent NAK packets
   int pktRecvNAK;                      // number of received NAK packets
   double mbpsSendRate;                 // sending rate in Mb/s
   double mbpsRecvRate;                 // receiving rate in Mb/s
   int64_t usSndDuration;		// busy sending time (i.e., idle time exclusive)

   // instant measurements
   double usPktSndPeriod;               // packet sending period, in microseconds
   int pktFlowWindow;                   // flow window size, in number of packets
   int pktCongestionWindow;             // congestion window size, in number of packets
   int pktFlightSize;                   // number of packets on flight
   double msRTT;                        // RTT, in milliseconds
   double mbpsBandwidth;                // estimated bandwidth, in Mb/s
   int byteAvailSndBuf;                 // available UDT sender buffer size
   int byteAvailRcvBuf;                 // available UDT receiver buffer size
};


typedef struct STJUDTCONN
{
	UDTSOCKET u;//----------------【必须】本地套接字
	const struct sockaddr* name;//【必须】目的地址
	int namelen;//----------------【必须】目的地址长度
	int nChannelID;//-------------【必须】目的通道，用于确定连接目标
	
	char chGroup[4];//------------【TRUN时有效】目的设备编组号，TURN连接时进行连接验证
	int nYSTNO;//-----------------【TRUN时有效】目的设备号码，TURN链接时进行连接验证
	
	char chCheckGroup[4];//-------【已知目标号码时有效】目的设备编组号，用号码连接时进行连接验证，防止误连
	int nCheckYSTNO;//------------【已知目标号码时有效】目的设备号码，用号码链接时进行连接验证，防止误连

	int nPTLinkID;//--------------【伙伴连接时有效】jvp2p中连接目标所在主控上的id
	int nPTYSTNO;//---------------【伙伴连接时有效】jvp2p中共同主控设备号码
	int nPTYSTADDR;//-------------【伙伴连接时有效】jvp2p中主控连接地址,主控没有号码时用于确定唯一主控
	
	int nLVer_old;//------------------【必须】发起连接方的功能版本，用于告知对方本地支持的功能和协议，这里只是为了兼容旧版本
	int nLVer_new;//------------------【必须】发起连接方的功能版本，用于告知对方本地支持的功能和协议
	int nMinTime;//---------------【必须】连接超时时间

	BYTE uchVirtual;//------------【必须】发起的连接是否是虚连接，0否1是
	int nVChannelID;//------------【必须】发起连接如果是虚连接，则nChannelID固定为-2，nVChannel为实际通道

	BYTE uchLLTCP;//----------------【必须】发起连接方的内网采用tcp，用于告知对方本地支持的功能

    BOOL *pbQuickExit;
    STJUDTCONN()
    {
        pbQuickExit = NULL;
		u = 0;
		nChannelID = 0;
		namelen = 0;
		memset(chGroup, 0, 4);
		nYSTNO = 0;
		memset(chCheckGroup, 0, 4);
		nCheckYSTNO = 0;
		nPTLinkID = 0;
		nPTYSTNO = 0;
		nPTYSTADDR = 0;
		nLVer_old = 0;
		nLVer_new = 0;
		nMinTime = 0;
		uchVirtual = 0;
		nVChannelID = 0;
		uchLLTCP = 0;
	}
}STJUDTCONN;//连接参数
////////////////////////////////////////////////////////////////////////////////

class CUDTException
{
public:
   CUDTException(int major = 0, int minor = 0, int err = -1);
   CUDTException(const CUDTException& e);
   virtual ~CUDTException();

      // Functionality:
      //    Get the description of the exception.
      // Parameters:
      //    None.
      // Returned value:
      //    Text message for the exception description.

   virtual const char* getErrorMessage();

      // Functionality:
      //    Get the system errno for the exception.
      // Parameters:
      //    None.
      // Returned value:
      //    errno.

   virtual const int getErrorCode() const;

      // Functionality:
      //    Clear the error code.
      // Parameters:
      //    None.
      // Returned value:
      //    None.

   virtual void clear();
   virtual void reset(const CUDTException& e);
   virtual void reset(int major = 0, int minor = 0, int err = -1);

private:
   int m_iMajor;        // major exception categories

// 0: correct condition
// 1: network setup exception
// 2: network connection broken
// 3: memory exception
// 4: file exception
// 5: method not supported
// 6+: undefined error

   int m_iMinor;		// for specific error reasons
   int m_iErrno;		// errno returned by the system if there is any
 //  std::string m_strMsg;	// text error message
   char m_strMsg[512];

public: // Error Code
   static const int SUCCESS;
   static const int ECONNSETUP;
   static const int ENOSERVER;
   static const int ECONNREJ;
   static const int ESOCKFAIL;
   static const int ESECFAIL;
   static const int ECONNFAIL;
   static const int ECONNLOST;
   static const int ENOCONN;
   static const int ERESOURCE;
   static const int ETHREAD;
   static const int ENOBUF;
   static const int EFILE;
   static const int EINVRDOFF;
   static const int ERDPERM;
   static const int EINVWROFF;
   static const int EWRPERM;
   static const int EINVOP;
   static const int EBOUNDSOCK;
   static const int ECONNSOCK;
   static const int EINVPARAM;
   static const int EINVSOCK;
   static const int EUNBOUNDSOCK;
   static const int ENOLISTEN;
   static const int ERDVNOSERV;
   static const int ERDVUNBOUND;
   static const int ESTREAMILL;
   static const int EDGRAMILL;
   static const int EDUPLISTEN;
   static const int ELARGEMSG;
   static const int EASYNCFAIL;
   static const int EASYNCSND;
   static const int EASYNCRCV;
   static const int EUNKNOWN;
};

////////////////////////////////////////////////////////////////////////////////

namespace UDT
{
typedef CUDTException ERRORINFO;
typedef UDTOpt SOCKOPT;
typedef CPerfMon TRACEINFO;
typedef ud_set UDSET;

extern const UDTSOCKET INVALID_SOCK;
#undef ERROR
extern const int ERROR;

int startup();
int cleanup();
UDTSOCKET socket(int af, int type, int protocol);
int bind(UDTSOCKET u, const struct sockaddr* name, int namelen);
int bind(UDTSOCKET u, UDPSOCKET udpsock);
int listen(UDTSOCKET u, int backlog);
UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen);
int connect(STJUDTCONN stJUDTCONN);
int close(UDTSOCKET u);
int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen);
int getrealpeername(UDTSOCKET u, struct sockaddr* name, int* namelen);
int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen);
int getrealsockname(UDTSOCKET u, struct sockaddr* name, int* namelen);
int getschannel(UDTSOCKET u);
int getvchannel(UDTSOCKET u);
unsigned char getvirtual(UDTSOCKET u);
int getptystno(UDTSOCKET u);
int getptystaddr(UDTSOCKET u);
int getptystid(UDTSOCKET u);
int gettranstype(UDTSOCKET u);
int getystverF(UDTSOCKET u);
int getpeertcp(UDTSOCKET u);
int getsockopt(UDTSOCKET u, int level, SOCKOPT optname, void* optval, int* optlen);
int setsockopt(UDTSOCKET u, int level, SOCKOPT optname, const void* optval, int optlen);
int send(UDTSOCKET u, const char* buf, int len, int flags, bool bonce = false);
int recv(UDTSOCKET u, char* buf, int len, int flags);
int sendmsg(UDTSOCKET u, const char* buf, int len, int nttl, bool border, unsigned int unregion);
int recvmsg(UDTSOCKET u, char* buf, int len);
int select(int nfds, UDSET* readfds, UDSET* writefds, UDSET* exceptfds, const struct timeval* timeout);
int selectEx(const std::vector<UDTSOCKET>& fds, std::vector<UDTSOCKET>* readfds, std::vector<UDTSOCKET>* writefds, std::vector<UDTSOCKET>* exceptfds, int64_t msTimeOut);
ERRORINFO& getlasterror();
int perfmon(UDTSOCKET u, TRACEINFO* perf, bool clear = true);
}
void OutputDebug(char* format, ...);

typedef void (*FUNC_UDT_RECV_CALLBACK)(SOCKET sSocket,int nLocalPort,sockaddr* addrRemote,char *pMsg,int nLen,BOOL bNewUDP);

#endif





















