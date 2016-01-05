// CLanSerch.h: interface for the CCLanSerch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLANSERCH_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_)
#define AFX_CLANSERCH_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
//开始
#define ICMP_ECHO 8  // 定义回显报文代码
#define ICMP_ECHOREPLY 0
#define ICMP_MIN 8  // 最小8字节ICMP包
#define MAX_ICMP_PACKET 1024 // 最大ICMP大小
#define MAX_IPSEC_NUM    10 //默认设置最大的IPSECTION 10个
#define SLEEP_PER_TIME    50 //默认设置每次sleep(50);
#pragma pack(4)
#define  JVC_BC_SELF   2

// IP头首部结构
struct IpHeader
{
    unsigned int h_len:4;   // 首部长度
    unsigned int version:4;   // IP版本
    unsigned char tos;    // 服务类型
    unsigned short total_len;  // 包总长度
    unsigned short ident;   // 标识符
    unsigned short frag_and_flags; // 标志
    unsigned char ttl;    // 生存周期
    unsigned char proto;   // protocol (TCP, UDP etc) 协议类型
    unsigned short checksum;  // IP检验和
    unsigned int sourceIP;   // 源地址IP
    unsigned int destIP;   // 目的地址IP
};
// ICMP 首部结构
struct IcmpHeader
{
    unsigned char i_type; // 类型
    unsigned char i_code; // 代码类型
    unsigned short i_cksum; // 检验和
    unsigned short i_id; // 地址
    unsigned short i_seq; // 发送顺序
    // 增加一个时间戳
    unsigned long timestamp;
};

//结束


#define  JVC_LANS 0
#define  JVC_BC   1

typedef ::std::vector<_Adapter> AdapterList;

class CCWorker;
class CCLanSerch
{
public:
    CCLanSerch();
    CCLanSerch(int nLPort, int nDesPort, CCWorker *pWorker, int nType);//type 0 设备搜索；1自定义广播
    virtual ~CCLanSerch();
    
    BOOL LANSerch(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[JVN_DEVICENAMELEN], int nTimeOut,BOOL isMobile,unsigned int unFrequence );
    void SearchFSIpSection();//搜索数组中的设备,
    int  AddFSIpSection(const IPSECTION * Ipsection ,int nSize ,BOOL bEnablePing );//添加待搜索IP段;
    BOOL Broadcast(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut);
    BOOL SendSelfDataFromBC(BYTE *pBuffer, int nSize, char *pchDeviceIP, int nDestPort);

    BOOL m_bPause[256];//暂停的通道
    BOOL IsPause();
    
#ifdef MOBILE_CLIENT

    int stopLanSearchMethod();

#endif
    
private:
    void GetAdapterInfo();
    
#ifndef WIN32
    static void* LANSRcvProc(void* pParam);
    static void* LANSSndProc(void* pParam);
    static void* PingLanIpProc(void *pParam);
#else
    static UINT WINAPI PingLanIpProc(LPVOID pParam);
    static UINT WINAPI LANSRcvProc(LPVOID pParam);
    static UINT WINAPI LANSSndProc(LPVOID pParam);
#endif
    
public:
    BOOL m_bOK;
    
    int   m_bNum[255];
    BOOL m_bisMobile ;                       //是否为手机;
    BOOL m_sendThreadExit;
    
private:
    BOOL m_bNeedSleep;//在lanSerch 的时候是否需要sleep
    
    int m_nType;
    CCWorker *m_pWorker;
    unsigned int m_unLANSID;//
    SOCKET m_SocketLANS;
    int m_nLANSPort;
    int m_nDesPort;
    
    int m_nTimeOut;
    DWORD m_dwBeginSerch;
    BOOL m_bTimeOut;
    BOOL m_bLANSerching;
    int m_nLANYSTNO;//正在搜索的号码
    int m_nLANCARDTYPE;//正在搜索的型号
    int m_nLANVariety;//正在搜索的产品种类
    char m_chDeviceName[JVN_DEVICENAMELEN];//正在搜索的设备别名
    char m_chLANGroup[4];//正在搜索的编组号
    
    BOOL m_bNewSerch;//是否有新的搜索
    BOOL m_bStopSerchImd;//是否立即停止当前搜索发送
    
    AdapterList m_IpList;
    
    BYTE m_uchData[10340];
    int m_nNeedSend;
    struct   timeval   m_tv;
    
    
    SOCKET m_socketRaw;
    
    fd_set m_fSet;
    
    sockaddr_in m_icmpaddr;
    
    char m_szIcmpData[MAX_ICMP_PACKET];//发送数据缓冲区
    
    char m_szRecvBuff[MAX_ICMP_PACKET];//接收数据缓冲区
    //指定IP段搜索相关
    char m_szSecIp[MAX_IPSEC_NUM*sizeof(IPSECTION)];//IP段缓冲区
    
    int  m_nIpSecSize;                  //IP记录大小;
    
    bool  m_isUsing ;                    //标记当前IP段数组是否被使用
    
    BOOL m_bPingEnd;
    BOOL m_bEnablePing;                         //暂停、继续ping线程
    unsigned int m_unFreq;                   //ping 的频率.默认30sPing 一次.
#ifndef WIN32
    pthread_t m_hLANSPingThread;//PING ip thread;
    pthread_t m_hLANSerchRcvThread;
    pthread_t m_hLANSerchSndThread;
    BOOL m_bRcvEnd;
    BOOL m_bSndEnd;
    pthread_mutex_t m_ct;
#else
    HANDLE m_hLANSPingThread;
    
    HANDLE m_hLANSerchRcvThread;
    HANDLE m_hLANSerchRcvStartEvent;//
    HANDLE m_hLANSerchRcvEndEvent;	
    
    HANDLE m_hLANSerchSndThread;
    HANDLE m_hLANSerchSndStartEvent;//
    HANDLE m_hLANSerchSndEndEvent;
    CRITICAL_SECTION m_ct;
#endif
    
    BOOL m_bFirstRun;
};

#endif // !defined(AFX_CLANSERCH_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_)
