// CHelpCtrl.h: interface for the CCHelpCtrl class.


#if !defined(AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_)
#define AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include <map>


class CCWorker;
class CCChannel;

/*助手和cv通信类型*/
#define JVC_HELP_KEEP		 0x01//交互心跳包
#define JVC_HELP_CONNECT	 0x03//分控请求助手连接主控
#define JVC_HELP_DISCONN	 0x04//分控请求助手断开主控

#define JVC_HELP_CVDATA      0x05//分控经小助手向主控发送的数据

#define JVC_HELP_CONNSTATUS  0x06//助手的实连接状态
#define JVC_HELP_DATA        0x07//助手向分控转交的普通数据 

#define JVC_HELP_VSTATUS     0x08//助手向分控发送的虚连接情况
#define JVC_HELP_NEWYSTNO    0x09//分控关注的新号码，助手需要考虑建立虚连接


typedef struct STCONNECTINFO
{
	BOOL bYST;
	int nLocalChannel;
	int nChannel;
	int nLocalPort;
	char chServerIP[16];
	int nServerPort;
	int nYSTNO;
	char chGroup[4];
	BOOL bLocalTry;
	char chPassName[MAX_PATH];
	char chPassWord[MAX_PATH];
	
	int nTURNType;
	int nConnectType;
	BOOL bCache;//作为高速缓存，全速上传
	
	int nShow;//连接方式 P2P/TURN 用于连接成功提示
	
	int nWhoAmI;//
	SOCKADDR_IN quickAddr;//快连地址
	
	///////////////////////////////////////////////////////////////以下添加不需要用在通信上
	unsigned short m_wLocalPort;//本地端口 给移动设备用
	SOCKET sRandSocket;
	BOOL isBeRequestVedio;//是否立即请求主控发送视频
    int nVIP ;
    int nOnlyTCP;
    
	STCONNECTINFO()
	{
        nOnlyTCP =0;
        isBeRequestVedio = TRUE;
		sRandSocket = 0;
		bYST = FALSE;
		nLocalChannel = 0;
		nChannel = 0;
		nLocalPort = 0;
		memset(chServerIP, 0, 16);
		nServerPort = 0;
		nYSTNO = 0;
        nVIP = 0;
		memset(chGroup, 0, 4);
		bLocalTry = FALSE;
		memset(chPassName, 0, MAX_PATH);
		memset(chPassWord, 0, MAX_PATH);
		
		nTURNType = 0;
		nConnectType = 0;
		bCache = FALSE;//作为高速缓存，全速上传
		
		nWhoAmI = 0;//
	}
}STCONNECTINFO;

class CCVirtualChannel;
typedef struct STVLINK
{
	char chGroup[4];
	int nYSTNO;
	BOOL bStatus;//虚连接状态 TRUE有效，FALSE无效
	BOOL bTURN;//是否转发，转发的话虚连接就没多少实际意义了，应该直接断开并每隔5分钟再进行一次连接尝试，看是否能建立直连

	int nChannel;//要连接的目标通道
	SOCKADDR_IN addrVirtual;//与该设备虚连接对应的地址

	int nConnectType;//虚连接的状态
	char chUserName[32];
	char chPasswords[32];

	DWORD dwLastVConnect;

	CCVirtualChannel* pVirtualChannel;
	int nFailedCount;
//	unsigned short m_wLocalPort;//本地端口 给移动设备用
	SOCKET sRandSocket;
	STVLINK()
	{
		memset(chGroup, 0, 4);
		nYSTNO = 0;
		bStatus = FALSE;
		bTURN = FALSE;
		nChannel = 0;
		nConnectType = 0;
		memset(chUserName, 0, 32);
		memset(chPasswords, 0, 32);
		dwLastVConnect = 0;
		pVirtualChannel = NULL;
		nFailedCount = 0;

//		m_wLocalPort = 0;
		sRandSocket = 0;
	}

}STVLINK;

typedef std::map<std::string, STVLINK > VCONN_List;//本地所有虚连接列表	云视通 ->数据

//本地TCP连接的结构	维护与助手的连接和查询 有无虚连接
class CCHelpCtrl;
typedef struct _LOCAL_TCP_CMD
{
	CCHelpCtrl* pCtrl;
	
	SOCKET sTcpCmdSocket;
	DWORD dwSendTime;//发送时间 s
	DWORD dwRecvTime;//接收时间 s
}LOCAL_TCP_CMD,*PLOCAL_TCP_CMD;

typedef std::map<SOCKET, PLOCAL_TCP_CMD>CMD_LIST;

//分控连接助手，发送 接收助手数据
typedef struct _LOCAL_TCP_DATA
{
	CCHelpCtrl* pCtrl;
	CCChannel* pChannel;
	
	DWORD dwLocalChannel;//本地标志	
	SOCKET sTcpDataSocket;
	DWORD dwSendTime;//发送时间 ms
	DWORD dwRecvTime;//接收时间 ms

}LOCAL_TCP_DATA,*PLOCAL_TCP_DATA;

typedef std::map<int, PLOCAL_TCP_DATA>DATA_LIST;


typedef struct STCONNBACK
{
	int nRet;//结果 0没数据 1成功 2失败
	BOOL bJVP2P;//是否是多播方式
	int nFYSTVER;
	BYTE pMsg[1024];
	int nSize;
	int  nTmp;//备用
	STCONNBACK()
	{
		bJVP2P = FALSE;
	}
}STCONNBACK;

//基本类 在CWorker中定义一个实例
class CCHelpCtrl  
{
public:
	CCHelpCtrl();
	CCHelpCtrl(CCWorker *pWorker);
	virtual ~CCHelpCtrl();

public:
	void ClearCache(void){return;};
	virtual BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize){return FALSE;};//璁剧疆��风��琛�
	virtual int GetHelpYSTNO(BYTE *pBuffer, int &nSize){return 0;};//��峰����风��琛�

	virtual int SearchYSTNO(STVLINK *stVLink){return 0;};//查询某号码基本信息

	DWORD JVGetTime();
	static void jvc_sleep(unsigned long time);//挂起

	virtual void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg){};//与主控端通信状态函数(连接状态)
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//实际连接成功后要求助手按照此连接连接主控
public:
	CCWorker* m_pWorker;
	int m_nHelpType;//启用类型1;2;3
	BOOL m_bExit;//是否退出工作线程
private:
};

//该类小助手端使用
class CCHelpCtrlH:public CCHelpCtrl  
{
public:
	CCHelpCtrlH();
	CCHelpCtrlH(CCWorker *pWorker);
	virtual ~CCHelpCtrlH();

	//向客户端发送数据	由channel调用
	int SendDataToCV(SOCKET s,char* pBuff,int nLen);
	//从客户端接收数据	由channel调用
	int ReceiveDataFromCV(SOCKET s,char* pBuff,int nMaxLen);
	
public:
	BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize);//STBASEYSTNO
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO

	BOOL StartWorkThread(int nPortCmd = 9000,int nPortData = 8000);//开启监听线程，开启日常业务线程	9000	8000
	
	BOOL VirtualConnect();//与某号码建立虚连接
	BOOL ConnectYST(STCONNECTINFO st,LOCAL_TCP_DATA* pTcp);//与某号码建立实连接，内部会先使用虚连接地址，不通时再原流程
	
	void DealwithLocalClientCmd();//接收cv的心跳 新添加虚连接 返回虚连接列表
	
	int CallbackFromChannel(SOCKET s,char* pBuff,int nLen);//channel收到数据后调用
	
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//实际连接成功后要求助手按照此连接连接主控
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg);//与主控端通信状态函数(连接状态)
#ifdef WIN32
	static UINT WINAPI WorkCmdProc(LPVOID pParam);//发送本地的虚拟连接列表 接收新的虚连接请求
	static UINT WINAPI WorkDataProc(LPVOID pParam);//接收客户端数据 发送主控 接收主控 发送到客户端
	static UINT WINAPI ListenProcCmd(LPVOID pParam);//监听线程，监听本地客户端连接
	static UINT WINAPI ListenProcData(LPVOID pParam);//监听线程，监听本地客户端连接
#else
	static void* WorkCmdProc(void* pParam);//发送本地的虚拟连接列表 接收新的虚连接请求
	static void* WorkDataProc(void* pParam);//接收客户端数据 发送主控 接收主控 发送到客户端
	static void* ListenProcCmd(void* pParam);//监听线程，监听本地客户端连接
	static void* ListenProcData(void* pParam);//监听线程，监听本地客户端连接
#endif

public:
	//锁cv与助手之间的数据连接
#ifndef WIN32
	pthread_mutex_t m_criticalsectionHList;
#else
	CRITICAL_SECTION m_criticalsectionHList;
#endif
	DATA_LIST m_TcpListDataH;//本机TCP连接 用于助手和cv之间数据传输

private:
	SOCKET m_ListenSockCmd;//小助手本地监听线程	监听客户连接 用于命令发送
	SOCKET m_ListenSockData;//小助手本地监听线程 用于传输真实数据
	
	VCONN_List m_VListH;//虚拟的连接
	
	CMD_LIST m_TcpListCmd;//本机TCP连接 命令和查询传输

#ifndef WIN32
	pthread_t m_hWorkCmdThread;//线程句柄
	pthread_t m_hWorkDataThread;//线程句柄
	pthread_t m_hListenCmdThread;//线程句柄
	pthread_t m_hListenDataThread;//线程句柄
	
	pthread_mutex_t m_criticalsectionVList;
#else
	HANDLE m_hWorkCmdThread;//线程句柄
	HANDLE m_hWorkDataThread;//线程句柄
	HANDLE m_hListenCmdThread;//线程句柄
	HANDLE m_hListenDataThread;//线程句柄
	
	CRITICAL_SECTION m_criticalsectionVList;//
#endif

};

//该类客户端使用,与小助手配合
class CCHelpCtrlP:public CCHelpCtrl  
{
public:
	CCHelpCtrlP();
	CCHelpCtrlP(CCWorker *pWorker);
	virtual ~CCHelpCtrlP();

public:
	BOOL ConnectHelp(char* pHelperName = "127.0.0.1",int nHelpPort = 9000);
	int SearchYSTNO(STVLINK *stVLink);//查询某号码基本信息
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg){};//与主控端通信状态函数(连接状态)

	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//实际连接成功后要求助手按照此连接连接主控
private:
	BOOL StartWorkThread();//开启日常业务线程 开启小助手数据接收线程
	
	#ifdef WIN32
		static UINT WINAPI CommWithHelpProc(LPVOID pParam);//同步help的虚拟连接信息
        static UINT WINAPI ConnectHelpProc(LPVOID pParam);//同步help的虚拟连接信息
	#else
		static void* CommWithHelpProc(void* pParam);//同步help的虚拟连接信息
        static void* ConnectHelpProc(void* pParam);//同步help的虚拟连接信息
	#endif
	
private:
	SOCKET m_ConnectCmdSock;//连接助手的TCP连接
	VCONN_List m_VListC;//cv 保存的虚拟连接状态 数据来自Help

	#ifndef WIN32
		pthread_t m_hCWHThread;//线程句柄
		pthread_t m_hConnHelpThread;
	#else
		HANDLE m_hCWHThread;//线程句柄
		HANDLE m_hEndEventWH;

		HANDLE m_hConnHelpThread;
	#endif
};

//该类
class CCHelpConnCtrl
{
public:
	CCHelpConnCtrl();
	virtual ~CCHelpConnCtrl();
	
public:
	BOOL ConnectYSTNO(STCONNECTINFO stConnInfo);
	int RecvConnResult(STCONNBACK *tconnback);
	
	int SendToHelp(BYTE *pBuffer, int nSize);
	int RecvFromHelp(BYTE *pBuffer, int nSize);
	int SendToHelpActive();//向助手发送心跳

	void DisConnectYSTNO();

public:
	SOCKET m_tcpsock;//连接助手	与助手发送接收数据
private:
	
};

//该类客户端使用，自己提速，不用小助手，用于手机等客户端
class CCHelpCtrlM:public CCHelpCtrl  
{
public:
	CCHelpCtrlM();
	CCHelpCtrlM(CCWorker *pWorker);
	virtual ~CCHelpCtrlM();
	
public:
	void ClearCache(void);
	int SearchYSTNO(STVLINK *stVLink);//��ヨ�㈡����风����烘��淇℃��
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg);//涓�涓绘�х�����淇＄�舵����芥��(杩���ョ�舵��)
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort);//瀹����杩���ユ��������瑕�姹���╂�������ф�よ����ヨ����ヤ富���
    
    void HelpRemove(char* pGroup,int nYST);
    
    
    int GetHelper(char* pGroup,int nYST,int *nCount);
    void StopHelp();
private:
	//添加号码
	virtual BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize);//设置号码表
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO
	BOOL StartWorkThread();
	
	void VirtualConnect();
	void VirtualGetConnect();//获取连接信息进行保存
	#ifdef WIN32	
		static UINT WINAPI WorkCmdProc(LPVOID pParam);
	#else
		static void*  WorkCmdProc(void* pParam);//检测虚连接状态
	#endif	

private:
	VCONN_List m_VListM;//虚拟的连接
	pthread_mutex_t m_VListMLock;
	int m_nLocalChannelIndex;//+1递增

	#ifndef WIN32
		pthread_t m_hWorkCmdThread;//线程句柄
	#else
		HANDLE m_hWorkCmdThread;//线程句柄
	#endif
    BOOL m_bExitSignal;
};


#endif // !defined(AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_)
