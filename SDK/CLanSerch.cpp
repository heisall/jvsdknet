// CLanSerch.cpp: implementation of the CCLanSerch class.
//
//////////////////////////////////////////////////////////////////////

#include "CLanSerch.h"
#include "CWorker.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCLanSerch::CCLanSerch()
{
}

CCLanSerch::CCLanSerch(int nLPort, int nDesPort, CCWorker *pWorker, int nType)
{
    memset(m_bPause,0,sizeof(m_bPause));
    
    m_bFirstRun = TRUE;
    m_nType = nType;
    m_bOK = FALSE;
    m_pWorker = pWorker;
    m_nLANSPort = 0;
    m_unLANSID = 1;
    m_nTimeOut = 0;
    m_dwBeginSerch = 0;
    m_bTimeOut = 0;
    m_bLANSerching = FALSE;
    m_nLANYSTNO = 0;
    m_nLANCARDTYPE = 0;
    m_nLANVariety = 0;
    memset(m_chLANGroup, 0, 4);
    memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    
    m_bNewSerch = FALSE;
    m_bStopSerchImd = TRUE;
    m_bEnablePing=TRUE;
    m_hLANSerchRcvThread = 0;
    m_hLANSerchSndThread = 0;
    m_hLANSPingThread = 0;
    m_bPingEnd = FALSE;
    
    m_bNeedSleep  =1;//默认clansearch 都需要sleep;
    m_bisMobile =FALSE;
    m_unFreq =(30*1000)/50;//600
#ifndef WIN32
    pthread_mutex_init(&m_ct, NULL);
    
    m_bRcvEnd = FALSE;
    m_bSndEnd = FALSE;
#else
    InitializeCriticalSection(&m_ct); //初始化临界区
    
    m_hLANSerchRcvStartEvent = 0;
    m_hLANSerchRcvEndEvent = 0;
    
    m_hLANSerchSndStartEvent = 0;
    m_hLANSerchSndEndEvent = 0;
#endif
    m_socketRaw=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    FD_ZERO(&m_fSet);
    memset(m_szIcmpData,0,MAX_ICMP_PACKET);
    memset(m_szRecvBuff,0,MAX_ICMP_PACKET);
    memset((char * )&m_icmpaddr,0,MAX_ICMP_PACKET);
    
    m_icmpaddr.sin_family = AF_INET;
    
    IcmpHeader* pIcmpHeader = (IcmpHeader*)m_szIcmpData;
    pIcmpHeader->i_type  = ICMP_ECHO;     // 设置回显报文
    pIcmpHeader->i_code  = 0;
#ifndef WIN32
    pIcmpHeader->i_id  = (unsigned short )getpid();               //linux获得当前进程ID
#else
    pIcmpHeader->i_id  = (unsigned short )GetCurrentProcessId();// 获取当前进程ID
#endif
    pIcmpHeader->i_cksum = 0;       // 初始化检验和为0
    pIcmpHeader->i_seq  = 0;       // 顺序符
    char* pDataPart = m_szIcmpData + sizeof(IcmpHeader);  // 给数据区赋地址
    memset(pDataPart, 'E', 32);      // 把数据区初始化
    memset(&m_bNum,0,255);          //将列表初始化为0
    m_tv.tv_sec=0;
    m_tv.tv_usec=100;
    memset(m_szSecIp,0,MAX_IPSEC_NUM*sizeof(IPSECTION));
    
    m_isUsing  = false;
    
    m_nIpSecSize = 0;
    int err;
    //套接字
    m_SocketLANS = socket(AF_INET, SOCK_DGRAM,0);
    SOCKADDR_IN addrSrv;
#ifndef WIN32
    addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(nLPort);
    //绑定套接字
    err = bind(m_SocketLANS, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
    if(err != 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
        {
            m_pWorker->m_Log.SetRunInfo(0,"初始化LANSerchSOCK失败.原因:绑定端口失败", __FILE__,__LINE__);
        }
        else
        {
            m_pWorker->m_Log.SetRunInfo(0,"init LANSerch sock faild.Info:bind port faild.", __FILE__,__LINE__);
        }
        closesocket(m_SocketLANS);
        
        return;
    }
    
    // 有效SO_BROADCAST选项
    int nBroadcast = 1;
    ::setsockopt(m_SocketLANS, SOL_SOCKET, SO_BROADCAST, (char*)&nBroadcast, sizeof(int));
    
    GetAdapterInfo();
    
#ifndef WIN32
    pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
    //创建本地监听线程
    if (0 != pthread_create(&m_hLANSerchRcvThread, pAttr, LANSRcvProc, this))
    {
        m_hLANSerchRcvThread = 0;
        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
        {
            m_pWorker->m_Log.SetRunInfo(0,"开启LANSerch服务失败.原因:创建线程失败",__FILE__,__LINE__);
        }
        else
        {
            m_pWorker->m_Log.SetRunInfo(0,"start LANSerch server failed.Info:create thread faild.",__FILE__,__LINE__);
        }
        return;
    }
    if (nType!=JVC_BC)//如果不是广播
    {
        
        //创建ping网关线程
        if (0 != pthread_create(&m_hLANSPingThread, pAttr, PingLanIpProc, this))
        {
            m_hLANSPingThread = 0;
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(0,"开启LANSerch服务失败.原因:创建线程失败",__FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(0,"start LANSerch server failed.Info:create thread faild.",__FILE__,__LINE__);
            }
            return;
        }
    }
#else
    //创建本地命令监听线程
    UINT unTheadID;
    if(m_hLANSerchRcvStartEvent > 0)
    {
        CloseHandle(m_hLANSerchRcvStartEvent);
        m_hLANSerchRcvStartEvent = 0;
    }
    if(m_hLANSerchRcvEndEvent > 0)
    {
        CloseHandle(m_hLANSerchRcvEndEvent);
        m_hLANSerchRcvEndEvent = 0;
    }
    //创建本地监听线程
    m_hLANSerchRcvStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hLANSerchRcvEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hLANSerchRcvThread = (HANDLE)_beginthreadex(NULL, 0, LANSRcvProc, (void *)this, 0, &unTheadID);
    if(m_hLANSerchRcvStartEvent > 0)
    {
        SetEvent(m_hLANSerchRcvStartEvent);
    }
    if (m_hLANSerchRcvThread == 0)//创建线程失败
    {
        if(m_hLANSerchRcvStartEvent > 0)
        {
            CloseHandle(m_hLANSerchRcvStartEvent);
            m_hLANSerchRcvStartEvent=0;
        }
        if(m_hLANSerchRcvEndEvent > 0)
        {
            CloseHandle(m_hLANSerchRcvEndEvent);
            m_hLANSerchRcvEndEvent=0;
        }
        
        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
        {
            m_pWorker->m_Log.SetRunInfo(0,"开启LANSerch服务失败.原因:创建线程失败",__FILE__,__LINE__);
        }
        else
        {
            m_pWorker->m_Log.SetRunInfo(0,"start LANSerch server failed.Info:create thread faild.",__FILE__,__LINE__);
        }
        return;
    }
    
    //创建本地ping线程
    if (nType!=JVC_BC)//如果不是广播
    {
        
        m_hLANSPingThread = (HANDLE)_beginthreadex(NULL, 0, PingLanIpProc, (void *)this, 0, &unTheadID);
        if (0==m_hLANSPingThread )
        {
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(0,"开启LANSerch服务失败.原因:创建线程失败",__FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(0,"start LANSerch server failed.Info:create thread faild.",__FILE__,__LINE__);
            }
            return;
        }
    }
#endif
    
    m_bOK = TRUE;
    m_nLANSPort = nLPort;
    m_nDesPort = nDesPort;
    //GetAdapterInfo();
}

CCLanSerch::~CCLanSerch()
{
#ifndef WIN32
    if (0 != m_hLANSerchRcvThread)
    {
        m_bRcvEnd = TRUE;
        pthread_join(m_hLANSerchRcvThread, NULL);
        m_hLANSerchRcvThread = 0;
        CCWorker::jvc_sleep(5);
    }
    if (0 != m_hLANSerchSndThread)
    {
        m_bSndEnd = TRUE;
        m_bStopSerchImd = TRUE;
//        m_bNewSerch = TRUE;
        pthread_join(m_hLANSerchSndThread, NULL);
        m_hLANSerchSndThread = 0;
        CCWorker::jvc_sleep(5);
    }
    if (0 != m_hLANSPingThread)
    {
        m_bPingEnd = TRUE;
        pthread_join(m_hLANSPingThread, NULL);
        m_hLANSPingThread = 0;
        CCWorker::jvc_sleep(5);
    }
#else
    //结束本地线程
    if(m_hLANSerchRcvEndEvent > 0)
    {
        SetEvent(m_hLANSerchRcvEndEvent);
        CCWorker::jvc_sleep(5);
    }
    if(m_hLANSerchSndEndEvent > 0)
    {
        SetEvent(m_hLANSerchSndEndEvent);
        CCWorker::jvc_sleep(5);
    }
    if(m_hLANSPingThread > 0)
    {
        m_bPingEnd = TRUE;
        CCWorker::jvc_sleep(5);
    }
    CCChannel::WaitThreadExit(m_hLANSerchRcvThread);
    CCChannel::WaitThreadExit(m_hLANSerchSndThread);
    CCChannel::WaitThreadExit(m_hLANSPingThread);
    if(m_hLANSerchRcvThread > 0)
    {
        CloseHandle(m_hLANSerchRcvThread);
        m_hLANSerchRcvThread = 0;
    }
    if(m_hLANSerchSndThread > 0)
    {
        CloseHandle(m_hLANSerchSndThread);
        m_hLANSerchSndThread = 0;
    }
    if(m_hLANSPingThread > 0)
    {
        CloseHandle(m_hLANSPingThread);
        m_hLANSPingThread = 0;
    }
#endif
    
    closesocket(m_SocketLANS);
    closesocket(m_socketRaw);//释放socket
    m_SocketLANS = 0;
    m_bOK = FALSE;
    
#ifndef WIN32
    pthread_mutex_destroy(&m_ct);
#else
    DeleteCriticalSection(&m_ct); //释放临界区
#endif
}

void CCLanSerch::GetAdapterInfo()
{
    m_IpList.clear();
    
    /*//由于该版本要用于手机端，多网卡问题统一暂不考虑
     #ifdef WIN32
     #include "IpHlpApi.h"
     #pragma comment (lib, "IpHlpApi.lib")
     #endif
     int nNum = 0;
     PIP_ADAPTER_INFO pAdapterInfo;
     PIP_ADAPTER_INFO pAdapter = NULL;
     DWORD dwRetVal = 0;
     
     pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
     unsigned long ulOutBufLen = sizeof(IP_ADAPTER_INFO);
     
     if(GetAdaptersInfo(pAdapterInfo,&ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
     {
     free(pAdapterInfo);
     pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
     }
     
     if((dwRetVal = GetAdaptersInfo(pAdapterInfo,&ulOutBufLen)) == NO_ERROR)
     {
     pAdapter = pAdapterInfo;
     while(pAdapter)
     {
     
     _Adapter info;
     memset(&info,0,sizeof(_Adapter));
     
     memcpy(info.AdapterName,pAdapter->AdapterName,sizeof(pAdapter->AdapterName));
     memcpy(info.Description,pAdapter->Description,sizeof(pAdapter->Description));
     
     memcpy(info.IP,pAdapter->IpAddressList.IpAddress.String,sizeof(pAdapter->IpAddressList.IpAddress.String));
     memcpy(info.SubMask,pAdapter-> IpAddressList.IpMask.String,sizeof(pAdapter->IpAddressList.IpMask.String));
     memcpy(info.GateWay,pAdapter->GatewayList.IpAddress.String,sizeof(pAdapter->GatewayList.IpAddress.String));
     
     
     if(strlen(info.GateWay) > 0)
     {
     BYTE n[4] = {0};
     sscanf(info.IP,"%d.%d.%d.%d",&n[0],&n[1],&n[2],&n[3]);
     sprintf(info.IpHead,"%d.%d.",n[0],n[1]);
     
     m_IpList.push_back(info);
     
     nNum ++;
     }
     pAdapter   =   pAdapter-> Next;
     }
     }
     else
     {
     free(pAdapterInfo);
     return nNum;
     }
     free(pAdapterInfo);
     */
    _Adapter info;
    memset(&info,0,sizeof(_Adapter));
    //本地地址
    NATList LocalIPList;
    m_pWorker->GetLocalIP(&LocalIPList);
    
    for(int i = 0;i < LocalIPList.size();i ++)
    {
        sprintf(info.IP,"%d.%d.%d.%d",LocalIPList[i].ip[0],LocalIPList[i].ip[1],LocalIPList[i].ip[2],LocalIPList[i].ip[3]);
        //BYTE n[4] = {0};
        unsigned int n[4] = {0};
        sscanf(info.IP,"%d.%d.%d.%d",&n[0],&n[1],&n[2],&n[3]);
        sprintf(info.IpHead,"%d.%d.",n[0],n[1]);
        info.nIP3 = n[2];
        m_bNum[info.nIP3] = 1;
        m_IpList.push_back(info);
    }
    
    return;
}

/*
BOOL CCLanSerch::LANSerch(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[JVN_DEVICENAMELEN], int nTimeOut,BOOL isMobile,unsigned int unFrequence )
{
    
    if(m_hLANSerchRcvThread <= 0 || m_SocketLANS <= 0)
    {
        return FALSE;
    }
    if (TRUE==isMobile)
    {
        m_bisMobile = TRUE;//标记手机搜索.
        m_bPingEnd = TRUE;//退出ping搜索线程.
    }
    if (unFrequence>10&&unFrequence<86400)//至少10S ping一次最长时间24h ping 一次。
    {
        m_unFreq = (unFrequence*1000)/50;
    }
    if(m_hLANSerchSndThread <= 0)
    {//先创建发送线程
#ifndef WIN32
        pthread_attr_t attr;
        pthread_attr_t *pAttr = &attr;
        unsigned long size = LINUX_THREAD_STACK_SIZE;
        size_t stacksize = size;
        pthread_attr_init(pAttr);
        if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
        {
            pAttr = NULL;
        }
        //创建本地监听线程
        if (0 != pthread_create(&m_hLANSerchSndThread, pAttr, LANSSndProc, this))
        {
            m_hLANSerchSndThread = 0;
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
            }
            return FALSE;
        }
#else
        //创建本地命令监听线程
        UINT unTheadID;
        if(m_hLANSerchSndStartEvent > 0)
        {
            CloseHandle(m_hLANSerchSndStartEvent);
            m_hLANSerchSndStartEvent = 0;
        }
        if(m_hLANSerchSndEndEvent > 0)
        {
            CloseHandle(m_hLANSerchSndEndEvent);
            m_hLANSerchSndEndEvent = 0;
        }
        //创建本地监听线程
        m_hLANSerchSndStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hLANSerchSndEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hLANSerchSndThread = (HANDLE)_beginthreadex(NULL, 0, LANSSndProc, (void *)this, 0, &unTheadID);
        if(m_hLANSerchSndStartEvent > 0)
        {
            SetEvent(m_hLANSerchSndStartEvent);
        }
        if (m_hLANSerchSndThread == 0)//创建线程失败
        {
            if(m_hLANSerchSndStartEvent > 0)
            {
                CloseHandle(m_hLANSerchSndStartEvent);
                m_hLANSerchSndStartEvent=0;
            }
            if(m_hLANSerchSndEndEvent > 0)
            {
                CloseHandle(m_hLANSerchSndEndEvent);
                m_hLANSerchSndEndEvent=0;
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
            }
            return FALSE;
        }
#endif
    }
    
    m_bNewSerch = FALSE;//置为没有新搜索
    m_bStopSerchImd = TRUE;//停止当前正在进行的搜索操作
    
    if(nTimeOut >= 0)
    {
        m_nTimeOut = nTimeOut;
    }
    
    m_unLANSID++;
    
    BYTE data[300];
    int nSLen = 0;
    int nSType = 0;
    int nType = JVN_REQ_LANSERCH;
    //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+[搜索参数(4)(号码\卡类型)+编组号(4)]+[设备别名长度(1)+设备别名(?)]
    memcpy(&data[0], &nType, 4);
    if(nYSTNO > 0)
    {//云视通号码优先级最高
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+号码(4)+编组号(4)
        nSType = JVN_CMD_LANSYST;
        nSLen = 20;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        memcpy(&data[20], &nYSTNO, 4);
        memcpy(&data[24], chGroup, 4);
        
        memcpy(m_chLANGroup, chGroup, 4);
        m_nLANYSTNO = nYSTNO;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = 0;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    else if(nCardType > 0)
    {//按卡型号搜索
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+卡型号(4)
        nSType = JVN_CMD_LANSTYPE;
        nSLen = 16;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        memcpy(&data[20], &nCardType, 4);
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = nCardType;
        m_nLANVariety = 0;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    else if(strlen(chDeviceName) > 0)
    {//按设备别名搜索
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+别名长度(1)+别名(?)
        int nNameLen=strlen(chDeviceName);
        nSType = JVN_CMD_LANSNAME;
        nSLen = nNameLen+13;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        data[20] = nNameLen;
        memcpy(&data[21], chDeviceName, nNameLen);
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = 0;
        memcpy(m_chDeviceName, chDeviceName, nNameLen);
    }
    else
    {//搜索全部
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+产品种类(1)
        nSType = JVN_CMD_LANSALL;
        nSLen = 13;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        data[20] = nVariety;
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = nVariety;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    
#ifndef WIN32
    pthread_mutex_lock(&m_ct);
#else
    EnterCriticalSection(&m_ct);
#endif
    
    m_nNeedSend = nSLen+8;
    memcpy(m_uchData, data, m_nNeedSend);
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ct);
#else
    LeaveCriticalSection(&m_ct);
#endif
    
    SOCKADDR_IN addrBcast;
    // 设置广播地址，这里的广播端口号
    addrBcast.sin_family = AF_INET;
    addrBcast.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
    
    addrBcast.sin_port = htons(m_nDesPort);
    
    int ntmp = CCChannel::sendtoclient(m_SocketLANS,(char *)data,nSLen+8,0,(SOCKADDR *)&addrBcast, sizeof(SOCKADDR),1);
    m_bTimeOut = FALSE;
    m_bLANSerching = TRUE;
    m_dwBeginSerch = CCWorker::JVGetTime();
    if(ntmp != nSLen+8)
    {
        return FALSE;
    }
    
    m_bNewSerch = TRUE;
    
    return TRUE;
}

 */

BOOL CCLanSerch::LANSerch(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[JVN_DEVICENAMELEN], int nTimeOut,BOOL isMobile,unsigned int unFrequence )
{
    
    if(m_hLANSerchRcvThread == 0 || m_SocketLANS <= 0)
    {
        return FALSE;
    }
    if (TRUE==isMobile)
    {
        m_bisMobile = TRUE;//标记手机搜索.
        m_bPingEnd = TRUE;//退出ping搜索线程.
    }
    if (unFrequence>10&&unFrequence<86400)//至少10S ping一次最长时间24h ping 一次。
    {
        m_unFreq = (unFrequence*1000)/50;
    }
  
     
//     m_bNewSerch = FALSE;//置为没有新搜索
//     m_bStopSerchImd = TRUE;//停止当前正在进行的搜索操作
    
    if(nTimeOut >= 0)
    {
        m_nTimeOut = nTimeOut;
    }
    
    m_unLANSID++;
    
    BYTE data[300];
    int nSLen = 0;
    int nSType = 0;
    int nType = JVN_REQ_LANSERCH;
    //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+[搜索参数(4)(号码\卡类型)+编组号(4)]+[设备别名长度(1)+设备别名(?)]
    memcpy(&data[0], &nType, 4);
    if(nYSTNO > 0)
    {//云视通号码优先级最高
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+号码(4)+编组号(4)
        nSType = JVN_CMD_LANSYST;
        nSLen = 20;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        memcpy(&data[20], &nYSTNO, 4);
        memcpy(&data[24], chGroup, 4);
        
        memcpy(m_chLANGroup, chGroup, 4);
        m_nLANYSTNO = nYSTNO;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = 0;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    else if(nCardType > 0)
    {//按卡型号搜索
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+卡型号(4)
        nSType = JVN_CMD_LANSTYPE;
        nSLen = 16;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        memcpy(&data[20], &nCardType, 4);
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = nCardType;
        m_nLANVariety = 0;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    else if(strlen(chDeviceName) > 0)
    {//按设备别名搜索
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+别名长度(1)+别名(?)
        int nNameLen=strlen(chDeviceName);
        nSType = JVN_CMD_LANSNAME;
        nSLen = nNameLen+13;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        data[20] = nNameLen;
        memcpy(&data[21], chDeviceName, nNameLen);
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = 0;
        memcpy(m_chDeviceName, chDeviceName, nNameLen);
    }
    else
    {//搜索全部
        //类型(4)+长度(4)+搜索端口(4)+搜索ID(4)+搜索类型(4)+产品种类(1)
        nSType = JVN_CMD_LANSALL;
        nSLen = 13;
        memcpy(&data[4], &nSLen, 4);
        memcpy(&data[8], &m_nLANSPort, 4);
        memcpy(&data[12], &m_unLANSID, 4);
        memcpy(&data[16], &nSType, 4);
        data[20] = nVariety;
        
        memset(m_chLANGroup, 0, 4);
        m_nLANYSTNO = 0;
        m_nLANCARDTYPE = 0;
        m_nLANVariety = nVariety;
        memset(m_chDeviceName, 0, JVN_DEVICENAMELEN);
    }
    
#ifndef WIN32
    pthread_mutex_lock(&m_ct);
#else
    EnterCriticalSection(&m_ct);
#endif
    
    m_nNeedSend = nSLen+8;
    memcpy(m_uchData, data, m_nNeedSend);
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ct);
#else
    LeaveCriticalSection(&m_ct);
#endif
    
    SOCKADDR_IN addrBcast;
    // 设置广播地址，这里的广播端口号
    addrBcast.sin_family = AF_INET;
    addrBcast.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
    
    addrBcast.sin_port = htons(m_nDesPort);
    
    int ntmp = CCChannel::sendtoclient(m_SocketLANS,(char *)data,nSLen+8,0,(SOCKADDR *)&addrBcast, sizeof(SOCKADDR),1);
    m_bTimeOut = FALSE;
    m_bLANSerching = TRUE;
    m_dwBeginSerch = CCWorker::JVGetTime();
    if(ntmp != nSLen+8)
    {
        return FALSE;
    }
    
    m_bNewSerch = TRUE;
	if(m_hLANSerchSndThread <= 0)
	{//先创建发送线程
#ifndef WIN32
		pthread_attr_t attr;
		pthread_attr_t *pAttr = &attr;
		unsigned long size = LINUX_THREAD_STACK_SIZE;
		size_t stacksize = size;
		pthread_attr_init(pAttr);
		if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
		{
			pAttr = NULL;
		}
		//创建本地监听线程
		if (0 != pthread_create(&m_hLANSerchSndThread, pAttr, LANSSndProc, this))
		{
			m_hLANSerchSndThread = 0;
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
			}
			return FALSE;
		}
#else
		//创建本地命令监听线程
		UINT unTheadID;
		if(m_hLANSerchSndStartEvent > 0)
		{
			CloseHandle(m_hLANSerchSndStartEvent);
			m_hLANSerchSndStartEvent = 0;
		}
		if(m_hLANSerchSndEndEvent > 0)
		{
			CloseHandle(m_hLANSerchSndEndEvent);
			m_hLANSerchSndEndEvent = 0;
		}
		//创建本地监听线程
		m_hLANSerchSndStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hLANSerchSndEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hLANSerchSndThread = (HANDLE)_beginthreadex(NULL, 0, LANSSndProc, (void *)this, 0, &unTheadID);
		if(m_hLANSerchSndStartEvent > 0)
		{
			SetEvent(m_hLANSerchSndStartEvent);
		}
		if (m_hLANSerchSndThread == 0)//创建线程失败
		{
			if(m_hLANSerchSndStartEvent > 0)
			{
				CloseHandle(m_hLANSerchSndStartEvent);
				m_hLANSerchSndStartEvent=0;
			}
			if(m_hLANSerchSndEndEvent > 0)
			{
				CloseHandle(m_hLANSerchSndEndEvent);
				m_hLANSerchSndEndEvent=0;
			}
			
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
			}
			return FALSE;
		}
	#endif
     }
    return TRUE;
}

#ifdef MOBILE_CLIENT
#ifndef WIND32
int CCLanSerch::stopLanSearchMethod()
#else
UINT CCLanSerch::stopLanSearchMethod()
#endif
{

	if (!m_sendThreadExit) {
		m_bSndEnd = TRUE;
		m_bStopSerchImd = TRUE;
        m_bNewSerch = TRUE;
		while (TRUE) {
			if (m_sendThreadExit) {
				break;
			}
			//OutputDebug("run here  channel discconect  line: %d\n",__LINE__);
			CCWorker::jvc_sleep(100);
		}
	} else {
		// printf("disconnect Channel m_recvThreadExit and m_playProExit\n");
	}

	return 1;
}

#endif

#ifndef WIN32
	void* CCLanSerch::LANSSndProc(void* pParam)
#else
UINT CCLanSerch::LANSSndProc(LPVOID pParam)
#endif
{
    CCLanSerch *pWorker = (CCLanSerch *)pParam;
#ifndef WIN32
    if(pWorker == NULL)
    {
        return NULL;
    }
#else
    if(pWorker == NULL)
    {
        return 0;
    }
    
    WaitForSingleObject(pWorker->m_hLANSerchSndStartEvent, INFINITE);
    if(pWorker->m_hLANSerchSndStartEvent > 0)
    {
        CloseHandle(pWorker->m_hLANSerchSndStartEvent);
        pWorker->m_hLANSerchSndStartEvent = 0;
    }
#endif
    
	pWorker->m_sendThreadExit = FALSE;
    DWORD dwend = 0;
    
    while (TRUE)
    {
#ifndef WIN32
        if(pWorker->m_bSndEnd)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hLANSerchSndEndEvent, 0))
        {
            break;
        }
#endif
        
        if(pWorker->m_bNewSerch)
        {//有新的搜索
            pWorker->m_bStopSerchImd = FALSE;
            pWorker->m_bNewSerch = FALSE;
            pWorker->SearchFSIpSection();//优先搜索手动增加的IP段
            char ip[30] = {0};
            SOCKADDR_IN addRemote;
            addRemote.sin_family = AF_INET ;
            addRemote.sin_addr.s_addr = INADDR_BROADCAST; //::inet_addr("255.255.255.255");
            addRemote.sin_port = htons(pWorker->m_nDesPort);
            pWorker->GetAdapterInfo();//重新获取下ip;
            BOOL bstop=FALSE;
            int ncount = pWorker->m_IpList.size();
            for(int k = 0; k < ncount;k ++)
            {
                //只检测第一个网卡，网关为空的情况暂时不考虑了
                
                char ip_head[30] = {0};
                strcpy(ip_head,pWorker->m_IpList[k].IpHead);
                //开始
                if (pWorker->m_bisMobile)//手机搜索流程;
                {
                    //从当前网卡ip段开始广播，与自己最相近的段成功率越高
                    if(pWorker->m_IpList[k].nIP3 >= 0 && pWorker->m_IpList[k].nIP3 < 256)
                    {
                        int j=0;
                        //从当前ip值向前发送10个段
                        int n = pWorker->m_IpList[k].nIP3 - 10;
                        n = ((n>=0)?n:0);
                        for(j=n; j<pWorker->m_IpList[k].nIP3; j++)
                        {
                            for(int i = 1;i < 254;i ++)
                            {
                                if(pWorker->m_bStopSerchImd)
                                {
                                    i=256;
                                    j=256;
                                    bstop = TRUE;
                                    break;
                                }
                                if(pWorker->IsPause())
                                {
                                    i --;
                                    CCWorker::jvc_sleep(50);
                                    continue;
                                }
                                sprintf(ip,"%s%d.%d",ip_head,j,i);
                                addRemote.sin_addr.s_addr = ::inet_addr(ip);
                                CCChannel::sendtoclientm(pWorker->m_SocketLANS,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                                
                                if(pWorker->m_nTimeOut > 0)
                                {
                                    dwend = CCWorker::JVGetTime();
                                    if(dwend > pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginSerch)
                                    {//超时
                                        i=256;
                                        j=256;
                                        break;
                                    }
                                }
                                
                                if (0==i%3)//sleep(1)
                                {
                                    CCWorker::jvc_sleep(40);
                                }
                            }
                        }
                        
                        if(bstop)
                        {
                            break;
                        }
                        //从当前ip值向后发送10个段
                        n = pWorker->m_IpList[k].nIP3 + 10;
                        n = ((n<=255)?n:255);
                        for(j=pWorker->m_IpList[k].nIP3; j<=n; j++)
                        {
                            for(int i = 1;i < 254;i ++)
                            {
                                if(pWorker->m_bStopSerchImd)
                                {
                                    i=256;
                                    j=256;
                                    bstop = TRUE;
                                    break;
                                }
                                if(pWorker->IsPause())
                                {
                                    i --;
                                    CCWorker::jvc_sleep(50);
                                    continue;
                                }
                                
                                sprintf(ip,"%s%d.%d",ip_head,j,i);
                                addRemote.sin_addr.s_addr = ::inet_addr(ip);
                                
                                CCChannel::sendtoclientm(pWorker->m_SocketLANS,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                                
                                if(pWorker->m_nTimeOut > 0)
                                {
                                    dwend = CCWorker::JVGetTime();
                                    if(dwend > pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginSerch)
                                    {//超时
                                        i=256;
                                        j=256;
                                        break;
                                    }
                                }
                                if (0==i%3)//sleep(1)
                                {
                                    CCWorker::jvc_sleep(40);
                                }
                            }
                        }
                        if(bstop)
                        {
                            break;
                        }
                        //向前面剩余段发送
                        n = pWorker->m_IpList[k].nIP3 - 10;
                        n = ((n>=0)?n:0);
                        for(j=0; j<n; j++)
                        {
                            for(int i = 1;i < 254;i ++)
                            {
                                if(pWorker->m_bStopSerchImd)
                                {
                                    i=256;
                                    j=256;
                                    bstop = TRUE;
                                    break;
                                }
                                if(pWorker->IsPause())
                                {
                                    i --;
                                    CCWorker::jvc_sleep(50);
                                    continue;
                                }
                                
                                sprintf(ip,"%s%d.%d",ip_head,j,i);
                                addRemote.sin_addr.s_addr = ::inet_addr(ip);
                                
                                CCChannel::sendtoclientm(pWorker->m_SocketLANS,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                                
                                if(pWorker->m_nTimeOut > 0)
                                {
                                    dwend = CCWorker::JVGetTime();
                                    if(dwend > pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginSerch)
                                    {//超时
                                        i=256;
                                        j=256;
                                        break;
                                    }
                                }
                                if (0==i%3)//sleep(5)
                                {
                                    CCWorker::jvc_sleep(40);
                                }
                            }
                        }
                        if(bstop)
                        {
                            break;
                        }
                        //向后面剩余段发送
                        n = pWorker->m_IpList[k].nIP3 + 10;
                        n = ((n<=255)?n:255);
                        for(j=n; j<256; j++)
                        {
                            for(int i = 1;i < 254; i++)
                            {
                                if(pWorker->m_bStopSerchImd)
                                {
                                    i=256;
                                    j=256;
                                    bstop = TRUE;
                                    break;
                                }
                                if(pWorker->IsPause())
                                {
                                    i --;
                                    CCWorker::jvc_sleep(50);
                                    continue;
                                }
                                
                                sprintf(ip,"%s%d.%d",ip_head,j,i);
                                addRemote.sin_addr.s_addr = ::inet_addr(ip);
                                
                                CCChannel::sendtoclientm(pWorker->m_SocketLANS,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                                
                                if(pWorker->m_nTimeOut > 0)
                                {
                                    dwend = CCWorker::JVGetTime();
                                    if(dwend > pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginSerch)
                                    {//超时
                                        i=256;
                                        j=256;
                                        break;
                                    }
                                }
                                if (0==i%3)//sleep(5)
                                {
                                    CCWorker::jvc_sleep(40);
                                }
                            }
                        }
                    }
                    
                }
                else
                {//正常流程 ;
                    for(int j =0;j<255;j++)
                    {
                        if(pWorker->m_bNum[j]==1)//也包括向自己IP列表发送数据. && j!=pWorker->m_IpList[k].nIP3)//不向自己所在的网段发送广播，因为已经发了
                        {
                            int n =0;
                            for(int k =1;k<254;k++)
                            {
                                if(pWorker->m_bStopSerchImd)
                                {
                                    bstop = TRUE;
                                    break;
                                }
                                sprintf(ip,"%s%d.%d",ip_head,j,k);
                                addRemote.sin_addr.s_addr = ::inet_addr(ip);
                                CCChannel::sendtoclientm(pWorker->m_SocketLANS,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                                if(pWorker->m_nTimeOut > 0)
                                {
                                    dwend = CCWorker::JVGetTime();
                                    if(dwend > pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginSerch)
                                    {//超时
                                        bstop=true;
                                        break;
                                    }
                                }
                                if(k - n >=6)//每发送6个IP之后sleep 1ms;Sleep是非常耗时间的- -
                                {
                                    if (1==pWorker->m_bNeedSleep )//需要sleep
                                    {
                                        CCWorker::jvc_sleep(1);
                                    }
                                    n=k;
                                }
                                
                            }//for(int k ...
                        }//if(pWorker ...
                        if(bstop)
                        {
                            break;
                        }
                    }//for (j =0...
                }//else ;
                //结束
            }//for (k =0...
        }//if(pWorker...
        else
        {
            CCWorker::jvc_sleep(10);
            break;
        }
    }//while(true)...
	pWorker->m_sendThreadExit = TRUE;
    pWorker->m_hLANSerchSndThread = 0;
    pWorker->m_bSndEnd = FALSE;

    return 0;
}

#ifndef WIN32
void* CCLanSerch::LANSRcvProc(void* pParam)
#else
UINT CCLanSerch::LANSRcvProc(LPVOID pParam)
#endif
{
    CCLanSerch *pWorker = (CCLanSerch *)pParam;
#ifndef WIN32
    if(pWorker == NULL)
    {
        return NULL;
    }
#else
    if(pWorker == NULL)
    {
        return 0;
    }
    
    WaitForSingleObject(pWorker->m_hLANSerchRcvStartEvent, INFINITE);
    if(pWorker->m_hLANSerchRcvStartEvent > 0)
    {
        CloseHandle(pWorker->m_hLANSerchRcvStartEvent);
        pWorker->m_hLANSerchRcvStartEvent = 0;
    }
#endif
    
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(SOCKADDR_IN);
    
    BYTE recBuf[RC_DATA_SIZE]={0};
    //	BYTE data[30]={0};
    int nRecvLen = 0;
    int nType = 0;
    unsigned int unSerchID = 0;
    int nRLen = 0;
    DWORD dwEnd = 0;
    int nNameLen = 0;
    STLANSRESULT stLSResult;
    memset(stLSResult.chGroup, 0, 4);
    stLSResult.nYSTNO = 0;
    stLSResult.nCardType = 0;
    stLSResult.nChannelCount = 0;
    memset(stLSResult.chClientIP, 0, 16);
    stLSResult.nClientPort = 0;
    stLSResult.nVariety = 0;
    memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
    stLSResult.nPrivateSize = 0;
    
    while (TRUE)
    {
#ifndef WIN32
        if(pWorker->m_bRcvEnd)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hLANSerchRcvEndEvent, 0))
        {
            break;
        }
#endif
        
        nRecvLen = 0;
        nRLen = 0;
        nType = 0;
        unSerchID = 0;
        stLSResult.nCardType = 0;
        stLSResult.nYSTNO = 0;
        stLSResult.nChannelCount = 0;
        memset(stLSResult.chClientIP, 0, 20);
        stLSResult.nClientPort = 0;
        stLSResult.nPrivateSize = 0;
        memset(recBuf, 0, RC_DATA_SIZE);
        nRecvLen = CCChannel::receivefrom(pWorker->m_SocketLANS,(char *)&recBuf, RC_DATA_SIZE, 0, (SOCKADDR*)&clientaddr,&addrlen,1);
        if(pWorker->m_nType == JVC_BC)
        {//接收：类型(1)+长度(4)+广播ID(4)+净载数据(?)
            if( nRecvLen > 0)
            {//接收：类型(1)+长度(4)+广播ID(4)+净载数据(?)
                if(recBuf[0] == JVN_RSP_BC)
                {
                    memcpy(&nRLen, &recBuf[1], 4);//长度(4)
                    memcpy(&unSerchID, &recBuf[5], 4);//搜索ID(4)
                    sprintf(stLSResult.chClientIP,"%s",inet_ntoa(clientaddr.sin_addr));
                    
                    if(unSerchID == pWorker->m_unLANSID)
                    {//属于当前广播，向调用层反馈
                        if(pWorker->m_nTimeOut <= 0)
                        {//不需判断超时
                            pWorker->m_pWorker->m_pfBCData(unSerchID, &recBuf[9], nRLen-4, stLSResult.chClientIP, FALSE);
                        }
                        else
                        {
                            dwEnd = CCWorker::JVGetTime();
                            if(dwEnd <= pWorker->m_dwBeginSerch + pWorker->m_nTimeOut && dwEnd >= pWorker->m_dwBeginSerch)
                            {//正常结果
                                pWorker->m_pWorker->m_pfBCData(unSerchID, &recBuf[9], nRLen-4, stLSResult.chClientIP, FALSE);
                            }
                            else
                            {//超时结果直接舍弃,判断是否提示超时
                                if(!pWorker->m_bTimeOut)
                                {
                                    pWorker->m_pWorker->m_pfBCData(unSerchID, &recBuf[9], nRLen-4, stLSResult.chClientIP, FALSE);
                                    pWorker->m_pWorker->m_pfBCData(unSerchID, NULL, 0, "", TRUE);
                                }
                                pWorker->m_bTimeOut = TRUE;
                                pWorker->m_bLANSerching = FALSE;
                            }
                        }
                    }
                    //过期数据舍弃
                }
            }
            else
            {
                if(pWorker->m_bLANSerching && pWorker->m_nTimeOut > 0 && !pWorker->m_bTimeOut)
                {//正在执行搜索,有计时要求，还没提示过
                    dwEnd = CCWorker::JVGetTime();
                    if(dwEnd >= pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwEnd < pWorker->m_dwBeginSerch)
                    {//超时结果直接舍弃,判断是否提示超时
                        pWorker->m_pWorker->m_pfBCData(pWorker->m_unLANSID, NULL, 0, "", TRUE);
                        pWorker->m_bTimeOut = TRUE;
                        pWorker->m_bLANSerching = FALSE;
                    }
                }
                
                CCWorker::jvc_sleep(10);
            }
            
            continue;//是自定义广播只执行到此，不执行设备搜索代码
        }
        
        //设备搜索
        if( nRecvLen > 0)
        {//接收：类型(4)+长度(4)+搜索ID(4)+卡系(4)+云视通号(4)+总通道数(4)+服务端口号(4)+编组号(4)+产品种类(1)+别名长度(1)+别名(?) + NetMod(4) + CurMod(4) + size(4) + info(?)
            memcpy(&nType, &recBuf[0], 4);
            if(nType == JVN_RSP_LANSERCH)
            {//新SDK协议 局域网搜索
                memcpy(&nRLen, &recBuf[4], 4);//长度(4)
                memcpy(&unSerchID, &recBuf[8], 4);//搜索ID(4)
                memcpy(&stLSResult.nCardType, &recBuf[12], 4);//卡系(4)
                memcpy(&stLSResult.nYSTNO, &recBuf[16], 4);//云视通号(4)
                memcpy(&stLSResult.nChannelCount, &recBuf[20], 4);//总通道数(4)
                memcpy(&stLSResult.nClientPort, &recBuf[24], 4);//服务端口号(4)
                memcpy(stLSResult.chGroup, &recBuf[28], 4);//编组号(4)
                stLSResult.nVariety = recBuf[32];//产品种类(1)
                nNameLen = recBuf[33];//别名长度(1)
                memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                if(nNameLen > 0 && nNameLen <= JVN_DEVICENAMELEN)
                {
                    memcpy(stLSResult.chDeviceName, &recBuf[34], nNameLen);//别名(?)
                }
                else
                {
                    memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                }
                
                sprintf(stLSResult.chClientIP,"%s",inet_ntoa(clientaddr.sin_addr));
                
                //////////////////////////////////////////////////////////////////////////
                if (nRecvLen > nNameLen+34)
                {
                    memcpy(&stLSResult.nNetMod,&recBuf[34+nNameLen],4);
                    memcpy(&stLSResult.nCurMod,&recBuf[34+nNameLen+4],4);
                    //printf("%s:%d........................netMOd:%d,cur: %d\n",__FILE__,__LINE__,stLSResult.nNetMod,stLSResult.nCurMod);
                    //printf("%s:%d............nRecvLen:%d,recbufstr:%s,add:%s,nSize: %d\n",__FILE__,__LINE__,nRecvLen,&recBuf[34+nNameLen],stLSResult.add,stLSResult.nSize);fflush(stdout);
                }
                else
                {
                    stLSResult.nNetMod = NET_MOD_UNKNOW;
                }
                //////////////////////////////////////////////////////////////////////////
                
                if(nRecvLen > nNameLen+42)
                {
                    memset(stLSResult.chPrivateInfo, 0, 500);
                    memcpy(&stLSResult.nPrivateSize, &recBuf[34+nNameLen+4+4], 4);
                    if(stLSResult.nPrivateSize > 0 && stLSResult.nPrivateSize < 500)
                    {
                        memcpy(stLSResult.chPrivateInfo, &recBuf[34+nNameLen+4+4+4], stLSResult.nPrivateSize);
                    }
                    else
                    {
                        stLSResult.nPrivateSize = 0;
                    }
                }
                else
                {
                    stLSResult.nPrivateSize = 0;
                }
                
                if(unSerchID == pWorker->m_unLANSID)
                {//属于当前搜索，向调用层反馈
                    if((pWorker->m_nLANVariety > 0 && stLSResult.nVariety != pWorker->m_nLANVariety)
                       || (strlen(pWorker->m_chDeviceName)>0 && strcmp(pWorker->m_chDeviceName,stLSResult.chDeviceName)))
                    {//非法数据
                    }
                    else
                    {
                        if(pWorker->m_nTimeOut <= 0)
                        {//不需判断超时
                            stLSResult.bTimoOut = FALSE;
                            pWorker->m_pWorker->m_pfLANSData(stLSResult);
                        }
                        else
                        {
                            dwEnd = CCWorker::JVGetTime();
                            if(dwEnd <= pWorker->m_dwBeginSerch + pWorker->m_nTimeOut && dwEnd >= pWorker->m_dwBeginSerch)
                            {//正常结果
                                stLSResult.bTimoOut = FALSE;
                                pWorker->m_pWorker->m_pfLANSData(stLSResult);
                            }
                            else
                            {//超时结果直接舍弃,判断是否提示超时
                                if(!pWorker->m_bTimeOut)
                                {
                                    stLSResult.bTimoOut = FALSE;
                                    pWorker->m_pWorker->m_pfLANSData(stLSResult);
                                    
                                    stLSResult.bTimoOut = TRUE;
                                    stLSResult.nYSTNO = 0;
                                    stLSResult.nCardType = 0;
                                    stLSResult.nVariety = 0;
                                    stLSResult.nClientPort = 0;
                                    stLSResult.nChannelCount = 0;
                                    memset(stLSResult.chClientIP, 0, 16);
                                    memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                                    pWorker->m_pWorker->m_pfLANSData(stLSResult);
                                }
                                pWorker->m_bTimeOut = TRUE;
                                pWorker->m_bLANSerching = FALSE;
                            }
                        }
                    }
                }
                //过期数据舍弃
            }
            else if(pWorker->m_nLANYSTNO <= 0 && pWorker->m_nLANCARDTYPE <= 0 && pWorker->m_nLANVariety <= 0 && strlen(pWorker->m_chDeviceName) <= 0)
            {//旧DVR支持,如果是本地按参数搜索，则这类dvr就属于不能判定的一类，果断丢弃
                PACKET stPacket;
                memset(&stPacket, 0, sizeof(PACKET));
                memcpy(&stPacket, &recBuf, sizeof(PACKET));
                
                if(stPacket.nPacketType == JVN_REQ_LANSERCH)
                {
                    nRLen = 0;
                    unSerchID = 0;
                    stLSResult.nCardType = 0xD800;
                    stLSResult.nYSTNO = 0;
                    stLSResult.nChannelCount = stPacket.nPacketCount;
                    stLSResult.nClientPort = 9101;
                    sprintf(stLSResult.chClientIP,"%s",inet_ntoa(clientaddr.sin_addr));
                    stLSResult.nVariety = 0;
                    memset(stLSResult.chGroup, 0, 4);
                    memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                    
                    if(pWorker->m_nTimeOut <= 0)
                    {//不需判断超时
                        stLSResult.bTimoOut = FALSE;
                        pWorker->m_pWorker->m_pfLANSData(stLSResult);
                    }
                    else
                    {
                        dwEnd = CCWorker::JVGetTime();
                        if(dwEnd <= pWorker->m_dwBeginSerch + pWorker->m_nTimeOut && dwEnd >= pWorker->m_dwBeginSerch)
                        {//正常结果
                            stLSResult.bTimoOut = FALSE;
                            pWorker->m_pWorker->m_pfLANSData(stLSResult);
                        }
                        else
                        {//超时结果直接舍弃,判断是否提示超时
                            if(!pWorker->m_bTimeOut)
                            {
                                stLSResult.bTimoOut = FALSE;
                                pWorker->m_pWorker->m_pfLANSData(stLSResult);
                                
                                stLSResult.bTimoOut = TRUE;
                                stLSResult.nYSTNO = 0;
                                stLSResult.nCardType = 0;
                                stLSResult.nVariety = 0;
                                stLSResult.nClientPort = 0;
                                stLSResult.nChannelCount = 0;
                                memset(stLSResult.chClientIP, 0, 16);
                                memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                                pWorker->m_pWorker->m_pfLANSData(stLSResult);
                            }
                            pWorker->m_bTimeOut = TRUE;
                            pWorker->m_bLANSerching = FALSE;
                        }
                    }
                }
            }
        }
        else
        {
            if(pWorker->m_bLANSerching && pWorker->m_nTimeOut > 0 && !pWorker->m_bTimeOut)
            {//正在执行搜索,有计时要求，还没提示过
                dwEnd = CCWorker::JVGetTime();
                if(dwEnd >= pWorker->m_dwBeginSerch + pWorker->m_nTimeOut || dwEnd < pWorker->m_dwBeginSerch)
                {//超时结果直接舍弃,判断是否提示超时
                    stLSResult.bTimoOut = TRUE;
                    stLSResult.nYSTNO = 0;
                    stLSResult.nCardType = 0;
                    stLSResult.nVariety = 0;
                    stLSResult.nClientPort = 0;
                    stLSResult.nChannelCount = 0;
                    memset(stLSResult.chClientIP, 0, 16);
                    memset(stLSResult.chDeviceName, 0, JVN_DEVICENAMELEN);
                    pWorker->m_pWorker->m_pfLANSData(stLSResult);
                    pWorker->m_bTimeOut = TRUE;
                    pWorker->m_bLANSerching = FALSE;
                }
            }
            CCWorker::jvc_sleep(10);
        }
    }
    return 0;
}


BOOL CCLanSerch::Broadcast(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut)
{
    if(m_hLANSerchRcvThread == 0 || m_SocketLANS <= 0)
    {
        return FALSE;
    }
    
    if(nSize <= 0 || nSize > 10240)
    {
        return FALSE;
    }
    /*	if (unFrequence>10&&unFrequence<86400)//至少10S ping一次最长时间24h ping 一次。
     {
     m_unFreq = (unFrequence*1000)/50;	
     }
     
     if(m_hLANSerchSndThread <= 0)
     {//先创建发送线程
     #ifndef WIN32
     pthread_attr_t attr;
     pthread_attr_t *pAttr = &attr;
     unsigned long size = LINUX_THREAD_STACK_SIZE;
     size_t stacksize = size;
     pthread_attr_init(pAttr);
     if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
     {
     pAttr = NULL;
     }
     //创建本地监听线程
     if (0 != pthread_create(&m_hLANSerchSndThread, pAttr, LANSSndProc, this))
     {
     m_hLANSerchSndThread = 0;
     if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
     {
     m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
     }
     else
     {
     m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
     }
     return FALSE;
     }
     #else
     //创建本地命令监听线程
     UINT unTheadID;
     if(m_hLANSerchSndStartEvent > 0)
     {
     CloseHandle(m_hLANSerchSndStartEvent);
     m_hLANSerchSndStartEvent = 0;
     }
     if(m_hLANSerchSndEndEvent > 0)
     {
     CloseHandle(m_hLANSerchSndEndEvent);
     m_hLANSerchSndEndEvent = 0;
     }
     //创建本地监听线程
     m_hLANSerchSndStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
     m_hLANSerchSndEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
     m_hLANSerchSndThread = (HANDLE)_beginthreadex(NULL, 0, LANSSndProc, (void *)this, 0, &unTheadID);
     if(m_hLANSerchSndStartEvent > 0)
     {
     SetEvent(m_hLANSerchSndStartEvent);
     }
     if (m_hLANSerchSndThread == 0)//创建线程失败
     {
     if(m_hLANSerchSndStartEvent > 0)
     {
     CloseHandle(m_hLANSerchSndStartEvent);
     m_hLANSerchSndStartEvent=0;
     }
     if(m_hLANSerchSndEndEvent > 0)
     {
     CloseHandle(m_hLANSerchSndEndEvent);
     m_hLANSerchSndEndEvent=0;
     }
     
     if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
     {
     m_pWorker->m_Log.SetRunInfo(0,"开启LANSerchSnd失败.原因:创建线程失败",__FILE__,__LINE__);
     }
     else
     {
     m_pWorker->m_Log.SetRunInfo(0,"start LANSerchSnd failed.Info:create thread faild.",__FILE__,__LINE__);
     }
     return FALSE;
     }
     #endif
     }
     
     
     m_bNewSerch = FALSE;//置为没有新搜索
     m_bStopSerchImd = TRUE;//停止当前正在进行的搜索操作
     */
    if(nTimeOut >= 0)
    {
        m_nTimeOut = nTimeOut;
    }
    
    m_unLANSID = nBCID;
    
    int nLen = nSize + 8;
    BYTE *pdata = new BYTE[nLen+5];
    //类型(1)+长度(4)+广播发起端口(4)+广播ID(4)+净载数据(?)
    pdata[0] = JVN_REQ_BC;
    
    memcpy(&pdata[1], &nLen, 4);
    memcpy(&pdata[5], &m_nLANSPort, 4);
    memcpy(&pdata[9], &m_unLANSID, 4);
    memcpy(&pdata[13], pBuffer, nSize);
    
#ifndef WIN32
    pthread_mutex_lock(&m_ct);
#else
    EnterCriticalSection(&m_ct);
#endif
    
    m_nNeedSend = nLen+5;
    memcpy(m_uchData, pdata, m_nNeedSend);
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ct);
#else
    LeaveCriticalSection(&m_ct);
#endif
    
    SOCKADDR_IN addrBcast;
    // 设置广播地址，这里的广播端口号    
    addrBcast.sin_family = AF_INET;   
    addrBcast.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");    
    addrBcast.sin_port = htons(m_nDesPort);
    
    int ntmp = CCChannel::sendtoclient(m_SocketLANS,(char *)pdata,nLen+5,0,(SOCKADDR *)&addrBcast, sizeof(SOCKADDR),1);
    
    m_bTimeOut = FALSE;
    m_bLANSerching = TRUE;
    m_dwBeginSerch = CCWorker::JVGetTime();
    if(ntmp != nLen+5)
    {
        delete[] pdata;
        return FALSE;
    }
    
    //	m_bNewSerch = TRUE;
    
    delete[] pdata;
    return TRUE;
}
#ifndef WIN32
void * CCLanSerch::PingLanIpProc(void* pParam)
#else
UINT CCLanSerch::PingLanIpProc(LPVOID pParam)
#endif
{
    CCLanSerch *pWorker = (CCLanSerch *)pParam;
#ifndef WIN32
    if(pWorker == NULL)
    {	
        return NULL;
    }
#else
    if(pWorker == NULL)
    {		
        return 0;
    }	
#endif 
    
#ifdef MOBILE_CLIENT
    return NULL;
#endif
    BOOL  bFirstSrch =TRUE;//标记第一次获得ip,需要立即执行搜索.
    int n  =0 ;//sleep(50)*n;
    DWORD dwSt=0;//记录每次ping 所需的时间
    DWORD dwEd=0;
    IcmpHeader* pIcmpHeader = reinterpret_cast<IcmpHeader*>(pWorker->m_szIcmpData);
    int nPackSize = 32 + sizeof(IcmpHeader);
    int nSeqNO = 0;
    int nRet = 0;
    char ip[32]={0};
    char ip_head[30] = {0};
    
    while (TRUE)
    {
        if(pWorker->m_bPingEnd || -1==pWorker->m_socketRaw)
        {
            break;
        }
        if (pWorker->m_bEnablePing)//如果需要ping网关
        {
            pWorker->GetAdapterInfo();
            if (pWorker->m_IpList.size()>0&&(n>=pWorker->m_unFreq||bFirstSrch))//取得网卡信息&&（睡眠到时||第一次取得网卡信息需要马上执行ping）
            {
                bFirstSrch=FALSE;
                n=0;//计数清零;
                strcpy(ip_head,pWorker->m_IpList[0].IpHead);
                dwSt=CCWorker::JVGetTime();
                for (int i = 0; i < 255; i++)//发送次数限制
                {
                    if (pWorker->m_bPingEnd)
                    {
                        break;
                    }
                    FD_ZERO(&pWorker->m_fSet); 
                    FD_SET(pWorker->m_socketRaw,&pWorker->m_fSet);
                    int status=select(pWorker->m_socketRaw+1, &pWorker->m_fSet, (fd_set*)NULL, (fd_set *)NULL, &pWorker->m_tv); 
                    if (status <0)
                    {
                        continue ;
                    }
                    sprintf(ip,"%s%d.%d",ip_head,i,1);
                    pIcmpHeader->i_cksum = 0; // 先把ICMP的检验和置零
                    pIcmpHeader->i_seq = nSeqNO++; // 发送顺序加一
#ifndef WIN32
                    pWorker->m_icmpaddr.sin_addr.s_addr = inet_addr(ip);
                    pIcmpHeader->timestamp=0;//时间戳先不设置
#else
                    pWorker->m_icmpaddr.sin_addr.S_un.S_addr = inet_addr(ip);
                    pIcmpHeader->timestamp = GetTickCount(); //记录时间
#endif
                    unsigned long ulChkSum = 0; // 给检验和置0
                    int nSize = nPackSize;
                    unsigned short  * pBuff =(unsigned short *) pWorker->m_szIcmpData;
                    while(nSize > 1)
                    {
                        ulChkSum += *pBuff++; // 把所有IP数据报的数据都加到一起
                        nSize -= sizeof(unsigned short );// 每次计算两个十二位二进制和  
                    }
                    if(nSize != 0)
                    {
                        ulChkSum += *(unsigned char *)pBuff; // 判断当只剩下一个字节没有加到检验和时在这里加上
                    }
                    ulChkSum = (ulChkSum >> 16) + (ulChkSum & 0XFFFF); // 先按32bit计算二进制和得到cksum，然后把高位低位相加
                    //得到16bit的和。
                    ulChkSum += (ulChkSum >>16); // 把高16位和低16位相加溢出的进位再加到校验和中
                    pIcmpHeader->i_cksum =(unsigned short )(~ulChkSum);  // 取反得到检验和
                    //校验结束
                    nRet = sendto(pWorker->m_socketRaw, (char * )pWorker->m_szIcmpData, nPackSize, 0, (sockaddr*)&pWorker->m_icmpaddr, sizeof(pWorker->m_icmpaddr));
                    if (nRet == -1)
                    {
                        continue;
                    }
                    int nSrcLen = sizeof(pWorker->m_icmpaddr);
                    if (FD_ISSET(pWorker->m_socketRaw,&pWorker->m_fSet))
                    {
#ifndef WIN32
                        nRet = recvfrom(pWorker->m_socketRaw,pWorker->m_szRecvBuff, sizeof(pWorker->m_szRecvBuff), 0, (sockaddr*)&pWorker->m_icmpaddr, (socklen_t*)&nSrcLen);		
#else 
                        nRet = recvfrom(pWorker->m_socketRaw, pWorker->m_szRecvBuff, sizeof(pWorker->m_szRecvBuff), 0, (sockaddr*)&pWorker->m_icmpaddr, &nSrcLen);
#endif 
                        if (nRet == -1)
                        {
                            continue;
                        }
                        unsigned char * p = (unsigned char * )&pWorker->m_icmpaddr.sin_addr.s_addr;	
                        pWorker->m_bNum[p[2]]= 1;//此网关可通0.1.[2].3
                    } 
                    DWORD dwCur = CCWorker::JVGetTime();
                    if (dwCur  - dwSt > 700)//ping 网关超过700毫秒说明有问题,可以提示手动增加优先搜索段.
                    {
                        break;
                    }
                }//for...
                dwEd=CCWorker::JVGetTime();
                if (dwEd - dwSt > 700)//发送时间大于700ms则说明此网络有问题
                {
                    pWorker->m_bNeedSleep =0;//不需要sleep
                }
                else 
                {
                    pWorker->m_bNeedSleep =1 ;//需要sleep
                }
            }
            else if(pWorker->m_IpList.size()<=0)//获得网卡信息失败,下次获得成功时立即ping .
            {
                bFirstSrch =TRUE;
            }
        }
        if(pWorker->m_bFirstRun)
        {
            pWorker->m_bFirstRun = FALSE;
            pWorker->m_pWorker->m_Helper.SearchDevice(pWorker->m_bNum,pWorker->m_bisMobile);
        }
        CCWorker::jvc_sleep(SLEEP_PER_TIME);
        n++;				
    }//while
#ifndef WIN32
    return NULL;
#else
    return 0;
#endif 
}
//开始
int  CCLanSerch::AddFSIpSection(const IPSECTION * Ipsection ,int nSize,BOOL bEnablePing)//添加优先搜索IP段
{
    m_bEnablePing =bEnablePing;
    if (NULL == Ipsection||nSize<=0||nSize>10*sizeof(IPSECTION))
    {	
        return -1;
    }
    if (false ==m_isUsing )//若当前数组未使用
    {
        m_isUsing = true;//标记当前正在使用
        memset(m_szSecIp,0,MAX_IPSEC_NUM*sizeof(IPSECTION));//将原来的数据清零
        m_nIpSecSize=0;   //数据长度清零
        memcpy(m_szSecIp,Ipsection,nSize);//将数据拷贝到内存中以便使用
        m_nIpSecSize=nSize;
        m_isUsing  =false;
        return 0;
    }
    //若当前数组正在被使用中，那么直接跳过，不执行任何操作
    return -1;
}

void CCLanSerch::SearchFSIpSection()//优先搜索指定IP段中的设备
{
    if (m_nIpSecSize<sizeof(IPSECTION)||m_nIpSecSize>10*sizeof(IPSECTION))//数组大小不合法
    {
        return ;//直接返回不做任何动作.
    }
    if (false == m_isUsing )//如果当前没有使用此数组
    {
        m_isUsing = true;//标记为正在使用
        SOCKADDR_IN addRemote;
        addRemote.sin_family = AF_INET;   
        addRemote.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
        IPSECTION* ipStart =NULL;
        int n =0;//记录IP组个数；
        int k =0,m =0;//记录开始段 ，类似(192.168.k.m)
        int s =0 ,o =0;//记录结束段 ，类似(192.168.s.o)
        char buf[16] ={0};
        for(int j = m_nIpSecSize;j>=sizeof(IPSECTION);)
        {
            ipStart=(IPSECTION*)(m_szSecIp+n*sizeof(IPSECTION)/*+(n==0?0:-1)*/);
            if(INADDR_NONE==(inet_addr(ipStart->startip))||INADDR_NONE==inet_addr(ipStart->endip))//开始ip结束ip如果不正确则忽略不搜索
            {
                j-=sizeof(IPSECTION);
                continue;
            }
            else
            {
                
                addRemote.sin_addr.s_addr=inet_addr(ipStart->startip);
                unsigned char * p = (unsigned char * )&addRemote.sin_addr.s_addr;
                k  = p[2];//段结构
                m  = p[3];
                addRemote.sin_addr.s_addr = inet_addr(ipStart->endip);
                s  = p[2];//段结构
                o  =p[3];
                if (k>s||k==s&&m>o)//IP不合法
                {
                    continue ;//不合法
                }
                for(int mm = k ;mm<=s ;mm++)
                {
                    for (int nn = m ;nn<=255;nn++)
                    {
                        sprintf(buf,"%d.%d.%d.%d", p[0], p[1],mm,nn);
                        addRemote.sin_addr.s_addr=inet_addr(buf);
                        addRemote.sin_port = htons(m_nDesPort);
                        CCChannel::sendtoclientm(m_SocketLANS,(char *)m_uchData,m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
                        if (mm == s &&nn>=o)
                        {
                            break;//所有ip循环完了
                        }
                    }
                }
            }
            n += 1;
            j-=sizeof(IPSECTION);
        }//for
        
        m_isUsing =false ;//标记数组未使用
        
    }//if (false == m_isUsing )
    
    return ;
}
//结束

BOOL CCLanSerch::IsPause()
{
    for(int i = 0;i < 256;i ++)
    {
        if(m_bPause[i])
            return TRUE;
    }
    return FALSE;
    
}