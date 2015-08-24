// CChannel.h: interface for the CCChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCHANNEL_H__633B6DEC_700B_41BD_B536_8132C3965D81__INCLUDED_)
#define AFX_CCHANNEL_H__633B6DEC_700B_41BD_B536_8132C3965D81__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CBufferCtrl.h"
#include "CPartnerCtrl.h"
#include "COldChannel.h"

#include "CHelpCtrl.h"

//#include "UpnpImpl.h"
#include<stdlib.h>
#include<time.h>
#define random() (9200 + rand()%50000)

#define JVN_TIME_CHECKLINK  300//500//连接检查周期
#define JVN_TIME_LINK       500//连接周期
#define JVN_TIME_SENDBMC    20//600//1000//3000//BM发送周期,做高速缓存时可放慢发送频率
#define JVN_TIME_SENDBM     40//200//500//1000//BM发送周期
#define JVN_TIME_REQBMD     30//300//600//1000//400//300//BMD请求周期
#define JVN_TIME_SENDBMD    0//BMD发送周期
#define JVN_TIME_RECV       0//数据接收周期
#define JVN_TIME_GT         1000//500//无效伙伴清理周期
#define JVN_TIME_PLAY       30//播放检查周期
#define JVN_TIME_TURNC      60000//检查播放效果周期


//尝试路线，即要走过的流程
#define JVN_WHO_H      1//小助手
#define JVN_WHO_P      2//客户端，小助手是独立线程，比如cv分控
#define JVN_WHO_M      3//客户端，小助手不存在独立线程，比如手机

#define JVN_HELPRET_NULL  0//没有号码信息
#define JVN_HELPRET_LOCAL 1//有该号码信息 同内网 本地直连即可
#define JVN_HELPRET_TURN  2//有该号码信息 转发 客户端本地直接转发即可
#define JVN_HELPRET_WAN   3//有该号码信息 外网直连 客户端从小助手连接及取数据即可


/***************连接状态*****************************************/
#define  NEW_IP              1//等待进行IP直连
#define  WAIT_IPRECHECK      2//等待IP连接有效性验证
#define  WAIT_VERCHECK       3//等待主版本验证
#define  WAIT_IPPWCHECK      4//等待身份验证

#define  OK                  5//连接成功
#define  FAILD               6//连接失败

#define  NEW_P2P             7//等待进行外网连接
#define  WAIT_S              8//等待服务器响应
#define  WAIT_S2             9//等待服务器响应
#define  RECVA_S             10//从服务器取得地址
#define  NEW_P2P_L           11//需要内网探测
#define  WAIT_LRECHECK       12//等待L连接有效性验证
#define  WAIT_LPWCHECK       14//等待身份验证
#define  NEW_P2P_N           15//需要外网直连
#define  WAIT_NRECHECK       16//等待N连接有效性验证
#define  WAIT_NPWCHECK       18//等待身份验证

#define  NEW_TURN            19//等待进行转发连接
#define  WAIT_TURN           20//等待转发地址
#define  RECV_TURN           21//取得转发地址
#define  WAIT_TSRECHECK      22//等待TURN连接有效性验证
#define  WAIT_TSPWCHECK      24//等待身份验证

#define  WAIT_IP_CONNECTOLD  25//兼容旧版主控,ip直连没收到预验证回复
#define  WAIT_LW_CONNECTOLD  26//兼容旧版主控,内网探测没收到预验证回复，连接旧版失败后还需要继续执行原连接过程
#define  WAIT_NW_CONNECTOLD  27//兼容旧版主控,外网探测没收到预验证回复
#define  WAIT_TSW_CONNECTOLD 28//兼容旧版主控,转发探测没收到预验证回复

#define  WAIT_IP_CONNECTOLD_F  29//兼容旧版主控
#define  WAIT_LW_CONNECTOLD_F  30//兼容旧版主控
#define  WAIT_NW_CONNECTOLD_F  31//兼容旧版主控
#define  WAIT_TSW_CONNECTOLD_F 32//兼容旧版主控

#define  OK_OLD                33//兼容旧版主控

#define  WAIT_SER_REQ         34//服务器号码查询，及询问服务器该号码是否上线
#define  WAIT_SER_RSP         35//等待服务器查询结果，顺便判断出每个服务器响应速度

#define  NEW_HAVEIP           36//使用已有的IP，等待进行IP直连

#define  NEW_YST              37//新云视通号码连接
#define  NEW_VIRTUAL_IP       38//有虚连接提供的ip
#define  NEW_HELP_CONN        39//从小助手连接和取数据
#define  WAIT_HELP_RET        40//从小助手等待连接结果
#define  HELP_OK              41//通过小助手连接成功

#define  NEW_TCP_IP           42//tcp连接
#define  NEW_TCP_P2PL         43//内网探测tcp连接
#define  NEW_VTCP_IP          44//tcp连接 从小助手的到的地址 连接失败后应该尝试原流程

#define  WAIT_TCPIPRECHECK    45//直连中等待tcp身份验证消息
#define  WAIT_LTCPRECHECK     46//内网探测中等待tcp身份验证消息 暂时没用到


#define  WAIT_INDEXSER_REQ    47//去索引服务器查询号码在线服务器，发送请求包
#define  WAIT_INDEXSER_RSP    48//去索引服务器查询号码在线服务器，接收返回包

#define  WAIT_MAKE_HOLE		  49//等待新流程连接
#define  NEW_HAVETCP           50//使用已有的IP，等待进行IP直连 TCP使用
#define  NEW_HAVE_LOCAL        51//使用已有的IP，等待进行IP直连 TCP使用


#define  WAIT_CHECK_A_NAT	52	//查询主控NAT类型
#define  WAIT_NAT_A			53	//等待主控NAT的返回

#define  DIRECT_CONNECT		  54//根据提供的参数直接连接

#define  REQ_FORWARD_TURNADDR		60	// 请求云视通服务器转发请求turn地址
#define  RECV_TURNLIST  61// 取得转发地址列表
#define  REQ_DEV_PUBADDR  62// 请求设备公网地址
#define  JVN_CHECK_DEVADDR        0x203
#define JVN_CONN_DEV            0x204
/**********************************************************************/

typedef struct NAT//分控所在的NAT结构
{
    unsigned char ip[4];//NAT的地址
    unsigned short port;//0~65535 在NAT上的端口
    int nSVer;//服务器版本号
    unsigned int unseraddr;//服务器ip
    NAT()
    {
        memset(ip, 0, 4);
        port = 0;
        nSVer = 0;
        unseraddr = 0;
    }
}NAT;
typedef ::std::vector<NAT> NATList;//本地的IP列表

typedef struct
{
    SOCKET sock;
    SOCKADDR_IN addr;
    DWORD dwStartTime;
}STCLIENTTMP;

typedef struct STSERVER
{
    SOCKADDR_IN addr;//服务器地址
    BOOL buseful;//服务器是否已进行过尝试
	BOOL bOK;//服务器是否可通
    int nver;//协议版本
}STSERVER;

typedef ::std::vector<STSERVER> ServerList;
typedef ::std::vector<STCLIENTTMP> ClientTmpList;

typedef struct STCONNPROCP
{
    SOCKET udpsocktmp;
    sockaddr_in sin;
    SOCKADDR sockAddr;
    int nSockAddrLen;
    BYTE chdata[JVN_ASPACKDEFLEN];
    ServerList slisttmp;
    
    DWORD dwendtime;
    
    char chAVersion[MAX_PATH];
    char chError[20480];
    
    //#ifndef WIN32
    char szIPAddress[100];
    //#else
    //LPSTR szIPAddress;
    //#endif
    
    STCONNPROCP()
    {
        udpsocktmp = 0;
    }
    
    ~STCONNPROCP()
    {
        if(udpsocktmp > 0)
        {
            closesocket(udpsocktmp);
            udpsocktmp = 0;
        }
    }
}STCONNPROCP;//连接线程临时参数
/*
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
 
	STCONNECTINFO()
	{
 bYST = FALSE;
 nLocalChannel = 0;
 nChannel = 0;
 nLocalPort = 0;
 memset(chServerIP, 0, 16);
 nServerPort = 0;
 nYSTNO = 0;
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
 */
typedef struct STYSTNOINFO
{
    int nYSTNO;//号码
    char strGroup[4];
    int nConnST;//连接状态0连接中；>0 连接成功，IP可直接使用；
    SOCKADDR_IN addrA;//连接成功所用的IP地址，其他相同连接可重用
    ServerList stSList;//服务器在线状态，如果在有效期内，其他连接不必重复获取
    DWORD dwSerTime;//服务器刷新时间单位ms 有效期为30秒(30000)
    SOCKET sSocket;//连接的套接字 UDT绑定时使用
}STYSTNOINFO;//云视通号码信息
typedef ::std::vector<STYSTNOINFO> YSTNOList;

typedef struct CYstSvrList
{
	ServerList addrlist;
	char chGroup[10];
	CYstSvrList()
	{
		memset(chGroup,0,sizeof(chGroup));
		addrlist.clear();
	}
}CYstSvrList;
typedef ::std::vector<CYstSvrList> CYstSvrGroupList; // 云视通服务器所有编组列表

class CCWorker;
class CCOldChannel;

class CMakeHoleC;

#ifdef WIN32
typedef HANDLE pthread_mutex_t;
#endif

class CCChannel
{
public:
    CCChannel();
    CCChannel(STCONNECTINFO stConnectInfo, CCWorker *pWorker);
    virtual ~CCChannel();
    
    BOOL SendData(BYTE uchType, BYTE *pBuffer,int nSize);
    BOOL DisConnect();
    
    int Send2A(BYTE *puchBuf, int nSize);
    
    BOOL CheckREQ(unsigned int unChunkID);
    BOOL SendBMDREQ2A(unsigned int unChunkID);//发送数据片请求
    
    int CheckVersion(short sV1, short sV2, short sV3, short sV4);
    
    int SendCMD(BYTE uchType, BYTE* pBuffer, int nSize);
    
    void GetPartnerInfo(char *pMsg, int &nSize);
    void ClearBuffer();
    
    static BOOL ServerIsExist(STCONNPROCP *pstConn,char* pServer);//判断服务器是否已经返回结果
    
    void UpdateYSTNOList();//更新连接地址

    int SelectAliveSvrList(ServerList OnlineSvrList);// 挑选出畅通的服务器地址列表 m_AliveSvrList，返回畅通服务器数量
	void ReqTurnAddrViaSvr(void);// 向畅通的服务器地址转发请求转发地址
	BOOL SendReqTurnAddr(SOCKADDR_IN addr,SOCKET stmp);
	int RcvTurnAddrList(SOCKET stmp);
	void AddYstSvr(STSERVER svr);
public://////////////////////////////////////////////////////////////////////////
    DWORD m_dwLastUpdateTime;//最后更新时间，加个间隔没必要经常更新
    BOOL m_bIsTurn;
    int m_nLocalChannel;//本地通道号 >=1
    int m_nChannel;//服务通道号 >=1
    
    int m_nLocalStartPort;
    UDTSOCKET m_ServerSocket;//对应服务的socket
    int m_nLinkID;//本连接在主控端的唯一标识
    UDTSOCKET m_ListenSocket;
    
    
    //TCP...........
    SOCKET m_ListenSocketTCP;//本地TCP监听套接字  用于接收分控连接 分控间首选TCP连接
    
    SOCKET m_SocketSTmp;//临时套接字，TCP连接及转发服务器交互使用
    SOCKADDR_IN m_addrAN;//主控外网地址
    SOCKADDR_IN m_addrAL;//主控内网地址
    SOCKADDR_IN m_addrTS;//转发服务器地址
    SOCKADDR_IN m_addrANOLD;//主控外网地址保留地址
    int m_nStatus;//主连接当前状态
    DWORD m_dwStartTime;//主连接关键计时
    SOCKADDR_IN m_addressA;
    BOOL m_bPassWordErr;//是否检测到身份验证回复
    
    SOCKADDR_IN m_addrLastLTryAL;//上次尝试过的内网探测地址
    
    //////////////////////////////////////////////////////////////////////////
    CCBaseBufferCtrl *m_pBuffer;
    //------伙伴网络性能-----------
    int m_nTimeOutCount;
    DWORD m_dwRUseTTime;//接收实际总耗时 ms
    int m_nLPSAvage;//本地下行带宽/该伙伴的上行带宽(KB/S)(请求数据量小,下载回复数据量大,所以下载为主)
    int m_nDownLoadTotalB;
    int m_nDownLoadTotalKB;
    int m_nDownLoadTotalMB;
    int m_nLastDownLoadTotalB;
    int m_nLastDownLoadTotalKB;
    int m_nLastDownLoadTotalMB;
    DWORD m_dwLastInfoTime;
    //------伙伴最新BM-------------
    unsigned int m_unBeginChunkID;//有效的起始数据块
    int m_nChunkCount;//有效地数据块数目
    
    int m_nLastRate;//上次提示的缓冲进度
    //////////////////////////////////////////////////////////////////////////
    
    BOOL m_bJVP2P;//是否多播方式(依赖于主控)
    
    BOOL m_bLan2A;//当前节点与主控同内网
    
    BOOL m_bDAndP;//下载回放中
    BOOL m_bCache;//是否作为高速缓存
    BOOL m_bTURN;//是否基本转发
    BOOL m_bOpenTurnAudio;//打开音频全转发
    
    int m_nFYSTVER;//远端云视通协议版本
    
    STCONNECTINFO m_stConnInfo;
    ServerList m_SList;//P2P连接时使用
    ServerList m_SListTurn;//TURN连接时使用
    ServerList m_SListBak;//备份暂无用
    
    ServerList m_ISList;//索引服务器列表
    
    BOOL m_bShowInfo;//是否已经对外提示过连接信息
    
    BYTE *m_preadBuf;//播放容器，由于多播方式时流式接收，没有空闲时刻，因此需要该容器
    BYTE *m_puchRecvBuf;//接收临时缓存，用于控制多次发送
    
    //助手与分控之间的通信包
    char *m_pchPartnerInfo;
    int m_nPartnerLen;
    
    CCPartnerCtrl m_PartnerCtrl;
    
    CCOldChannel *m_pOldChannel;//对旧版主控兼容
    
    BOOL m_recvThreadExit;//接收数据线程 线程退出标志
    BOOL m_connectThreadExit;//连接线程退出标志
    
    int m_nProtocolType;//连接协议类型 TYPE_PC_UDP/TYPE_PC_TCP/TYPE_MO_TCP 用于内部选用收发函数
    
    NATList m_NatList;
    
    SOCKADDR_IN m_addrConnectOK;//连接成功的地址保存的主控地址
    SOCKET m_sQueryIndexSocket;//查询检索服务器的套接字
    int ReceiveIndexFromServer(STCONNPROCP *pstConn,BOOL bFirstCall);
    
	CYstSvrList m_GroupSvrList; //组全部服务列表
	CYstSvrList m_AliveSvrList;// 可能通畅的服务器地址列表，随机排序
	ServerList m_TurnSvrAddrList; // 转发服务器地址列表
private://////////////////////////////////////////////////////////////////////////
    BOOL SendDataTCP(BYTE uchType, BYTE *pBuffer,int nSize);
    
    BOOL SendHelpData(BYTE uchType, BYTE *pBuffer,int nSize);
    
    BOOL CheckInternalIP(const unsigned int ip_addr);
    
    void NatTry2Partner(char chIP[16], int nPort);
    //
    BOOL ConnectIP();
    BOOL ConnectIPTCP();
    BOOL SendReCheck(BOOL bYST);
    int RecvReCheck(int &nver, char chAVersion[MAX_PATH]);
    BOOL SendPWCheck();
    int RecvPWCheck(int &nPWData);
    BOOL StartWorkThread();
    BOOL StartHelpWorkThread();
    BOOL SendSP2P(SOCKADDR_IN addrs, int nIndex, char *pchError);
    BOOL SendSP2PTCP(SOCKADDR_IN addrs, int nIndex, char *pchError);
    int RecvS(SOCKADDR_IN addrs, int nIndex, char *pchError);
    int RecvSTCP(int nIndex, char *pchError);
    BOOL ConnectLocalTry(int nIndex, char *pchError);
    BOOL ConnectNet(int nIndex, char *pchError);
    BOOL ConnectNetTCP(int nIndex, char *pchError);
    BOOL ConnectNetTry(SOCKADDR_IN addrs, int nIndex, char *pchError);
    BOOL SendSTURN(SOCKADDR_IN addrs, int nIndex, char *pchError);
    int RecvSTURN(int nIndex, char *pchError);
    BOOL ConnectTURN(int nIndex, char *pchError);
    
    void SetBM(unsigned int unChunkID, int nCount, DWORD dwCTime[10]);
    
    BOOL ParseMsg(BYTE *chMap);//解析接收到的数据包，主要作用是检查是否接收完毕，以及区分粘包
    BOOL ParseHelpMsg(BYTE *chMap);//解析接收到的数据包，主要作用是检查是否接收完毕，以及区分粘包
    
    void ProcessDisConnect();
    
    //打洞函数
    BOOL Punch(int nYSTNO,int sock,SOCKADDR_IN *addrA);
    //预打洞函数
    void PrePunch(int sock,SOCKADDR_IN addrA);
    
    /*连接处理函数*/
    void DealNewYST(STCONNPROCP *pstConn);
    void DealNewVirtualIP(STCONNPROCP *pstConn);
    void DealNewHelpConn(STCONNPROCP *pstConn);
    void DealWaitHelpRET(STCONNPROCP *pstConn);
    void DealHelpOK(STCONNPROCP *pstConn);
    
    void DealWaitSerREQ(STCONNPROCP *pstConn);
	int WaitDevPubAddr(SOCKET stmp,ServerList &list);
	int WaitTurnAddrList(STCONNPROCP *pstConn);
	void DealWaitReqDevPubAddr(STCONNPROCP *pstConn);
	int DealRcvCompleteTurn(STCONNPROCP *pstConn);
	void DealReqCompleteTurn(STCONNPROCP *pstConn);

    void DealWaitSerRSP(STCONNPROCP *pstConn);
    void DealNewTCP(STCONNPROCP *pstConn);//直接TCP连接
    void DealNewIP(STCONNPROCP *pstConn);
    void DealWaitIPRECheck(STCONNPROCP *pstConn);
    void DealWaitIPConnectOldF(STCONNPROCP *pstConn);
    void DealWaitIPPWCheck(STCONNPROCP *pstConn);
    void DealOK(STCONNPROCP *pstConn);
    void DealFAILD(STCONNPROCP *pstConn);
    void DealNEWP2P(STCONNPROCP *pstConn);
    void DealWaitS(STCONNPROCP *pstConn);
    void DealNEWP2PL(STCONNPROCP *pstConn);
    void DealWaitLRECheck(STCONNPROCP *pstConn);
    void DealWaitLWConnectOldF(STCONNPROCP *pstConn);
    void DealWaitLPWCheck(STCONNPROCP *pstConn);
    void DealNEWP2PN(STCONNPROCP *pstConn);
    void DealWaitNReCheck(STCONNPROCP *pstConn);
    void DealWaitNWConnectOldF(STCONNPROCP *pstConn);
    void DealWaitNPWCheck(STCONNPROCP *pstConn);
    
    void DealNEWTURN(STCONNPROCP *pstConn);
    void DealWaitTURN(STCONNPROCP *pstConn);
    void DealRecvTURN(STCONNPROCP *pstConn);
	BOOL DealRecvTURNLIST(STCONNPROCP *pstConn);
    void DealWaitTSReCheck(STCONNPROCP *pstConn);
    void DealWaitTSWConnectOldF(STCONNPROCP *pstConn);
    void DealWaitTSPWCheck(STCONNPROCP *pstConn);
    
    void DealNEWTCPIP(STCONNPROCP *pstConn);
    void DealNEWTCPP2PL(STCONNPROCP *pstConn);
    void DealNEWVTCPIP(STCONNPROCP *pstConn);
    
    void DealWaitIndexSerREQ(STCONNPROCP *pstConn);
    void DealWaitIndexSerRSP(STCONNPROCP *pstConn);
    
    void DealWaitNatSerREQ(STCONNPROCP *pstConn);//通过服务器查询主控的NAT类型 在将来检索服务器添加后可以取消此步骤
    void DealWaitNatSerRSP(STCONNPROCP *pstConn);//等待主控NAT类型
    
    void DealMakeHole(STCONNPROCP *pstConn);
    void StartConnThread();
    void GetSerAndBegin(STCONNPROCP *pstConn);
    
    int CheckNewHelp();//检查是否已经内网探测到该号码 0没有 1 tcp可以 2 udp可以(暂时没有写)
    
#ifndef WIN32
    static void* ConnProc(void* pParam);//发送线程，用于处理主连接
    static void* RecvProc(void* pParam);//接收线程，用于接收主控数据
    static void* PartnerProc(void* pParam);//接收线程，用于接收伙伴交互数据
    static void* PTConnectProc(void* pParam);//伙伴连接线程，由于连接会引起短暂阻塞，为不影响数据收发放入单独线程
    static void* RecvProcTCP(void* pParam);//接收线程，勇于接收TCP数据
    static void* RecvHelpProc(void* pParam);//接收线程
#else
    static UINT WINAPI ConnProc(LPVOID pParam);//发送线程，用于处理主连接
    static UINT WINAPI RecvProc(LPVOID pParam);//接收线程，用于接收主控数据
    static UINT WINAPI PartnerProc(LPVOID pParam);//接收线程，用于接收伙伴交互数据
    static UINT WINAPI PTConnectProc(LPVOID pParam);//伙伴连接线程，由于连接会引起短暂阻塞，为不影响数据收发放入单独线程
    static UINT WINAPI RecvProcTCP(LPVOID pParam);//接收线程，勇于接收TCP数据
    static UINT WINAPI RecvHelpProc(LPVOID pParam);//接收线程
#endif
    
public://////////////////////////////////////////////////////////////////////////
#ifndef WIN32
    static int receivefromm(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr * from,int * fromlen,int ntimeoverSec);//毫秒
    static int receivefrom(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr * from,int * fromlen,int ntimeoverSec);
    static int sendtoclient(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverSec);
    static int sendtoclientm(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverm);
    static int connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec);
#else
    static int receivefromm(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr FAR * from,int FAR * fromlen,int ntimeoverSec);//毫秒
    static int receivefrom(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr FAR * from,int FAR * fromlen,int ntimeoverSec);
    static int sendtoclient(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverSec);
    static int sendtoclientm(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverm);
    static int connectnb(SOCKET s,struct sockaddr FAR * to,int namelen,int ntimeoverSec);
#endif
    
    static int tcpreceive(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec);
    static int tcpreceive2(SOCKET s,char *pchbuf,int nsize,int ntimeoverMSec);
    static int tcpsend(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec);
    
    static int udpsenddata(UDTSOCKET sock, char *pchbuf, int nsize, BOOL bcomplete);//发送udp数据(是否一次发送完整)
    static int tcpsenddata(SOCKET sock, char *pchbuf, int nsize, BOOL bcomplete);//发送tcp数据(是否一次发送完整)
    static void WaitThreadExit(HANDLE &hThread);//强制退出线程
    
    BOOL ConnectStatus(SOCKET s,SOCKADDR_IN* addr,int nTimeOut,BOOL bFinish,UDTSOCKET uSocket);
    
private://////////////////////////////////////////////////////////////////////////
	BOOL m_bAcceptChat;//接受语音请求
	BOOL m_bAcceptText;//接受文本请求

	BOOL m_bPass;//身份验证成功
	CCWorker *m_pWorker;

	DWORD m_dwLastCommTime;//与助手最后的通信时间
	BOOL m_bExit;

	DWORD m_dwLastDataTime;//最后一次收到数据的时间

	int m_nRecvPos;//临时缓存数据包已发送长度
	int m_nRecvPackLen;//临时缓存数据包总长度

	int m_nOCount;//收到的帧数
	DWORD m_dBeginTime;//视频数据计时
	DWORD m_dTimeUsed;
	BOOL m_bError;//数据出错，此时视频帧必须从I帧开始继续接收，防止出现马赛克

	BOOL m_bDisConnectShow;//是否已经提示过连接断开

	CCHelpConnCtrl *m_pHelpConn;//小助手连接

	::std::vector<NAT> m_NATListTEMP;//本地的IP 和NAT 列表
	int m_nNatTypeA;//主控的网络类型，通过第一个查询服务器得到

	#ifndef WIN32
		pthread_t m_hConnThread;//线程句柄
		pthread_t m_hRecvThread;//线程句柄
		pthread_t m_hPartnerThread;//线程句柄
		pthread_t m_hPTCThread;//线程句柄
		pthread_t m_hTurnConnThread;//直连转发
		BOOL m_bEndC;
		BOOL m_bEndR;
		BOOL m_bEndPT;
		BOOL m_bEndPTC;

		pthread_mutex_t m_ct;
	#else
		HANDLE m_hConnThread;//线程句柄
		HANDLE m_hStartEventC;
		HANDLE m_hEndEventC;

		HANDLE m_hRecvThread;//线程句柄
		HANDLE m_hStartEventR;
		HANDLE m_hEndEventR;

		HANDLE m_hPartnerThread;//线程句柄
		HANDLE m_hStartEventPT;
		HANDLE m_hEndEventPT;

		HANDLE m_hPTCThread;//线程句柄
		HANDLE m_hStartEventPTC;
		HANDLE m_hEndEventPTC;

		CRITICAL_SECTION m_ct;
	#endif

		//以下是请求转发服务器地址 多次请求
		int m_nSendRequestTurnTimes;//请求转发地址的次数

		SOCKADDR_IN m_RequestTurnAddr;//请求的云视通服务器地址
		char m_strRequestTurnData[64];
		int m_nRequestTurnTimes;
		SOCKET m_sRequestTurn;
		DWORD m_dwSendTurnTime;
		void ReRequestTurnAddr();

		//以下是请求检索 多次请求

		char m_strRequestIndexData[64];
		int m_nRequestIndexTimes;
		SOCKET m_sRequestIndex;
		DWORD m_dwSendIndexTime;
		void ReRequestIndexAddr();
    
    
    int m_nReconnectTimes;
	void AddCSelfServer();


};

#endif // !defined(AFX_CCHANNEL_H__633B6DEC_700B_41BD_B536_8132C3965D81__INCLUDED_)
