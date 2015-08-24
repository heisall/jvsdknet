// CWorker.h: interface for the CCWorker class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CWORKER_H__E0CD44B9_C03B_4533_9E5D_DCEE7DC603BD__INCLUDED_)
#define AFX_CWORKER_H__E0CD44B9_C03B_4533_9E5D_DCEE7DC603BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CChannel.h"
#include "RunLog.h"
#include "CConnectCtrl.h"
#include "CLanSerch.h"
#include "SDNSCtrl.h"
#include "CHelpCtrl.h"
#include "CLanTool.h"

char* GetPrivateString(char* lpAppName,char* lpKeyName,char* lpValue,char* lpFileName);


#define JVC_CONNECT   1
#define JVC_INVALID   2
#define JVC_DISCONN   3

#define JVNC_ISP_DX   1//µÁ–≈∑˛ŒÒ∆˜
#define JVNC_ISP_WT   2//Õ¯Õ®∑˛ŒÒ∆˜
#define JVNC_ISPMAX   2//√ø÷÷‘À”™…Ã—°»°µƒ∑˛ŒÒ∆˜ ˝ƒø

//#define JVC_MSS   20000//10000

#define JVC_LANTOOL_TIME  10000//æ÷”ÚÕ¯…˙≤˙π§æﬂÀ—À˜∆µ¬ øÿ÷∆
#define JVCLIENT_VERSION "v2.0.76.3.42[private:v2.0.75.13 20150710.1]"//此版本号仅用于linux打印
#define JVCLIENT_VERSION_N 2007513

#define JVN_YSTVER 20140319//20130116

//“—”––≠“È∞Ê±æ*************************************
#define JVN_YSTVER4 20140319//∏√∞Ê±æ≤…”√sendmsg∑Ω Ω∑¢ÀÕ
#define JVN_YSTVER3 20131019//–≠“È∞Ê±æ
//◊™∑¢“Ù∆µº∞ºÊ»›
#define JVN_YSTVER2 20130731//–≠“È∞Ê±æ
// ÷ª˙ø…“‘÷ª∑¢πÿº¸÷°÷ß≥÷∂‡∑÷∆¡£¨–Ë“™¡Ω∂ÀÕ¨ ±÷ß≥÷

#define JVN_YSTVER1 20130116//–≠“È∞Ê±æ
//Œ’ ÷∞¸‘ˆº” —È÷§∫≈¬Î£¨–≠“È∞Ê±æ
//∏√∞Êø™ º÷ß≥÷ ”∆µ ˝æ›—œ∏Ò≤•∑≈ ±º‰¥¡£¨º¥£∫¿‡–Õ+≥§∂»+IID(4) + FID(2) + FTIME(4)+ ˝æ›«¯

//-------------∏¸‘Á
//∏√∞Ê ”∆µ ˝æ›£∫¿‡–Õ+≥§∂»+ FRAMEINDEX(4) +  ˝æ›«¯
//ª˘±æƒ⁄»›

typedef struct
{
    short sver1;
    short sver2;
    short sver3;
    short sver4;
}STVERSION;

typedef struct
{
    char chIP[16];
    int nPort;
    int nISP;//µÁ–≈ªÚÕ¯Õ®£¨≤ªƒ‹≈–∂œµƒ»´πÈŒ™Õ¯Õ®
}STSIP;

typedef struct STSAVESIP
{
    char chGroup[4];
    char chIP[16];
    int nPort;
    int nISP;//µÁ–≈ªÚÕ¯Õ®£¨≤ªƒ‹≈–∂œµƒ»´πÈŒ™Õ¯Õ®
    STSAVESIP()
    {
        memset(chGroup, 0, 4);
        memset(chIP, 0, 16);
        nPort = 0;
        nISP = 0;
    }
}STSAVESIP;

typedef struct
{
    char chgroup[4];
}STGROUP;

typedef struct
{
    HANDLE hThread;
    int nIndex;
}STCONNTHREAD;

#define RC_DATA_SIZE	192*800
typedef struct tagPACKET
{
    int	nPacketType:5;		//∞¸µƒ¿‡–Õ
    int	nPacketCount:8;		//∞¸◊‹ ˝
    int	nPacketID:8;		//∞¸–Ú∫≈
    int	nPacketLen:11;		//∞¸µƒ≥§∂»
    char acData[RC_DATA_SIZE];
} PACKET, *PPACKET;

typedef struct STGETSTATUS
{
    char chGroup[4];
    int nYSTNO;
    int nStatus;//0≤È—Ø ß∞‹£ª1‘⁄œﬂ£ª2≤ª‘⁄œﬂ
    DWORD dwLastTime;
    STGETSTATUS()
    {
        memset(chGroup, 0, 4);
        nYSTNO = 0;
        nStatus = 0;
        dwLastTime = 0;
    }
}STGETSTATUS;

typedef ::std::vector<STGROUP> GroupList;

struct SERVER_INFO
{
    UDTSOCKET m_iID;//UDT ±‡∫≈
    int nSVer;//∑˛ŒÒ∆˜∞Ê±æ∫≈
    
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
};//∑˛ŒÒ∆˜∑µªÿµƒ ˝æ›±£¥Ê

class CSUpnpCtrl;
#include "MakeHoleC.h"
#include "CHelper.h"

struct DEMO_DATA//—› æµ„Ω·ππ
{
    char chGroup[4];
    int nYST;
    int nChannelNum;
    
    char strName[32];
    char strPwd[32];
    
    char strIP[32];
    int nPort;
};

typedef ::std::vector<DEMO_DATA> DEMO_LIST;

struct DEVICE_QUERY_DATA
{
    char pGroup[10];
    int nYST;
    int nTimeOut;
    FUNC_DEVICE_CALLBACK callBack;
    CCWorker *pWorker;
};

class CCRtmpChannel;
typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 成功 2 失败 3 断开 4 异常断开
typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

typedef std::map<int, CCRtmpChannel* > RTMP_LIST;

typedef struct CSELF_DEFINE_SERVER
{
    char strDomain[1024];//域名
    BOOL bIsDomain;//是否是设置的域名
    char strGroup[4];//编组
    char strIP[20];//域名对应的ip  如果设定的域名 在新添加时或重启设备时解析 中间过程中不解析
    int nPort;//端口
}CSELF_DEFINE_SERVER;

typedef ::std::vector<CSELF_DEFINE_SERVER> CDEFINE_SERVER;//自定义服务器地址列表

typedef struct CSERVER_PORT
{
    int nServerPort;//编组对应的端口
    char strGroup[4];//编组
    
}CSERVER_PORT;
typedef ::std::vector<CSERVER_PORT> CSERVER_PORT_LIST;//服务器对应编组的端口列表

class CCWorker
{
public:
    CCWorker();
    CCWorker(int nLocalStartPort);
    CCWorker(int nLocalStartPort, char* pfWriteReadData);
    virtual ~CCWorker();
    
    void ConnectServerDirect(int nLocalChannel,int nChannel,
                             char *pchServerIP,int nServerPort,
                             char *pPassName,char *pPassWord,
                             BOOL bCache,
                             int nConnectType=TYPE_PC_UDP,
                             BOOL isBeRequestVedio=TRUE
							 ,int nOnlyTCP=0
							 );//÷±Ω”¡¨Ω”Õ®µ¿∑˛ŒÒ
    void ConnectServerByYST(int nLocalChannel,int nChannel,
                            int nYSTNO,char chGroup[4],
                            char *pPassName, char *pPassWord,
                            BOOL bLocalTry,
                            int nTURNType,
                            BOOL bCache,
                            int nConnectType=TYPE_PC_UDP,
                            BOOL isBeRequestVedio=TRUE,int nVIP=0);//Õ®π˝YST∫≈¬Î¡¨Ω”∑˛ŒÒ
    
    
    void DisConnect(int nLocalChannel);//∂œø™Õ®µ¿∑˛ŒÒ
    
    BOOL SendData(int nLocalChannel, BYTE uchType, BYTE *pBuffer,int nSize);//∑¢ÀÕ ˝æ›
    
    void GetPartnerInfo(int nLocalChannel, char *pMsg, int &nSize);
    
    void EnableLog(bool bEnable);
    void SetLanguage(int nLgType);
    void ClearBuffer(int nLocalChannel);//«Âø’±æµÿª∫¥Ê
    
    BOOL StartLANSerchServer(int nLPort, int nServerPort);
    void StopLANSerchServer();
    BOOL LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[JVN_DEVICENAMELEN], int nTimeOut,BOOL isMobile,unsigned int unFrequence );
    int AddFSIpSection(const IPSECTION * Ipsection ,int nSize ,BOOL bEnablePing );//ÃÌº”¥˝À—À˜IP∂Œ;
#ifdef MOBILE_CLIENT
	int StopSearchThread();
#endif
    BOOL StartBCServer(int nLPort, int nServerPort);
    void StopBCServer();
    BOOL DoBroadcast(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut);
    
    BOOL GetSerList(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort);//ªÒ»°∑˛ŒÒ∆˜¡–±Ì
    
    //ªÒ»°ºÏÀ˜∑˛ŒÒ∆˜¡–±Ì
    BOOL GetIndexServerList(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort);
    BOOL IndexServerList_Download(char chGroup[4], std::vector<STSIP> &IPList, int nWebIndex, int nLocalPort);
    BOOL IndexServerList_Load(char chGroup[4], std::vector<STSIP> &IPList);
    BOOL IndexServerList_Save(char chGroup[4], std::vector<STSIP> &IPList);
    
    
    BOOL SetLocalFilePath(char chLocalPath[MAX_PATH]);
    
    bool SetWebName(char *pchDomainName,char *pchPathName);
    
    int WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS);
	int WANGetBatchChannelCount(char *pChannelNum, int nYSTNOCnt, int nTimeOutS);
    /*–°÷˙ ÷¡¨Ω”Ã·ÀŸ*/
    BOOL EnableHelp(BOOL bEnable, int nType);
    BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize);
    int GetHelpYSTNO(BYTE *pBuffer, int &nSize);
    
    int GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut);
    
    void pushtmpsock(UDTSOCKET udtsock);
    
    //¥”∫≈¬Îª∫¥Ê÷–ªÒ»°‘∆ ”Õ®–≈œ¢0√ª”–£ª1¡¨Ω”≥…π¶¡À÷±Ω””√À˚µƒµÿ÷∑£ª2ø…“‘”√∑˛ŒÒ∆˜µÿ÷∑¡–±Ì
    //∫Û–¯ªπø…“‘‘Ÿœ∏ªØ£¨∑˛ŒÒ∆˜∑µªÿ¡Àµƒ“≤ø…“‘÷ÿ”√
	int YSTNOCushion(char* pGroup,int nYSTNO,int nType);
	int GetYSTNOInfo(char* pGroup,int nYSTNO, ServerList &slist, SOCKADDR_IN &addrA, char chIPA[16], int &nport,SOCKET& s);
    void WriteYSTNOInfo(char* pGroup,int nYSTNO, ServerList slist, SOCKADDR_IN addrA, int nConnect,SOCKET s = 0);//∏¸–¬∫≈¬Îª∫¥Ê
    void UpdateYSTNOInfo(char* pGroup,int nYSTNO, ServerList slist,SOCKADDR_IN addrA,SOCKET s = 0);//∏¸–¬¡–±Ì ±º‰
    
    void ConnectChange(int nLocalChannel, BYTE uchType, char *pMsg, int nPWData,const char* pFile,const int nLine,const char* pFun);//”Î÷˜øÿ∂ÀÕ®–≈◊¥Ã¨∫Ø ˝(¡¨Ω”◊¥Ã¨)
    void NormalData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight);// µ ±º‡øÿ¥¶¿Ì( ’µΩ µ ±º‡øÿ ˝æ›)
    //‘∂≥Ãªÿ∑≈∫Ø ˝( ’µΩ‘∂≥Ãªÿ∑≈ ˝æ›)
    void CheckResult(int nLocalChannel,BYTE *pBuffer, int nSize);//¬ºœÒºÏÀ˜Ω·π˚¥¶¿Ì∫Ø ˝( ’µΩ¬ºœÒºÏÀ˜Ω·π˚)
    void ChatData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize);//”Ô“Ù¡ƒÃÏ∫Ø ˝(‘∂≥Ã”Ô“Ù)
    void TextData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize);//Œƒ±æ¡ƒÃÏ∫Ø ˝(‘∂≥ÃŒƒ±æ¡ƒÃÏ)
    void DownLoad(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nFileLen);//‘∂≥Ãœ¬‘ÿ
    void PlayData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame, BYTE uchHelpType, BYTE *pHelpBuf, int nHelpSize);//‘∂≥Ãªÿ∑≈
    void BufRate(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nRate);//ª∫≥ÂΩ¯∂»
    
    int EnableLANTool(int nEnable, int nLPort, int nServerPort);
    int LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut);
    
    int SendCMD(int nLocalChannel, BYTE uchType, BYTE* pBuffer, int nSize);
    
    void GetNATADDR(NATList pNatListAll,NATList *pNatList, SOCKADDR_IN addrs, int &nSVer);//channelµ˜”√
    //	int GetWANIPList(ServerList slisttmp,NATList *pNatList,int nTimeOut);//ªÒ»°±æª˙µƒÕ‚Õ¯NAT
    
    void GetLocalIP(NATList *pNatList);//∂¡»°ƒ⁄¥Ê÷–µƒ±æª˙IP£¨À≠”√À≠µ˜”√ ≤ª“™‘ŸÃÌº”∂¡»°±æª˙IPµƒ∫Ø ˝
    
    static void jvc_sleep(unsigned long time);//π“∆
    static DWORD JVGetTime();
    
    void AddUdtRecv(SOCKET sSocket,int nLocalPort,sockaddr* addrRemote,char *pMsg,int nLen,BOOL bNewUDP);//UDTø‚∑µªÿµƒUDP ˝æ›∞¸
    
    int SendUdpDataForMobile(SOCKET s,SOCKADDR_IN ServerSList,int nYSTNO,BYTE* pIP,int nSize);// ÷ª˙∑¢ÀÕ≤È—Ø µ•∏ˆ∑˛ŒÒ∆˜∑¢ÀÕ ΩµµÕ∑¢∞¸∆µ¬
    int SendUdpData(SOCKET s,ServerList ServerSList,int nYSTNO,BYTE* pIP,int nSize);//∑¢ÀÕ≤È—Ø
    int GetUdpData(SOCKET s,int nYSTNO,UDP_LIST* UdpList);//¥”UDT∑µªÿ ˝æ›÷–µ√µΩ≤È—ØΩ·π˚
    
    int GetConnectInfo(char* pGroup,int nYST,SOCKET &s,char* pIP,int &nPort);//≤È—Ø–¬µƒ÷˙ ÷◊ ‘¥ 0 √ª”– ˝æ› 1 ø…“‘TCP¡¨Ω” 2ø…“‘UDP¡¨Ω”
    
    void AddHelpConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort);// µº ¡¨Ω”≥…π¶∫Ûµ˜”√
    
    void WriteMobileFile(char* strName,char* pData,int nLen);
    int ReadMobileFile(char* strName,char* pData,int nLen);
    
    void HelpRemove(char* pGroup,int nYST);//¥”÷˙ ÷÷–…æ≥˝∫≈¬Î
	int GetHelper(char* pGroup,int nYST,int *nCount);//锟斤拷询锟斤拷锟斤拷状态
    
    int GetDemo(BYTE* pBuff,int nBuffSize);//≤È—Ø—› æµ„¡–±Ì
    
    BOOL GetDemoList(int nWebIndex);//
    
    BOOL YstIsDemo(char* pGroup,int nYST);//∫≈¬Î «∑Ò «—› æµ„
    BOOL GetDemoAddr(char* pGroup,int nYST,char* pIP,int &nPort,char *pUser,char* pPwd);
    
    void LoadDemoFile();
    BOOL DownLoadFile(char *pServer,char* pPath,char* pFileName,char *pHeadFilter = NULL);//œ¬‘ÿÕ¯“≥Œƒº˛ ±£¥Ê≥…÷∏∂®µƒŒƒº˛√˚
    BOOL ParseIndexFile();//Ω‚Œˆ≥ˆÀ˜“˝∑˛ŒÒ∆˜
    
    
    //¡˜√ΩÃÂ”–πÿ
    pthread_mutex_t m_RtmpLock;
    RTMP_LIST m_RtmpChannelList;
    
	bool ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout);//锟斤拷锟斤拷锟斤拷媒锟斤拷锟斤拷锟斤拷锟�
    
    void ShutdownRTMP(int nLocalChannel);//∂œø™¡˜√ΩÃÂ∑˛ŒÒ∆˜ ÷∏∂®¡¨Ω”
    void ShutdownAllRTMP();//∂œø™¡˜√ΩÃÂ∑˛ŒÒ∆˜ »´≤ø
	void ClearHelpCache(void);
	int SetMTU(int nMtu);	//set mtu
    int stopHelp();
    
    int SetSelfServer(char* pGroup,char* pServerList);
    
    int SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
    
    
    int SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
    
    
    CDEFINE_SERVER m_CSelfDefineServer;
    
    
    CSERVER_PORT_LIST m_CServerPortList;//分控服务器与编组对应的端口

    void AddYstSvr(char* chGroup,STSERVER addr);//向chGroup组增加一个服务器地址
	void AddYstSvr(char* chGroup,SOCKADDR_IN addr);//向chGroup组增加一个服务器地址
	void ShowYstSvr(void);//显示所有组的云视通服务器列表
	void GetGroupSvrList(char* chGroup,CYstSvrList &grouplist); //获取特定组的所有云视通服务器列表
	int GetGroupSvrListIndex(char* chGroup);//获取特定组的所有云视通服务器列表索引，没有则新建
public://///////////////////////////////////////////////////////////////////
	CYstSvrGroupList m_YstSvrList; //云视通服务器所有编组列表
    FUNC_CCONNECT_CALLBACK m_pfConnect;
    FUNC_CNORMALDATA_CALLBACK m_pfNormalData;
    FUNC_CCHECKRESULT_CALLBACK m_pfCheckResult;
    FUNC_CCHATDATA_CALLBACK m_pfChatData;
    FUNC_CTEXTDATA_CALLBACK m_pfTextData;
    FUNC_CDOWNLOAD_CALLBACK m_pfDownLoad;
    FUNC_CPLAYDATA_CALLBACK m_pfPlayData;
    FUNC_CBUFRATE_CALLBACK m_pfBufRate;
    FUNC_CLANSDATA_CALLBACK m_pfLANSData;
    FUNC_CBCDATA_CALLBACK m_pfBCData;
    FUNC_CLANTDATA_CALLBACK m_pfLANTData;
    
    FUNC_COMM_DATA_CALLBACK m_pfWriteReadData;
    
    CRunLog m_Log;
    int m_nLanguage;
    bool m_bNeedLog;//∏√±Í÷æø…‘⁄¡¨Ω”π˝≥Ã÷–”√”⁄»∑∂® «∑Ò‘§œ»±£¡Ù–≈œ¢
    STVERSION m_stVersion;
    GroupList m_GroupList;
    GroupList m_IndexGroupList;
    
    UDTSOCKET m_WorkerUDTSocket;//◊‹Ã◊Ω”◊÷
    SOCKET m_WorkerUDPSocket;//◊‹Ã◊Ω”◊÷
    SOCKET m_WorkerTCPSocket;//◊‹Ã◊Ω”◊÷
    
    CCHelpCtrl *m_pHelpCtrl;//–°÷˙ ÷π¶ƒ‹
    int m_nLocalPortWan;
    
    pthread_mutex_t m_ReceiveLock;//udt Ω” ’À¯
    UDP_LIST m_UdpDataList;//Ω” ’µΩµƒudt∑µªÿ ˝æ›
    
    CMakeGroup m_MakeHoleGroup;//¥Ú∂¥π‹¿Ì¿‡
    
    CCHelper m_Helper;//–¬ÃÌº”÷˙ ÷
    DEMO_LIST m_DemoList;
    
#ifdef MOBILE_CLIENT
    DWORD m_dwConnectTime;
#endif
    void QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack);
#ifndef WIN32
    static void* QueryDeviceProc(void* pParam);
#else
    static UINT WINAPI QueryDeviceProc(LPVOID pParam);
#endif
    BYTE m_BroadCastdata[100];
    int m_nBroadCastLen;
private:///////////////////////////////////////////////////////////////////////////
    void GetCurrentPath(char chCurPath[MAX_PATH]);
    
    BOOL DownLoadFirst(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort);
    
    void CreateShare();
    
    static void MultiMaskRemove(char* pBuff,int nLen);
    static void MultiMaskAdd(char* pBuff,int nLen);
    
    void GetLocalIPList();
    
    char* GetLocalMobileIP(char* pIP,BYTE* bIP);//±æ∫Ø ˝Œ™ ÷ª˙ π”√ ªÒ»°µ±«∞IP£¨÷ª»°◊Ó∫Û“ª∏ˆµÿ÷∑
    
#ifndef WIN32
    static void* PTListenProc(void* pParam);//ªÔ∞È¡¨Ω”º‡Ã˝œﬂ≥Ã
    static void* GTProc(void* pParam);//ªÔ∞È¡¨Ω”«Â¿Ìœﬂ≥Ã
    static void* SetUpnpProc(void* pParam);//…Ë÷√Upnpœﬂ≥Ã
    static void* GetIPNatProc(void* pParam);
    char* itoa(int num,char *str,int radix);
#else
    static UINT WINAPI PTListenProc(LPVOID pParam);//ªÔ∞È¡¨Ω”º‡Ã˝œﬂ≥Ã
    static UINT WINAPI GTProc(LPVOID pParam);//ªÔ∞È¡¨Ω”«Â¿Ìœﬂ≥Ã
    static UINT WINAPI SetUpnpProc(LPVOID pParam);//…Ë÷√Upnpœﬂ≥Ã
    static UINT WINAPI GetIPNatProc(LPVOID pParam);
#endif
    
private:////////////////////////////////////////////////////////////////////////////
    int m_nLocalStartPort;
    ::std::vector<CCChannel*> m_pChannels;
    
    /*…Ë±∏À—À˜œ‡πÿ*/
    CCLanSerch *m_pLanSerch;
    
    /*æ÷”ÚÕ¯◊‘∂®“Âπ„≤•œ‡πÿ*/
    CCLanSerch *m_pBC;
    
    /*æ÷”ÚÕ¯…˙≤˙π§æﬂœ‡πÿ*/
    CCLanTool *m_pLanTool;
    DWORD m_dwLastLANToolTime;//…œ¥ŒÀ—À˜ ±º‰
    
    char m_chLocalPath[MAX_PATH];//±æµÿ¬∑æ∂
    
    char m_strDomainName[MAX_PATH];//◊‘∂®“Â”Ú√˚
    char m_strPathName[MAX_PATH];//◊‘∂®“ÂÕ¯’æ¬∑æ∂
    
    CCConnectCtrl m_connectctrl;
    
    YSTNOList m_stYSTNOLIST;//‘∆ ”Õ®∫≈¬Î–≈œ¢ª∫¥Ê(µÿ÷∑)
    YSTNOList m_stYSTCushion;//‘∆ ”Õ®∫≈¬Î–≈œ¢ª∫¥Ê( «∑Ò”–Õ®µ¿‘⁄¡¨Ω”)
    
    ::std::vector<UDTSOCKET> m_UDTSockTemps;
    
    /*∫≈¬Î◊¥Ã¨≤È—Ø∆µ¬ øÿ÷∆*/
    DWORD m_dwLastTime;//…œ¥Œµƒ≤È—Ø ±º‰
    ::std::vector<STGETSTATUS> m_GetStatus;//≤È—Ø◊¥Ã¨º«¬º
    
#ifndef WIN32
    pthread_t m_hPTListenThread;
    pthread_t m_hGTThread;
    pthread_t m_hNatGetThread;
    
    BOOL m_bPTListenEnd;
    BOOL m_bGTEnd;
    BOOL m_bNatGetEnd;
    
    pthread_mutex_t m_criticalsection;
    pthread_mutex_t m_ctGroup;
    pthread_mutex_t m_ctND;
    pthread_mutex_t m_ctCC;
    pthread_mutex_t m_ctDL;
    pthread_mutex_t m_ctPD;
    pthread_mutex_t m_ctYSTNO;
    pthread_mutex_t m_ctclearsock;
    pthread_mutex_t m_ctlocalip;
#else
    HANDLE m_hPTListenThread;
    HANDLE m_hPTListenStartEvent;//
    HANDLE m_hPTListenEndEvent;
    
    HANDLE m_hGTThread;
    HANDLE m_hGTStartEvent;//
    HANDLE m_hGTEndEvent;
    
    HANDLE m_hNatGetThread;
    HANDLE m_hNatGetStartEvent;//
    HANDLE m_hNatGetEndEvent;
    
    CRITICAL_SECTION m_criticalsection;
    CRITICAL_SECTION m_ctGroup;
    CRITICAL_SECTION m_ctND;
    CRITICAL_SECTION m_ctCC;
    CRITICAL_SECTION m_ctDL;
    CRITICAL_SECTION m_ctPD;
    CRITICAL_SECTION m_ctYSTNO;
    CRITICAL_SECTION m_ctclearsock;
    
    CRITICAL_SECTION m_ctlocalip;
#endif
    
    NATList m_LocalIPList;//±æµÿIP¡–±Ì
    
    char m_chGroupList[50][4];//±æµÿ±‡◊ÈºØ∫œ
    int m_nGroupCount;//±æµÿ±‡◊È∏ˆ ˝
    
    //∑÷øÿÃÌº”upnp
    CSUpnpCtrl *m_pUpnpCtrl;
};

#endif // !defined(AFX_CWORKER_H__E0CD44B9_C03B_4533_9E5D_DCEE7DC603BD__INCLUDED_)





















