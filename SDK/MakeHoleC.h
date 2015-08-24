// MakeHoleC.h: interface for the CMakeHoleC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_)
#define AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CChannel.h"
#include "CVirtualChannel.h"

//A IP1 B IP2 ,X Y 端口
#define CHECK_TYPE_AX_AX 1
#define CHECK_TYPE_AX_BY 2
#define CHECK_TYPE_BX_BX 3
#define CHECK_TYPE_BX_BY 4

BOOL AddrIsTheSame(sockaddr* addrRemote,sockaddr* addrRemote2);//判断2个地址是否是相同 4字节地址 2字节端口

class CMakeHoleC;

#define JVN_CMD_YSTCHECK2     0xB1//??????????????????SDK?? ????NAT??

class CLocker
{
public:
	CLocker(pthread_mutex_t& lock,char* pName = "",int nLine = 0);
	~CLocker();
	
	static void enterCS(pthread_mutex_t& lock);
	static void leaveCS(pthread_mutex_t& lock);
	
private:
	pthread_mutex_t& m_Mutex;            // Alias name of the mutex to be protected
	int m_iLocked;                       // Locking status

	char m_strName[1000];
	int m_nLine;
};
#define JVN_REQ_EXCONNA2        0xB0
#define JVN_REQ_NAT				0x101


#ifdef _WIN32
#define MAX_NUM 1//单次打开本地的端口数量，用于同时连接主控  参考teamviewer修改多端口一次连接
#else
#define MAX_NUM 1//单次打开本地的端口数量，用于同时连接主控 嵌入式系统只用一个吧先
#endif

#define JVN_CMD_NEW_TRYTOUCH	0xb2//新的打洞包 互发包
#define JVN_CMD_A_HANDSHAKE		0xb3//新的打洞包 主控发给分控 
#define JVN_CMD_B_HANDSHAKE		0xb4//新的打洞包 分控发给主控
#define JVN_CMD_AB_HANDSHAKE	0xb5//主控发给分控 分控收到后可以连接


//
//互发 0xb2，主控收到发送0xb3,分控收到发送0xb4
//主控收到0xb4 发送 0xb5,启动监听
//分控收到0xb3 发送 0xb4
//分控收到0xb4 直接连接

typedef struct 
{
	SOCKET sSocket;//本地套接字
	
	SOCKADDR_IN addrRemote;//远程端口
	int nChannel;//连接的通道
	int nTimeout;//超时时间ms

	void* pChannel;
	BOOL bConnected;//是否已经连接过
	BOOL bIsHole;//是否是打洞包
}CONNECT_INFO;//UDT返回的数据包

typedef ::std::vector<CONNECT_INFO >CONNECT_LIST;//可以使用UDT连接

class CMakeHoleC  
{
public:
	void PrePunch(SOCKET s,SOCKADDR_IN addrA);

	BOOL AddConnect(CONNECT_INFO info,BOOL bFirst = FALSE);//添加一个可以连接的目标

	BOOL ConnectA(SOCKET s,SOCKADDR_IN *remote,int nTimeOut,UDTSOCKET& uSocket);//连接主控，收到包测试下
	void CreatUDT(SOCKET s);//创建UDT 用于多播接收
	CMakeHoleC(char *pGroup,int nYst,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatA,int nNatB,int nUpnp = 0,void* pChannel = NULL,BOOL bVirtual = FALSE);
	virtual ~CMakeHoleC();

	unsigned long m_dwStartTime;
	void Start();
	void End();

	SOCKET m_sWorkSocket;//9200套接字 固定端口
	int m_nNatA,m_nNatB;

	BOOL m_bRuning;

	BOOL m_bTryLocal;
	int m_nChannel;

	CONNECT_LIST m_UdtConnectList;//可以连接的列表
	pthread_mutex_t m_Connect;
	BOOL m_bConnectOK;

	void* m_pParent;
	BOOL m_bVirtual;//虚连接标致

	char m_strGroup[4];
	int m_nYst;

	ServerList m_ServerSList;//服务器列表

	SOCKADDR_IN m_addA;//主控地址
	SOCKADDR_IN m_addLocalA;//主控地址 内网

	int m_nLocalPortWan;//upnp端口

	SOCKET m_sLocalSocket[MAX_NUM];//本地连接套接字

	SOCKET m_sLocalSocket2;//本地连接套接字 连接成功的那一个

	UDTSOCKET m_udtSocket;//用于多播监听用

	NATList m_NatList;//本地对应服务器的外网NAT

	NATList m_NATListTEMP;//临时连接使用的所有的NAT 和 内网地址

	int GetLocalNat(int nMinTime);//外网端口和ip，局域网只是用老的固定端口
	void GetNATADDR();

	int GetAddress();//获取主控地址
	
#ifndef WIN32
	static void* ConnectThread(void* pParam);//直接连接线程，根据取得的地址连接
#else
	static UINT WINAPI ConnectThread(LPVOID pParam);//直接连接线程，根据取得的地址连接
#endif

#ifndef WIN32
	static void* ConnectRemoteProc(void* pParam);//发送线程，用于处理主连接
#else
	static UINT WINAPI ConnectRemoteProc(LPVOID pParam);//发送线程，用于处理主连接
#endif
	
#ifndef WIN32
	pthread_t m_hConnThread;//线程句柄
	BOOL m_bEndC;
	pthread_mutex_t m_ct;
#else
	HANDLE m_hConnThread;//线程句柄
	HANDLE m_hStartEventC;
	HANDLE m_hEndEventC;
	
	CRITICAL_SECTION m_ct;
#endif

	CCWorker* m_pWorker;
	int m_nConnectNum;//连接数量，最后变化为0时关闭套接字

};

typedef std::map<std::string, CMakeHoleC* > CONN_List;//正在连接的列表

#define NAT_TYPE_0_UNKNOWN			0	//未知类型 或没有探测出来的类型
#define NAT_TYPE_0_PUBLIC_IP		1	//公网,分控可以直接连接
#define NAT_TYPE_1_FULL_CONE		2	//全透明,分控可以直接连接
#define NAT_TYPE_2_IP_CONE			3	//ip限定
#define NAT_TYPE_3_PORT_CONE		4	//端口限定
#define NAT_TYPE_4_SYMMETRIC		5	//对称


class CMakeGroup//记录着所有的正在连接的信息
{
public:
	CMakeGroup();
	~CMakeGroup();

	void Start(CCWorker* pWorker);
	void SetConnect(SOCKET s,int nType);//设置连接数 nType = 1 连接上 +1，nType = -1 断开 -1
	
	CCWorker* m_pWorker;
	pthread_mutex_t m_MakeLock;
	CONN_List m_ConnectList;//正在连接的队列

	BOOL CheckConnect(char *strGroup,int nYstNo);//检查是否有正在连接的动作,一个号码只允许一个在穿透

	BOOL AddConnect(void* pChannel,char *strGroup,int nYstNo,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatTypeA,int nUpnp,BOOL bVirtual);

	int GetNatType(char* strGroup);//NAT类型检测 0 未知，1 公网， 2~5为4种类型
	BOOL IsThisLocal(SOCKADDR_IN);//地址是否在本地地址列表中 用于公网判断

	ServerList m_ServerList;//所有服务器列表
	NATList m_LocalIPList;//本地IP列表

	int m_nNatTypeB;//类型检测 0 未知，1 公网， 2~5为4种类型


#ifndef WIN32
	static void* CheckNatProc(void* pParam);//NAT检测线程
#else
	static UINT WINAPI CheckNatProc(LPVOID pParam);//NAT检测线程
#endif
	
#ifndef WIN32
	pthread_t m_hCheckThread;//线程句柄
	BOOL m_bCheckEndC;
	pthread_mutex_t m_ctCheck;
#else
	HANDLE m_hCheckThread;//线程句柄
	HANDLE m_hCheckStartEventC;
	HANDLE m_hCheckEndEventC;
	
	CRITICAL_SECTION m_ctCheck;
#endif

};

#endif // !defined(AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_)
