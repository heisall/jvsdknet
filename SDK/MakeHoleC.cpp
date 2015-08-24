// MakeHoleC.cpp: implementation of the CMakeHoleC class.
//
//////////////////////////////////////////////////////////////////////

#include "MakeHoleC.h"

#include "CWorker.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern CCWorker *g_pCWorker;
extern int JVC_MSS;
CLocker::CLocker(pthread_mutex_t& lock,char* pName,int nLine):
m_Mutex(lock),
m_iLocked()
{
    char str[1000] = {0};
    sprintf(str,"Lock>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s %d\n",pName,nLine);
    strcpy(m_strName,pName);
    m_nLine = nLine;
#ifndef WIN32
    //	printf(str);
    m_iLocked = pthread_mutex_lock(&m_Mutex);
#else
    //OutputDebugString(str);
    m_iLocked = WaitForSingleObject(m_Mutex, INFINITE);
#endif/**/
}

// Automatically unlock in destructor
CLocker::~CLocker()
{
    char str[1000] = {0};
    sprintf(str,"Unlock<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<...........  %s %d\n",m_strName,m_nLine);
#ifndef WIN32
    //printf(str);
    if (0 == m_iLocked)
        pthread_mutex_unlock(&m_Mutex);
#else
    //OutputDebugString(str);
    if (WAIT_FAILED != m_iLocked)
        ReleaseMutex(m_Mutex);
#endif/**/
}

void CLocker::enterCS(pthread_mutex_t& lock)
{
#ifndef WIN32
    pthread_mutex_lock(&lock);
#else
    WaitForSingleObject(lock, INFINITE);
#endif
}

void CLocker::leaveCS(pthread_mutex_t& lock)
{
#ifndef WIN32
    pthread_mutex_unlock(&lock);
#else
    ReleaseMutex(lock);
#endif
}

CMakeHoleC::CMakeHoleC(char *pGroup,int nYst,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatA,int nNatB,int nUpnp,void* pChannel,BOOL bVirtual)
{
    m_pParent = pChannel;
    m_bVirtual = bVirtual;
    m_bConnectOK = FALSE;
#ifndef WIN32
    pthread_mutex_init(&m_Connect, NULL);
#else
    m_Connect = CreateMutex(NULL, false, NULL);
#endif
    
    m_sWorkSocket = 0;
    m_sLocalSocket2 = 0;
    m_udtSocket = 0;
    m_nConnectNum = 0;
    
    m_dwStartTime = CCWorker::JVGetTime();
    m_bRuning = TRUE;
    m_bTryLocal = bLocalTry;
    m_nChannel = nChannel;
    
    m_nNatA = nNatA;
    m_nNatB = nNatB;
    
    strcpy(m_strGroup,pGroup);
    m_nYst = nYst;
    
    m_ServerSList.clear();
    for(int i = 0;i < ServerList.size();i ++)
    {
        m_ServerSList.push_back(ServerList[i]);
    }
    m_nLocalPortWan = nUpnp;
    
    memset(&m_addA,0,sizeof(SOCKADDR_IN));
    memset(&m_addLocalA,0,sizeof(SOCKADDR_IN));
    //	m_nLocalPort = 0;
    //	m_nStatus = 0;
    
    for(int k = 0;k < MAX_NUM;k ++)
        m_sLocalSocket[k] = 0;
    
    Start();
}

CMakeHoleC::~CMakeHoleC()
{
    End();
}

void CMakeHoleC::End()
{
    m_bRuning = FALSE;
    
    if(m_sLocalSocket2 > 0)
    {
        closesocket(m_sLocalSocket2);
        m_sLocalSocket2 = 0;
    }
    
#ifndef WIN32
    if (0 != m_hConnThread)
    {
        m_bEndC = TRUE;
        pthread_join(m_hConnThread, NULL);
        m_hConnThread = 0;
    }
#else
    if(m_hEndEventC>0)
    {
        SetEvent(m_hEndEventC);
    }
    
    CCWorker::jvc_sleep(10);
    CCChannel::WaitThreadExit(m_hConnThread);
    
#endif
}

void CMakeHoleC::Start()
{
    int k = 0;
    for(k = 0;k < MAX_NUM;k ++)
    {
        if(m_sLocalSocket[k] != 0)
            closesocket(m_sLocalSocket[k]);
    }
    
    for(k = 0;k < MAX_NUM;k ++)
    {
        m_sLocalSocket[k] = socket(AF_INET, SOCK_DGRAM,0);
        
        SOCKADDR_IN addrSrv = {0};
#ifndef WIN32
        addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
        addrSrv.sin_family = AF_INET;
        addrSrv.sin_port = htons(0);
        
        //绑定套接字
        bind(m_sLocalSocket[k], (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
        
        int nReuse = 1;
        setsockopt(m_sLocalSocket[k], SOL_SOCKET, SO_REUSEADDR, (char *)&nReuse, sizeof(int));
        
        struct sockaddr_in sin;
        int len = sizeof(sin);
#ifdef WIN32
        if(getsockname(m_sLocalSocket[k], (struct sockaddr *)&sin, &len) != 0)
#else
            if(getsockname(m_sLocalSocket[k], (struct sockaddr *)&sin, (socklen_t* )&len) != 0)
#endif
            {
                printf("getsockname() error:%s\n", strerror(errno));
                closesocket(m_sLocalSocket[k]);
                m_sLocalSocket[k] = 0;
            }
        
        //		int m_wLocalPort = ntohs(sin.sin_port);
        
        //		OutputDebug("Local port = %d   SOCKET = %d",m_wLocalPort,m_sLocalSocket[k]);
    }
    
    
    memset(&m_addA,0,sizeof(SOCKADDR_IN));
    memset(&m_addLocalA,0,sizeof(SOCKADDR_IN));
    
    //初始化参数
#ifndef WIN32
    m_bEndC = FALSE;
    
    pthread_mutex_init(&m_ct, NULL); //初始化临界区
#else
    m_hStartEventC = 0;
    m_hEndEventC = 0;
    
    InitializeCriticalSection(&m_ct); //初始化临界区
#endif
    
    //	m_nStatus = 1;
    //获取地址，打洞线程
    {
        //启动线程
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
        if (0 != pthread_create(&m_hConnThread, pAttr, ConnectRemoteProc, this))
        {
            m_hConnThread = 0;
        }
#else
        //开启连接线程
        UINT unTheadID;
        m_hStartEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        m_hConnThread = (HANDLE)_beginthreadex(NULL, 0, ConnectRemoteProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventC);
        if (m_hConnThread == 0)//创建线程失败
        {
            if(m_hStartEventC > 0)
            {
                CloseHandle(m_hStartEventC);
                m_hStartEventC = 0;
            }
            
            if(m_hEndEventC > 0)
            {
                CloseHandle(m_hEndEventC);
                m_hEndEventC = 0;
            }
        }
#endif
        
    }
    //连接线程
    {
        //启动线程
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
        if (0 != pthread_create(&m_hConnThread, pAttr, ConnectThread, this))
        {
            m_hConnThread = 0;
        }
#else
        //开启连接线程
        UINT unTheadID;
        m_hStartEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        m_hConnThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThread, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventC);
        if (m_hConnThread == 0)//创建线程失败
        {
            if(m_hStartEventC > 0)
            {
                CloseHandle(m_hStartEventC);
                m_hStartEventC = 0;
            }
            
            if(m_hEndEventC > 0)
            {
                CloseHandle(m_hEndEventC);
                m_hEndEventC = 0;
            }
        }
#endif
    }
    
}

int CMakeHoleC::GetLocalNat(int nMinTime)
{
    //	OutputDebug("查询NAT........\n\n\n\n");
    m_NatList.clear();
    DWORD dwTime = CCWorker::JVGetTime();
    
    //向S发送请求 类型(4)+长度(4)+ID(4)
    char data[1024] = {0};
    int nType = JVN_CMD_YSTCHECK2;
    memcpy(&data[0], &nType, 4);
    int nL = 4;
    memcpy(&data[4], &nL, 4);
    memcpy(&data[8], &dwTime, 4);//保证唯一 使用
    
    nL += 8;
    
    BYTE pNatList[1024] = {0};
    
    int nServerNum = 0;
    char ip[30] = {0};
    BYTE bip[4] = {0};
    unsigned short port = 0;
    int size = m_ServerSList.size();
    int k = 0;
    
    for(k = 0;k < size;k ++)//向每个服务器
    {
        SOCKADDR_IN serv_addr = {0};
        memcpy(&serv_addr,&m_ServerSList[k].addr,sizeof(SOCKADDR_IN));
        
        serv_addr.sin_port = htons(8000);
        
        //sendto(m_sLocalSocket, data, nL, 0, (sockaddr *)&serv_addr, sizeof(sockaddr_in));
        //sendto(m_sLocalSocket, data, nL, 0, (sockaddr *)&serv_addr, sizeof(sockaddr_in));
        for(int m = 0;m < MAX_NUM;m ++)
            CCChannel::sendtoclientm(m_sLocalSocket[m], data, nL, 0, (sockaddr *)&serv_addr, sizeof(sockaddr_in), 1);
        
    }
    sockaddr_in addr;
    unsigned long dwbegin = CCWorker::JVGetTime();
    unsigned long dwend = CCWorker::JVGetTime();
    while (dwend < nMinTime + dwbegin)
    {
        dwend = CCWorker::JVGetTime();
        int len = sizeof(sockaddr_in);
        //	//OutputDebug("recv.....");
        memset(data,0,1024);
        int n = 0;
        
        //OutputDebug("recv start.......%d",m_sLocalSocket);
        for(int m = 0;m < MAX_NUM;m ++)
        {
            n = CCChannel::receivefromm(m_sLocalSocket[m], (char *)data, 1024, 0, (sockaddr *)&addr, &len,100);
            //OutputDebug("recv end.......%d                  %d = ",m_sLocalSocket,n);
            //n = recvfrom(m_sLocalSocket,data,1024,0,(sockaddr *)&addr, &len);
            
            if(n > 0)
            {
                int nType = 0;
                memcpy(&nType,data,4);
                if(nType == JVN_CMD_YSTCHECK2)
                {
                    int uid = 0;//发送时的UDT编号
                    memcpy(&uid, &data[8], 4);
                    
                    NAT nat;
                    memcpy(&nat.ip, &data[12], 4);
                    
                    memcpy(&nat.port, &data[16], 2);
                    
                    memcpy(&nat.nSVer, &data[18], 4);
                    
                    memcpy(&nat.unseraddr,&addr.sin_addr, 4);
                    
                    m_NatList.push_back(nat);
                    //					OutputDebug("Net = %d.%d.%d.%d:%d  (%s)",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port,inet_ntoa(addr.sin_addr));
                    
                    if(m_nLocalPortWan > 0)
                    {
                        nat.port = m_nLocalPortWan;
                        
                        //OutputDebug("Add local NAT %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
                        m_NatList.push_back(nat);
                    }
                    nServerNum ++;
                }
            }
        }
        
        CCWorker::jvc_sleep(10);
    }
    //	OutputDebug("Get NAT number = %d\n\n\n",nServerNum);
    return nServerNum;
    return 0;
}

void CMakeHoleC::GetNATADDR()
{
    NATList LocalIPList;//本地IP列表
    g_pCWorker->GetLocalIP(&LocalIPList);
    
    NAT nat;
    nat.port = m_nNatB;
    LocalIPList.push_back(nat);
    
    m_NATListTEMP.clear();
    
    int ncount = LocalIPList.size();
    int i = 0;
    for(i=0; i<ncount; i++)
    {
        NAT nat = LocalIPList[i];
        m_NATListTEMP.push_back(nat);
    }
    
    BOOL bfind = FALSE;
    ncount = m_NatList.size();
    for(i=0; i<ncount; i++)
    {
        //////////////////////////////////////////////////////////////////////////
        bfind = FALSE;
        int ncn = m_NATListTEMP.size();
        for(int ni = 0; ni<ncn; ni++)
        {
            if(m_NatList[i].port == m_NATListTEMP[ni].port
               && m_NatList[i].ip[0] == m_NATListTEMP[ni].ip[0]
               && m_NatList[i].ip[1] == m_NATListTEMP[ni].ip[1]
               && m_NatList[i].ip[2] == m_NATListTEMP[ni].ip[2]
               && m_NatList[i].ip[3] == m_NATListTEMP[ni].ip[3])
            {//匹配，找到记录,采取先读后下载方式
                bfind = TRUE;
                ////OutputDebug("NAT exist %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
                break;
            }
        }
        
        if(!bfind)
        {
            //				//OutputDebug("Add local NAT %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
            m_NATListTEMP.push_back(m_NatList[i]);
            //			//OutputDebug("NAT %d",pNatListAll[i].port);
            
        }
        //////////////////////////////////////////////////////////////////////////
    }
    //	//OutputDebug("Nat num = %d",pNatList->size());
    
}

BOOL AddrIsTheSame(sockaddr* addrRemote,sockaddr* addrRemote2)
{
    for(int i = 0;i < 6;i ++)
    {
        if(addrRemote->sa_data[i] != addrRemote2->sa_data[i])
            return FALSE;
    }
    return TRUE;
    
}

void CMakeHoleC::PrePunch(SOCKET s,SOCKADDR_IN addrA)
{
    //向A发送打洞包
    BYTE data[1024]={0};
    data[0] = JVN_CMD_TRYTOUCH;
    //	s = m_sWorkSocket;
    
    if(m_sWorkSocket != s)
    {
        SOCKADDR_IN desAN;
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        
        //		OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        desAN.sin_port = htons(ntohs(desAN.sin_port)+1);
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//下一端口
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//下一端口
        //	OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        
        SOCKADDR_IN addrT;
        memcpy(&addrT,&addrA,sizeof(SOCKADDR_IN));
        addrT.sin_port = htons(8000);
        
        int nCount = 40;
        int nStart = ntohs(addrA.sin_port) - nCount / 2;
        
        int i = 0;
        for(;i < nCount/2;i ++)
        {
            desAN.sin_port = htons(nStart + i);
            CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
            CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
            //	OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        }
        for(;i < nCount;i ++)
        {
            desAN.sin_port = htons(nStart + i);
            CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
            CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
        }
        //8000
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        
        //原端口
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        
        //8000
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        
        //原端口 + 1
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        desAN.sin_port = htons(ntohs(desAN.sin_port)+1);
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        CCChannel::sendtoclientm(m_sWorkSocket, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
    }
    else
    {
        SOCKADDR_IN desAN;
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        
        //		OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        desAN.sin_port = htons(ntohs(desAN.sin_port)+1);
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//下一端口
        //	OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        
        SOCKADDR_IN addrT;
        memcpy(&addrT,&addrA,sizeof(SOCKADDR_IN));
        addrT.sin_port = htons(8000);
        
        int nCount = 40;
        int nStart = ntohs(addrA.sin_port) - nCount / 2;
        
        int i = 0;
        for(;i < nCount/2;i ++)
        {
            desAN.sin_port = htons(nStart + i);
            CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
            //	OutputDebug(" ============= %d",ntohs(desAN.sin_port));
        }
        for(;i < nCount;i ++)
        {
            desAN.sin_port = htons(nStart + i);
            CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);
        }
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
        
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&addrT, sizeof(SOCKADDR_IN), 1);
        
        //原端口 + 1
        memcpy(&desAN,&addrA,sizeof(SOCKADDR_IN));
        desAN.sin_port = htons(ntohs(desAN.sin_port)+1);
        CCChannel::sendtoclientm(s, (char*)data, 20, 0, (sockaddr *)&desAN, sizeof(SOCKADDR_IN), 1);//当前端口
    }
    
}

BOOL CMakeHoleC::AddConnect(CONNECT_INFO info,BOOL bFirst)
{
    CLocker lock(m_Connect,__FILE__,__LINE__);
    
    BOOL bFind = FALSE;
    for(::std::vector<CONNECT_INFO >::iterator i = m_UdtConnectList.begin(); i != m_UdtConnectList.end();i ++)
    {
        CONNECT_INFO pkg = {0};
#ifdef WIN32
        memcpy(&pkg,i,sizeof(CONNECT_INFO));
#else
        memcpy(&pkg.addrRemote,&i->addrRemote,sizeof(pkg.addrRemote));
        pkg.bConnected = i->bConnected;
        pkg.nChannel = i->nChannel;
        pkg.nTimeout = i->nTimeout;
        pkg.pChannel = i->pChannel;
        pkg.sSocket = i->sSocket;
#endif
        if(pkg.sSocket == info.sSocket
           && AddrIsTheSame((sockaddr*)&pkg.addrRemote,(sockaddr*)&info.addrRemote)
           && pkg.bIsHole == info.bIsHole)
        {
            bFind = TRUE;
            break;
        }
    }
    
    if(!bFind)
    {
        PrePunch(info.sSocket,info.addrRemote);
        if(bFirst)
        {
            info.bIsHole = TRUE;
            m_UdtConnectList.insert(m_UdtConnectList.begin(),info);
        }
        else
        {
            m_UdtConnectList.push_back(info);
        }
        //		OutputDebug("添加连接地址 %s : %d",inet_ntoa(info.addrRemote.sin_addr),ntohs(info.addrRemote.sin_port));
        return TRUE;
    }
    return FALSE;
}

int CMakeHoleC::GetAddress()
{
    int nMaxTimeOut = 10 * 1000;//20s
    if(!m_bVirtual)
    {
        CCChannel* pChannel = (CCChannel* )m_pParent;
        if(pChannel->m_stConnInfo.nConnectType == TYPE_MO_UDP
           || pChannel->m_stConnInfo.nConnectType == TYPE_3GMO_UDP
           || pChannel->m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)
        {
            nMaxTimeOut = 3 * 1000;
        }
    }
    
    memset(&m_addLocalA,0,sizeof(SOCKADDR_IN));
    
    char chdata[1024]={0};
    //构造NAT列表
    BYTE strIP[1024]  = {0};
    unsigned short wNAT_PORT[20] = {0};
    int maxIP = m_NATListTEMP.size() > 20 ?20:m_NATListTEMP.size();
    int nSize = maxIP * 6;
    for(int k = 0;k < maxIP;k ++)
    {
        memcpy(&strIP[k * 6 + 0],m_NATListTEMP[k].ip,4);
        memcpy(&strIP[k * 6 + 4],&m_NATListTEMP[k].port,2);
        
        ////OutputDebug("%d.%d.%d.%d %d",m_NATListTEMP[k].ip[0],m_NATListTEMP[k].ip[1],m_NATListTEMP[k].ip[2],m_NATListTEMP[k].ip[3],m_NATListTEMP[k].port);
    }
    
    //向服务器发送本地的信息nAT 局域网IP
    
    BYTE data1[1024] = {0};
    int nType = JVN_REQ_EXCONNA2;//JVN_REQ_CONNA;
    
    memcpy(&data1[0], &nType, 4);
    memcpy(&data1[4], &m_nYst, 4);
    memcpy(&data1[8], strIP, nSize);
    nSize += 8;
    //	OutputDebug("查询主控.......");
    
    SOCKADDR_IN addr = {0};
    
    unsigned long dwStart = CCWorker::JVGetTime();
    unsigned long dwEnd = CCWorker::JVGetTime();
    char data[1024] = {0};
    int len = 0;
    BOOL bOK = FALSE;
    int nMaxSendTimes = 5,nTimes = 0;;
    while(dwEnd <= dwStart + nMaxTimeOut && !bOK)
    {
#ifndef WIN32
        if(m_bEndC)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(m_hConnThread, 0))
        {
            if(m_hConnThread > 0)
            {
                CloseHandle(m_hConnThread);
                m_hConnThread = 0;
            }
            
            break;
        }
#endif
        if(nTimes < nMaxSendTimes)
        {
            nTimes ++;
            for (int i=0; i< m_ServerSList.size(); i++) {
                //固定端口查询
                m_pWorker->SendUdpDataForMobile(m_sWorkSocket,m_ServerSList[i].addr,m_nYst,&data1[8], nSize - 8);
            }
            
            
            //获取主控 连接	 新查询
            for(int m = 0;m < MAX_NUM;m ++)
            {
                for(int i = 0;i < m_ServerSList.size();i ++)
                {
                    STSERVER srv = {0};
                    memcpy(&srv,&m_ServerSList[i],sizeof(STSERVER));
                    
                    CCChannel::sendtoclientm(m_sLocalSocket[m], (char*)data1, nSize, 0, (sockaddr *)&srv.addr, sizeof(SOCKADDR_IN), 1);
                }
            }
            CCWorker::jvc_sleep(10);
            
        }
        dwEnd = CCWorker::JVGetTime();
        len = sizeof(SOCKADDR_IN);
        int n = 0;
        UDP_LIST list;
        list.clear();
        n = m_pWorker->GetUdpData(m_sWorkSocket,m_nYst,&list);
        if(n > 0)
        {
            UDP_PACKAGE pkg = {0};
            
            for(::std::vector<UDP_PACKAGE >::iterator i = list.begin(); i != list.end();i ++)
            {
#ifdef WIN32
                memcpy(&pkg,i,sizeof(UDP_PACKAGE));
#else
                memcpy(&pkg.addrRemote,&i->addrRemote,sizeof(SOCKADDR_IN));
                pkg.nLen = i->nLen;
                pkg.nLocal = i->nLocal;
                pkg.sSocket = i->sSocket;
                pkg.time = i->time;
                memcpy(pkg.cData,i->cData,sizeof(pkg.cData));
#endif
                int nType = 0;
                memcpy(&nType,pkg.cData,4);
                if(nType == JVN_CMD_TRYTOUCH)
                {
                    CONNECT_INFO info = {0};
                    info.sSocket = pkg.sSocket;
                    memcpy(&info.addrRemote,&pkg.addrRemote,sizeof(sockaddr));
                    
                    info.nChannel = m_nChannel;
                    info.nTimeout = 1500;
                    
                    info.pChannel = m_pParent;
                    info.bIsHole = TRUE;
                    
                    if(AddConnect(info,TRUE))
                    {
                        //						OutputDebug("recv=======>>>>> hole. %s : %d ",inet_ntoa(info.addrRemote.sin_addr),ntohs(info.addrRemote.sin_port));
                    }
                    
                }
                else if(nType == YST_A_NEW_ADDRESS)
                {
                    SOCKADDR_IN m_addrAN,m_addrAL;
                    memcpy(&m_addrAN,&pkg.cData[8],sizeof(SOCKADDR_IN));
                    memcpy(&m_addrAL,&pkg.cData[8 + sizeof(SOCKADDR_IN)],sizeof(SOCKADDR_IN));
                    m_addrAN.sin_family = AF_INET;
                    m_addrAL.sin_family = AF_INET;
                    
                    //添加服务器返回的新的主控地址
                    CONNECT_INFO info = {0};
                    info.sSocket = pkg.sSocket;
                    memcpy(&info.addrRemote,&m_addrAN,sizeof(sockaddr));
                    
                    info.nChannel = m_nChannel;
                    info.nTimeout = 1500;
                    
                    info.pChannel = m_pParent;
                    
                    if(AddConnect(info))
                    {
                        //OutputDebug("query new addr  %s:%.",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                    }
                }
                else if(nType == JVN_RSP_CONNA)
                {
                    SOCKADDR_IN m_addrANOLD,m_addrAL;
                    memcpy(&m_addrANOLD,&pkg.cData[8],sizeof(SOCKADDR_IN));
                    memcpy(&m_addrAL,&pkg.cData[8 + sizeof(SOCKADDR_IN)],sizeof(SOCKADDR_IN));
                    m_addrANOLD.sin_family = AF_INET;
                    m_addrAL.sin_family = AF_INET;
                    
                    BOOL bTryLocal = pkg.cData[40];
                    
                    //添加服务器返回的主控地址
                    if(bTryLocal)
                    {
                        CONNECT_INFO info = {0};
                        info.sSocket = pkg.sSocket;
                        memcpy(&info.addrRemote,&m_addrAL,sizeof(sockaddr));
                        
                        info.nChannel = m_nChannel;
                        info.nTimeout = 1000;
                        
                        info.pChannel = m_pParent;
                        
                        if(AddConnect(info))
                        {
                            //							OutputDebug("recv old Nat %s:%d\n",inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
                        }
                    }
                    CONNECT_INFO info = {0};
                    info.sSocket = pkg.sSocket;
                    memcpy(&info.addrRemote,&m_addrANOLD,sizeof(sockaddr));
                    
                    info.nChannel = m_nChannel;
                    info.nTimeout = 1500;
                    
                    info.pChannel = m_pParent;
                    
                    if(AddConnect(info))
                    {
                        //						OutputDebug("recv old Nat %s:%d\n",inet_ntoa(m_addrANOLD.sin_addr),ntohs(m_addrANOLD.sin_port));
                    }
                }
            }
        }
        //固定端口没有收到
        SOCKET s = 0;
        if(n <= 0)
        {
            //n = recvfrom(m_sLocalSocket, (char *)data, 1024, 0, (sockaddr *)&addr, &len);
            for(int m = 0;m < MAX_NUM;m ++)
            {
                n = CCChannel::receivefrom(m_sLocalSocket[m], (char *)data, 1024, 0, (sockaddr *)&addr, &len, 1);
                if(n > 0)
                {
                    s = m_sLocalSocket[m];
                    break;
                }
            }
        }
        if(n == 8)//
        {
            int ntp = 0;
            memcpy(&ntp, &data[0], 4);
            int nYst = 0;
            memcpy(&nYst, &data[4], 4);
            //OutputDebug("recv 0x%X   %s:%d    %d",ntp,inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),n);
            if(nYst == m_nYst)
            {
                if(ntp == JVN_CMD_NEW_TRYTOUCH)//第一个包
                {
                    ntp = JVN_CMD_B_HANDSHAKE;
                    memcpy(&data[0],&ntp,4);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    
                    //					OutputDebug("recv 0x%X   %s:%d ",ntp,inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                    //					OutputDebug("recv 第一个包");
                }
                else if(ntp == JVN_CMD_A_HANDSHAKE)//主控发第二个包
                {
                    ntp = JVN_CMD_AB_HANDSHAKE;
                    memcpy(&data[0],&ntp,4);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    //					OutputDebug("recv 主控发第二个包");
                }
                else if(ntp == JVN_CMD_AB_HANDSHAKE)//可以连接了
                {
                    ntp = JVN_CMD_AB_HANDSHAKE;
                    memcpy(&data[0],&ntp,4);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    
                    CONNECT_INFO info = {0};
                    info.sSocket = s;
                    memcpy(&info.addrRemote,&addr,sizeof(sockaddr));
                    
                    info.nChannel = m_nChannel;
                    info.nTimeout = 1500;
                    
                    info.pChannel = m_pParent;
                    
                    AddConnect(info);
                    //					OutputDebug("新打洞成功 可以连接.\n\n\n");
                }
                else//未知类型 发送探测包
                {
                    ntp = JVN_CMD_NEW_TRYTOUCH;
                    memcpy(&data[0],&ntp,4);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&addr, sizeof(SOCKADDR_IN), 1);
                    //					OutputDebug("recv 未知类型 发送探测包");
                }
            }
        }
        else if(n > 0)
        {
            int nType = 0;
            memcpy(&nType,&data[0],4);
            int nYst = 0;
            memcpy(&nYst,&data[4],4);
            //OutputDebug("recv 0x%X YST = %d  %s:%d %d",nType,nYst,inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),n);
            if(JVN_REQ_CONNA == nType && nYst == m_nYst)//服务器返回地址
            {
                BOOL bTryLocal = data[40];
                
                nType = JVN_CMD_NEW_TRYTOUCH;
                memcpy(&data[0],&nType,4);
                SOCKADDR_IN add = {0},addl = {0};
                memcpy(&add, &data[8], sizeof(SOCKADDR_IN));
                memcpy(&addl, &data[8+sizeof(SOCKADDR_IN)], sizeof(SOCKADDR_IN));
                
                add.sin_family = AF_INET;
                addl.sin_family = AF_INET;
                CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&add, sizeof(SOCKADDR_IN), 1);
                CCChannel::sendtoclientm(s, (char*)data, 8, 0, (sockaddr *)&add, sizeof(SOCKADDR_IN), 1);
                
                //				OutputDebug("发送探测包 %s : %d",inet_ntoa(add.sin_addr), ntohs(add.sin_port));
                
                //添加服务器返回的主控地址
                if(bTryLocal)
                {
                    CONNECT_INFO info = {0};
                    info.sSocket = s;
                    memcpy(&info.addrRemote,&addl,sizeof(sockaddr));
                    
                    info.nChannel = m_nChannel;
                    info.nTimeout = 1000;
                    
                    info.pChannel = m_pParent;
                    
                    AddConnect(info);
                }
                CONNECT_INFO info = {0};
                info.sSocket = s;
                memcpy(&info.addrRemote,&add,sizeof(sockaddr));
                
                info.nChannel = m_nChannel;
                info.nTimeout = 1500;
                
                info.pChannel = m_pParent;
                
                AddConnect(info);
            }
            else if(JVN_REQ_NAT == nType && nYst == m_nYst)//服务器返回地址
            {
                ;
            }
        }
        CCWorker::jvc_sleep(10);
        {
            CLocker lock(m_Connect,__FILE__,__LINE__);
            bOK = m_bConnectOK;
        }
    }
    return 2;
}

#ifndef WIN32
void* CMakeHoleC::ConnectThread(void* pParam)
#else
UINT WINAPI CMakeHoleC::ConnectThread(LPVOID pParam)
#endif
{
    CMakeHoleC* pMakeHole = (CMakeHoleC* )pParam;
    
    BOOL bConnect = FALSE;
    DWORD dwTime = CCWorker::JVGetTime();
    DWORD dwEnd = CCWorker::JVGetTime();
    int nMaxTimeOut = 10 * 1000;//20s
    BOOL bIsMobile = FALSE;
    if(!pMakeHole->m_bVirtual)
    {
        CCChannel* pChannel = (CCChannel* )pMakeHole->m_pParent;
        if(pChannel->m_stConnInfo.nConnectType == TYPE_MO_UDP
           || pChannel->m_stConnInfo.nConnectType == TYPE_3GMO_UDP
           || pChannel->m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)
        {
            nMaxTimeOut = 5 * 1000;
            bIsMobile = TRUE;
        }
    }
    
    int nTimes = 1;
    UDTSOCKET uSocket = 0;
    while(dwEnd < dwTime + nMaxTimeOut && !bConnect)
    {
        dwEnd = CCWorker::JVGetTime();
        CONNECT_INFO pkg = {0};
        BOOL bFind = FALSE;
        {
            CLocker lock(pMakeHole->m_Connect,__FILE__,__LINE__);
            
            //尝试 CONNECT_LIST 列表，每个尝试连接
            //			OutputDebug("num = %d",pMakeHole->m_UdtConnectList.size());
            for(::std::vector<CONNECT_INFO >::iterator i = pMakeHole->m_UdtConnectList.begin(); i != pMakeHole->m_UdtConnectList.end();i ++)
            {
                if(!i->bConnected)
                {
                    i->bConnected = TRUE;
                    
#ifdef WIN32
                    memcpy(&pkg,i,sizeof(CONNECT_INFO));
#else
                    memcpy(&pkg.addrRemote,&i->addrRemote,sizeof(pkg.addrRemote));
                    pkg.bConnected = i->bConnected;
                    pkg.nChannel = i->nChannel;
                    pkg.nTimeout = i->nTimeout;
                    pkg.pChannel = i->pChannel;
                    pkg.sSocket = i->sSocket;
                    pkg.bIsHole = i->bIsHole;
#endif
                    
                    bFind = TRUE;
                    
                    break;//找到一个可以连接的
                }
            }
        }
        if(bFind)
        {
            //			if(pkg.bIsHole)
            //				OutputDebug("...........Connect .%d   %s : %d",nTimes,inet_ntoa(pkg.addrRemote.sin_addr),ntohs(pkg.addrRemote.sin_port));
            nTimes ++;
            if(pMakeHole->ConnectA(pkg.sSocket,&pkg.addrRemote,pkg.nTimeout,uSocket))
            {
                //						OutputDebug("Connect sucess");
                if(pMakeHole->m_bVirtual)
                {
                    CCVirtualChannel* pChannel = (CCVirtualChannel* )pkg.pChannel;
                    pChannel->ConnectStatus(pkg.sSocket,&pkg.addrRemote,pkg.nTimeout,FALSE,uSocket);
                }
                else
                {
                    CCChannel* pChannel = (CCChannel* )pkg.pChannel;
                    pChannel->ConnectStatus(pkg.sSocket,&pkg.addrRemote,pkg.nTimeout,FALSE,uSocket);
                }
                bConnect = TRUE;
                pMakeHole->m_bConnectOK = TRUE;
                //连接成功 通知channel直接继续向下进行
                if(pMakeHole->m_sWorkSocket != pkg.sSocket)
                    pMakeHole->m_sLocalSocket2 = pkg.sSocket;
            }
        }
        CCWorker::jvc_sleep(10);
        
    }
    //没有连接成功，重新查找下列表中有没有收到打洞包的，有就连接一下
    if(!bConnect && !bIsMobile)
    {
        CONNECT_INFO pkg = {0};
        BOOL bFind = FALSE;
        {
            CLocker lock(pMakeHole->m_Connect,__FILE__,__LINE__);
            
            for(::std::vector<CONNECT_INFO >::iterator i = pMakeHole->m_UdtConnectList.begin(); i != pMakeHole->m_UdtConnectList.end();i ++)
            {
                if(!i->bConnected && i->bIsHole)
                {
                    i->bConnected = TRUE;
                    
#ifdef WIN32
                    memcpy(&pkg,i,sizeof(CONNECT_INFO));
#else
                    memcpy(&pkg.addrRemote,&i->addrRemote,sizeof(pkg.addrRemote));
                    pkg.bConnected = i->bConnected;
                    pkg.nChannel = i->nChannel;
                    pkg.nTimeout = i->nTimeout;
                    pkg.pChannel = i->pChannel;
                    pkg.sSocket = i->sSocket;
                    pkg.bIsHole = i->bIsHole;
#endif
                    
                    bFind = TRUE;
                    
                    break;//找到一个可以连接的
                }
            }
        }
        if(bFind)
        {
            //			OutputDebug("最后连接一下...........Connect .%d   %s : %d",nTimes,inet_ntoa(pkg.addrRemote.sin_addr),ntohs(pkg.addrRemote.sin_port));
            nTimes ++;
            if(pMakeHole->ConnectA(pkg.sSocket,&pkg.addrRemote,1500,uSocket))
            {
                //						OutputDebug("Connect sucess");
                if(pMakeHole->m_bVirtual)
                {
                    CCVirtualChannel* pChannel = (CCVirtualChannel* )pkg.pChannel;
                    pChannel->ConnectStatus(pkg.sSocket,&pkg.addrRemote,pkg.nTimeout,FALSE,uSocket);
                }
                else
                {
                    CCChannel* pChannel = (CCChannel* )pkg.pChannel;
                    pChannel->ConnectStatus(pkg.sSocket,&pkg.addrRemote,pkg.nTimeout,FALSE,uSocket);
                }
                bConnect = TRUE;
                pMakeHole->m_bConnectOK = TRUE;
                //连接成功 通知channel直接继续向下进行
                if(pMakeHole->m_sWorkSocket != pkg.sSocket)
                    pMakeHole->m_sLocalSocket2 = pkg.sSocket;
            }
        }
    }
    //	OutputDebug("ConnectThread End  .........%10d\n\n\n",CCWorker::JVGetTime() - dwTime);
    if(!bConnect)//没有连接成功 全部关闭套接字
    {
        if(pMakeHole->m_bVirtual)
        {
            CCVirtualChannel* pChannel = (CCVirtualChannel* )pMakeHole->m_pParent;
            pChannel->ConnectStatus(0,NULL,0,TRUE,0);
        }
        else
        {
            CCChannel* pChannel = (CCChannel* )pMakeHole->m_pParent;
            pChannel->ConnectStatus(0,NULL,0,TRUE,0);
        }
        
        for(int k = 0;k < MAX_NUM;k ++)
        {
            if(pMakeHole->m_sLocalSocket[k] != 0)
            {
                closesocket(pMakeHole->m_sLocalSocket[k]);
                pMakeHole->m_sLocalSocket[k] = 0;
            }
        }
    }
    else//
    {
        for(int k = 0;k < MAX_NUM;k ++)
        {
            if(pMakeHole->m_sLocalSocket[k] != 0 && pMakeHole->m_sLocalSocket[k] != pMakeHole->m_sLocalSocket2)
            {
                closesocket(pMakeHole->m_sLocalSocket[k]);
                pMakeHole->m_sLocalSocket[k] = 0;
            }
        }
    }
    return 0;
}


#ifndef WIN32
void* CMakeHoleC::ConnectRemoteProc(void* pParam)
#else
UINT WINAPI CMakeHoleC::ConnectRemoteProc(LPVOID pParam)
#endif
{
    CMakeHoleC* pMakeHole = (CMakeHoleC* )pParam;
    //#ifdef WIN32
    //	OutputDebugString("开始连接....\n");
    //#else
    //	printf("开始连接....\n");
    //#endif
    DWORD dwStartTime = CCWorker::JVGetTime();
    //	char str[1200] = {0};
    {
        //1 获取外部的NAT地址 2 根据获取的地址请求服务器连接 3 服务器返回主控地址通知连接线程 4 打洞（连接线程在并行进行）
        pMakeHole->GetLocalNat(500);//本机外网地址 //m_sLocalSocket
        
        int SVer = 0;
        pMakeHole->GetNATADDR();//合并收到的地址
        
        pMakeHole->GetAddress();
    }
    /*
     
     {
     for(int m = 0;m < MAX_NUM;m ++)
     {
     if(pMakeHole->m_sLocalSocket[m] > 0 && pMakeHole->m_sLocalSocket2 != pMakeHole->m_sLocalSocket[m])
     {
     closesocket(pMakeHole->m_sLocalSocket[m]);
     pMakeHole->m_sLocalSocket[m] = 0;
     }
     
     }
     }
     */
    //	OutputDebug("Thread end.=======================Cost Time %d\n\n\n\n",CCWorker::JVGetTime() - dwStartTime);
    return 0;
}

/////////////////////////////////////////////////////////////////////
CMakeGroup::CMakeGroup()
{
    m_nNatTypeB = 0;
    
    m_ServerList.clear();
    
#ifndef WIN32
    m_hCheckThread = 0;
#else
    m_hCheckEndEventC = 0;
#endif
    
#ifndef WIN32
    pthread_mutex_init(&m_MakeLock, NULL);
#else
    m_MakeLock = CreateMutex(NULL, false, NULL);
#endif
    
}

CMakeGroup::~CMakeGroup()
{
    CLocker lock(m_MakeLock,__FILE__,__LINE__);
    for(CONN_List::iterator k = m_ConnectList.begin(); k != m_ConnectList.end(); ++ k)
    {
        CMakeHoleC* pMake = (CMakeHoleC* )k->second;
        delete pMake;
    }
    m_ConnectList.clear();
    
#ifndef WIN32
    if (0 != m_hCheckThread)
    {
        m_bCheckEndC = TRUE;
        pthread_join(m_hCheckThread, NULL);
        m_hCheckThread = 0;
    }
    pthread_mutex_destroy(&m_MakeLock);
#else
    if(m_hCheckEndEventC>0)
    {
        SetEvent(m_hCheckEndEventC);
        m_hCheckEndEventC = 0;
    }
    CloseHandle(m_MakeLock);
#endif
    
}




void CMakeGroup::Start(CCWorker* pWorker)
{
    m_pWorker = pWorker;
    
#ifndef WIN32
    m_bCheckEndC = FALSE;
    
    pthread_mutex_init(&m_ctCheck, NULL); //初始化临界区
#else
    
    m_hCheckStartEventC = 0;
    m_hCheckEndEventC = 0;
    
    InitializeCriticalSection(&m_ctCheck); //初始化临界区
#endif
    //NAT检测线程启动
    {
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
        pthread_create(&m_hCheckThread, pAttr, CheckNatProc, this);
#else
        //开启连接线程
        UINT unTheadID;
        m_hCheckStartEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hCheckEndEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hCheckThread = (HANDLE)_beginthreadex(NULL, 0, CheckNatProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hCheckStartEventC);
        if (m_hCheckThread == 0)//创建线程失败
        {
            //清理Conn线程
            if(m_hCheckStartEventC > 0)
            {
                CloseHandle(m_hCheckStartEventC);
                m_hCheckStartEventC = 0;
            }
            
            if(m_hCheckEndEventC > 0)
            {
                CloseHandle(m_hCheckEndEventC);
                m_hCheckEndEventC = 0;
            }
        }
#endif
    }
}

BOOL CMakeGroup::CheckConnect(char *strGroup,int nYstNo)
{
    char strYST[30] = {0};
    sprintf(strYST,"%s%d",strGroup,nYstNo);
    
    CLocker lock(m_MakeLock,__FILE__,__LINE__);
    CONN_List ::iterator i = m_ConnectList.find(std::string(strYST));
    
    //查找列表中是否已经存在
    if(i != m_ConnectList.end())
    {
        return TRUE;
    }
    
    return FALSE;
}

BOOL CMakeGroup::AddConnect(void* pChannel,char *strGroup,int nYstNo,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatTypeA,int nUpnp,BOOL bVirtual)
{
    CLocker lock(m_MakeLock,__FILE__,__LINE__);
    
    char strYST[50] = {0};
    sprintf(strYST,"%s%d",strGroup,nYstNo);
    CONN_List ::iterator i = m_ConnectList.find(std::string(strYST));
    
    //查找列表中是否已经存在
    if(i != m_ConnectList.end())
    {
        CMakeHoleC* pMake = (CMakeHoleC* )i->second;
        if(pMake && pMake->m_nConnectNum <= 0)
        {
            for(int m = 0;m < MAX_NUM;m ++)
            {
                closesocket(pMake->m_sLocalSocket[m]);
                pMake->m_sLocalSocket[m] = 0;
            }
        }
        if(pMake && pMake->m_sLocalSocket2 <= 0)
        {
            delete pMake;
            m_ConnectList.erase(strYST);
            //			OutputDebug("清理过期的...");
        }
        else
        {
            //			OutputDebug("正在连接...\n\n\n");
            return FALSE;
        }
        
    }
    CMakeHoleC* pMake = new CMakeHoleC(strGroup,nYstNo,nChannel,bLocalTry,ServerList,nNatTypeA,m_nNatTypeB,nUpnp,pChannel,bVirtual);
    pMake->m_sWorkSocket = m_pWorker->m_WorkerUDPSocket;
    pMake->m_pWorker = m_pWorker;
    m_ConnectList[std::string(strYST)] = pMake;
    //	OutputDebug("Add connect... %s\n\n\n",strYST);
    
    return TRUE;
}

BOOL CMakeGroup::IsThisLocal(SOCKADDR_IN addr)//地址是否在本地地址列表中
{
    if(m_LocalIPList.size() <= 0)
    {
        m_pWorker->GetLocalIP(&m_LocalIPList);
    }
    char strNatIP[30] = {0};
    strcpy(strNatIP,inet_ntoa(addr.sin_addr));
    int nS = m_LocalIPList.size();
    char strLocalIp[30] = {0};
    for(int i = 0;i < nS;i ++)
    {
        NAT nat = m_LocalIPList[i];
        sprintf(strLocalIp,"%d.%d.%d.%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3]);
#ifdef WIN32
        if(stricmp(strLocalIp,strNatIP) == 0)
#else
            if(strcasecmp(strLocalIp,strNatIP) == 0)
#endif
                return TRUE;
    }
    return FALSE;
}

int CMakeGroup::GetNatType(char* strGroup)
{
    if(m_ServerList.size() <= 0)
    {
        if(!m_pWorker->GetSerList(strGroup, m_ServerList, 1, 0))
        {
            return 0;
        }
    }
    
    //创建套接字
    
    SOCKET sNatCheckSocket = socket(AF_INET, SOCK_DGRAM,0);
    SOCKADDR_IN addrL;
#ifndef WIN32
    addrL.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrL.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrL.sin_family = AF_INET;
    addrL.sin_port = htons(0);
    //	OutputDebug("SOCKET = %d\n",sNatCheckSocket);
    //绑定套接字
    int err = bind(sNatCheckSocket, (SOCKADDR *)&addrL, sizeof(SOCKADDR));
    if(err != 0)
    {
        closesocket(sNatCheckSocket);
        sNatCheckSocket = 0;
        return 0;
    }
    
    int n = m_ServerList.size();
    
    char strSend[1024] = {0};
    char strRecv[1024] = {0};
    
    SOCKADDR_IN natAddr = {0};
    BOOL bRet = FALSE;
    SOCKADDR_IN netAX = {0},netBX = {0};
    SOCKADDR_IN netRetAX = {0};
    for(int i = 0;i < n - 1;i ++)//对于每一个服务器循环
    {
        memcpy(&netAX,&m_ServerList[i].addr,sizeof(SOCKADDR_IN));
        memcpy(&netBX,&m_ServerList[i + 1].addr,sizeof(SOCKADDR_IN));
        
        netAX.sin_port = htons(ntohs(netAX.sin_port) + 10000);//在原来基础上+ 10000
        netBX.sin_port = htons(ntohs(netBX.sin_port) + 10001);
        
        int nType = CHECK_TYPE_AX_AX;
        DWORD nRandom = CCWorker::JVGetTime();//尽量唯一
        
        memcpy(&strSend[0],&nType,4);
        memcpy(&strSend[4],&nRandom,4);
        
        /*		//临时测试 正式删除
         
         #ifndef WIN32
         netAX.sin_addr.s_addr = inet_addr("58.56.109.125");
         #else
         netAX.sin_addr.S_un.S_addr = inet_addr("58.56.109.125");
         #endif
         //netAX.sin_addr.S_un.S_addr = inet_addr("58.56.19.182");
         
         
         #ifndef WIN32
         netBX.sin_addr.s_addr = inet_addr("58.56.109.123");
         #else
         netBX.sin_addr.S_un.S_addr = inet_addr("58.56.109.123");
         #endif
         
         netAX.sin_family = AF_INET;
         netAX.sin_port = htons(ntohs(netAX.sin_port) + 10000);
         netBX.sin_family = AF_INET;
         netBX.sin_port = htons(ntohs(netBX.sin_port) + 10001);
         */
        int dd = CCChannel::sendtoclientm(sNatCheckSocket, strSend, 8, 0, (sockaddr *)&netAX, sizeof(SOCKADDR_IN),1);
        
        //OutputDebug("query CHECK_TYPE_AX_AX %s:%d",inet_ntoa(netAX.sin_addr), ntohs(netAX.sin_port));
        DWORD d = CCWorker::JVGetTime();
        DWORD dwSt = d;
        //判断公网 AX_AX
        while(d < dwSt + 1500)
        {
            //OutputDebug("Wait for data...");
            d = CCWorker::JVGetTime();
            int addrlen = sizeof(SOCKADDR_IN);
            int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
            if(nNatLen > 0)
            {
                int nT = 0;
                int nR = 0;
                memcpy(&nT,&strRecv[0],4);
                memcpy(&nR,&strRecv[4],4);
                
                if(nT == nType && nR == nRandom)
                {
                    bRet = TRUE;
                    memcpy(&netRetAX,&strRecv[8],sizeof(SOCKADDR_IN));
                    
                    if(IsThisLocal(netRetAX))
                    {
                        closesocket(sNatCheckSocket);
                        sNatCheckSocket = 0;
                        //						OutputDebug("本地网络类型 公网.");
                        m_nNatTypeB = NAT_TYPE_0_PUBLIC_IP;
                        return 1;//公网
                    }
                    break;
                }
            }
            CCWorker::jvc_sleep(10);
        }
        if(!bRet)
        {
            //			OutputDebug("No return.\n\n\n");
            continue;
        }
        //		OutputDebug("CHECK_TYPE_AX_AX  = %s:%d",inet_ntoa(netRetAX.sin_addr), ntohs(netRetAX.sin_port));
        
        nType = CHECK_TYPE_AX_BY;
        memcpy(&strSend[0],&nType,4);
        memcpy(&strSend[4],&nRandom,4);
        memcpy(&strSend[8],&netBX,sizeof(SOCKADDR_IN));
        
        //判断透明NAT AX_BY
        dd = CCChannel::sendtoclientm(sNatCheckSocket, strSend, 8 + sizeof(SOCKADDR_IN), 0, (sockaddr *)&netAX, sizeof(SOCKADDR_IN),1);
        
        //OutputDebug("query CHECK_TYPE_AX_BY %s:%d",inet_ntoa(netAX.sin_addr), ntohs(netAX.sin_port));
        d = CCWorker::JVGetTime();
        dwSt = d;
        bRet = FALSE;
        while(d < dwSt + 1000)//
        {
            d = CCWorker::JVGetTime();
            int addrlen = sizeof(SOCKADDR);
            int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
            if(nNatLen > 0)
            {
                int nT = 0;
                int nR = 0;
                memcpy(&nT,&strRecv[0],4);
                memcpy(&nR,&strRecv[4],4);
                
                if(nT == nType && nR == nRandom)
                {
                    nType = CHECK_TYPE_BX_BX;
                    memcpy(&strSend[0],&nType,4);
                    memcpy(&strSend[4],&nRandom,4);
                    //判断透明 BX_BX  判断2个返回的NAT是否一致
                    dd = CCChannel::sendtoclientm(sNatCheckSocket, strSend, 8, 0, (sockaddr *)&netBX, sizeof(SOCKADDR_IN),1);
                    
                    //AddLog(&pDlg->m_edLog,"query CHECK_TYPE_BX_BX");
                    d = CCWorker::JVGetTime();
                    dwSt = d;
                    bRet = FALSE;
                    while(d < dwSt + 1000)//
                    {
                        d = CCWorker::JVGetTime();
                        int addrlen = sizeof(SOCKADDR);
                        int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
                        if(nNatLen > 0)
                        {
                            int nT = 0;
                            DWORD nR = 0;
                            memcpy(&nT,&strRecv[0],4);
                            memcpy(&nR,&strRecv[4],4);
                            
                            if(nT == nType && nR == nRandom)
                            {
                                bRet = TRUE;
                                SOCKADDR_IN netRetBX = {0};
                                memcpy(&netRetBX,&strRecv[8],sizeof(SOCKADDR));
                                
                                //AddLog(&pDlg->m_edLog,"CHECK_TYPE_BX_BX  = %s:%d",inet_ntoa(netRetBX.sin_addr), ntohs(netRetBX.sin_port));
                                
                                if(netRetBX.sin_port == netRetAX.sin_port)
                                {
                                    closesocket(sNatCheckSocket);
                                    sNatCheckSocket = 0;
                                    //									OutputDebug("本地网络类型 透明NAT.");
                                    m_nNatTypeB = NAT_TYPE_1_FULL_CONE;
                                    return 2;//透明NAT
                                }
                            }
                            break;
                        }
                        CCWorker::jvc_sleep(10);
                    }
                    
                    break;
                }
            }
            CCWorker::jvc_sleep(10);
        }
        
        nType = CHECK_TYPE_BX_BX;
        memcpy(&strSend[0],&nType,4);
        memcpy(&strSend[4],&nRandom,4);
        //判断对称 BX_BX
        dd = CCChannel::sendtoclientm(sNatCheckSocket, strSend, 8, 0, (sockaddr *)&netBX, sizeof(SOCKADDR_IN),1);
        
        //		OutputDebug("query CHECK_TYPE_BX_BX %s:%d",inet_ntoa(netBX.sin_addr), ntohs(netBX.sin_port));
        
        d = CCWorker::JVGetTime();
        dwSt = d;
        bRet = FALSE;
        while(d < dwSt + 1000)//
        {
            d = CCWorker::JVGetTime();
            int addrlen = sizeof(SOCKADDR);
            int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
            if(nNatLen > 0)
            {
                int nT = 0;
                int nR = 0;
                memcpy(&nT,&strRecv[0],4);
                memcpy(&nR,&strRecv[4],4);
                
                if(nT == nType && nR == nRandom)
                {
                    bRet = TRUE;
                    SOCKADDR_IN netRetBX = {0};
                    memcpy(&netRetBX,&strRecv[8],sizeof(SOCKADDR));
                    
                    if(netRetBX.sin_port != netRetAX.sin_port)
                    {
                        closesocket(sNatCheckSocket);
                        sNatCheckSocket = 0;
                        //						OutputDebug("本地网络类型 对称NAT.");
                        m_nNatTypeB = NAT_TYPE_4_SYMMETRIC;
                        return 5;//对称NAT
                    }
                    
                    break;
                }
            }
            CCWorker::jvc_sleep(10);
        }
        if(!bRet)
            continue;
        
        nType = CHECK_TYPE_BX_BY;
        memcpy(&strSend[0],&nType,4);
        memcpy(&strSend[4],&nRandom,4);
        //判断IP还是端口NAT BX_BY
        dd = CCChannel::sendtoclientm(sNatCheckSocket, strSend, 8, 0, (sockaddr *)&netBX, sizeof(SOCKADDR_IN),1);
        
        //OutputDebug("query CHECK_TYPE_BX_BY %s:%d",inet_ntoa(netBX.sin_addr), ntohs(netBX.sin_port));
        
        d = CCWorker::JVGetTime();
        dwSt = d;
        bRet = FALSE;
        while(d < dwSt + 1000)//
        {
            d = CCWorker::JVGetTime();
            int addrlen = sizeof(SOCKADDR);
            int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
            if(nNatLen > 0)
            {
                int nT = 0;
                int nR = 0;
                memcpy(&nT,&strRecv[0],4);
                memcpy(&nR,&strRecv[4],4);
                
                if(nT == nType && nR == nRandom)
                {
                    bRet = TRUE;
                    
                    break;
                }
            }
            CCWorker::jvc_sleep(10);
        }
        if(bRet)
        {
            closesocket(sNatCheckSocket);
            sNatCheckSocket = 0;
            //	OutputDebug("本地网络类型 ip 受限 NAT.");
            m_nNatTypeB = NAT_TYPE_2_IP_CONE;
            return 3;//ip NAT
            
        }
        else
        {
            closesocket(sNatCheckSocket);
            sNatCheckSocket = 0;
            //	OutputDebug("本地网络类型 端口受限NAT.");
            m_nNatTypeB = NAT_TYPE_3_PORT_CONE;
            return 4;//port NAT
        }
    }
    
    if(sNatCheckSocket > 0)
    {
        closesocket(sNatCheckSocket);
        sNatCheckSocket = 0;
    }
    
    //	OutputDebug("%s : %d",__FILE__,__LINE__);
    
    return  0;
}

#ifndef WIN32
void* CMakeGroup::CheckNatProc(void* pParam)
#else
UINT WINAPI CMakeGroup::CheckNatProc(LPVOID pParam)
#endif
{
    CMakeGroup* pMakeHole = (CMakeGroup* )pParam;
    
    unsigned long dwStart = 0;//保证一运行该函数就先查询
    unsigned long dwEnd = CCWorker::JVGetTime();
    while(TRUE)
    {
#ifndef WIN32
        if(pMakeHole->m_bCheckEndC)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pMakeHole->m_hCheckEndEventC, 0))
        {
            if(pMakeHole->m_hCheckEndEventC > 0)
            {
                CloseHandle(pMakeHole->m_hCheckEndEventC);
                pMakeHole->m_hCheckEndEventC = 0;
            }
            break;
        }
#endif
        dwEnd = CCWorker::JVGetTime();
        if(dwEnd > dwStart + 3 * 60 * 1000)//三分钟查询一次，查询不出来时默认上次成功取得的类型
        {
            pMakeHole->GetNatType("A");
            dwStart = dwEnd;
        }
        
        CCWorker::jvc_sleep(100);
    }
    return 0;
}

void CMakeGroup::SetConnect(SOCKET s, int nType)
{
    //	OutputDebug("SOCKET = %d    %d",s,nType);
    if(nType == 1 || nType == -1)
    {
        CLocker lock(m_MakeLock,__FILE__,__LINE__);
        for(CONN_List::iterator k = m_ConnectList.begin(); k != m_ConnectList.end(); ++ k)
        {
            CMakeHoleC* pMake = (CMakeHoleC* )k->second;
            
            if(pMake->m_sLocalSocket2 == s && pMake->m_sLocalSocket != 0)
            {
                if(nType == 1)
                {
                    pMake->m_nConnectNum ++;
                }
                else if(nType == -1)
                {
                    pMake->m_nConnectNum --;
                }
                //				OutputDebug("SOCKET = %d    %d",s,pMake->m_nConnectNum);
                if(pMake->m_nConnectNum <= 0)
                {
                    char strYST[30] = {0};
                    sprintf(strYST,"%s%d",pMake->m_strGroup,pMake->m_nYst);
                    
                    for(int m = 0;m < MAX_NUM;m ++)
                    {
                        if(pMake->m_sLocalSocket[m] > 0)
                        {
                            closesocket(pMake->m_sLocalSocket[m]);
                            pMake->m_sLocalSocket[m] = 0;
                        }
                        
                    }
                    delete pMake;
                    m_ConnectList.erase(strYST);
                }
                
                break;
            }
        }
    }
}

BOOL CMakeHoleC::ConnectA(SOCKET s, SOCKADDR_IN *remote,int nTimeOut,UDTSOCKET& uSocket)
{
    uSocket = 0;
    UDTSOCKET uServerSocket = 0;
    uServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    
    BOOL bReuse = TRUE;
    UDT::setsockopt(uServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
    UDT::setsockopt(uServerSocket, 0, UDT_MSS, &len1, sizeof(int));
    //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
    len1=1500*1024;
    UDT::setsockopt(uServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
    
    len1=1000*1024;
    UDT::setsockopt(uServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
    if (UDT::ERROR == UDT::bind(uServerSocket, s))
    {OutputDebug("MakeHoleC  line: %d",__LINE__);
        if(uServerSocket > 0)
        {
            UDT::close(uServerSocket);
        }
        uServerSocket = 0;
        
        return FALSE;
    }
    
    //将套接字置为非阻塞模式
    BOOL block = FALSE;
    UDT::setsockopt(uServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
    UDT::setsockopt(uServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
    LINGER linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    UDT::setsockopt(uServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
    
    STJUDTCONN stcon;
    stcon.u = uServerSocket;
    stcon.name = (SOCKADDR *)remote;
    stcon.namelen = sizeof(SOCKADDR);
    stcon.nChannelID = m_nChannel;
    stcon.nLVer_new = JVN_YSTVER;
    stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
    stcon.nMinTime = nTimeOut;
    
    stcon.uchLLTCP = 0;
    if(m_bVirtual)
    {
        stcon.nChannelID = -2;//m_stConnInfo.nChannel;
        stcon.nCheckYSTNO = m_nYst;
        memcpy(stcon.chCheckGroup, m_strGroup, 4);
        stcon.uchVirtual = 1;
        stcon.nVChannelID = m_nChannel;
    }
    
    SOCKADDR_IN srv = {0};
    char strServer[100] = {0};
    memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
    
    sprintf(strServer,"============== connecting a %s:%d  %d  uServerSocket = %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__,uServerSocket);
    //	OutputDebug(strServer);
    
    if(UDT::ERROR == UDT::connect(stcon))//2500
    {
        if(uServerSocket > 0)
        {
            UDT::close(uServerSocket);
        }
        uServerSocket = 0;
        
        return FALSE;
    }
    else
    {
        //OutputDebug("connect ip ok.\n\n\n\n\n\n\n");
        //UDT::close(uServerSocket);
        uSocket = uServerSocket;
        
        CreatUDT(s);
        return TRUE;
    }
}

void CMakeHoleC::CreatUDT(SOCKET s)
{
    //UDT:创建本地UDT套接字,绑定，监听,
    m_udtSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    
    BOOL bReuse = TRUE;
    UDT::setsockopt(m_udtSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
    UDT::setsockopt(m_udtSocket, 0, UDT_MSS, &len1, sizeof(int));
    //////////////////////////////////////////////////////////////////////////
    
#ifdef MOBILE_CLIENT
    len1=1500*1024;
    UDT::setsockopt(m_udtSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
    
    len1=1000*1024;
    UDT::setsockopt(m_udtSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
    
    
    int nret = 0;
    
    nret = UDT::bind(m_udtSocket,s);
    if(UDT::ERROR == nret)
    {
        if(m_udtSocket > 0)
        {
            UDT::close(m_udtSocket);
        }
        m_udtSocket = 0;
        
        return ;
    }
    
    //////////////////////////////////////////////////////////////////////////
    //将套接字置为非阻塞模式
    BOOL block = FALSE;
    UDT::setsockopt(m_udtSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
    UDT::setsockopt(m_udtSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
    LINGER linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    UDT::setsockopt(m_udtSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
    
    if (UDT::ERROR == UDT::listen(m_udtSocket, 30))
    {
        if(m_udtSocket > 0)
        {
            UDT::close(m_udtSocket);
        }
        m_udtSocket = 0;
        
        return;
    }
}
