// CWorker.cpp: implementation of the CCWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "CWorker.h"

#include "SUpnpCtrl.h"
#include "JVN_DBG.h"
void *g_pThis = NULL;

#ifdef WIN32
#include ".\RTMP\CRtmpChannel.h"
#else
#include "CRtmpChannel.h"
#endif

int JVC_MSS = 20000;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern bool isWriteLog;
char g_chLocalPath[200] = {0};
void writeLog(char* format, ...)
{
//    return;
#ifdef MOBILE_CLIENT
  if (isWriteLog) 
	{
        char data[1024] = {0};
        char szData[1024] = {0};
        va_list va_args;
        va_start(va_args, format);
        
        size_t length = vsnprintf(NULL, 0,format, va_args);
        int result = vsnprintf(szData, length + 1, format, va_args);
        
        time_t now = time(0);
        tm *tnow = localtime(&now);
        
        sprintf(data,"%02d:%02d:%02d new %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
        
        //    printf(data);
        
        va_end(va_args);
        
        FILE *pfile = NULL;
        char strFileName[MAX_PATH] = {0};
        sprintf(strFileName,"%s/%s",g_chLocalPath,"temperrolog.txt");
        pfile = fopen(strFileName,"a+");
        if(pfile)
        {
            fwrite(data,sizeof(char),strlen(data),pfile);
            fclose(pfile);
        }
    }
#endif
    
}


void deleteLog()
{
    if (isWriteLog) {
        char strFileName[MAX_PATH] = {0};
        sprintf(strFileName,"%s/%s",g_chLocalPath,"temperrolog.txt");
        remove(strFileName);
    }
    
}


CCWorker::CCWorker()
{
    m_pHelpCtrl = NULL;
    m_pUpnpCtrl = NULL;
}
CCWorker::CCWorker(int nLocalStartPort)
{
    
    printf("constructor cworker!");
    m_pUpnpCtrl = NULL;
    m_pHelpCtrl = NULL;
    m_stVersion.sver1 = 2;
    m_stVersion.sver2 = 0;
    m_stVersion.sver3 = 0;
    m_stVersion.sver4 = 22;//73;//20131210.1//22;//20120618.1
    
    memset(m_chLocalPath, 0, MAX_PATH);
    memset(m_strDomainName,0,MAX_PATH);
    memset(m_strPathName,0,MAX_PATH);
    g_pThis = this;
    
    m_pLanSerch = NULL;
    m_pBC = NULL;
    m_pLanTool = NULL;
    m_dwLastLANToolTime = 0;
    
    m_dwLastTime = 0;
    m_GetStatus.clear();
    
    m_GroupList.clear();
    m_bNeedLog = FALSE;
    m_nLanguage = JVN_LANGUAGE_ENGLISH;
    
    UDT::startup();
    m_nLocalStartPort = nLocalStartPort;
    m_pChannels.clear();
    
    m_hPTListenThread = 0;
    m_hGTThread = 0;
    m_hNatGetThread = 0;
    
    m_nLocalPortWan = 0;
    m_LocalIPList.clear();
    
    m_nGroupCount = 23;
    for(int i=0; i<50; i++)
    {
        memset(m_chGroupList[i], 0, 4);
    }
	m_YstSvrList.clear();
	CYstSvrList list;

	strcpy(list.chGroup,"A");
	list.addrlist.clear();
	m_YstSvrList.push_back(list);

	strcpy(list.chGroup,"B");
	list.addrlist.clear();
	m_YstSvrList.push_back(list);

	strcpy(list.chGroup,"S");
	list.addrlist.clear();
	m_YstSvrList.push_back(list);
    
    strcpy(m_chGroupList[0],"A");
    strcpy(m_chGroupList[1],"B");
    strcpy(m_chGroupList[2],"S");
    strcpy(m_chGroupList[3],"H");
    strcpy(m_chGroupList[4],"C");
    strcpy(m_chGroupList[5],"V");
    strcpy(m_chGroupList[6],"D");
    strcpy(m_chGroupList[7],"SC");
    strcpy(m_chGroupList[8],"SD");
    strcpy(m_chGroupList[9],"SE");
    strcpy(m_chGroupList[10],"SF");
    strcpy(m_chGroupList[11],"SH");
    strcpy(m_chGroupList[12],"ST");
    strcpy(m_chGroupList[13],"SK");
    strcpy(m_chGroupList[14],"SL");
    strcpy(m_chGroupList[15],"SN");
    strcpy(m_chGroupList[16],"SP");
    strcpy(m_chGroupList[17],"SQ");
    strcpy(m_chGroupList[18],"SW");
    strcpy(m_chGroupList[19],"SY");
    strcpy(m_chGroupList[20],"SV");
    strcpy(m_chGroupList[21],"SG");
    strcpy(m_chGroupList[22],"N");


#ifndef WIN32
    printf("jvclient************%s\n",JVCLIENT_VERSION);
    
    m_bPTListenEnd = FALSE;
    m_bGTEnd = FALSE;
    m_bNatGetEnd = FALSE;
    
    pthread_mutex_init(&m_criticalsection, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctGroup, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctND, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctCC, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctPD, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctDL, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctYSTNO, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctclearsock, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctlocalip, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ReceiveLock, NULL); //≥??oa???Ωá?ˉ
#else
    InitializeCriticalSection(&m_criticalsection); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctGroup); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctND); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctCC); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctPD); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctDL); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctYSTNO); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctclearsock); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctlocalip); //≥??oa???Ωá?ˉ
    
    m_ReceiveLock = CreateMutex(NULL, false, NULL);
    
    //??Ω”?÷??
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1,1);
    WSAStartup(wVersionRequested, &wsaData);
    
#endif
#ifndef WIN32
	pthread_mutex_init(&m_RtmpLock, NULL);
#else
	m_RtmpLock = CreateMutex(NULL, false, NULL);
#endif
    GetLocalIPList();
    
    m_UDTSockTemps.clear();
    
    CreateShare();
    
    
}

CCWorker::CCWorker(int nLocalStartPort, char* pfWriteReadData)
{
    
    strcpy(g_chLocalPath, pfWriteReadData) ;
    
    
    m_pUpnpCtrl = NULL;
    m_pHelpCtrl = NULL;
    m_stVersion.sver1 = 2;
    m_stVersion.sver2 = 0;
    m_stVersion.sver3 = 0;
    m_stVersion.sver4 = 22;//73;//20131210.1//22;//20120618.1
    
    memset(m_chLocalPath, 0, MAX_PATH);
    memset(m_strDomainName,0,MAX_PATH);
    memset(m_strPathName,0,MAX_PATH);
    strcpy(m_chLocalPath, pfWriteReadData);
    g_pThis = this;
    
    m_pLanSerch = NULL;
    m_pBC = NULL;
    m_pLanTool = NULL;
    m_pBCSelf = NULL;
    m_dwLastLANToolTime = 0;
    
    m_dwLastTime = 0;
    m_GetStatus.clear();
    
    m_GroupList.clear();
    m_bNeedLog = FALSE;
    m_nLanguage = JVN_LANGUAGE_ENGLISH;
    
    UDT::startup();
    m_nLocalStartPort = nLocalStartPort;
    m_pChannels.clear();
    
    m_hPTListenThread = 0;
    m_hGTThread = 0;
    m_hNatGetThread = 0;
    
    m_nLocalPortWan = 0;
    m_LocalIPList.clear();
    
    m_nGroupCount = 23;
    for(int i=0; i<50; i++)
    {
        memset(m_chGroupList[i], 0, 4);
    }
    strcpy(m_chGroupList[0],"A");
    strcpy(m_chGroupList[1],"B");
    strcpy(m_chGroupList[2],"S");
    strcpy(m_chGroupList[3],"H");
    strcpy(m_chGroupList[4],"C");
    strcpy(m_chGroupList[5],"V");
    strcpy(m_chGroupList[6],"D");
    strcpy(m_chGroupList[7],"SC");
    strcpy(m_chGroupList[8],"SD");
    strcpy(m_chGroupList[9],"SE");
    strcpy(m_chGroupList[10],"SF");
    strcpy(m_chGroupList[11],"SH");
    strcpy(m_chGroupList[12],"ST");
    strcpy(m_chGroupList[13],"SK");
    strcpy(m_chGroupList[14],"SL");
    strcpy(m_chGroupList[15],"SN");
    strcpy(m_chGroupList[16],"SP");
    strcpy(m_chGroupList[17],"SQ");
    strcpy(m_chGroupList[18],"SW");
    strcpy(m_chGroupList[19],"SY");
    strcpy(m_chGroupList[20],"SV");
    strcpy(m_chGroupList[21],"SG");
    strcpy(m_chGroupList[22],"N");
    
#ifndef WIN32
    printf("jvclient************%s\n",JVCLIENT_VERSION);
    
    m_bPTListenEnd = FALSE;
    m_bGTEnd = FALSE;
    m_bNatGetEnd = FALSE;
    
    pthread_mutex_init(&m_criticalsection, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctGroup, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctND, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctCC, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctPD, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctDL, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctYSTNO, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctclearsock, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ctlocalip, NULL); //≥??oa???Ωá?ˉ
    pthread_mutex_init(&m_ReceiveLock, NULL); //≥??oa???Ωá?ˉ
#else
    InitializeCriticalSection(&m_criticalsection); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctGroup); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctND); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctCC); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctPD); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctDL); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctYSTNO); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctclearsock); //≥??oa???Ωá?ˉ
    InitializeCriticalSection(&m_ctlocalip); //≥??oa???Ωá?ˉ
    
    m_ReceiveLock = CreateMutex(NULL, false, NULL);
    
    //??Ω”?÷??
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1,1);
    WSAStartup(wVersionRequested, &wsaData);
    
#endif
#ifndef WIN32
	pthread_mutex_init(&m_RtmpLock, NULL);
#else
	m_RtmpLock = CreateMutex(NULL, false, NULL);
#endif
    GetLocalIPList();
    
    m_UDTSockTemps.clear();
    
    CreateShare();
    //    DWORD begin =  CCWorker::JVGetTime();
    //     printf("--------------------------------begin\n");
    //    int result = WANGetChannelCount("A",362,5);
    //    DWORD end = CCWorker::JVGetTime();
    //    printf("--------------------------------count result: %d, spendTime: %d\n",result,(end-begin));
}


CCWorker::~CCWorker()
{
    /*π?±’??≥?*/
#ifndef WIN32
    if (0 != m_hPTListenThread)
    {
        m_bPTListenEnd = TRUE;
        pthread_join(m_hPTListenThread, NULL);
        m_hPTListenThread = 0;
    }
    if (0 != m_hGTThread)
    {
        m_bGTEnd = TRUE;
        pthread_join(m_hGTThread, NULL);
        m_hGTThread = 0;
    }
    if (0 != m_hNatGetThread)
    {
        m_bNatGetEnd = TRUE;
        pthread_join(m_hNatGetThread, NULL);
        m_bNatGetEnd = 0;
    }
#else
    if(m_hPTListenEndEvent>0)
    {
        SetEvent(m_hPTListenEndEvent);
    }
    if(m_hGTEndEvent>0)
    {
        SetEvent(m_hGTEndEvent);
    }
    if(m_hNatGetEndEvent>0)
    {
        SetEvent(m_hNatGetEndEvent);
    }
    CCChannel::WaitThreadExit(m_hPTListenThread);
    CCChannel::WaitThreadExit(m_hGTThread);
    CCChannel::WaitThreadExit(m_hNatGetThread);
    if(m_hPTListenThread > 0)
    {
        CloseHandle(m_hPTListenThread);
        m_hPTListenThread = 0;
    }
    if(m_hGTThread > 0)
    {
        CloseHandle(m_hGTThread);
        m_hGTThread = 0;
    }
#endif
    
    /*π?±’à?”–?¨Ω”*/
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    int nCount = m_pChannels.size();
    int i=0;
    for(i=0; i<nCount; i++)
    {
        if(m_pChannels[i] != NULL)
        {
            m_pChannels[i]->DisConnect();
            delete m_pChannels[i];
            m_pChannels[i] = NULL;
        }
    }
    m_pChannels.clear();
    UDT::cleanup();
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    /*??∑≈à?”–???±??Ω”?÷*/
#ifndef WIN32
    pthread_mutex_lock(&m_ctclearsock);
#else
    EnterCriticalSection(&m_ctclearsock);
#endif
    nCount = m_UDTSockTemps.size();
    for(i=0; i<nCount; i++)
    {
        UDT::close(m_UDTSockTemps[i]);
    }
    m_UDTSockTemps.clear();
#ifndef WIN32
    pthread_mutex_unlock(&m_ctclearsock);
#else
    LeaveCriticalSection(&m_ctclearsock);
#endif
    
    if(m_pLanSerch != NULL)
    {
        delete m_pLanSerch;
        m_pLanSerch = NULL;
    }
    if(m_pBC != NULL)
    {
        delete m_pBC;
        m_pBC = NULL;
    }
    if(m_pLanTool != NULL)
    {
        delete m_pLanTool;
        m_pLanTool = NULL;
    }
    
#ifndef WIN32
    pthread_mutex_destroy(&m_criticalsection); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctGroup); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctND); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctCC); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctPD); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctDL); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctYSTNO); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctclearsock); //≥??oa???Ωá?ˉ
    pthread_mutex_destroy(&m_ctlocalip); //≥??oa???Ωá?ˉ
#else
    WSACleanup();
    DeleteCriticalSection(&m_criticalsection); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctGroup); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctND); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctCC); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctPD); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctDL); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctYSTNO); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctclearsock); //≥??oa???Ωá?ˉ
    DeleteCriticalSection(&m_ctlocalip); //≥??oa???Ωá?ˉ
    
#endif
    
    if(m_pUpnpCtrl)
    {
        m_pUpnpCtrl->DelPortMap(m_nLocalPortWan,"UDP");
        delete m_pUpnpCtrl;
    }
}

void CCWorker::CreateShare()
{
    //初始化服务器对应的端口
    {//A
        CSERVER_PORT info = {9010,"A"};
        
        m_CServerPortList.push_back(info);
    }
    {//B
        CSERVER_PORT info = {9110,"B"};
        
        m_CServerPortList.push_back(info);
    }
    {//S
        CSERVER_PORT info = {9210,"S"};
        
        m_CServerPortList.push_back(info);
    }
    {//SC
        CSERVER_PORT info = {10000,"SC"};
        
        m_CServerPortList.push_back(info);
    }
    {//SD
        CSERVER_PORT info = {10002,"SD"};
        
        m_CServerPortList.push_back(info);
    }
    {//SE
        CSERVER_PORT info = {10004,"SE"};
        
        m_CServerPortList.push_back(info);
    }
    {//SF
        CSERVER_PORT info = {10006,"SF"};
        
        m_CServerPortList.push_back(info);
    }
    {//SH
        CSERVER_PORT info = {10008,"SH"};
        
        m_CServerPortList.push_back(info);
    }
    {//ST
        CSERVER_PORT info = {10010,"ST"};
        
        m_CServerPortList.push_back(info);
    }
    {//SK
        CSERVER_PORT info = {10012,"SK"};
        
        m_CServerPortList.push_back(info);
    }
    {//SL
        CSERVER_PORT info = {10014,"SL"};
        
        m_CServerPortList.push_back(info);
    }
    {//SN
        CSERVER_PORT info = {10016,"SN"};
        
        m_CServerPortList.push_back(info);
    }
    {//SP
        CSERVER_PORT info = {10018,"SP"};
        
        m_CServerPortList.push_back(info);
    }
    {//SQ
        CSERVER_PORT info = {10020,"SQ"};
        
        m_CServerPortList.push_back(info);
    }
    {//SW
        CSERVER_PORT info = {10022,"SW"};
        
        m_CServerPortList.push_back(info);
    }
    {//N
        CSERVER_PORT info = {10024,"N"};
        
        m_CServerPortList.push_back(info);
    }
    {//Y
        CSERVER_PORT info = {10026,"Y"};
        
        m_CServerPortList.push_back(info);
    }
    {//SV
        CSERVER_PORT info = {10028,"SV"};
        
        m_CServerPortList.push_back(info);
    }
    {//SY
        CSERVER_PORT info = {10030,"SY"};
        
        m_CServerPortList.push_back(info);
    }
    {//C
        CSERVER_PORT info = {10034,"C"};
        
        m_CServerPortList.push_back(info);
    }
    {//D
        CSERVER_PORT info = {10036,"D"};
        
        m_CServerPortList.push_back(info);
    }
    {//SG
        CSERVER_PORT info = {10038,"SG"};
        
        m_CServerPortList.push_back(info);
    }
    
    LoadDemoFile();
    //	m_Helper.Start(this);
    m_MakeHoleGroup.Start(this);
    //￥￥Ω???Ω”?÷
    //////////////////////////////////////////////////////////////////////////
    //￥￥Ω????±UDP??Ω”?÷
    m_WorkerUDPSocket = socket(AF_INET, SOCK_DGRAM,0);
    
    SOCKADDR_IN addrSrv;
#ifndef WIN32
    addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(m_nLocalStartPort);//∞???μΩ÷∏?????o?à??
    
    //∞?????Ω”?÷
    int err = bind(m_WorkerUDPSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
    if(err != 0)
    {
        char ch[1000]={0};
        sprintf(ch,"%d",m_nLocalStartPort);
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?.÷∏???à???…??±a’o￡¨∏?”√àêa˙?à?? ±?￥??˙”√?à??:", __FILE__,__LINE__,ch);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed.old port is invalid°í°?use new prot. old port:", __FILE__,__LINE__,ch);
        }
        
#ifndef WIN32
        addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
        addrSrv.sin_family = AF_INET;
        addrSrv.sin_port = htons(0);//∞???μΩàêa˙?à??
        
        bind(m_WorkerUDPSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
    }
    //÷?–?aò?°?à?? ∑?÷π9200??∞?
    {
        
        struct sockaddr_in sin;
        int len = sizeof(sin);
#ifdef WIN32
        if(getsockname(m_WorkerUDPSocket, (struct sockaddr *)&sin, &len) != 0)
#else
            if(getsockname(m_WorkerUDPSocket, (struct sockaddr *)&sin, (socklen_t* )&len) != 0)
#endif
            {
                printf("getsockname() error:%s\n", strerror(errno));
            }
        
        m_nLocalStartPort = ntohs(sin.sin_port);
        
        //		OutputDebug("PORT = %d SOCKET = %d",m_nLocalStartPort,m_WorkerUDPSocket);
    }
    
    int nReuse = TRUE;
    setsockopt(m_WorkerUDPSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nReuse, sizeof(int));
    
    m_WorkerUDTSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    
    BOOL bReuse = TRUE;
    UDT::setsockopt(m_WorkerUDTSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_MSS, &len1, sizeof(int));
    //////////////////////////////////////////////////////////////////////////
    
#ifdef MOBILE_CLIENT
//    len1=1000*1024;
//    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_SNDBUF, &len1, sizeof(int));
//    len1=1500*1024;
//    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_RCVBUF, &len1, sizeof(int));
    len1=1500*1024;
    UDT::setsockopt(m_WorkerUDTSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
    
    len1=1000*1024;
    UDT::setsockopt(m_WorkerUDTSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
    
    if (UDT::ERROR == UDT::bind(m_WorkerUDTSocket, m_WorkerUDPSocket))
    {//∞???μΩ÷∏???à????∞?￡¨∏???∞???μΩàêa˙?à??
        if(m_WorkerUDPSocket > 0)
        {
            closesocket(m_WorkerUDPSocket);
        }
        m_WorkerUDPSocket = 0;
        
        if(m_WorkerUDTSocket > 0)
        {
            UDT::close(m_WorkerUDTSocket);
        }
        m_WorkerUDTSocket = 0;
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?.?¨Ω”÷?????∞?(?…?????à??±a’o) ?í?∏:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
        }
    }
    
    //Ω′??Ω”?÷÷√??∑??????￡?Ω
	BOOL block = FALSE;
	UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
	UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
    LINGER linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
    //////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////
    //tcp
    //////////////////////////////////////////////////////////////////////////
    m_WorkerTCPSocket = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN name;
    int namelen = sizeof(SOCKADDR_IN);
    if (-1 == UDT::getsockname(m_WorkerUDTSocket, (SOCKADDR*)&name, &namelen))
    {
        closesocket(m_WorkerTCPSocket);
        m_WorkerTCPSocket = 0;
        //"?¨Ω”??∞?.aò?°?à????∞?");
    }
    
#ifndef WIN32
    name.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    name.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    
    name.sin_family = AF_INET;
    
#ifndef WIN32
    if (bind(m_WorkerTCPSocket,(SOCKADDR *)&name, namelen) < 0)
#else
        if (SOCKET_ERROR == bind(m_WorkerTCPSocket,(SOCKADDR *)&name, namelen))
#endif
        {
            closesocket(m_WorkerTCPSocket);
            m_WorkerTCPSocket = 0;
        }
    int iSockStatus = 0;
    
#ifndef WIN32
    int flags=0;
    flags = fcntl(m_WorkerTCPSocket, F_GETFL, 0);
    fcntl(m_WorkerTCPSocket, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long ulBlock = 1;
    iSockStatus = ioctlsocket(m_WorkerTCPSocket, FIONBIO, (unsigned long*)&ulBlock);
#endif
    
    int nSetSize = 128*1024;
    setsockopt(m_WorkerTCPSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
    nSetSize = 128*1024;
    setsockopt(m_WorkerTCPSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));
    
    //Ω′??Ω”?÷÷√??≤aμ?￥??￥￥??ì?íμ?????
    linger.l_onoff = 1;//0;
    linger.l_linger = 0;
    iSockStatus = setsockopt(m_WorkerTCPSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
    
#ifndef WIN32
    if (listen(m_WorkerTCPSocket, 30) < 0)
#else
        if (SOCKET_ERROR == listen(m_WorkerTCPSocket, 30))
#endif
        {
            closesocket(m_WorkerTCPSocket);
            m_WorkerTCPSocket = 0;
        }
    
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
    if (0 != pthread_create(&m_hPTListenThread, pAttr, PTListenProc, this))
    {
        m_hPTListenThread = 0;
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω?a?∞èo?????≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create ptls thread failed.", __FILE__,__LINE__);
        }
    }
    if (0 != pthread_create(&m_hGTThread, pAttr, GTProc, this))
    {
        m_hGTThread = 0;
        
        if (0 != m_hPTListenThread)
        {
            m_bPTListenEnd = TRUE;
            pthread_join(m_hPTListenThread, NULL);
            m_hPTListenThread = 0;
        }
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω????ì??≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create gt thread failed.", __FILE__,__LINE__);
        }
    }
    if (0 != pthread_create(&m_hNatGetThread, pAttr, GetIPNatProc, this))
    {
        m_hNatGetThread = 0;
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω?NAT??≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create nat thread failed.", __FILE__,__LINE__);
        }
    }
    if(m_pUpnpCtrl == NULL)
    {
        m_pUpnpCtrl = new CSUpnpCtrl(this);
        
        pthread_t m_hSetUpnpThread;
        pthread_attr_t attr;
        pthread_attr_t *pAttr = &attr;
        unsigned long size = LINUX_THREAD_STACK_SIZE;
        size_t stacksize = size;
        pthread_attr_init(pAttr);
        if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
        {
            pAttr = NULL;
        }
        if (0 != pthread_create(&m_hSetUpnpThread, pAttr, SetUpnpProc, this))
        {
            m_hSetUpnpThread = 0;
            return;
        }
    }
#else
    //￥￥Ω?±?μ?a?∞è￥??ì??≥?
    UINT unTheadID;
    m_hPTListenStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hPTListenEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hPTListenThread = (HANDLE)_beginthreadex(NULL, 0, PTListenProc, (void *)this, 0, &unTheadID);
    SetEvent(m_hPTListenStartEvent);
    if (m_hPTListenThread == 0)//￥￥Ω???≥???∞?
    {
        //???ìRecvP??≥?
        if(m_hPTListenStartEvent > 0)
        {
            CloseHandle(m_hPTListenStartEvent);
            m_hPTListenStartEvent = 0;
        }
        if(m_hPTListenEndEvent > 0)
        {
            CloseHandle(m_hPTListenEndEvent);
            m_hPTListenEndEvent = 0;
        }
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω?a?∞èo?????≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create ptls thread failed.", __FILE__,__LINE__);
        }
    }
    
    m_hGTStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hGTEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hGTThread = (HANDLE)_beginthreadex(NULL, 0, GTProc, (void *)this, 0, &unTheadID);
    SetEvent(m_hGTStartEvent);
    if (m_hGTThread == 0)//￥￥Ω???≥???∞?
    {
        if(m_hGTStartEvent > 0)
        {
            CloseHandle(m_hGTStartEvent);
            m_hGTStartEvent = 0;
        }
        if(m_hGTEndEvent > 0)
        {
            CloseHandle(m_hGTEndEvent);
            m_hGTEndEvent = 0;
        }
        
        //???ìRecvP??≥?
        if(m_hPTListenEndEvent>0)
        {
            SetEvent(m_hPTListenEndEvent);
        }
        CCChannel::WaitThreadExit(m_hPTListenThread);
        
        if(m_hPTListenStartEvent > 0)
        {
            CloseHandle(m_hPTListenStartEvent);
            m_hPTListenStartEvent = 0;
        }
        if(m_hPTListenEndEvent > 0)
        {
            CloseHandle(m_hPTListenEndEvent);
            m_hPTListenEndEvent = 0;
        }
        if(m_hPTListenThread > 0)
        {
            CloseHandle(m_hPTListenThread);
            m_hPTListenThread = 0;
        }
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω????ì??≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create gt thread failed.", __FILE__,__LINE__);
        }
    }
    m_hNatGetStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hNatGetEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hNatGetThread = (HANDLE)_beginthreadex(NULL, 0, GetIPNatProc, (void *)this, 0, &unTheadID);
    SetEvent(m_hNatGetStartEvent);
    if (m_hNatGetThread == 0)//￥￥Ω???≥???∞?
    {
        if(m_hNatGetStartEvent > 0)
        {
            CloseHandle(m_hNatGetStartEvent);
            m_hNatGetStartEvent = 0;
        }
        if(m_hNatGetEndEvent > 0)
        {
            CloseHandle(m_hNatGetEndEvent);
            m_hNatGetEndEvent = 0;
        }
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0, "??∞?. ￥￥Ω?NAT??≥???∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0, "failed. create nat thread failed.", __FILE__,__LINE__);
        }
    }
    if(m_pUpnpCtrl == NULL)
    {
        m_pUpnpCtrl = new CSUpnpCtrl(this);
        
        UINT unTheadID;
        HANDLE m_hSetUpnpThread;
        m_hSetUpnpThread = (HANDLE)_beginthreadex(NULL, 0, SetUpnpProc, (void *)this, 0, &unTheadID);
        if (m_hSetUpnpThread == 0)//￥￥Ω???≥???∞?
        {
            return;
        }
    }
#endif
}

//??????μ?∑??ò
void CCWorker::DisConnect(int nLocalChannel)
{
    if(nLocalChannel == -1)
    {//????à?”–
#ifndef WIN32
        pthread_mutex_lock(&m_criticalsection);
#else
        EnterCriticalSection(&m_criticalsection);
#endif
        int nCount = m_pChannels.size();
        for(int i=0; i<nCount; i++)
        {
            if(m_pChannels[i] == NULL)
            {//??μù??–?o??o
                m_pChannels.erase(m_pChannels.begin() + i);
                nCount = m_pChannels.size();
                i--;
                continue;
            }
            
            m_pChannels[i]->DisConnect();
            delete m_pChannels[i];
            m_pChannels[i] = NULL;
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount = m_pChannels.size();
            i--;
            ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_DISOK,NULL,0,__FILE__,__LINE__,__FUNCTION__);
            continue;
        }
#ifndef WIN32
        pthread_mutex_unlock(&m_criticalsection);
#else
        LeaveCriticalSection(&m_criticalsection);
#endif
    }
    else
    {//?????≥“a?∑
        //		int nLocalPort=0;
      
#ifndef WIN32
        pthread_mutex_lock(&m_criticalsection);
#else
        EnterCriticalSection(&m_criticalsection);
#endif
        int nCount = m_pChannels.size();
        for(int i=0; i<nCount; i++)
        {
            if(m_pChannels[i] == NULL)
            {//??μù??–?o??o
                m_pChannels.erase(m_pChannels.begin() + i);
                nCount = m_pChannels.size();
                i--;
                continue;
            }
            if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
            {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?????¨Ω”
                if (m_pChannels[i]->m_nStatus==6 || m_pChannels[i]->m_nStatus==OK_OLD) {
                    m_pChannels[i]->DisConnect();
                    printf("run here 1\n");
                    delete m_pChannels[i];
                    
                    m_pChannels[i] = NULL;
                    m_pChannels.erase(m_pChannels.begin() + i);
                    
#ifndef WIN32
                    pthread_mutex_unlock(&m_criticalsection);
#else
                    LeaveCriticalSection(&m_criticalsection);
#endif
                    ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_DISOK,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    //                    printf("run here 2\n");
                }else{
                    //                    m_pChannels[i]->m_bExit = TRUE;
                    //                    printf("run here 2 1\n");
                    m_pChannels[i]->DisConnect();
                    //                    if (m_pChannels[i]->m_hConnThread!=0) {
                    //                        pthread_cancel(m_pChannels[i]->m_hConnThread);
                    //                    }
                    //                    printf("run here 2 2\n");
                    delete m_pChannels[i];
                    
                    m_pChannels[i] = NULL;
                    m_pChannels.erase(m_pChannels.begin() + i);
                    
#ifndef WIN32
                    pthread_mutex_unlock(&m_criticalsection);
#else
                    LeaveCriticalSection(&m_criticalsection);
#endif
                    
                    
                    ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_DISOK,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    //                    printf("run here 2 3\n");
                }
                
                return;
            }
            if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
               && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
                   || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
            {//??μù??–?o??o(??–?μ?∫????…–≠“èμ??o±???FAILD,socketμ??è??‘ú”–??÷÷?è∫?)
                
                //				nLocalPortDel.push_back(m_pChannels[i]->m_nLocalStartPort);
                
                delete m_pChannels[i];
                m_pChannels[i] = NULL;
                m_pChannels.erase(m_pChannels.begin() + i);
                nCount = m_pChannels.size();
                i--;
                
                continue;
            }
        }
        
#ifndef WIN32
        pthread_mutex_unlock(&m_criticalsection);
#else
        LeaveCriticalSection(&m_criticalsection);
#endif
        
        ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_NOCONN,NULL,0,__FILE__,__LINE__,__FUNCTION__);
    }
    
    return;
}

//∑￠à?????
BOOL CCWorker::SendData(int nLocalChannel, BYTE uchType, BYTE *pBuffer,int nSize)
{

#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            //			nLocalPortDel.push_back(m_pChannels[i]->m_nLocalStartPort);
            
            //			delete m_pChannels[i];
            //			m_pChannels[i] = NULL;
            //			m_pChannels.erase(m_pChannels.begin() + i);
            //			nCount--;
            //			i--;
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”
            if(!m_pChannels[i]->SendData(uchType, pBuffer, nSize))
            {
                //				delete m_pChannels[i];
                //				m_pChannels[i] = NULL;
                //				m_pChannels.erase(m_pChannels.begin() + i);
                
#ifndef WIN32
                pthread_mutex_unlock(&m_criticalsection);
#else
                LeaveCriticalSection(&m_criticalsection);
#endif
                
                //				ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_DISCONNE, NULL);
                //				if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                //				{
                //					m_Log.SetRunInfo(nLocalChannel, "∑￠à???????∞?,π?±’∏√?¨Ω”", __FILE__,__LINE__);
                //				}
                //				else
                //				{
                //					m_Log.SetRunInfo(nLocalChannel, "SendData failed, disconnect.", __FILE__,__LINE__);
                //				}
                
                return FALSE;
            }
#ifndef WIN32
            pthread_mutex_unlock(&m_criticalsection);
#else
            LeaveCriticalSection(&m_criticalsection);
#endif
            
            return TRUE;
            
        }
    }
    
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    return FALSE;
}

int CCWorker::SendCMD(int nLocalChannel, BYTE uchType, BYTE* pBuffer, int nSize)
{
    
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”
            int nret = m_pChannels[i]->SendCMD(uchType, pBuffer, nSize);
            
#ifndef WIN32
            pthread_mutex_unlock(&m_criticalsection);
#else
            LeaveCriticalSection(&m_criticalsection);
#endif
            
            return nret;
        }
    }
    
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    return 0;
}

/*??≤?∫???￡¨aò?°μ±?∞???o*/
void CCWorker::GetCurrentPath(char chCurPath[MAX_PATH])
{
    if(strlen(m_chLocalPath) >= 2)
    {
        strcpy(chCurPath, m_chLocalPath);
        return;
    }
    
    int i = 0;
    int iLastSperate = 0;
    
#ifndef WIN32
    std::string _exeName = "/proc/self/exe";
    size_t linksize = MAX_PATH;
    char achCurPath[MAX_PATH]={0};
    int ncount = readlink(_exeName.c_str() , achCurPath, linksize);
    if(ncount < 0 || ncount >= MAX_PATH)
    {
        memset(achCurPath,0,MAX_PATH);
    }
    else
    {
        achCurPath[ncount] = '\0';
    }
#else
    TCHAR achCurPath[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), achCurPath, MAX_PATH);
#endif
    for (i=0; i<MAX_PATH; i++)
    {
#ifndef WIN32
        if (achCurPath[i] == '/')
#else
            if (achCurPath[i] == '\\')
#endif
            {
                iLastSperate = i;
            }
            else if(achCurPath[i] == '\0')
            {
                break;
            }
    }
    
    if (iLastSperate > 0 && i < MAX_PATH)
    {
        achCurPath[iLastSperate] = '\0';
    }
    else
    {
        //return;//?∑??π?≥§￡¨?°?∑????∞?
    }
    memset(chCurPath, 0, MAX_PATH);
    strcpy(chCurPath, achCurPath);
    return;
}

BOOL MOGetLine(char *pchread, int line, int &npostmp, char *pacBuff)
{
    if(pchread == NULL || pacBuff == NULL || line <= 0 || npostmp >= line)
    {
        return FALSE;
    }
    if(npostmp < 0)
    {
        npostmp = 0;
    }
    for(int i=npostmp; i<line-1; i++)
    {
        if(pchread[i] == '\r' && pchread[i+1] == '\n')
        {
            memcpy(pacBuff, &pchread[npostmp], i-npostmp);
            npostmp = i+2;
            return TRUE;
        }
        if(pchread[i] == '\0')
        {
            return FALSE;
        }
    }
    return FALSE;
}
//aò?°∑??ò???–±ì
BOOL CCWorker::GetSerList(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort)
{
    //////////////////////////////////////////////////////////////////////////
    //	STSERVER sts;
    //	sts.addr.sin_addr.S_un.S_addr = inet_addr("58.56.109.122");//60.217.227.246//60.215.129.99//123.134.66.138////192.154.96.58
    //	sts.addr.sin_family = AF_INET;
    //	sts.addr.sin_port = htons(9010);
    //	sts.buseful = TRUE;
    //	sts.nver = 0;
    //	SList.push_back(sts);//o??o
    //	return TRUE;
    //////////////////////////////////////////////////////////////////////////
    SList.clear();
    
    sockaddr_in sin;
    ::std::vector<STSIP> stIPList;
    
    int nSockAddrLen = sizeof(SOCKADDR);
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    
    int nSerCount = 0 ;
    char acBuff[MAX_PATH] = {0};
    
    BOOL bFindSite=FALSE;
    char chSiteIP[30]={0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//√a”–√??∑÷∏???∑??￡¨‘ú‘?dvr??≤…”√π????∑???ˉ≤a???μo?μ±?∞?∑??
        sprintf(acBuff, "%s/YST_%s.dat", "./data",chGroup);
    }
    else
    {//?π”√÷∏??μ??∑??
        sprintf(acBuff, "%s/YST_%s.dat", m_chLocalPath,chGroup);
    }
#else
    sprintf(acBuff, "%s\\YST_%s.dat", chCurPath,chGroup);
#endif
    
    BOOL bfind = FALSE;
#ifndef WIN32
    pthread_mutex_lock(&m_ctGroup);
#else
    EnterCriticalSection(&m_ctGroup);
#endif
    int ngcount = m_GroupList.size();
    for(int c=0; c<ngcount; c++)
    {
        if(0 == strcmp(chGroup, m_GroupList[c].chgroup))
        {//??≈‰￡¨’“μΩo??o,≤…?°????∫???‘?∑Ω?Ω
            bfind = TRUE;
            break;
        }
    }
    if(!bfind)
    {
        //√?￥?‘à––≥?–ú?¨“a∏?±??è∫≈÷a??‘?“a￥?￡¨±?√??à∑—
        STGROUP stg;
        memcpy(stg.chgroup, chGroup, 4);
        m_GroupList.push_back(stg);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctGroup);
#else
    LeaveCriticalSection(&m_ctGroup);
#endif
    
    if(!bfind)
    {//√a’“μΩo??o,≤…?°????‘?∫????°∑Ω?Ω￡¨’?—?￥??ì??±￡÷§√?￥?‘à––≥?–ú￡¨μ?“a￥?????∏?–?∑??ò???–±ì
        BOOL bfindgroup = FALSE;
        for(int l=0; l<m_nGroupCount; l++)
        {
            if(strcmp(chGroup, m_chGroupList[l]) == 0)
            {
                bfindgroup = TRUE;
            }
        }
        if(!bfindgroup)
        {//±??è≤a‘?‘§…?μ?∑?????￡¨–?“?????‘?
            if(DownLoadFirst(chGroup, SList, nWebIndex, nLocalPort))
            {
                return TRUE;
            }
        }
        //		//return DownLoadFirst(chGroup, SList, nWebIndex, nLocalPort);
    }
    
    //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT//?÷a˙??a??à￡¨￥”??￥ê???°
    
    //////////////////////////////////////////////////////////////////////////
    char chread[10240]={0};
    int nread=0;
    char stName[200] = {0};
    sprintf(stName, "%s%s.dat","YST_",chGroup);
    nread = ReadMobileFile(stName,chread,10240);
    
    //	m_pfWriteReadData(2,(unsigned char *)chGroup,"YST_",(unsigned char *)chread,&nread);//YST read ???èá??∞éá??????∞é?áΩê?∞
    if(nread > 0)
    {
        int npostmp = 0;
        memset(acBuff, 0, MAX_PATH);
        while(MOGetLine(chread,nread,npostmp,acBuff))//YST
        {
            //∑??ò??μ?÷∑Ω?√?
            for (unsigned int m=0; m<strlen(acBuff); m++)
            {
                acBuff[m] ^= m;
            }
            STSIP stIP;
            memset(stIP.chIP, 0, 16);
            int i=0;
            char chPort[16] = {0};
            for(i=0; i<MAX_PATH; i++)
            {
                if(acBuff[i] == ':' || acBuff[i] == '\0')
                {
                    break;
                }
            }
            memcpy(stIP.chIP, acBuff, i);
            memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i-1);
            stIP.nPort = atoi(chPort);
            if(stIP.nPort > 0)
            {
                stIPList.push_back(stIP);
                nSerCount++;
            }
        }
    }
    
#else//?‰à??è??￡¨￥”??o????°
    //’“μΩo??o,≤…?°????∫???‘?∑Ω?Ω
    ::std::string line;
    ::std::ifstream localfile(acBuff);
    while(::std::getline(localfile,line))
    {
        memset(acBuff, 0, MAX_PATH);
        line.copy(acBuff,MAX_PATH,0);
        //∑??ò??μ?÷∑Ω?√?
        for (unsigned int m=0; m<strlen(acBuff); m++)
        {
            acBuff[m] ^= m;
        }
        STSIP stIP;
        memset(stIP.chIP, 0, 16);
        int i=0;
        char chPort[16] = {0};
        for(i=0; i<MAX_PATH; i++)
        {
            if(acBuff[i] == ':' || acBuff[i] == '\0')
            {
                break;
            }
        }
        if(i > 0)
        {
            memcpy(stIP.chIP, acBuff, i);
            memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i+1);
        }
        
        stIP.nPort = atoi(chPort);
        if(stIP.nPort > 0)
        {
            stIPList.push_back(stIP);
            nSerCount++;
        }
    }
#endif
    
    if (nSerCount == 0)
    {
        //￥à￥?∏????ú∑??ò???–±ì?∫∑￠secunew0314
        memset(acBuff, 0, MAX_PATH);
#ifndef WIN32
        if(strlen(m_chLocalPath) <= 0)
        {//√a”–√??∑÷∏???∑??￡¨‘ú‘?dvr??≤…”√π????∑???ˉ≤a???μo?μ±?∞?∑??
            sprintf(acBuff, "%s/YST_%s.dat", "./data",chGroup);
        }
        else
        {//?π”√÷∏??μ??∑??
            sprintf(acBuff, "%s/YST_%s.dat", m_chLocalPath,chGroup);
        }
#else
        sprintf(acBuff, "%s\\YST_%s.dat", chCurPath,chGroup);
#endif
        
        FILE *pfile=NULL;
        
        char recvBuf[1024] = {0};
        char IPBuf[10 * 1024] = {0};
        char target[1024] = {0};
        char target_e[1024] = {0};
        int our_socket;
        struct sockaddr_in our_address;
        
        strcpy(target, "GET http://");
        strcpy(target_e, "GET ");
        if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
        {
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
                //strcat(target_e, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
                //strcat(target_e, JVN_WEBSITE2);
            }
            strcat(target, JVN_WEBFOLDER);
            strcat(target, chGroup);
            strcat(target, JVN_YSTLIST_ALL);
            strcat(target," HTTP/1.1\r\n");
            strcat(target,"Host:");
            //////////////////////////////////////////////////////////////////////////
            strcat(target_e, JVN_WEBFOLDER);
            strcat(target_e, chGroup);
            strcat(target_e, JVN_YSTLIST_ALL);
            strcat(target_e," HTTP/1.1\r\n");
            strcat(target_e,"Host:");
            //////////////////////////////////////////////////////////////////////////
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
                strcat(target_e, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
                strcat(target_e, JVN_WEBSITE2);
            }
        }
        else
        {
            strcat(target, m_strDomainName);
            strcat(target, m_strPathName);
            
            strcat(target," HTTP/1.1\r\n");
            strcat(target,"Host:");
            
            strcat(target, m_strDomainName);
            //////////////////////////////////////////////////////////////////////////
            strcat(target_e, m_strDomainName);
            strcat(target_e, m_strPathName);
            
            strcat(target_e," HTTP/1.1\r\n");
            strcat(target_e,"Host:");
            
            strcat(target_e, m_strDomainName);
            //////////////////////////////////////////////////////////////////////////
        }
        
        strcat(target,"\r\n");
        strcat(target,"Accept:*/*\r\n");
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e,"\r\n");
        strcat(target_e,"Accept:*/*\r\n");
        strcat(target_e, JVN_AGENTINFO);
        //////////////////////////////////////////////////////////////////////////
        strcat(target,"Connection:Keep-Alive\r\n\r\n");
        strcat(target_e,"Connection:Keep-Alive\r\n\r\n");
        
        struct hostent * site=NULL;
        if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
        {
            if(1 == nWebIndex)
            {
                char severname[] = JVN_WEBSITE1;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
            }
            else
            {
                char severname[] = JVN_WEBSITE2;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
            }
        }
        else
        {
            site = gethostbyname (m_strDomainName);
            if(site == NULL)
            {
                CSDNSCtrl csdns;
                bFindSite = csdns.GetIPByDomain(m_strDomainName, chSiteIP);
            }
        }
        
        if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
            }
            
            return FALSE;
        }
        
        LINGER linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
        
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = htons(nLocalPort);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                return FALSE;
            }
        }
        
        if(site == NULL && !bFindSite)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:aò?°”ú√?μ?÷∑??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:get hostsite failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
        
        memset(&our_address, 0, sizeof(struct sockaddr_in));
        our_address.sin_family = AF_INET;
        
        if(site != NULL)
        {
            memcpy (&our_address.sin_addr, site->h_addr_list[0], site->h_length);
        }
        else if(bFindSite)
        {
            our_address.sin_addr.s_addr = inet_addr(chSiteIP);
        }
        
        our_address.sin_port = htons(80);
        
        if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
        
        
        if(CCChannel::tcpsend(our_socket,target,strlen(target),3)<=0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:?ú∑??ò??∑￠à???????∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:send data to server failed.", __FILE__,__LINE__);
            }
            closesocket(our_socket);
            return FALSE;
        }
        int nLen = 0;
        int tryTimes=0;
        while(tryTimes < 3)
        {
            int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,2);
            if(rlen <0 || (nLen+rlen)>1024)
            {
                break;
            }
            memcpy(&IPBuf[nLen], recvBuf, rlen);
            nLen += rlen;
            tryTimes++;
        }
        closesocket(our_socket);
        our_socket = 0;
        
        if(nLen == 0)
        {//√a?’μΩ??∫?????￡¨∞￥–?????∞?÷?–?????“a￥?
            if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
                }
                
                return FALSE;
            }
            
            LINGER linger2;
            linger2.l_onoff = 1;
            linger2.l_linger = 0;
            setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger2, sizeof(LINGER));
            
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            sin.sin_family = AF_INET;
            sin.sin_port = htons(nLocalPort);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
#ifndef WIN32
                sin.sin_addr.s_addr = INADDR_ANY;
#else
                sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
                sin.sin_family = AF_INET;
                sin.sin_port = htons(0);
                if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
                {
                    if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                    {
                        m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                    }
                    
                    closesocket(our_socket);
                    return FALSE;
                }
            }
            
            if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                return FALSE;
            }
            
            if(CCChannel::tcpsend(our_socket,target_e,strlen(target_e),3)<=0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:?ú∑??ò??∑￠à???????∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:send data to server failed.", __FILE__,__LINE__);
                }
                closesocket(our_socket);
                return FALSE;
            }
            tryTimes=0;
            while(tryTimes < 3)
            {
                int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,2);
                if(rlen <0 || (nLen+rlen)>1024)
                {
                    break;
                }
                memcpy(&IPBuf[nLen], recvBuf, rlen);
                nLen += rlen;
                tryTimes++;
            }
        }
        
        if(our_socket > 0)
        {
            closesocket(our_socket);
        }
        
        //Ω???μ?÷∑
        if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
        {
            //–￥??o?μΩ”?”√≤?
            unsigned char writeBuffer[10240] ={0};
            int writeLen = 0;
            
            char chIP[50] = {0};
            char chPort[50] = {0};
            //char chTmp[50]={0};
            int nStart = 0, nMid = 0, nEnd = 0;
            for(int i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P' && IPBuf[i+2] == ':')
                {//’“μΩ???o?a÷√
                    nStart = i+3;
                    nMid = nStart;
                    nEnd = nMid;
                    
                    for(int j=nStart; j<nLen; j++)
                    {
                        if(IPBuf[j] == ':')
                        {//’“μΩ∑÷∏ù∑?,÷?￥àIP?…Ω?≥?
                            nMid = j+1;
                            nEnd = nMid;
                            
                            for(int k=nMid; k<nLen; k++)
                            {
                                if(IPBuf[k] < '0' || IPBuf[k] > '9')
                                {
                                    nEnd = k;
                                    break;
                                }
                                if(k==nLen-1)
                                {
                                    nEnd = k+1;
                                }
                            }
                            break;
                        }
                    }
                    
                    STSIP stIP;
                    memset(stIP.chIP, 0, 16);
                    
                    memcpy(chIP, &IPBuf[nStart], nMid-nStart-1);
                    memcpy(stIP.chIP, &IPBuf[nStart], nMid-nStart-1);//
                    memcpy(chPort, &IPBuf[nMid], nEnd-nMid);
                    chIP[nMid-nStart-1] = ':';
                    memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                    chPort[nEnd-nMid]='\0';
                    stIP.nPort = atoi(chPort);
                    if(i>0 && IPBuf[i-1] == 'D')
                    {
                        stIP.nISP = JVNC_ISP_DX;
                    }
                    else
                    {
                        stIP.nISP = JVNC_ISP_WT;//≤a??≈–??μ?μ??è??“a∏≈πè???ˉ??
                    }
                    
                    if(stIP.nPort > 0)
                    {
                        stIPList.push_back(stIP);
                        nSerCount++;
                    }
                    
                    if(strlen(chIP) > 0)
                    {
                        //o”√?
                        for (unsigned int m=0; m<strlen(chIP); m++)
                        {
                            chIP[m] ^= m;
                        }
                        chIP[nEnd-nStart] = '\r';
                        chIP[nEnd-nStart+1] = '\n';
                        
                        if(pfile==NULL)
                        {
                            pfile = fopen(acBuff, "wb");
                        }
                        if(pfile != NULL)
                        {
                            fwrite(chIP,sizeof(char),nEnd-nStart+2,pfile);
                        }
                        
                        memcpy(writeBuffer+writeLen,chIP , nEnd-nStart+2);
                        //printf("write to file ip: %s, port: %d\n",chIP,stIP.nPort);
                        writeLen += nEnd-nStart+2;
                    }
                    
                    i = nEnd+1;
                }
                else if(IPBuf[i] == '\0')
                {
                    break;
                }
            }
            
#ifdef MOBILE_CLIENT//?÷a˙??a??à￡¨￥”??￥ê???°
            if (writeLen>0)
            {
                //a?μ?–￥≤???
                //m_pfWriteReadData(1,(unsigned char *)chGroup,"YST_",writeBuffer, &writeLen);//YST write
                
                char stName[200] = {0};
                sprintf(stName, "%s%s.dat","YST_",chGroup);
                WriteMobileFile(stName,(char* )writeBuffer,writeLen);
                
            }
#endif
        }
        else//?π”√ –?”ú√??± –?“?’“μΩo”√?μ?–≈?￠Ω?√?
        {
            char chIP[50] = {0};
            char chPort[50] = {0};
            int nStart = 0, nMid = 0, nEnd = 0;
            int i=0;
            for(i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P'
                   && IPBuf[i+2] == '_'
                   && IPBuf[i+3] == 'S' && IPBuf[i+4] == 'T'
                   && IPBuf[i+5] == 'A' && IPBuf[i+6] == 'R'
                   && IPBuf[i+7] == 'T')//IP_START:
                {//’“μΩ???o?a÷√
                    nStart = i+9;
                    memmove(IPBuf,&IPBuf[nStart],nLen-nStart);
                    nLen-=nStart;
                    IPBuf[nLen] = '\0';
                    MultiMaskRemove(IPBuf,nLen);
                    break;
                }
            }
            for(i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P' && IPBuf[i+2] == ':')
                {
                    nStart = i + 3;
                    nMid = nStart;
                    nEnd = nMid;
                    
                    for(int j=nStart; j<nLen; j++)
                    {
                        if(IPBuf[j] == ':')
                        {//’“μΩ∑÷∏ù∑?,÷?￥àIP?…Ω?≥?
                            nMid = j+1;
                            nEnd = nMid;
                            
                            for(int k=nMid; k<nLen; k++)
                            {
                                if(IPBuf[k] < '0' || IPBuf[k] > '9')
                                {
                                    nEnd = k;
                                    break;
                                }
                                if(k==nLen-1)
                                {
                                    nEnd = k+1;
                                }
                            }
                            break;
                        }
                    }
                    
                    STSIP stIP;
                    memset(stIP.chIP, 0, 16);
                    
                    memcpy(chIP, &IPBuf[nStart], nMid-nStart-1);
                    memcpy(stIP.chIP, &IPBuf[nStart], nMid-nStart-1);//
                    memcpy(chPort, &IPBuf[nMid], nEnd-nMid);
                    chIP[nMid-nStart-1] = ':';
                    memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                    chPort[nEnd-nMid]='\0';
                    stIP.nPort = atoi(chPort);
                    
                    if(stIP.nPort > 0)
                    {
                        stIPList.push_back(stIP);
                        nSerCount++;
                    }
                    
                    if(strlen(chIP) > 0)
                    {
                        //o”√?
                        for (unsigned int m=0; m<strlen(chIP); m++)
                        {
                            chIP[m] ^= m;
                        }
                        chIP[nEnd-nStart] = '\r';
                        chIP[nEnd-nStart+1] = '\n';
                        
                        if(pfile == NULL)
                        {
                            pfile = fopen(acBuff, "wb");
                        }
                        
                        if(pfile != NULL)
                        {
                            fwrite(chIP,sizeof(char),nEnd-nStart+2,pfile);
                        }
                    }
                    
                    i = nEnd;
                }
                else if(IPBuf[i] == '\0')
                {
                    break;
                }
            }
        }
        
        if(pfile != NULL)
        {
            fclose(pfile);
        }
    }
    
    if(nSerCount>0)
    {
        STSERVER stserver;
        for (int i = 0;i<nSerCount;i++)
        {
#ifndef WIN32
            stserver.addr.sin_addr.s_addr = inet_addr(stIPList[i].chIP);
#else
            stserver.addr.sin_addr.S_un.S_addr = inet_addr(stIPList[i].chIP);
#endif
            stserver.addr.sin_family = AF_INET;
            stserver.addr.sin_port = htons(stIPList[i].nPort);
            stserver.buseful = TRUE;
            stserver.nver = 0;
            SList.push_back(stserver);//o??o
        }
        
    }
    else
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∑??ò???–±ì???’", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:no server is find.", __FILE__,__LINE__);
        }
        
        return FALSE;
    }
    //printf("************************************ 2 readFromFile chGroup: %s,nSerCount: %d\n",chGroup,SList.size());
    return TRUE;
}

//aò?°∑??ò???–±ì
BOOL CCWorker::DownLoadFirst(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort)
{
    sockaddr_in sin;
    ::std::vector<STSIP> stIPList;
    
    int nSockAddrLen = sizeof(SOCKADDR);
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    
    int nSerCount = 0 ;
    char acBuff[MAX_PATH] = {0};
    
    BOOL bFindSite=FALSE;
    char chSiteIP[30]={0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//‚àöa‚Äù‚Äì‚àö??‚àë√∑‚àè???‚àë??Ôø°¬®‚Äò√∫‚Äò?dvr??‚â§‚Ä¶‚Äù‚àöœÄ????‚àë???Àâ‚â§a???Œºo?Œº¬±?‚àû?‚àë??
        sprintf(acBuff, "%s/YST_%s.dat", "./data",chGroup);
    }
    else
    {//?œÄ‚Äù‚àö√∑‚àè??Œº??‚àë??
        sprintf(acBuff, "%s/YST_%s.dat", m_chLocalPath,chGroup);
    }
#else
    sprintf(acBuff, "%s\\YST_%s.dat", chCurPath,chGroup);
#endif
    
    if(nSerCount == 0)
    {
        //Ôø•√†Ôø•?‚àè????√∫‚àë??√≤???‚Äì¬±√¨?‚à´‚àëÔø†secunew0314
        memset(acBuff, 0, MAX_PATH);
        
#ifndef WIN32
        if(strlen(m_chLocalPath) <= 0)
        {//‚àöa‚Äù‚Äì‚àö??‚àë√∑‚àè???‚àë??Ôø°¬®‚Äò√∫‚Äò?dvr??‚â§‚Ä¶‚Äù‚àöœÄ????‚àë???Àâ‚â§a???Œºo?Œº¬±?‚àû?‚àë??
            sprintf(acBuff, "%s/YST_%s.dat", "./data",chGroup);
        }
        else
        {//?œÄ‚Äù‚àö√∑‚àè??Œº??‚àë??
            sprintf(acBuff, "%s/YST_%s.dat", chCurPath,chGroup);
        }
#else
        sprintf(acBuff, "%s\\YST_%s.dat", chCurPath,chGroup);
#endif
        
        FILE *pfile=NULL;
        //pfile = fopen(acBuff, "wb");
        
        char recvBuf[1024] = {0};
        char IPBuf[1024] = {0};
        char target[1024] = {0};
        char target_e[1024] = {0};
        int our_socket;
        struct sockaddr_in our_address;
        
        strcpy(target, "GET http://");
        strcpy(target_e, "GET ");
        if(strlen(m_strDomainName) == 0)//?œÄ‚Äù‚àöœÄ‚Ä≤√†?Œº?‚Äù√∫‚àö?
        {
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
                //strcat(target_e, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
                //strcat(target_e, JVN_WEBSITE2);
            }
            strcat(target, JVN_WEBFOLDER);
            strcat(target, chGroup);
            strcat(target, JVN_YSTLIST_ALL);
            strcat(target," HTTP/1.1\r\n");
            strcat(target,"Host:");
            //////////////////////////////////////////////////////////////////////////
            strcat(target_e, JVN_WEBFOLDER);
            strcat(target_e, chGroup);
            strcat(target_e, JVN_YSTLIST_ALL);
            strcat(target_e," HTTP/1.1\r\n");
            strcat(target_e,"Host:");
            //////////////////////////////////////////////////////////////////////////
            
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
                strcat(target_e, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
                strcat(target_e, JVN_WEBSITE2);
            }
        }
        else
        {
            strcat(target, m_strDomainName);
            strcat(target, m_strPathName);
            
            strcat(target," HTTP/1.1\r\n");
            strcat(target,"Host:");
            
            strcat(target, m_strDomainName);
            //////////////////////////////////////////////////////////////////////////
            strcat(target_e, m_strDomainName);
            strcat(target_e, m_strPathName);
            
            strcat(target_e," HTTP/1.1\r\n");
            strcat(target_e,"Host:");
            
            strcat(target_e, m_strDomainName);
            //////////////////////////////////////////////////////////////////////////
        }
        
        strcat(target,"\r\n");
        strcat(target,"Accept:*/*\r\n");
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e,"\r\n");
        strcat(target_e,"Accept:*/*\r\n");
        strcat(target_e, JVN_AGENTINFO);
        //////////////////////////////////////////////////////////////////////////
        strcat(target,"Connection:Keep-Alive\r\n\r\n");
        strcat(target_e,"Connection:Keep-Alive\r\n\r\n");
        
        struct hostent * site=NULL;
        if(strlen(m_strDomainName) == 0)//?œÄ‚Äù‚àöœÄ‚Ä≤√†?Œº?‚Äù√∫‚àö?
        {
            if(1 == nWebIndex)
            {
                char severname[] = JVN_WEBSITE1;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
                
            }
            else
            {
                char severname[] = JVN_WEBSITE2;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
            }
        }
        else
        {
            site = gethostbyname (m_strDomainName);
            if(site == NULL)
            {
                CSDNSCtrl csdns;
                bFindSite = csdns.GetIPByDomain(m_strDomainName, chSiteIP);
            }
        }
        
        if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?,Ôø•Ôø•Œ©?web??Œ©‚Äù?√∑??‚àû?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
            }
            
            //return FALSE;
            goto READFILE;
        }
        
        LINGER linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
        
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        
        sin.sin_family = AF_INET;
        sin.sin_port = htons(nLocalPort);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:‚àû???web??Œ©‚Äù?√∑??‚àû?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                //return FALSE;
                goto READFILE;
                
            }
        }
        
        if(site == NULL && !bFindSite)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:a√≤?¬∞‚Äù√∫‚àö?Œº?√∑‚àë??‚àû?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:get hostsite failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            //return FALSE;
            goto READFILE;
        }
        
        memset(&our_address, 0, sizeof(struct sockaddr_in));
        our_address.sin_family = AF_INET;
        if(site != NULL)
        {
            memcpy (&our_address.sin_addr, site->h_addr_list[0], site->h_length);
        }
        else if(bFindSite)
        {
            our_address.sin_addr.s_addr = inet_addr(chSiteIP);
        }
        our_address.sin_port = htons(80);
        
        if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:web?¬®Œ©‚Äù??‚àû?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            //return FALSE;
            goto READFILE;
        }
        
        if(CCChannel::tcpsend(our_socket,target,strlen(target),3)<=0)
        {
            closesocket(our_socket);
            goto READFILE;
        }
        
        int nLen = 0;
        int tryTimes=0;
        while(tryTimes<2)
        {
            int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
            if(rlen < 0 || (nLen+rlen)>1024)
            {
                break;
            }
            memcpy(&IPBuf[nLen], recvBuf, rlen);
            nLen += rlen;
            tryTimes++;
        }
        
        closesocket(our_socket);
        our_socket = 0;
        
        if(nLen == 0)
        {//‚àöa?‚ÄôŒºŒ©??‚à´?????Ôø°¬®‚àûÔø•‚Äì?????‚àû?√∑?‚Äì?????‚ÄúaÔø•?
            if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?,Ôø•Ôø•Œ©?web??Œ©‚Äù?√∑??‚àû?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
                }
                
                //return FALSE;
                goto READFILE;
            }
            
            LINGER linger2;
            linger2.l_onoff = 1;
            linger2.l_linger = 0;
            setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger2, sizeof(LINGER));
            
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            
            sin.sin_family = AF_INET;
            sin.sin_port = htons(nLocalPort);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
#ifndef WIN32
                sin.sin_addr.s_addr = INADDR_ANY;
#else
                sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
                
                sin.sin_family = AF_INET;
                sin.sin_port = htons(0);
                if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
                {
                    if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                    {
                        m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:‚àû???web??Œ©‚Äù?√∑??‚àû?", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                    }
                    
                    closesocket(our_socket);
                    //return FALSE;
                    goto READFILE;
                    
                }
            }
            
            if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:web?¬®Œ©‚Äù??‚àû?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                //return FALSE;
                goto READFILE;
            }
            
            if(CCChannel::tcpsend(our_socket,target_e,strlen(target_e),3)<=0)
            {
                closesocket(our_socket);
                goto READFILE;
            }
            
            tryTimes=0;
            while(tryTimes<2)
            {
                int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
                if(rlen < 0 || (nLen+rlen)>1024)
                {
                    break;
                }
                memcpy(&IPBuf[nLen], recvBuf, rlen);
                nLen += rlen;
                tryTimes++;
            }
        }
        
        if(our_socket > 0)
        {
            closesocket(our_socket);
        }
        
        //Œ©???Œº?√∑‚àë
        if(strlen(m_strDomainName) == 0)
        {//?œÄ‚Äù‚àöœÄ‚Ä≤√†?Œº?‚Äù√∫‚àö?
            char chIP[50] = {0};
            char chPort[50] = {0};
            //char chTmp[50]={0};
            int nStart = 0, nMid = 0, nEnd = 0;
            for(int i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P' && IPBuf[i+2] == ':')
                {//‚Äô‚ÄúŒºŒ©???o?a√∑‚àö
                    nStart = i+3;
                    nMid = nStart;
                    nEnd = nMid;
                    
                    for(int j=nStart; j<nLen; j++)
                    {
                        if(IPBuf[j] == ':')
                        {//‚Äô‚ÄúŒºŒ©‚àë√∑‚àè√π‚àë?,√∑?Ôø•√†IP?‚Ä¶Œ©?‚â•?
                            nMid = j+1;
                            nEnd = nMid;
                            
                            for(int k=nMid; k<nLen; k++)
                            {
                                if(IPBuf[k] < '0' || IPBuf[k] > '9')
                                {
                                    nEnd = k;
                                    break;
                                }
                                if(k==nLen-1)
                                {
                                    nEnd = k+1;
                                }
                            }
                            break;
                        }
                    }
                    
                    STSIP stIP;
                    memset(stIP.chIP, 0, 16);
                    
                    memcpy(chIP, &IPBuf[nStart], nMid-nStart-1);
                    memcpy(stIP.chIP, &IPBuf[nStart], nMid-nStart-1);//
                    memcpy(chPort, &IPBuf[nMid], nEnd-nMid);
                    chIP[nMid-nStart-1] = ':';
                    memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                    chPort[nEnd-nMid]='\0';
                    stIP.nPort = atoi(chPort);
                    if(i>0 && IPBuf[i-1] == 'D')
                    {
                        stIP.nISP = JVNC_ISP_DX;
                    }
                    else
                    {
                        stIP.nISP = JVNC_ISP_WT;//‚â§a??‚âà‚Äì??Œº?Œº??√®??‚Äúa‚àè‚âàœÄ√®???Àâ??
                    }
                    
                    if(stIP.nPort > 0)
                    {
                        stIPList.push_back(stIP);
                        nSerCount++;
                        
                        
                        {
                            STSERVER stserver;
#ifndef WIN32
                            stserver.addr.sin_addr.s_addr = inet_addr(stIP.chIP);
#else
                            stserver.addr.sin_addr.S_un.S_addr = inet_addr(stIP.chIP);
#endif
                            stserver.addr.sin_family = AF_INET;
                            stserver.addr.sin_port = htons(stIP.nPort);
                            stserver.buseful = TRUE;
                            stserver.nver = 0;
                            SList.push_back(stserver);//º«¬º
                            
                        }
                        
                    }
                    
                    if(strlen(chIP) > 0)
                    {
                        //o‚Äù‚àö?
                        for (unsigned int m=0; m<strlen(chIP); m++)
                        {
                            chIP[m] ^= m;
                        }
                        chIP[nEnd-nStart] = '\r';
                        chIP[nEnd-nStart+1] = '\n';
                        
                        if(pfile==NULL)
                        {
                            pfile = fopen(acBuff, "wb");
                        }
                        if(pfile != NULL)
                        {
                            fwrite(chIP,sizeof(char),nEnd-nStart+2,pfile);
                        }
                    }
                    
                    i = nEnd+1;
                }
                else if(IPBuf[i] == '\0')
                {
                    break;
                }
            }
        }
        else//?œÄ‚Äù‚àö ‚Äì?‚Äù√∫‚àö??¬± ‚Äì?‚Äú?‚Äô‚ÄúŒºŒ©o‚Äù‚àö?Œº?‚Äì‚âà?Ôø†Œ©?‚àö?
        {
            char chIP[50] = {0};
            char chPort[50] = {0};
            int nStart = 0, nMid = 0, nEnd = 0;
            int i=0;
            for(i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P'
                   && IPBuf[i+2] == '_'
                   && IPBuf[i+3] == 'S' && IPBuf[i+4] == 'T'
                   && IPBuf[i+5] == 'A' && IPBuf[i+6] == 'R'
                   && IPBuf[i+7] == 'T')//IP_START:
                {//‚Äô‚ÄúŒºŒ©???o?a√∑‚àö
                    nStart = i+9;
                    memmove(IPBuf,&IPBuf[nStart],nLen-nStart);
                    nLen-=nStart;
                    IPBuf[nLen] = '\0';
                    MultiMaskRemove(IPBuf,nLen);
                    break;
                }
            }
            for(i=0; i<nLen; i++)
            {
                memset(chIP, 0, 50);
                memset(chPort, 0, 50);
                if(IPBuf[i] == 'I' && IPBuf[i+1] == 'P' && IPBuf[i+2] == ':')
                {
                    nStart = i + 3;
                    nMid = nStart;
                    nEnd = nMid;
                    
                    for(int j=nStart; j<nLen; j++)
                    {
                        if(IPBuf[j] == ':')
                        {//‚Äô‚ÄúŒºŒ©‚àë√∑‚àè√π‚àë?,√∑?Ôø•√†IP?‚Ä¶Œ©?‚â•?
                            nMid = j+1;
                            nEnd = nMid;
                            
                            for(int k=nMid; k<nLen; k++)
                            {
                                if(IPBuf[k] < '0' || IPBuf[k] > '9')
                                {
                                    nEnd = k;
                                    break;
                                }
                                if(k==nLen-1)
                                {
                                    nEnd = k+1;
                                }
                            }
                            break;
                        }
                    }
                    
                    STSIP stIP;
                    memset(stIP.chIP, 0, 16);
                    
                    memcpy(chIP, &IPBuf[nStart], nMid-nStart-1);
                    memcpy(stIP.chIP, &IPBuf[nStart], nMid-nStart-1);//
                    memcpy(chPort, &IPBuf[nMid], nEnd-nMid);
                    chIP[nMid-nStart-1] = ':';
                    memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                    chPort[nEnd-nMid]='\0';
                    stIP.nPort = atoi(chPort);
                    
                    if(stIP.nPort > 0)
                    {
                        stIPList.push_back(stIP);
                        nSerCount++;
                    }
                    
                    if(strlen(chIP) > 0)
                    {
                        //o‚Äù‚àö?
                        for (unsigned int m=0; m<strlen(chIP); m++)
                        {
                            chIP[m] ^= m;
                        }
                        chIP[nEnd-nStart] = '\r';
                        chIP[nEnd-nStart+1] = '\n';
                        
                        if(pfile == NULL)
                        {
                            pfile = fopen(acBuff, "wb");
                        }
                        
                        if(pfile != NULL)
                        {
                            fwrite(chIP,sizeof(char),nEnd-nStart+2,pfile);
                        }
                    }
                    
                    i = nEnd;
                }
                else if(IPBuf[i] == '\0')
                {
                    break;
                }
            }
        }
        
        if(pfile != NULL)
        {
            fclose(pfile);
        }
    }
READFILE:
    if(nSerCount > 0)
    {
        STSERVER stserver;
        for (int i = 0;i<nSerCount;i++)
        {
#ifndef WIN32
            stserver.addr.sin_addr.s_addr = inet_addr(stIPList[i].chIP);
#else
            stserver.addr.sin_addr.S_un.S_addr = inet_addr(stIPList[i].chIP);
#endif
            stserver.addr.sin_family = AF_INET;
            stserver.addr.sin_port = htons(stIPList[i].nPort);
            stserver.buseful = TRUE;
            stserver.nver = 0;
            SList.push_back(stserver);//o??o
        }
    }
    else
    {//Ôø•‚Äù¬±?Œº????¬∞‚àë??√≤??
        ::std::string line;
        ::std::ifstream localfile(acBuff);
        while(::std::getline(localfile,line))
        {
            memset(acBuff, 0, MAX_PATH);
            line.copy(acBuff,MAX_PATH,0);
            //‚àë??√≤??Œº?√∑‚àëŒ©?‚àö?
            for (unsigned int m=0; m<strlen(acBuff); m++)
            {
                acBuff[m] ^= m;
            }
            STSIP stIP;
            memset(stIP.chIP, 0, 16);
            int i=0;
            char chPort[16] = {0};
            for(i=0; i<MAX_PATH; i++)
            {
                if(acBuff[i] == ':' || acBuff[i] == '\0')
                {
                    break;
                }
            }
            if(i > 0)
            {
                memcpy(stIP.chIP, acBuff, i);
                memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i+1);
            }
            
            stIP.nPort = atoi(chPort);
            if(stIP.nPort > 0)
            {
                stIPList.push_back(stIP);
                nSerCount++;
            }
        }
        
        if(nSerCount>0)
        {
            STSERVER stserver;
            for (int i = 0;i<nSerCount;i++)
            {
#ifndef WIN32
                stserver.addr.sin_addr.s_addr = inet_addr(stIPList[i].chIP);
#else
                stserver.addr.sin_addr.S_un.S_addr = inet_addr(stIPList[i].chIP);
#endif
                stserver.addr.sin_family = AF_INET;
                stserver.addr.sin_port = htons(stIPList[i].nPort);
                stserver.buseful = TRUE;
                stserver.nver = 0;
                SList.push_back(stserver);//o??o
            }
        }
        else
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"a√≤?¬∞‚àë??√≤????‚àû?.‚Äò‚â†‚Äú√∫:‚àë??√≤???‚Äì¬±√¨???‚Äô", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:no server is find.", __FILE__,__LINE__);
            }
            
            return FALSE;
        }
    }
    
    return TRUE;
}

//÷±Ω”?¨Ω”??μ?∑??ò
void CCWorker::ConnectServerDirect(int nLocalChannel,
                                   int nChannel,
                                   char *pchServerIP,
                                   int nServerPort,
                                   char *pPassName,
                                   char *pPassWord,
                                   BOOL bCache,
                                   int nConnectType,
                                   BOOL isBeRequestVedio
								   ,int nOnlyTCP
								   )
{
    //deleteLog();
    writeLog("************************ConnectServerDirect nLocalChannel: %d,nChannel:%d, ip: %s,port: %d,name: %s,pwd: %s,nConnectType: %d\n",nLocalChannel,nChannel,pchServerIP,nServerPort,pPassName,pPassWord,nConnectType);
    OutputDebug("ConnectServerDirect nLocalChannel: %d,nChannel:%d, ip: %s,port: %d,name: %s,pwd: %s,nConnectType: %d\n",nLocalChannel,nChannel,pchServerIP,nServerPort,pPassName,pPassWord,nConnectType);

#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            delete m_pChannels[i];
            m_pChannels[i] = NULL;
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
            m_pChannels[i]->SendData(JVN_CMD_VIDEO, NULL, 0);
            
#ifndef WIN32
            pthread_mutex_unlock(&m_criticalsection);
#else
            LeaveCriticalSection(&m_criticalsection);
#endif
            
            ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_RECONN, NULL,0,__FILE__,__LINE__,__FUNCTION__);
            
            return;
        }
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    STCONNECTINFO stConninfo;
    stConninfo.nOnlyTCP =nOnlyTCP;
    stConninfo.isBeRequestVedio = isBeRequestVedio;
    
    stConninfo.bYST = FALSE;
    stConninfo.nLocalChannel = nLocalChannel;
    
    stConninfo.nChannel = nChannel;
    
    stConninfo.nYSTNO = 0;
    
    memset(stConninfo.chServerIP, 0, 16);
    memcpy(stConninfo.chServerIP, pchServerIP, strlen(pchServerIP));
    
    stConninfo.nServerPort = nServerPort;
    
    memset(stConninfo.chPassName, 0, MAX_PATH);
    memcpy(stConninfo.chPassName, pPassName, strlen(pPassName));
    
    memset(stConninfo.chPassWord, 0, MAX_PATH);
    memcpy(stConninfo.chPassWord, pPassWord, strlen(pPassWord));
    
    stConninfo.nTURNType = JVN_TRYTURN;
    stConninfo.nConnectType = nConnectType;
    stConninfo.nLocalPort = m_nLocalStartPort;//nLocalPort;
    
    stConninfo.bCache = bCache;
    
    stConninfo.nWhoAmI = JVN_WHO_P;//?“????a??à
    
    if(nConnectType == TYPE_MO_UDP || nConnectType == TYPE_3GMO_UDP || nConnectType == TYPE_3GMOHOME_UDP)
    {
        stConninfo.nWhoAmI = JVN_WHO_M;
    }
    
    
    CCChannel *p = new CCChannel(stConninfo, this);
    if(p == NULL)
    {
        
        //		m_connectctrl.DeletePort(nLocalPort);//???ì?à??o??o
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            char chMsg[] = "??μ??‘??￥￥Ω???∞?!";
            ConnectChange(stConninfo.nLocalChannel, JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            m_Log.SetRunInfo(stConninfo.nLocalChannel, "?÷”ú?ˉ∑Ω?Ω?¨Ω”÷?????∞?.‘≠“ú:￥￥Ω???μ??‘????∞?", __FILE__,__LINE__);
        }
        else
        {
            char chMsg[] = "create channel object failed!";
            ConnectChange(stConninfo.nLocalChannel, JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            m_Log.SetRunInfo(stConninfo.nLocalChannel, "Local connect failed.Info:create channel object failed.", __FILE__,__LINE__);
        }
        
        return;
    }
    
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    if(m_pLanSerch)
    {
        m_pLanSerch->m_bPause[nLocalChannel] = TRUE;
    }
    m_pChannels.push_back(p);
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    return;
}

//??π?YST∫≈???¨Ω”∑??ò
void CCWorker::ConnectServerByYST(int nLocalChannel,
                                  int nChannel,
                                  int nYSTNO,
                                  char chGroup[4],
                                  char *pPassName,
                                  char *pPassWord,
                                  BOOL bLocalTry,
                                  int nTURNType,
                                  BOOL bCache,
                                  int nConnectType,
                                  BOOL isBeRequestVedio,int nVIP)
{
    //deleteLog();
    writeLog("************************ConnectServerByYST nLocalChannel: %d,nChannel:%d, yst: %d,group: %s,name: %s,pwd: %s,nConnectType: %d, isBeRequestVedio:%d, nVIP: %d\n",nLocalChannel,nChannel,nYSTNO,chGroup,pPassName,pPassWord,nConnectType, isBeRequestVedio,nVIP);
    printf("ConnectServerByYST nLocalChannel: %d,nChannel:%d, yst: %d,group: %s,name: %s,pwd: %s,nConnectType: %d, isBeRequestVedio:%d, nVIP: %d\n",nLocalChannel,nChannel,nYSTNO,chGroup,pPassName,pPassWord,nConnectType, isBeRequestVedio,nVIP);
    if(JVN_ONLYTURN != nTURNType)
    {
        if(m_pLanSerch)
        {
            m_Helper.AddYST(chGroup,nYSTNO,m_pLanSerch->m_bNum,m_pLanSerch->m_bisMobile);
        }
        else
        {
            m_Helper.AddYST(chGroup,nYSTNO,0,FALSE);
        }
    }
    
    //nTURNType = JVN_ONLYTURN;//‘?Ω′TURNΩ?”√
   
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            delete m_pChannels[i];
            m_pChannels[i] = NULL;
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
            m_pChannels[i]->SendData(JVN_CMD_VIDEO, NULL, 0);
            
#ifndef WIN32
            pthread_mutex_unlock(&m_criticalsection);
#else
            LeaveCriticalSection(&m_criticalsection);
#endif
            
            ConnectChange(nLocalChannel, JVN_CCONNECTTYPE_RECONN,NULL,0,__FILE__,__LINE__,__FUNCTION__);
            
            return;
        }
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    STCONNECTINFO stConninfo;
    stConninfo.isBeRequestVedio = isBeRequestVedio;
    stConninfo.bYST = TRUE;
    stConninfo.nLocalChannel = nLocalChannel;
    
    stConninfo.nChannel = nChannel;
    
    stConninfo.nYSTNO = nYSTNO;
    
    stConninfo.nVIP = nVIP;
    memset(stConninfo.chGroup, 0, 4);
    memcpy(stConninfo.chGroup, chGroup, strlen(chGroup));
    
    stConninfo.bLocalTry = bLocalTry;
    
    memset(stConninfo.chPassName, 0, MAX_PATH);
    memcpy(stConninfo.chPassName, pPassName, strlen(pPassName));
    
    memset(stConninfo.chPassWord, 0, MAX_PATH);
    memcpy(stConninfo.chPassWord, pPassWord, strlen(pPassWord));
    
    if(nTURNType != JVN_NOTURN && nTURNType != JVN_TRYTURN && nTURNType != JVN_ONLYTURN)
    {
        nTURNType = JVN_TRYTURN;
    }
    stConninfo.nTURNType = nTURNType;
    stConninfo.nConnectType = nConnectType;
    
    stConninfo.nLocalPort = m_nLocalStartPort;//nLocalPort;
    
    stConninfo.bCache = bCache;
    
    stConninfo.nWhoAmI = JVN_WHO_P;
    if(m_pHelpCtrl != NULL)
    {
        stConninfo.nWhoAmI = m_pHelpCtrl->m_nHelpType;
    }
    else
    {
        if(nConnectType == TYPE_MO_UDP || nConnectType == TYPE_3GMO_UDP || nConnectType == TYPE_3GMOHOME_UDP)
        {
            stConninfo.nWhoAmI = JVN_WHO_M;
        }
    }
    
    CCChannel *p = new CCChannel(stConninfo, this);
    if(p == NULL)
    {
        //		m_connectctrl.DeletePort(nLocalPort);//???ì?à??o??o
        
        if(JVN_LANGUAGE_CHINESE ==m_nLanguage)
        {
            char chMsg[] = "??μ??‘??￥￥Ω???∞?!";
            ConnectChange(stConninfo.nLocalChannel, JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            m_Log.SetRunInfo(stConninfo.nLocalChannel, "YST∑Ω?Ω?¨Ω”÷?????∞?.‘≠“ú:￥￥Ω???μ??‘????∞?", __FILE__,__LINE__);
        }
        else
        {
            char chMsg[] = "create channel object failed!";
            ConnectChange(stConninfo.nLocalChannel, JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            m_Log.SetRunInfo(stConninfo.nLocalChannel, "YST connect failed.Info:create channel object failed.", __FILE__,__LINE__);
        }
        
        return;
    }
    
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    if(m_pLanSerch)
    {
        m_pLanSerch->m_bPause[nLocalChannel] = TRUE;
    }
    m_pChannels.push_back(p);
    
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    return;
}

//a?∞è–≈?￠
void CCWorker::GetPartnerInfo(int nLocalChannel, char *pMsg, int &nSize)
{
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”
            m_pChannels[i]->GetPartnerInfo(pMsg, nSize);
            break;
        }
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
}

#ifndef WIN32
void* CCWorker::PTListenProc(void* pParam)
#else
UINT WINAPI CCWorker::PTListenProc(LPVOID pParam)
#endif
{
    CCWorker *pWorker = (CCWorker *)pParam;
    
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
    
    ::WaitForSingleObject(pWorker->m_hPTListenStartEvent, INFINITE);
    if(pWorker->m_hPTListenStartEvent > 0)
    {
        CloseHandle(pWorker->m_hPTListenStartEvent);
        pWorker->m_hPTListenStartEvent = 0;
    }
#endif
    
    //UDP............................................
    //UDT:o???,
    if (UDT::ERROR == UDT::listen(pWorker->m_WorkerUDTSocket, 30))
    {
#ifdef WIN32
        return 0;
#else
        return NULL;
#endif
    }
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(SOCKADDR_IN);
    UDTSOCKET clientsock;
    LINGER linger;
    //////////////////////////////////////////////////////////////////////////
    
    int ntcpcount=0;
    int nTCPLinkID = 0;
    ClientTmpList tcpclients;
    tcpclients.reserve(100);
    tcpclients.clear();
    
    int iSockStatus = 0;
    //	unsigned long ulBlock = 1;
    
    DWORD dwendtime = 0;
    BYTE uchBuf[10240] = {0};
    int nLenRead = 0;
    int nLenTemp = 0;
    
    while(TRUE)
    {
#ifndef WIN32
        if(pWorker->m_bPTListenEnd)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hPTListenEndEvent, 0))
        {
            if(pWorker->m_hPTListenEndEvent > 0)
            {
                CloseHandle(pWorker->m_hPTListenEndEvent);
                pWorker->m_hPTListenEndEvent = 0;
            }
            
            break;
        }
#endif
        
        BOOL bAcceptOne = FALSE;
        addrlen = sizeof(SOCKADDR_IN);
        //------------------------------------------------------------------------------
        //μ?￥?Ω”??a?∞èμ?UDP?¨Ω”
        //------------------------------------------------------------------------------
        if (UDT::INVALID_SOCK != (clientsock = UDT::accept(pWorker->m_WorkerUDTSocket, (sockaddr*)&clientaddr, &addrlen)))
        {
            bAcceptOne = TRUE;
            //Ω′??Ω”?÷÷√??∑??????￡?Ω
            BOOL block = FALSE;
            UDT::setsockopt(clientsock, 0, UDT_RCVSYN, &block, sizeof(BOOL));
            UDT::setsockopt(clientsock, 0, UDT_SNDSYN, &block, sizeof(BOOL));
            linger.l_onoff = 0;
            linger.l_linger = 0;
            UDT::setsockopt(clientsock, 0, UDT_LINGER, &linger, sizeof(LINGER));
            
            int nD = UDT::getschannel(clientsock);//aò?°∑￠???¨Ω”’?à?‘?÷???…?μ?id,∏√id‘??¨“a∏?÷???…???“a￡¨??÷???”–?…??÷?∏￥
            int nYST = UDT::getptystno(clientsock);//aò?°÷???∫≈??
            int nADD = UDT::getptystaddr(clientsock);//aò?°÷????¨Ω”μ?÷∑
            int nAID = UDT::getptystid(clientsock);//aò?°?¨Ω”??±íà?‘?÷???…?μ?id
            
            if(nAID <= 0 || (nYST <= 0 && nADD <= 0))
            {//?…∞ê??≤?SDKa?∞è?¨Ω” Ω?”√
                UDT::close(clientsock);
                jvc_sleep(5);
                continue;
            }
            
#ifndef WIN32
            pthread_mutex_lock(&pWorker->m_criticalsection);
#else
            EnterCriticalSection(&pWorker->m_criticalsection);
#endif
            int nCount = pWorker->m_pChannels.size();
            for(int i=0; i<nCount; i++)
            {
                if(pWorker->m_pChannels[i] == NULL)
                {//??–?o??o
                    continue;
                }
                if(pWorker->m_pChannels[i]->m_nStatus != OK || !pWorker->m_pChannels[i]->m_bJVP2P)//?…–≠“è
                {//??–?o??o
                    continue;
                }
                
                if(pWorker->m_pChannels[i]->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                    //char ch[100]={0};
                    //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                    //OutputDebugString(ch);
                    if(nYST > 0)
                    {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                        if(nYST == pWorker->m_pChannels[i]->m_stConnInfo.nYSTNO)
                        {//??±í???¨￡¨??a?∞èπ??μ
                            //OutputDebugString("ptlistenproc nyst==...........\n");
                            pWorker->m_pChannels[i]->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                            break;
                        }
                    }
                    else
                    {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                        if(nADD == pWorker->m_pChannels[i]->m_addressA.sin_addr.s_addr)
                        {//??±í???¨￡¨??a?∞èπ??μ
                            //char ch[100]={0};
                            //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                            //OutputDebugString(ch);
                            pWorker->m_pChannels[i]->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                            break;
                        }
                    }
                }
            }
#ifndef WIN32
            pthread_mutex_unlock(&pWorker->m_criticalsection);
#else
            LeaveCriticalSection(&pWorker->m_criticalsection);
#endif
            
            if(pWorker->m_pHelpCtrl && pWorker->m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
            {//÷˙?÷
                CCHelpCtrlH* pHelpH = (CCHelpCtrlH* )pWorker->m_pHelpCtrl;
#ifndef WIN32
                pthread_mutex_lock(&pHelpH->m_criticalsectionHList);
#else
                EnterCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
                for(DATA_LIST::iterator j = pHelpH->m_TcpListDataH.begin(); j != pHelpH->m_TcpListDataH.end(); ++ j)
                {
                    PLOCAL_TCP_DATA pData = j->second;
                    
                    if(pData == NULL || pData->pChannel == NULL)
                    {//??–?o??o
                        continue;
                    }
                    if(pData->pChannel->m_nStatus != OK || !pData->pChannel->m_bJVP2P)//?…–≠“è
                    {//??–?o??o
                        continue;
                    }
                    
                    if(pData->pChannel->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                    {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                        //char ch[100]={0};
                        //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                        //OutputDebugString(ch);
                        if(nYST > 0)
                        {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                            if(nYST == pData->pChannel->m_stConnInfo.nYSTNO)
                            {//??±í???¨￡¨??a?∞èπ??μ
                                //OutputDebugString("ptlistenproc nyst==...........\n");
                                
                                pData->pChannel->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                break;
                            }
                        }
                        else
                        {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                            if(nADD == pData->pChannel->m_addressA.sin_addr.s_addr)
                            {//??±í???¨￡¨??a?∞èπ??μ
                                //char ch[100]={0};
                                //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                                //OutputDebugString(ch);
                                pData->pChannel->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                break;
                            }
                        }
                    }
                }
#ifndef WIN32
                pthread_mutex_unlock(&pHelpH->m_criticalsectionHList);
#else
                LeaveCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
            }
        }
        if(!bAcceptOne)//π????à??√a”–?’μΩ –??à??Ω”?’
        {
            SOCKADDR_IN clientaddr;
            int addrlen = sizeof(SOCKADDR_IN);
            
            CLocker lock(pWorker->m_MakeHoleGroup.m_MakeLock,__FILE__,__LINE__);
            
            for(CONN_List::iterator k = pWorker->m_MakeHoleGroup.m_ConnectList.begin(); k != pWorker->m_MakeHoleGroup.m_ConnectList.end(); ++ k)
            {
                CMakeHoleC* pMakeHole = (CMakeHoleC* )k->second;
                
                if(pMakeHole->m_udtSocket <= 0)//√a”–?ù??o???
                    continue;
                
                if (UDT::INVALID_SOCK != (clientsock = UDT::accept(pMakeHole->m_udtSocket, (sockaddr*)&clientaddr, &addrlen)))
                {
                    bAcceptOne = TRUE;
                    //Ω′??Ω”?÷÷√??∑??????￡?Ω
                    BOOL block = FALSE;
                    UDT::setsockopt(clientsock, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                    UDT::setsockopt(clientsock, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                    linger.l_onoff = 0;
                    linger.l_linger = 0;
                    UDT::setsockopt(clientsock, 0, UDT_LINGER, &linger, sizeof(LINGER));
                    
                    int nD = UDT::getschannel(clientsock);//aò?°∑￠???¨Ω”’?à?‘?÷???…?μ?id,∏√id‘??¨“a∏?÷???…???“a￡¨??÷???”–?…??÷?∏￥
                    int nYST = UDT::getptystno(clientsock);//aò?°÷???∫≈??
                    int nADD = UDT::getptystaddr(clientsock);//aò?°÷????¨Ω”μ?÷∑
                    int nAID = UDT::getptystid(clientsock);//aò?°?¨Ω”??±íà?‘?÷???…?μ?id
                    
                    if(nAID <= 0 || (nYST <= 0 && nADD <= 0))
                    {//?…∞ê??≤?SDKa?∞è?¨Ω” Ω?”√
                        UDT::close(clientsock);
                        jvc_sleep(5);
                        continue;
                    }
                    
#ifndef WIN32
                    pthread_mutex_lock(&pWorker->m_criticalsection);
#else
                    EnterCriticalSection(&pWorker->m_criticalsection);
#endif
                    int nCount = pWorker->m_pChannels.size();
                    for(int i=0; i<nCount; i++)
                    {
                        if(pWorker->m_pChannels[i] == NULL)
                        {//??–?o??o
                            continue;
                        }
                        if(pWorker->m_pChannels[i]->m_nStatus != OK || !pWorker->m_pChannels[i]->m_bJVP2P)//?…–≠“è
                        {//??–?o??o
                            continue;
                        }
                        
                        if(pWorker->m_pChannels[i]->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                        {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                            //char ch[100]={0};
                            //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                            //OutputDebugString(ch);
                            if(nYST > 0)
                            {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                                if(nYST == pWorker->m_pChannels[i]->m_stConnInfo.nYSTNO)
                                {//??±í???¨￡¨??a?∞èπ??μ
                                    //OutputDebugString("ptlistenproc nyst==...........\n");
                                    pWorker->m_pChannels[i]->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                    break;
                                }
                            }
                            else
                            {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                                if(nADD == pWorker->m_pChannels[i]->m_addressA.sin_addr.s_addr)
                                {//??±í???¨￡¨??a?∞èπ??μ
                                    //char ch[100]={0};
                                    //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                                    //OutputDebugString(ch);
                                    pWorker->m_pChannels[i]->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                    break;
                                }
                            }
                        }
                    }
#ifndef WIN32
                    pthread_mutex_unlock(&pWorker->m_criticalsection);
#else
                    LeaveCriticalSection(&pWorker->m_criticalsection);
#endif
                    
                    if(pWorker->m_pHelpCtrl && pWorker->m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
                    {//÷˙?÷
                        CCHelpCtrlH* pHelpH = (CCHelpCtrlH* )pWorker->m_pHelpCtrl;
#ifndef WIN32
                        pthread_mutex_lock(&pHelpH->m_criticalsectionHList);
#else
                        EnterCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
                        for(DATA_LIST::iterator j = pHelpH->m_TcpListDataH.begin(); j != pHelpH->m_TcpListDataH.end(); ++ j)
                        {
                            PLOCAL_TCP_DATA pData = j->second;
                            
                            if(pData == NULL || pData->pChannel == NULL)
                            {//??–?o??o
                                continue;
                            }
                            if(pData->pChannel->m_nStatus != OK || !pData->pChannel->m_bJVP2P)//?…–≠“è
                            {//??–?o??o
                                continue;
                            }
                            
                            if(pData->pChannel->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                            {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                                //char ch[100]={0};
                                //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                                //OutputDebugString(ch);
                                if(nYST > 0)
                                {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                                    if(nYST == pData->pChannel->m_stConnInfo.nYSTNO)
                                    {//??±í???¨￡¨??a?∞èπ??μ
                                        //OutputDebugString("ptlistenproc nyst==...........\n");
                                        
                                        pData->pChannel->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                        break;
                                    }
                                }
                                else
                                {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                                    if(nADD == pData->pChannel->m_addressA.sin_addr.s_addr)
                                    {//??±í???¨￡¨??a?∞èπ??μ
                                        //char ch[100]={0};
                                        //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                                        //OutputDebugString(ch);
                                        pData->pChannel->m_PartnerCtrl.AcceptPartner(clientsock, clientaddr, nD);
                                        break;
                                    }
                                }
                            }
                        }
#ifndef WIN32
                        pthread_mutex_unlock(&pHelpH->m_criticalsectionHList);
#else
                        LeaveCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
                    }
                    
                    //					OutputDebug("?’μΩ?¨Ω”...........local port %X %d  connect num = %d",pAcceptHole,pAcceptHole->m_wLocalPort,pAcceptHole->m_nConnectNum);
                    break;
                }
            }
        }
        /*		if(!bAcceptOne)
         {
         //???ì≥§?±o‰√a”–?¨Ω”μ?o??? ∫? ?¨Ω”?′≤?????μ?
         #ifndef WIN32
         pthread_mutex_lock(&pWorker->m_MakeHoleGroup.m_MakeLock);
         #else
         EnterCriticalSection(&pWorker->m_MakeHoleGroup.m_MakeLock);
         #endif
         DWORD dwTime = GetTime();
         int nSize = pWorker->m_MakeHoleGroup.m_ConnectList.size();
         for(int m = 0;m < nSize;m ++)
         {
         CMakeHoleC* pMakeHole = pWorker->m_MakeHoleGroup.m_ConnectList[m];
         
         // “—?≠π?±’     √a”–?¨Ω”π?≤￠≥¨?±μ?
         if((pMakeHole->m_sLocalSocket <= 0) || (pMakeHole->m_dwTime > 0 && dwTime - pMakeHole->m_dwTime > 60 * 1000))
         {
         pWorker->m_MakeHoleList.erase(pWorker->m_MakeHoleList.begin() + m);
         delete pMakeHole;
         
         break;
         }
         }
         #ifndef WIN32
         pthread_mutex_unlock(&pWorker->m_MakeHoleGroup.m_MakeLock);
         #else
         LeaveCriticalSection(&pWorker->m_MakeHoleGroup.m_MakeLock);
         #endif
         CSWorker::jvs_sleep(5);
         continue;
         }*/
        //------------------------------------------------------------------------------
        //μ?￥?Ω”??a?∞èμ?TCP?¨Ω”
        //------------------------------------------------------------------------------
        if(pWorker->m_WorkerTCPSocket > 0)
        {
#ifndef WIN32
            if ((clientsock = accept(pWorker->m_WorkerTCPSocket, (sockaddr*)&clientaddr, (socklen_t*)&addrlen)) > 0)
#else
                if ((clientsock = accept(pWorker->m_WorkerTCPSocket, (sockaddr*)&clientaddr, &addrlen)) > 0)
#endif
                {
                    //OutputDebugString("accept tcp..............\n");
                    STCLIENTTMP tcpclient;
                    tcpclient.sock = clientsock;
                    memcpy(&tcpclient.addr, &clientaddr, sizeof(SOCKADDR_IN));
                    tcpclient.dwStartTime = CCWorker::JVGetTime();
                    tcpclients.push_back(tcpclient);
                    
                    int nSetSize = 128*1024;
                    setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
                    nSetSize = 128*1024;
                    setsockopt(clientsock, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));
                    
                    // ≤a?π”√Nagleà?∑?
                    //				BOOL bNoDelay = TRUE;
                    //				setsockopt(clientsock, SOL_SOCKET, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));
                    
                    //Ω′??Ω”?÷÷√??≤aμ?￥??￥￥??ì?íμ?????
                    LINGER linger;
                    linger.l_onoff = 1;//0;
                    linger.l_linger = 0;
                    iSockStatus = setsockopt(clientsock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
                }
            
            //±è?˙Ω”?’∞??¨Ω”????
            ntcpcount = tcpclients.size();
            dwendtime = CCWorker::JVGetTime();
            for(int i=0; i<ntcpcount; i++)
            {
                if(tcpclients[i].sock <= 0)
                {
                    //OutputDebugString("tcprecv sock<0 erase\n");
                    tcpclients.erase(tcpclients.begin() + i);
                    i--;
                    ntcpcount--;
                    continue;
                }
                if(dwendtime > tcpclients[i].dwStartTime + JVN_TIME_WAITLRECHECK)
                {
                    //OutputDebugString("tcprecv timeout erase\n");
                    tcpclients.erase(tcpclients.begin() + i);
                    i--;
                    ntcpcount--;
                    continue;
                }
                
                //???oΩ”?’??μ?o∞?¨Ω”??–?????
                memset(uchBuf, 0, 30);
                nLenRead = CCPartnerCtrl::tcpreceive(tcpclients[i].sock,(char *)uchBuf, 21,1);//9
                if( nLenRead == 9)
                {//Ω”?’ ??–?+≥§??+∑￠???¨Ω”μ?a?∞èID + PTLinkID+∫≈??+ip
                    //////////////////////////////////////////////////////////////////////////
                    //?…∞ê??≤?SDKa?∞è?¨Ω” Ω?”√
                    closesocket(tcpclients[i].sock);
                    tcpclients[i].sock = 0;
                    tcpclients.erase(tcpclients.begin() + i);
                    i--;
                    ntcpcount--;
                    jvc_sleep(5);
                    continue;
                    //////////////////////////////////////////////////////////////////////////
                }
                else if(nLenRead == 21)
                {
                    memcpy(&nLenTemp, &uchBuf[1], 4);
                    memcpy(&nTCPLinkID, &uchBuf[5], 4);
                    int nAID = 0, nYST = 0;
                    unsigned int nADD = 0;
                    memcpy(&nAID, &uchBuf[9], 4);
                    memcpy(&nYST, &uchBuf[13], 4);
                    memcpy(&nADD, &uchBuf[17], 4);
                    if(nAID <= 0 || (nYST <= 0 && nADD <= 0) || uchBuf[0] != JVN_CMD_TCP || nLenTemp != 16)
                    {//?…∞ê??≤?SDKa?∞è?¨Ω” Ω?”√
                        closesocket(tcpclients[i].sock);
                        tcpclients[i].sock = 0;
                        tcpclients.erase(tcpclients.begin() + i);
                        i--;
                        ntcpcount--;
                        jvc_sleep(5);
                        continue;
                    }
#ifndef WIN32
                    pthread_mutex_lock(&pWorker->m_criticalsection);
#else
                    EnterCriticalSection(&pWorker->m_criticalsection);
#endif
                    int nCount = pWorker->m_pChannels.size();
                    for(int j=0; j<nCount; j++)
                    {
                        if(pWorker->m_pChannels[j] == NULL)
                        {//??–?o??o
                            continue;
                        }
                        if(pWorker->m_pChannels[j]->m_nStatus != OK || !pWorker->m_pChannels[j]->m_bJVP2P)//?…–≠“è
                        {//??–?o??o
                            continue;
                        }
                        
                        if(pWorker->m_pChannels[j]->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                        {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                            //char ch[100]={0};
                            //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                            //OutputDebugString(ch);
                            if(nYST > 0)
                            {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                                if(nYST == pWorker->m_pChannels[j]->m_stConnInfo.nYSTNO)
                                {//??±í???¨￡¨??a?∞èπ??μ
                                    //OutputDebugString("ptlistenproc nyst==...........\n");
                                    pWorker->m_pChannels[j]->m_PartnerCtrl.AcceptPartner(tcpclients[i].sock, tcpclients[i].addr, nTCPLinkID, TRUE);
                                    tcpclients.erase(tcpclients.begin() + i);
                                    i--;
                                    ntcpcount--;
                                    break;
                                }
                            }
                            else
                            {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                                if(nADD == pWorker->m_pChannels[j]->m_addressA.sin_addr.s_addr)
                                {//??±í???¨￡¨??a?∞èπ??μ
                                    //char ch[100]={0};
                                    //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                                    //OutputDebugString(ch);
                                    pWorker->m_pChannels[j]->m_PartnerCtrl.AcceptPartner(tcpclients[i].sock, tcpclients[i].addr, nTCPLinkID, TRUE);
                                    tcpclients.erase(tcpclients.begin() + i);
                                    i--;
                                    ntcpcount--;
                                    break;
                                }
                            }
                        }
                    }
#ifndef WIN32
                    pthread_mutex_unlock(&pWorker->m_criticalsection);
#else
                    LeaveCriticalSection(&pWorker->m_criticalsection);
#endif
                    
                    if(pWorker->m_pHelpCtrl && pWorker->m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
                    {
                        CCHelpCtrlH* pHelpH = (CCHelpCtrlH* )pWorker->m_pHelpCtrl;
#ifndef WIN32
                        pthread_mutex_lock(&pHelpH->m_criticalsectionHList);
#else
                        EnterCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
                        for(DATA_LIST::iterator j = pHelpH->m_TcpListDataH.begin(); j != pHelpH->m_TcpListDataH.end(); ++ j)
                        {
                            PLOCAL_TCP_DATA pData = j->second;
                            
                            if(pData == NULL || pData->pChannel == NULL)
                            {//??–?o??o
                                continue;
                            }
                            if(pData->pChannel->m_nStatus != OK || !pData->pChannel->m_bJVP2P)//?…–≠“è
                            {//??–?o??o
                                continue;
                            }
                            
                            if(pData->pChannel->m_nLinkID == nAID)//?‘∑Ω?¨Ω”μ???±í“?∫?±?μ?“a÷?
                            {//∏√??μ?“—Ωˉ––π??¨Ω”￡¨?·??“—”–?¨Ω”
                                //char ch[100]={0};
                                //sprintf(ch,"[%d]ptlistenproc nd==nlinkid=%d............\n",pWorker->m_pChannels[i]->m_nLocalChannel,nAID);
                                //OutputDebugString(ch);
                                if(nYST > 0)
                                {//”–∫≈??‘ú≈–??∫≈??o￥?…?∑????“a÷???
                                    if(nYST == pData->pChannel->m_stConnInfo.nYSTNO)
                                    {//??±í???¨￡¨??a?∞èπ??μ
                                        //OutputDebugString("ptlistenproc nyst==...........\n");
                                        pData->pChannel->m_PartnerCtrl.AcceptPartner(tcpclients[i].sock, tcpclients[i].addr, nTCPLinkID, TRUE);
                                        tcpclients.erase(tcpclients.begin() + i);
                                        i--;
                                        ntcpcount--;
                                        break;
                                    }
                                }
                                else
                                {//√a”–∫≈??￡¨?…????“‘ip∑Ω?Ω?¨Ω”￡¨≈–??ip?∑????“a÷???
                                    if(nADD == pData->pChannel->m_addressA.sin_addr.s_addr)
                                    {//??±í???¨￡¨??a?∞èπ??μ
                                        //char ch[100]={0};
                                        //sprintf(ch,"[%d]ptlistenproc nadd==...........\n",pWorker->m_pChannels[i]->m_nLocalChannel);
                                        //OutputDebugString(ch);
                                        pData->pChannel->m_PartnerCtrl.AcceptPartner(tcpclients[i].sock, tcpclients[i].addr, nTCPLinkID, TRUE);
                                        tcpclients.erase(tcpclients.begin() + i);
                                        i--;
                                        ntcpcount--;
                                        break;
                                    }
                                }
                            }
                        }
#ifndef WIN32
                        pthread_mutex_unlock(&pHelpH->m_criticalsectionHList);
#else
                        LeaveCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
                    }
                }
                else if(nLenRead > 0)
                {//?¨Ω”???￠??–? ???˙∏√?￥Ω”(∏√￥?“ú??????–°￡¨‘?????“a￥????’?′)
                    //OutputDebugString("tcprecv recverr\n");
                    closesocket(tcpclients[i].sock);
                    tcpclients[i].sock = 0;
                    tcpclients.erase(tcpclients.begin() + i);
                    i--;
                    ntcpcount--;
                    jvc_sleep(5);
                    continue;
                }
            }
        }
        
        jvc_sleep(5);
    }
    
    ntcpcount=tcpclients.size();
    for(int i=0; i<ntcpcount; i++)
    {
        if(tcpclients[i].sock > 0)
        {
            closesocket(tcpclients[i].sock);
        }
    }
    tcpclients.clear();
    
#ifdef WIN32
    return 0;
#else
    return NULL;
#endif
}

#ifndef WIN32
void* CCWorker::GTProc(void* pParam)
#else
UINT WINAPI CCWorker::GTProc(LPVOID pParam)
#endif
{
    CCWorker *pWorker = (CCWorker *)pParam;
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
    
    ::WaitForSingleObject(pWorker->m_hGTStartEvent, INFINITE);
    if(pWorker->m_hGTStartEvent > 0)
    {
        CloseHandle(pWorker->m_hGTStartEvent);
        pWorker->m_hGTStartEvent = 0;
    }
    
    //…?÷√?à??”≥…‰
    //	pWorker->SetPortMapping();
#endif
    if(!pWorker->GetDemoList(1))
    {
        pWorker->GetDemoList(2);
    }
    int l = 0;
    for(l=0; l<pWorker->m_nGroupCount; l++)
    {
        std::vector<STSIP> IPList;
        
        if(!pWorker->IndexServerList_Download(pWorker->m_chGroupList[l], IPList, 1, 0))
        {
            if(pWorker->IndexServerList_Download(pWorker->m_chGroupList[l], IPList, 2, 0))
            {
                pWorker->IndexServerList_Save(pWorker->m_chGroupList[l], IPList);
                STGROUP stg;
                memcpy(stg.chgroup, pWorker->m_chGroupList[l], 4);
                pWorker->m_IndexGroupList.push_back(stg);
                //				OutputDebug("Download index server %s.%d",pWorker->m_chGroupList[l],__LINE__);
            }
        }
        else
        {
            pWorker->IndexServerList_Save(pWorker->m_chGroupList[l], IPList);
            STGROUP stg;
            memcpy(stg.chgroup, pWorker->m_chGroupList[l], 4);
            pWorker->m_IndexGroupList.push_back(stg);
            //			OutputDebug("Download index server %s.%d",pWorker->m_chGroupList[l],__LINE__);
        }
    }
    if(!pWorker->DownLoadFile(JVN_WEBSITE1,"/down/yst/YST_server.txt","server.txt","[Group]"))
    {
        if(!pWorker->DownLoadFile(JVN_WEBSITE2,"/down/yst/YST_server.txt","server.txt","[Group]"))
        {

//            ServerList SList;
//            for(l=0; l<pWorker->m_nGroupCount; l++)
//            {
//                if(!pWorker->DownLoadFirst(pWorker->m_chGroupList[l], SList, 1, 0))
//                {
//                    pWorker->DownLoadFirst(pWorker->m_chGroupList[l], SList, 2, 0);
//                    
//					//			OutputDebug("Download server %s.%d",pWorker->m_chGroupList[l],__LINE__);
//                }
//                else
//                {
//					//			OutputDebug("Download server %s.%d",pWorker->m_chGroupList[l],__LINE__);
//                }
//            }
        }
        else
        {
            pWorker->ParseIndexFile();
        }
    }
    else
    {
        pWorker->ParseIndexFile();
      
    }
    
    //////////////////////////////////////////////////////////////////////////
     ServerList SList;
	for(l=0; l<pWorker->m_nGroupCount; l++)
	{
	#ifdef WIN32
		if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hGTEndEvent, 0))
		{
			_endthread();
			return 0;
		}
	#endif
		if(!pWorker->DownLoadFirst(pWorker->m_chGroupList[l], SList, 1, 0))
		{
		#ifdef WIN32
			if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hGTEndEvent, 0))
			{
				_endthread();
				return 0;
			}
		#endif
			if(pWorker->DownLoadFirst(pWorker->m_chGroupList[l], SList, 2, 0))
			{
				for (int i = 0; i < SList.size(); i++)
				{
					pWorker->AddYstSvr(pWorker->m_chGroupList[l],SList[i].addr);
				}
			}

//			OutputDebug("Download server %s.%d",pWorker->m_chGroupList[l],__LINE__);
		}
		else
		{
			for (int i = 0; i < SList.size(); i++)
			{
				pWorker->AddYstSvr(pWorker->m_chGroupList[l],SList[i].addr);
			}
		}
		SList.clear();
	}
	pWorker->ShowYstSvr();
    //////////////////////////////////////////////////////////////////////////
    
    DWORD dwbegin = CCWorker::JVGetTime();
    DWORD dwend = 0;
    DWORD dwSP = 30000;//30000;
    
    while(TRUE)
    {
#ifndef WIN32
        if(pWorker->m_bGTEnd)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hGTEndEvent, 0))
        {
            if(pWorker->m_hGTEndEvent > 0)
            {
                CloseHandle(pWorker->m_hGTEndEvent);
                pWorker->m_hGTEndEvent = 0;
            }
            
            break;
        }
#endif
        
        
        dwend = CCWorker::JVGetTime();
        if(dwend < dwbegin || dwend > dwbegin + dwSP)
        {
#ifndef WIN32
            pthread_mutex_lock(&pWorker->m_ctclearsock);
#else
            EnterCriticalSection(&pWorker->m_ctclearsock);
#endif
            int nCount = pWorker->m_UDTSockTemps.size();
            for(int i=0; i<nCount-2; i++)
            {//?′≤????ìa·￥ê‘??÷?‘÷–??…??‰à???μ?μ?????￡¨‘?’?—?Ω???￡¨?í?∏‘≠“ú￥?≤è
                UDT::close(pWorker->m_UDTSockTemps[i]);
                pWorker->m_UDTSockTemps.erase(pWorker->m_UDTSockTemps.begin() + i);
                nCount--;
                i--;
                continue;
            }
#ifndef WIN32
            pthread_mutex_unlock(&pWorker->m_ctclearsock);
#else
            LeaveCriticalSection(&pWorker->m_ctclearsock);
#endif
            
            dwbegin = CCWorker::JVGetTime();
        }
        
        CCWorker::jvc_sleep(10);
    }
    
#ifdef WIN32
    return 0;
#else
    return NULL;
#endif
}

BOOL CCWorker::StartLANSerchServer(int nLPort, int nServerPort)
{
    int nport = 9400;
    int nsport = 9103;
    
    if(nLPort >= 0)
    {
        nport = nLPort;
    }
    
    if(nServerPort > 0)
    {
        nsport = nServerPort;
    }
    
    if(m_pLanSerch != NULL)
    {
        return TRUE;
    }
    
    m_pLanSerch = new CCLanSerch(nport, nsport, this, JVC_LANS);
    if(m_pLanSerch != NULL)
    {
        if(m_pLanSerch->m_bOK)
        {
            return TRUE;
        }
        delete m_pLanSerch;
        m_pLanSerch = NULL;
        return FALSE;
    }
    else
    {
        return FALSE;
    }
}

void CCWorker::StopLANSerchServer()
{
    if(m_pLanSerch != NULL)
    {
        delete m_pLanSerch;
    }
    m_pLanSerch = NULL;
}

#ifdef MOBILE_CLIENT
int CCWorker::StopSearchThread(){

	 if(m_pLanSerch == NULL)
	    {
	        return 0;
	    }
	   return m_pLanSerch->stopLanSearchMethod();
}
#endif
BOOL CCWorker::LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[JVN_DEVICENAMELEN], int nTimeOut,BOOL isMobile,unsigned int unFrequence)
{
    if(m_pLanSerch == NULL)
    {
        return FALSE;
    }
    if(unFrequence == 0)
    {
        unFrequence = 30;
    }
    return m_pLanSerch->LANSerch(chGroup, nYSTNO, nCardType, nVariety, chDeviceName, nTimeOut,isMobile,unFrequence);
}

BOOL CCWorker::StartBCServer(int nLPort, int nServerPort)
{
    int nport = 9500;
    int nsport = 9106;
    
    if(nLPort >= 0)
    {
        nport = nLPort;
    }
    
    if(nServerPort > 0)
    {
        nsport = nServerPort;
    }
    
    if(m_pBC != NULL)
    {
        return TRUE;
    }
    
    m_pBC = new CCLanSerch(nport, nsport, this, JVC_BC);
    if(m_pBC != NULL)
    {
        if(m_pBC->m_bOK)
        {
            return TRUE;
        }
        delete m_pBC;
        m_pBC = NULL;
        return FALSE;
    }
    else
    {
        return FALSE;
    }
}

void CCWorker::StopBCServer()
{
    if(m_pBC != NULL)
    {
        delete m_pBC;
    }
    m_pBC = NULL;
}

BOOL CCWorker::DoBroadcast(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut)
{
    if(m_pBC == NULL)
    {
        return FALSE;
    }
    return m_pBC->Broadcast(nBCID, pBuffer, nSize, nTimeOut);
}

/****************************************************************************
 *√?≥?  : JVC_EnableLANTool
 *π???  : ???ù∑??ò?…“‘à—à?≈‰÷√?÷”ú?ˉ÷–μ?…?±∏
 *≤???  : [IN] nLPort      ±?μ?∑??ò?à??￡¨<0?±???¨??9600
 [IN] nServerPort …?±∏?à∑??ò?à??￡¨<=0?±???¨??9104,Ω?“è?≥“a”√?¨??÷μ”?∑??ò?à??≈‰
 [IN] LANTData    à—à?Ω·π?a?μ?∫???
 *∑μa?÷μ: TRUE/FALSE
 *?‰à?  : ???ù?àà—à?∑??ò≤≈?…“‘Ω”?’à—à?Ω·π?￡¨à—à???o???π?JVC_LANToolDeviceΩ”??÷∏??
 *****************************************************************************/
int CCWorker::EnableLANTool(int nEnable, int nLPort, int nServerPort)
{
    int nport = 9600;
    int nsport = 9104;
    
    if(nLPort >= 0)
    {
        nport = nLPort;
    }
    
    if(nServerPort > 0)
    {
        nsport = nServerPort;
    }
    
    if(nEnable == 1)
    {//???ù∑??ò
        if(m_pLanTool != NULL)
        {
            return 1;
        }
        
        m_pLanTool = new CCLanTool(nport, nsport, this);
        if(m_pLanTool != NULL)
        {
            if(m_pLanTool->m_bOK)
            {
                return 1;
            }
            delete m_pLanTool;
            m_pLanTool = NULL;
            return 0;
        }
        else
        {
            return 0;
        }
    }
    else
    {//π?±’∑??ò
        if(m_pLanTool != NULL)
        {
            delete m_pLanTool;
        }
        m_pLanTool = NULL;
        return 1;
    }
}

int CCWorker::LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut)
{
    if(m_pLanTool == NULL)
    {
        return 0;
    }
    DWORD dwnow = JVGetTime();
    if(dwnow < m_dwLastLANToolTime || dwnow > m_dwLastLANToolTime + JVC_LANTOOL_TIME)
    {//?±o‰‘?–ì
        if(m_pLanTool->LANToolDevice(chPName, chPWord, nTimeOut))
        {
            return 1;
        }
    }
    return 0;
}

BOOL CCWorker::SetLocalFilePath(char chLocalPath[MAX_PATH])
{
    if(strlen(chLocalPath) < 3)
    {
        return FALSE;
    }
    
#ifndef WIN32
    if(mkdir(chLocalPath,S_IRWXU) < 0)
    {
        //...
    }
#else
    //≈–?????o??∑ò￥ê‘?￡¨≤a￥ê‘?‘ú￥￥Ω?
    SECURITY_ATTRIBUTES s;
    s.nLength=sizeof(SECURITY_ATTRIBUTES);
    s.lpSecurityDescriptor=NULL;
    s.bInheritHandle=TRUE;
    if(!CreateDirectory(chLocalPath, &s))
    {
        if(ERROR_ALREADY_EXISTS!=GetLastError())
        {
            return FALSE;//??o?o–≤a￥ê‘?μ′￥￥Ω???∞?
        }
    }
#endif
    
    strcpy(m_chLocalPath, chLocalPath);
    
    return TRUE;
}

void CCWorker::MultiMaskAdd(char* pBuff,int nLen)
{
    char strMask[] = "zxcvbnm,./\';lkjhgfdsaqwertyuiop[]=-0987654321!@#$%^&*()_+}{POIUYTREWQASDFGHJKL:|?><MNBVCXZ";
    int size = strlen(strMask);
    
    int a = 0xAA;
    int b = 0xBB;
    //ax + b
    int index = 0;
    char ch = ' ';
    for(int i=0;i<nLen;i++)
    {
        index = (a * i + b)%size;
        ch = strMask[index];
        pBuff[i] ^= ch;
    }
}

void CCWorker::MultiMaskRemove(char* pBuff,int nLen)
{
    char strMask[] = "zxcvbnm,./\';lkjhgfdsaqwertyuiop[]=-0987654321!@#$%^&*()_+}{POIUYTREWQASDFGHJKL:|?><MNBVCXZ";
    int size = strlen(strMask);
    
    int a = 0xAA;
    int b = 0xBB;
    //ax + b
    int index = 0;
    char ch = ' ';
    for(int i=0;i<nLen;i++)
    {
        index = (a * i + b)%size;
        ch = strMask[index];
        pBuff[i] ^= ch;
    }
}

/****************************************************************************
 *√?≥?  : WANGetChannelCount
 *π???  : ??π????ˉaò?°?≥∏?‘??”??∫≈??à???”–μ???μ?????
 *≤???  : [IN]  chGroup   ±??è∫≈
 [IN]  nYstNO    ‘??”??∫≈??
 [IN]  nTimeOutS μ?￥?≥¨?±?±o‰(√?)
 *∑μa?÷μ: >0  ≥…π?,??μ???
 0  ??∞?￡¨≤???”–??
 -1 ??∞?￡¨∫≈???￥…???
 -2 ??∞?￡¨÷???∞ê±?Ω??…￡¨≤a÷?≥÷∏√≤è—?
 -3 ??∞?￡¨?‰à?‘≠“ú
 *?‰à?  : ??–?(4)+≥§??(4)+?”??–?(1)+±??è∫≈(4)+‘??”??∫≈??(4)[+??μ?????(4)]
 0????￡¨1’?≥￡a?∏￥￡¨2√a…???￡¨3…????àμ′∞ê±??′?…?￥…?￥′??μ???
 *****************************************************************************/
int CCWorker::WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS)
{
    if(strlen(chGroup) <= 0 || nYSTNO <= 0 || nTimeOutS <= 0)
    {
        return 0;
    }
    
    int nret = -3;
    ServerList SList;
    sockaddr_in sin;
    SOCKADDR sockAddr;
    int nSockAddrLen = sizeof(SOCKADDR);
    
	if(!GetSerList(chGroup, SList, 1, 0))
	{
		if(!GetSerList(chGroup, SList, 2, 0))
		{
			writeLog("GetSerList failed  line: %d",__LINE__);
			return -3;
		}
	}
    //’“μΩ∑??ò???–±ì
    SOCKET socketTemp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketTemp < 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.‘≠“ú:???±??Ω”?÷??–?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count faild.Info:sock invlaid.", __FILE__,__LINE__);
        }
        writeLog("socketTemp < 0  line: %d",__LINE__);
        return -3;
    }
    
#ifndef WIN32
    sin.sin_addr.s_addr = INADDR_ANY;
#else
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    
    if (bind(socketTemp, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        shutdown(socketTemp,SD_BOTH);
        closesocket(socketTemp);
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.∞???∑??ò????Ω”?÷??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count.bind server sock failed.", __FILE__,__LINE__);
        }
		writeLog("bind  line: %d",__LINE__);
        return -3;
    }
    
    int nSerCount = SList.size();
    if(nSerCount>0)
    {
        //??–?(4)+≥§??(4)+?”??–?(1)+±??è∫≈(4)+‘??”??∫≈??(4)[+??μ?????(4)]
        char chdata[30]={0};
        for (int i = 0;i<nSerCount;i++)
        {
            int nLen = 9;
            int ndata = JVN_CMD_CHANNELCOUNT;
            memset(chdata, 0, 30);
            memcpy(chdata, &ndata, 4);//??–?
            memcpy(&chdata[4], &nLen, 4);
            chdata[8] = 0;
            memcpy(&chdata[9], chGroup, 4);
            memcpy(&chdata[13], &nYSTNO, 4);
            CCChannel::sendtoclient(socketTemp, chdata, nLen+8, 0, (sockaddr *)&SList[i].addr, sizeof(sockaddr_in), 1);
        }
    }
    else
    {
        shutdown(socketTemp,SD_BOTH);
        closesocket(socketTemp);
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.‘≠“ú:∑??ò???–±ì???’", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count faild.Info:no server find.", __FILE__,__LINE__);
        }
		writeLog("nSerCount<0  line: %d",__LINE__);
        return -3;
    }
    
    //???o—≠a∑
    int nType;
    DWORD dwTimeOld,dwTimeNow;
    dwTimeOld = CCWorker::JVGetTime();
    BOOL bRun = TRUE;
    
    int nRecvcount = 0;
    BYTE recvBuf[JVN_ASPACKDEFLEN] = {0};
    while(bRun)
    {
        int ret = CCChannel::receivefrom(socketTemp, (char *)&recvBuf, JVN_ASPACKDEFLEN, 0, &sockAddr, &nSockAddrLen, 2);
        if ( ret> 0)
        {//??–?(4)+≥§??(4)+?”??–?(1)+±??è∫≈(4)+‘??”??∫≈??(4)[+??μ?????(4)]
            memcpy(&nType, &recvBuf[0], 4);
            switch(nType)
            {
                case JVN_CMD_CHANNELCOUNT://?’μΩ?à∑??ò????”?
                {
                    int nyst = 0;
                    char chRecvGroup[4]={0};
                    memcpy(chRecvGroup, &recvBuf[9], 4);
                    memcpy(&nyst, &recvBuf[13], 4);
                    if(recvBuf[8] != 0 && nyst == nYSTNO)
                    {//∫?∑?
                        nRecvcount++;
                        //0????￡¨1’?≥￡a?∏￥￡¨2√a…???￡¨3…????àμ′∞ê±??′?…?￥…?￥′??μ???
                        if(recvBuf[8] == 1)
                        {
                            memcpy(&nret, &recvBuf[17], 4);
                            if(nret > 0 && nret < 500)
                            {
                                shutdown(socketTemp,SD_BOTH);
                                closesocket(socketTemp);
                                return nret;
                            }
                        }
                        else if(recvBuf[8] == 2)
                        {
                            if(nret == -3)
                            {
                                nret = -1;//√a…???
                            }
                        }
                        else if(recvBuf[8] == 3)
                        {
                            nret = -2;//∞ê±??′?…
                        }
                    }
                    nRecvcount++;
                    break;
                }
                default://?’μΩ?‰à?????∞????˙
                    break;
            }
        }
        else
        {//≈–??Ω”?’π?≥???∑ò≥¨?±
            dwTimeNow = CCWorker::JVGetTime();
            if(nRecvcount>=nSerCount)
            {//“—?’μΩ?à??πaa?∏￥,?…“‘Ω·?ˉ
                bRun = FALSE;
                break;
            }
            else if ((dwTimeNow < dwTimeOld) || (dwTimeNow  > dwTimeOld + nTimeOutS*1000))
            {//√a”–??μΩ??πaμ?a?∏￥￡¨μ′“—?≠≥¨?±
                break;
            }
            
        }
    }
    OutputDebug("**********wanlangetcount: %d",nret);
    shutdown(socketTemp,SD_BOTH);
    closesocket(socketTemp);
    return nret;
}

/****************************************************************************
*���绉�  : WANGetBatchChannelCount
*������  : ���杩�妫�绱㈡����″�ㄦ�归����峰��浜�瑙������风�������锋�������������绘��
*������  : [IN]  pChannelNum   ��风��淇℃��
         [IN]  nYSTNOCnt    浜�瑙������风��涓����
		 [IN]  nTimeOutS 绛�寰�瓒���舵�堕��(绉�)
*杩�������: 1  ������锛���峰����扮��姣�涓�浜�瑙������风�������锋�������������绘�板��CHANNEL_NUM���wChannelNum涓�
         0  澶辫触锛������版��璇�
	 -1 澶辫触锛���朵��������

*��朵��  :
		璇锋�����锛�
			1瀛�������绫诲�� + 4瀛�������杞芥�版����垮害 + 1瀛������风����� + 4瀛�������缁� + 4瀛������风��
		杩�������锛�
			1瀛�������绫诲�� + 4瀛�������杞芥�版����垮害 + 1瀛������风����� + 4瀛�������缁� + 4瀛������风�� + 2瀛�������������

*****************************************************************************/

int CCWorker::WANGetBatchChannelCount(char *pChannelNum, int nYSTNOCnt, int nTimeOutS)
{

	if(!pChannelNum || nYSTNOCnt <= 0 || nTimeOutS <= 0)
	{
		return 0;
	}
	int nret = -1;

	CHANNEL_NUM *p = (CHANNEL_NUM*)pChannelNum;

	ServerList SList;
	sockaddr_in sin;
	SOCKADDR sockAddr;
	int nSockAddrLen = sizeof(SOCKADDR);

	if(!GetIndexServerList("A",SList, 1, 0))
	{
		if(!GetIndexServerList("A",SList, 2, 0))
		{
			return -1;
		}
	}
	//��惧�版����″�ㄥ��琛�
	SOCKET socketTemp = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketTemp < 0)
	{
		if(JVN_LANGUAGE_CHINESE == m_nLanguage)
		{
			m_Log.SetRunInfo(0,"��峰����������峰け璐�.������:涓存�跺����ュ��������", __FILE__,__LINE__);
		}
		else
		{
			m_Log.SetRunInfo(0,"get channel count faild.Info:sock invlaid.", __FILE__,__LINE__);
		}

		return -1;
	}

#ifndef WIN32
	sin.sin_addr.s_addr = INADDR_ANY;
#else
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(0);

	if (bind(socketTemp, (struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		shutdown(socketTemp,SD_BOTH);
		closesocket(socketTemp);

		if(JVN_LANGUAGE_CHINESE == m_nLanguage)
		{
			m_Log.SetRunInfo(0,"��峰����������峰け璐�.缁�瀹������″�ㄥ����ュ��澶辫触", __FILE__,__LINE__);
		}
		else
		{
			m_Log.SetRunInfo(0,"get channel count.bind server sock failed.", __FILE__,__LINE__);
		}

		return -1;
	}

	int nSerCount = SList.size();
	if(nSerCount > 0)
	{
		//1瀛�������绫诲�� + 4瀛�������杞芥�版����垮害 + 1瀛������风����� + 4瀛�������缁� + 4瀛������风��
		int j = 0;
		int nTransLen;
		int nTransYSTNO;
		int nLen = nYSTNOCnt*10;//10 = 4瀛�������缁� + 4瀛������风�� + 2瀛�������������
		char chdata[JVN_ASPACKDEFLEN]={0};

		memset(chdata, 0, JVN_ASPACKDEFLEN);
		int ndata = JVN_CMD_BATCH_CHANNELNUM;
		memcpy(chdata, &ndata, 1);//绫诲��
		nTransLen = htonl(nLen);//瀛����搴�杞����
		memcpy(&chdata[1], &nTransLen, 4);
		memcpy(&chdata[5], &nYSTNOCnt, 1);

		for(j = 0; j < nYSTNOCnt; j++)
		{
			memcpy(&chdata[6+8*j], p->chGroup, 4);
			nTransYSTNO = htonl(p->nYSTNO);
			memcpy(&chdata[10+8*j], &nTransYSTNO, 4);

			p += 1;

		}

		for (int i = 0;i < nSerCount;i++)
		{
			CCChannel::sendtoclient(socketTemp, chdata, nLen+5, 0, (sockaddr *)&SList[i].addr, sizeof(sockaddr_in), 1);
		}
	}
	else
	{
		shutdown(socketTemp,SD_BOTH);
		closesocket(socketTemp);

		if(JVN_LANGUAGE_CHINESE == m_nLanguage)
		{
			m_Log.SetRunInfo(0,"��峰����������峰け璐�.������:�����″�ㄥ��琛ㄤ负绌�", __FILE__,__LINE__);
		}
		else
		{
			m_Log.SetRunInfo(0,"get channel count faild.Info:no server find.", __FILE__,__LINE__);
		}

		return -1;
	}

	for(int j = 0; j < nYSTNOCnt; j++)
	{
		p--;
	}

	//寮�濮�寰����
	int nType;
	DWORD dwTimeOld,dwTimeNow;
	dwTimeOld = CCWorker::JVGetTime();
	BOOL bRun = TRUE;
	int nRecvLen = 0;
	int nRecvcount = 0;
	BYTE recvBuf[JVN_ASPACKDEFLEN] = {0};

	while(bRun)
	{
		if (CCChannel::receivefrom(socketTemp, (char *)&recvBuf, JVN_ASPACKDEFLEN, 0, &sockAddr, &nSockAddrLen, 2) > 0)
		{//1瀛�������绫诲�� + 4瀛�������杞芥�版����垮害 + 1瀛������风����� + 4瀛�������缁� + 4瀛������风�� + 2瀛�������������
			nType = (int)recvBuf[0];
			nRecvLen = ntohl(*(int*)&(recvBuf[1]));

			if (RUN_MODE == RUN_MODE_DBG)
			{
				g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,__FUNCTION__,"JOV lib nRecvLen:%d, type:0x%x!\n", nRecvLen, nType);
			}
			switch(nType)
			{
				case JVN_CMD_BATCH_CHANNELNUM://��跺�颁��妫�绱㈡����″�ㄥ��搴�
					{
						int nYSTNum = 0;
						int k = 0;
						nYSTNum = (int)recvBuf[5];
						if(nYSTNum != nYSTNOCnt) //��跺�扮����版��璺���������风��涓���版�版��涓�涓����
						{
							if (RUN_MODE == RUN_MODE_DBG)
							{
								g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,__FUNCTION__,"JOV ystnum is not all count:%d!\n", recvBuf[5]);
							}
							nret = -1;
						} else {
							for (k = 0; k < nYSTNum; k++)
							{
								int recvYSTNO = ntohl(*(int*)&recvBuf[10+10*k]);
								char recvGroup[4];
								memcpy(recvGroup, &recvBuf[6+10*k], 4);
								unsigned short usChannelNum = ntohs(*(unsigned short*)&(recvBuf[14+10*k]));//缁�姣�涓�缁����浣�涓������������拌�����
								if((!strcmp(p->chGroup, recvGroup))&&(p->nYSTNO == recvYSTNO))//纭�淇�姣�涓���风�������������板��纭�
								{
									if(usChannelNum < 0 || usChannelNum > 256)
									{
										shutdown(socketTemp,SD_BOTH);
										closesocket(socketTemp);
										return -1;
									}
									p->wChannelNum = usChannelNum;
								}
								p += 1;
							}
							nRecvcount++;
							shutdown(socketTemp,SD_BOTH);
							closesocket(socketTemp);
							return 1;
						}
						nRecvcount++;
						break;
					}
				default://��跺�板�朵����版�����涓㈠��
					break;
			}
		}
		else
		{//��ゆ����ユ�惰��绋�������瓒����
			dwTimeNow = CCWorker::JVGetTime();
			if(nRecvcount>=nSerCount)
			{//宸叉�跺�颁��瓒冲�����澶�,���浠ョ�����
				bRun = FALSE;
				break;
			}
			else if ((dwTimeNow < dwTimeOld) || (dwTimeNow  > dwTimeOld + nTimeOutS*1000))
			{//娌℃�������拌冻澶�������澶�锛�浣�宸茬��瓒����
				g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,__FUNCTION__,"JOV Recv Data TimeOut!");
				break;
			}
		}
	}

	shutdown(socketTemp,SD_BOTH);
	closesocket(socketTemp);
	return nret;
}

int CCWorker::YSTNOCushion(char* pGroup,int nYSTNO,int nType)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctYSTNO);
#else
    EnterCriticalSection(&m_ctYSTNO);
#endif
    if(nType == 0)
    {//o?≤è??∑ò”–∫≈??‘??¨Ω”
        for(int i=0; i<m_stYSTCushion.size(); i++)
        {
			if(m_stYSTCushion[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTCushion[i].strGroup) == 0)
            {
                DWORD dwend = CCWorker::JVGetTime();
                if(dwend < m_stYSTCushion[i].dwSerTime || dwend > 10000 + m_stYSTCushion[i].dwSerTime)
                {//π???????
                    m_stYSTCushion.erase(m_stYSTCushion.begin()+i);
#ifndef WIN32
                    pthread_mutex_unlock(&m_ctYSTNO);
#else
                    LeaveCriticalSection(&m_ctYSTNO);
#endif
					OutputDebug("------------------555555555555----------channel , yst: %d, line: %d\n",nYSTNO,__LINE__);
                    return 0;//??–?????μ±≥…√a”–￥??ì
                }
                else
                {
#ifndef WIN32
                    pthread_mutex_unlock(&m_ctYSTNO);
#else
                    LeaveCriticalSection(&m_ctYSTNO);
#endif
                    
                    return 1;//”–’?‘??¨Ω”μ???μ?￡¨∫?–ˉ?¨Ω””?∏√μ?￥?
                }
            }
        }
    }
    else if(nType > 0)
    {//–￥??∫≈??
        for(int i=0; i<m_stYSTCushion.size(); i++)
        {
			if(m_stYSTCushion[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTCushion[i].strGroup) == 0)
            {
                m_stYSTCushion[i].dwSerTime = CCWorker::JVGetTime();
#ifndef WIN32
                pthread_mutex_unlock(&m_ctYSTNO);
#else
                LeaveCriticalSection(&m_ctYSTNO);
#endif
                
                return 0;
            }
        }
        
        STYSTNOINFO styst;
		strcpy(styst.strGroup,pGroup);
        styst.nYSTNO = nYSTNO;
        styst.dwSerTime = CCWorker::JVGetTime();
        m_stYSTCushion.push_back(styst);
#ifndef WIN32
        pthread_mutex_unlock(&m_ctYSTNO);
#else
        LeaveCriticalSection(&m_ctYSTNO);
#endif
        
        return 0;
        
    }
    else
    {//…?≥?∫≈??
        for(int i=0; i<m_stYSTCushion.size(); i++)
        {
			if(m_stYSTCushion[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTCushion[i].strGroup) == 0)
            {
                m_stYSTCushion.erase(m_stYSTCushion.begin()+i);
#ifndef WIN32
                pthread_mutex_unlock(&m_ctYSTNO);
#else
                LeaveCriticalSection(&m_ctYSTNO);
#endif
				OutputDebug("－－－－－－－－－－－777777－－－－－－－－－get ser and begin delete connecting,channel: %d, yst: %d",nYSTNO);
                return 0;
            }
        }
    }
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ctYSTNO);
#else
    LeaveCriticalSection(&m_ctYSTNO);
#endif
    
    return 0;//√a’“μΩa∫￥êo??o
}
//￥”∫≈??a∫￥ê÷–aò?°‘??”??–≈?￠0√a”–￡a1?¨Ω”≥…π??à÷±Ω””√à?μ?μ?÷∑￡a2?…“‘”√∑??ò??μ?÷∑?–±ì
//∫?–ˉaπ?…“‘‘??∏a?￡¨∑??ò??∑μa??àμ?“≤?…“‘÷?”√
int CCWorker::GetYSTNOInfo(char* pGroup,int nYSTNO, ServerList &slist, SOCKADDR_IN &addrA, char chIPA[16], int &nport,SOCKET& s)
{
    int nret = -1;
    s = 0;
#ifndef WIN32
    pthread_mutex_lock(&m_ctYSTNO);
#else
    EnterCriticalSection(&m_ctYSTNO);
#endif
    
    DWORD dwend = CCWorker::JVGetTime();
    for(int i=0; i<m_stYSTNOLIST.size(); i++)
    {
		if(m_stYSTNOLIST[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTNOLIST[i].strGroup) == 0)
        {
            if(dwend < m_stYSTNOLIST[i].dwSerTime || dwend > 60000 + m_stYSTNOLIST[i].dwSerTime)
            {
                nret = 0;
                break;//π???????￡¨??–?
            }
            else if(dwend < 60000 + m_stYSTNOLIST[i].dwSerTime && m_stYSTNOLIST[i].nConnST >= 1)
            {
                memcpy(&addrA, &m_stYSTNOLIST[i].addrA, sizeof(SOCKADDR_IN));
                sprintf(chIPA, "%s", inet_ntoa(addrA.sin_addr));
                nport = ntohs(addrA.sin_port);
                
                slist = m_stYSTNOLIST[i].stSList;
                s = m_stYSTNOLIST[i].sSocket;
                
                nret = 1;
                break;//“—”–??μ??¨Ω”≥…π?
            }
            else if(m_stYSTNOLIST[i].nConnST == 0 //–?“?∑??ò???–±ì?±μπ???…“‘≤??o???±o‰
                    && dwend >= m_stYSTNOLIST[i].dwSerTime
                    && dwend <= 60000 + m_stYSTNOLIST[i].dwSerTime)
            {
                slist = m_stYSTNOLIST[i].stSList;
                nret = 2;
                break;//“—”–??μ??°μΩ∑??ò??μ?÷∑
            }
        }
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctYSTNO);
#else
    LeaveCriticalSection(&m_ctYSTNO);
#endif
    
    return nret;
}

//∏?–?∫≈??a∫￥ê
void CCWorker::UpdateYSTNOInfo(char* pGroup,int nYSTNO, ServerList slist,SOCKADDR_IN addrA,SOCKET s)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctYSTNO);
#else
    EnterCriticalSection(&m_ctYSTNO);
#endif
    
    BOOL bfind = FALSE;
    for(int i=0; i<m_stYSTNOLIST.size(); i++)
    {
		if(m_stYSTNOLIST[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTNOLIST[i].strGroup) == 0)
        {
            bfind = TRUE;
            if(m_stYSTNOLIST[i].nConnST <= 0)
                m_stYSTNOLIST[i].nConnST = 1;
            
            //?¨Ω”≥…π?￡¨∏?–?μ?÷∑
            memcpy(&m_stYSTNOLIST[i].addrA, &addrA, sizeof(SOCKADDR_IN));
            m_stYSTNOLIST[i].dwSerTime = CCWorker::JVGetTime();
            
            m_stYSTNOLIST[i].sSocket = s;
            
            break;
        }
    }
    if(!bfind)
    {
        //?¨Ω”≥…π?￡¨∏?–?μ?÷∑
        STYSTNOINFO styst;
		strcpy(styst.strGroup,pGroup);
        styst.nYSTNO = nYSTNO;
        styst.dwSerTime = CCWorker::JVGetTime();
        memcpy(&styst.addrA, &addrA, sizeof(SOCKADDR_IN));
        styst.stSList = slist;
        styst.nConnST = 1;
        styst.sSocket = s;
        m_stYSTNOLIST.push_back(styst);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctYSTNO);
#else
    LeaveCriticalSection(&m_ctYSTNO);
#endif
}

//∏?–?∫≈??a∫￥ê
void CCWorker::WriteYSTNOInfo(char* pGroup,int nYSTNO, ServerList slist, SOCKADDR_IN addrA, int nConnect,SOCKET s)
{
    if(nConnect == 1)
    {
        m_Helper.UpdateTime(pGroup,nYSTNO,s,(SOCKADDR_IN* )&addrA);
    }
    m_MakeHoleGroup.SetConnect(s,nConnect);
    
#ifndef WIN32
    pthread_mutex_lock(&m_ctYSTNO);
#else
    EnterCriticalSection(&m_ctYSTNO);
#endif
    
    BOOL bfind = FALSE;
    for(int i=0; i<m_stYSTNOLIST.size(); i++)
    {
		if(m_stYSTNOLIST[i].nYSTNO == nYSTNO && strcasecmp(pGroup,m_stYSTNOLIST[i].strGroup) == 0)
        {
            bfind = TRUE;
            
            m_stYSTNOLIST[i].nConnST += nConnect;
            //			m_stYSTNOLIST[i].nConnST = jvs_max(m_stYSTNOLIST[i].nConnST, 0);
            
            //			OutputDebug("?ìo” ”––??¨Ω” ======== current num = %d   %d",m_stYSTNOLIST[i].nConnST,nConnect);
            if(nConnect > 0)
            {//?¨Ω”≥…π?￡¨∏?–?μ?÷∑
                memcpy(&m_stYSTNOLIST[i].addrA, &addrA, sizeof(SOCKADDR_IN));
                m_stYSTNOLIST[i].dwSerTime = CCWorker::JVGetTime();
                
                m_stYSTNOLIST[i].sSocket = s;
                //					OutputDebug("UDP ?¨Ω”");
                
                //			OutputDebug("%s : %d", inet_ntoa(addrA.sin_addr),ntohs(addrA.sin_port));
                
                break;
            }
            else if(nConnect == 0)
            {//?°∑??ò??μ?÷∑≥…π?￡¨∏?–?∑??ò??
                m_stYSTNOLIST[i].stSList = slist;
                m_stYSTNOLIST[i].dwSerTime = CCWorker::JVGetTime();
                break;
            }
            else if(m_stYSTNOLIST[i].nConnST <= 0)
            {//…?≥?∏√∫≈??
                m_stYSTNOLIST.erase(m_stYSTNOLIST.begin()+i);
                i--;
                break;
            }
            break;
        }
    }
    if(!bfind)
    {
        if(nConnect > 0)
        {//?¨Ω”≥…π?￡¨∏?–?μ?÷∑
            STYSTNOINFO styst;
			strcpy(styst.strGroup,pGroup);
            styst.nYSTNO = nYSTNO;
            styst.dwSerTime = CCWorker::JVGetTime();
            memcpy(&styst.addrA, &addrA, sizeof(SOCKADDR_IN));
            styst.nConnST = 1;
            styst.stSList = slist;
            styst.sSocket = s;
            m_stYSTNOLIST.push_back(styst);
        }
        else if(nConnect == 0)
        {//?°∑??ò??μ?÷∑≥…π?￡¨∏?–?∑??ò??
            STYSTNOINFO styst;
			strcpy(styst.strGroup,pGroup);
            styst.nYSTNO = nYSTNO;
            styst.dwSerTime = CCWorker::JVGetTime();
            memcpy(&styst.addrA, &addrA, sizeof(SOCKADDR_IN));
            styst.nConnST = 0;
            styst.stSList = slist;
            styst.sSocket = 0;
            m_stYSTNOLIST.push_back(styst);
        }
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctYSTNO);
#else
    LeaveCriticalSection(&m_ctYSTNO);
#endif
}

//???’±?μ?a∫￥ê
void CCWorker::ClearBuffer(int nLocalChannel)
{
#ifndef WIN32
    pthread_mutex_lock(&m_criticalsection);
#else
    EnterCriticalSection(&m_criticalsection);
#endif
    
    int nCount = m_pChannels.size();
    for(int i=0; i<nCount; i++)
    {
        if(m_pChannels[i] == NULL)
        {//??μù??–?o??o
            m_pChannels.erase(m_pChannels.begin() + i);
            nCount--;
            i--;
            continue;
        }
        if(m_pChannels[i]->m_nStatus == FAILD && m_pChannels[i]->m_ServerSocket <= 0
           && (m_pChannels[i]->m_pOldChannel == NULL//–?–≠“è??–?o??o
               || (m_pChannels[i]->m_pOldChannel != NULL && m_pChannels[i]->m_pOldChannel->m_bCanDelS)))//?…–≠“è
        {//??μù??–?o??o
            continue;
        }
        
        if(m_pChannels[i]->m_nLocalChannel == nLocalChannel)
        {//∏√??μ?“—Ωˉ––π??¨Ω”
            m_pChannels[i]->ClearBuffer();
            
#ifndef WIN32
            pthread_mutex_unlock(&m_criticalsection);
#else
            LeaveCriticalSection(&m_criticalsection);
#endif
            
            return;
            
        }
    }
    
#ifndef WIN32
    pthread_mutex_unlock(&m_criticalsection);
#else
    LeaveCriticalSection(&m_criticalsection);
#endif
    
    return;
}

void CCWorker::pushtmpsock(UDTSOCKET udtsock)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctclearsock);
#else
    EnterCriticalSection(&m_ctclearsock);
#endif
    
    //m_UDTSockTemps.push_back(udtsock);//’???≤a?￠o￥π?±’?????à∑?÷π?±o‰≤ó“???μ?±?￥?∞—…?￥???π?±’?è–?
    UDT::close(udtsock);//’???‘??±?‘’?—?￥??ì￡¨∑ò‘ú“????Ω∏?????￡¨“a??oê???…÷???￡a?????…÷???π?±’?±o‰≥§￡a
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ctclearsock);
#else
    LeaveCriticalSection(&m_ctclearsock);
#endif
}

/****************************************************************************
 *√?≥?  : EnableHelp
 *π???  : ?ù”√/?￡”√??à??￥Ω”∑??ò
 *≤???  : [IN] bEnable TRUE???ù/FALSEπ?±’
 [IN] nType  μ±?∞?π”√’???à≠￡¨μ±bEnable??TRUE?±”––?
 1 μ±?∞?π”√’???‘??”??–°÷˙?÷
 2 μ±?∞?π”√’???‘??”??PC??a??à
 3 μ±?∞?π”√’???‘??”???÷a˙??a??à
 *∑μa?÷μ: ??
 *?‰à?  : ?ù”√∏√π???∫?￡¨?ˉ?áSDKa·?‘…???μ?∫≈??Ωˉ––?¨Ω”?·à?μ?”≈a?￡a
 ?ù”√∏√π???∫?￡¨?ˉ?áSDKa·÷?≥÷–°÷˙?÷∫???a??à÷?o‰Ωˉ––Ωaa?￡a
 ?áπ?∑÷???à÷?≥÷–°÷˙?÷Ωˉ≥?￡¨‘ú”√–°÷˙?÷?à?π”√nType=1￡¨??a??à?π”√nType=2o￥?…￡a
 ?áπ???a??à≤a÷?≥÷–°÷˙?÷Ωˉ≥?￡¨‘ú??a??à?π”√nType=3o￥?…￡¨±??á?÷a˙??a??à￡a
 *****************************************************************************/
BOOL CCWorker::EnableHelp(BOOL bEnable, int nType)
{
    if(bEnable)
    {//???ù∑??ò
        if(m_pHelpCtrl != NULL)
        {//∑??ò“—???ù￡¨≈–?????ù??–?
            if(m_pHelpCtrl->m_nHelpType == nType)
            {
                return TRUE;
            }
            
            delete m_pHelpCtrl;
            m_pHelpCtrl = NULL;
        }
        
        if(nType == JVN_WHO_H)
        {//–°÷˙?÷
            m_pHelpCtrl = new CCHelpCtrlH(this);
            if(m_pHelpCtrl != NULL)
            {
                return TRUE;
            }
        }
        else if(nType == JVN_WHO_P)
        {//?π”√–°÷˙?÷μ???a??à
            m_pHelpCtrl = new CCHelpCtrlP(this);
            if(m_pHelpCtrl != NULL)
            {
                return TRUE;
            }
        }
        else if(nType == JVN_WHO_M)
        {//??a??à?‘o∫?·à?￡¨≤a”√–°÷˙?÷
            if(m_WorkerUDTSocket==0)
            {
                
                m_WorkerUDTSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                
                BOOL bReuse = TRUE;
                UDT::setsockopt(m_WorkerUDTSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                //////////////////////////////////////////////////////////////////////////
                int len1 = JVC_MSS;
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_MSS, &len1, sizeof(int));
                //////////////////////////////////////////////////////////////////////////
                
#ifdef MOBILE_CLIENT
                //    len1=1000*1024;
                //    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_SNDBUF, &len1, sizeof(int));
                //    len1=1500*1024;
                //    UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_RCVBUF, &len1, sizeof(int));
                len1=1500*1024;
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                
                len1=1000*1024;
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                
                if (UDT::ERROR == UDT::bind(m_WorkerUDTSocket, m_WorkerUDPSocket))
                {//∞???μΩ÷∏???à????∞?￡¨∏???∞???μΩàêa˙?à??
                    if(m_WorkerUDPSocket > 0)
                    {
                        closesocket(m_WorkerUDPSocket);
                    }
                    m_WorkerUDPSocket = 0;
                    
                    if(m_WorkerUDTSocket > 0)
                    {
                        UDT::close(m_WorkerUDTSocket);
                    }
                    m_WorkerUDTSocket = 0;
                    
                    if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                    {
                        m_Log.SetRunInfo(0, "??∞?.?¨Ω”÷?????∞?(?…?????à??±a’o) ?í?∏:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        m_Log.SetRunInfo(0, "failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                }
                
                //Ω′??Ω”?÷÷√??∑??????￡?Ω
                BOOL block = FALSE;
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                LINGER linger;
                linger.l_onoff = 0;
                linger.l_linger = 0;
                UDT::setsockopt(m_WorkerUDTSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));

            }
            m_pHelpCtrl = new CCHelpCtrlM(this);
            if(m_pHelpCtrl != NULL)
            {
                return TRUE;
            }
        }
        
        return FALSE;
    }
    else
    {
        if(m_pHelpCtrl != NULL)
        {
            delete m_pHelpCtrl;
            m_pHelpCtrl = NULL;
        }
    }
    return TRUE;
}

/****************************************************************************
 *√?≥?  : SetHelpYSTNO
 *π???  : …?÷√?‘?≥–?‘??”??∫≈??μ?∏?÷˙÷?≥÷
 *≤???  : [IN] pBuffer ‘??”??∫≈??o?∫?￡¨”…STBASEYSTNOΩ·ππ?è≥…￡a±??á”–?Ω∏?∫≈??
 STBASEYSTNO st1,STBASEYSTNO st1;
 pBufferμ?????????:
 memcpy(bBuffer, &st1, sizeof(STBASEYSTNO));
 memcpy(&bBuffer[sizeof(STBASEYSTNO)], &st2, sizeof(STBASEYSTNO));
 [IN] nSize   pBufferμ??μo?”––?≥§??￡a?áπ????Ω∏?∫≈??‘ú??￡∫
 2*sizeof(STBASEYSTNO);
 *∑μa?÷μ: ??
 *?‰à?  : ‘??”??–°÷˙?÷?à?π”√￡a
 ??a??à≤a÷?≥÷–°÷˙?÷?±??a??à?π”√￡a
 
 ?ìo”∫?￡¨?ˉ?áSDKa·?‘’?–?‘??”??∫≈??Ωˉ––?¨Ω”?·à?μ?”≈a?￡a
 ’???≥??o…?÷√￡¨≥?–ú‘à––÷–??a??à“≤a·”––?–?μ?∫≈??￡¨
 a·???¨?ìo”μΩ??≤?≤￠?·??∏ˉ–°÷˙?÷?à￡a
 STBASEYSTNOS ‘??”??∫≈??,STYSTNO??“?≤??￥JVNSDKDef.h
 *****************************************************************************/
BOOL CCWorker::SetHelpYSTNO(BYTE *pBuffer, int nSize)
{
    if(m_pHelpCtrl != NULL)
    {
        if(m_pHelpCtrl->m_nHelpType == JVN_WHO_H || m_pHelpCtrl->m_nHelpType == JVN_WHO_M)
        {
            return m_pHelpCtrl->SetHelpYSTNO(pBuffer, nSize);
        }
    }
    
    return FALSE;
}

/****************************************************************************
 *√?≥?  : GetHelpYSTNO
 *π???  : aò?°μ±?∞“—÷?μ?‘??”??∫≈????μ?
 *≤???  : [IN/OUT] pBuffer ”…μ?”√’???±???￥ê￡a
 ∑μa?‘??”??∫≈??o?∫?￡¨”…STBASEYSTNOΩ·ππ?è≥…￡a±??á”–?Ω∏?∫≈??
 STBASEYSTNO st1,STBASEYSTNO st1;
 pBufferμ?????????:
 memcpy(bBuffer, &st1, sizeof(STBASEYSTNO));
 memcpy(&bBuffer[sizeof(STBASEYSTNO)], &st2, sizeof(STBASEYSTNO));
 [IN/OUT] nSize   μ?”√?±￥′??μ???pBufferμ??μo???±?≥§??￡¨
 μ?”√∫?∑μa?μ???pBufferμ??μo?”––?≥§??￡a?áπ????Ω∏?∫≈??‘ú??￡∫
 2*sizeof(STBASEYSTNO);
 *∑μa?÷μ: -1  ￥ì??￡¨≤???”–??￡¨pBuffer???’aú??￥?–°≤a??“‘￥ê￥￠Ω·π?￡a
 0  ∑??ò?￥???ù
 1  ≥…π?
 *?‰à?  : ‘??”??–°÷˙?÷?à?π”√￡a
 ??a??à≤a÷?≥÷–°÷˙?÷?±??a??à?π”√￡a
 
 ’???≥?–ú‘à––÷–“—÷?μ?à?”–∫≈??￡¨o￥–°÷˙?÷a·?‘’?–?∫≈??Ωˉ––?¨Ω””≈a?÷?≥÷￡a
 ∏√Ω”??Ω?”√”?≤è—?￡¨”…”???≤?a·?‘???ìo”￡¨≤è—?Ω·π?≤aa·≥§??”––?￡a
 STBASEYSTNOS ‘??”??∫≈??,STYSTNO??“?≤??￥JVNSDKDef.h
 *****************************************************************************/
int CCWorker::GetHelpYSTNO(BYTE *pBuffer, int &nSize)
{
    if(m_pHelpCtrl != NULL)
    {
        if(m_pHelpCtrl->m_nHelpType == JVN_WHO_H || m_pHelpCtrl->m_nHelpType == JVN_WHO_M || m_pHelpCtrl->m_nHelpType == JVN_WHO_P)
        {
            return m_pHelpCtrl->GetHelpYSTNO(pBuffer, nSize);
        }
    }
    
    return 0;
}

/****************************************************************************
 *√?≥?  : GetYSTStatus
 *π???  : aò?°?≥∏?‘??”??∫≈??μ?‘????￥?¨
 *≤???  : [IN] chGroup  ‘??”??∫≈??μ?±??è∫≈￡a
 [IN] nYSTNO   ‘??”??∫≈??
 [IN] nTimeOut ≥¨?±?±o‰(√?)￡¨Ω?“è>=2√?
 *∑μa?÷μ: -1  ￥ì??￡¨≤???”–??￡¨chGroup???’aú??nYSTNO<=0￡a
 0  ±?μ?≤è—??′?μ∑±￡¨…‘∫?÷??‘
 1  ∫≈??‘???
 2  ∫≈??≤a‘???
 3  ≤è—???∞?￡¨aπ≤a??≈–??∫≈????∑ò‘???
 *?‰à?  : 1.?￠“?￡¨∏√∫??????∞Ω???”√”??÷a˙,pc?à‘?≤a‘?–ì?π”√￡a
 2.∏√∫????‘?¨“a∏?∫≈??≤a‘?–ì?μ∑±μ?”√￡¨o‰∏ù>=10s;
 3.∏√∫????‘≤a?¨∫≈??≤a‘?–ì?μ∑±μ?”√￡¨o‰∏ù>=1s;
 *****************************************************************************/
int CCWorker::GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut)
{
    if(strlen(chGroup) <= 0 || nYSTNO <= 0 || nTimeOut <= 0 || nTimeOut > 60)
    {
        return -1;
    }
    
    DWORD dwcur = CCWorker::JVGetTime();
    if(dwcur < m_dwLastTime)
    {//o??±??–?￡¨?…????μ?’??à?μ?≥?±o‰
        m_dwLastTime = dwcur;
    }
    else if(dwcur < m_dwLastTime + 1000)
    {//?μ∑±μ?Ωˉ––?à≤è—?
        return 0;
    }
    
    int ncount = m_GetStatus.size();
    for(int i=0; i<ncount; i++)
    {
        if(m_GetStatus[i].nYSTNO == nYSTNO && memcmp(chGroup, m_GetStatus[i].chGroup, 4) == 0)
        {//’“μΩ“—?≠????π?μ?∫≈??
            if(dwcur < m_GetStatus[i].dwLastTime + 10000)
            {//?¨“a∏?∫≈??≤è—?μ??′?μ∑±?à￡¨≤a‘?–ì
                return 0;
            }
            m_GetStatus.erase(m_GetStatus.begin()+i);
            i--;
            ncount--;
        }
    }
    
    //≤è—??μ??…?∫?∑??à￡¨‘?–ìo?–ˉ≤è—?
    //aò?°∑??ò???–±ì
    int nret = 3;
    ServerList SList;
    sockaddr_in sin;
    SOCKADDR sockAddr;
    int nSockAddrLen = sizeof(SOCKADDR);
    
    if(!GetSerList(chGroup, SList, 1, 0))
    {
        if(!GetSerList(chGroup, SList, 2, 0))
        {
            m_dwLastTime = CCWorker::JVGetTime();
            STGETSTATUS sts;
            sts.dwLastTime = m_dwLastTime;
            memcpy(sts.chGroup, chGroup, 4);
            sts.nYSTNO = nYSTNO;
            m_GetStatus.push_back(sts);
            return 3;
        }
    }
    
    //’“μΩ∑??ò???–±ì
    SOCKET socketTemp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketTemp < 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.‘≠“ú:???±??Ω”?÷??–?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count faild.Info:sock invlaid.", __FILE__,__LINE__);
        }
        
        m_dwLastTime = CCWorker::JVGetTime();
        STGETSTATUS sts;
        sts.dwLastTime = m_dwLastTime;
        memcpy(sts.chGroup, chGroup, 4);
        sts.nYSTNO = nYSTNO;
        m_GetStatus.push_back(sts);
        return 3;
    }
    
#ifndef WIN32
    sin.sin_addr.s_addr = INADDR_ANY;
#else
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);//0
    
    if (bind(socketTemp, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        shutdown(socketTemp,SD_BOTH);
        closesocket(socketTemp);
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.∞???∑??ò????Ω”?÷??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count.bind server sock failed.", __FILE__,__LINE__);
        }
        
        m_dwLastTime = CCWorker::JVGetTime();
        STGETSTATUS sts;
        sts.dwLastTime = m_dwLastTime;
        memcpy(sts.chGroup, chGroup, 4);
        sts.nYSTNO = nYSTNO;
        m_GetStatus.push_back(sts);
        return 3;
    }
    
    //?ú∑??ò??∑￠à?????
    int nSerCount = SList.size();
    //nSerCount = 1;
    if(nSerCount>0)
    {
        //∑￠à?￡∫??–?(4)+≥§??(4)+?”??–?(1)+±??è∫≈(4)+‘??”??∫≈??(4)
        char chdata[20]={0};
        int nt = JVN_CMD_YSTCHECK;
        int nl = 9;
        memcpy(&chdata[0], &nt, 4);
        memcpy(&chdata[4], &nl, 4);
        chdata[8] = 2;//≤è—????? ?ˉ±?”??¨Ω”?±μ?≤è—?????
        memcpy(&chdata[9], chGroup, 4);
        memcpy(&chdata[13], &nYSTNO, 4);
        nl += 8;
        for(int i=0; i<nSerCount; i++)
        {
            int n = CCChannel::sendtoclient(socketTemp, chdata, nl, 0, (sockaddr *)&SList[i].addr, sizeof(sockaddr_in), 1);
            int nn=0;
        }
    }
    else
    {
        shutdown(socketTemp,SD_BOTH);
        closesocket(socketTemp);
        
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°??μ?∫≈??∞?.‘≠“ú:∑??ò???–±ì???’", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get channel count faild.Info:no server find.", __FILE__,__LINE__);
        }
        
        m_dwLastTime = CCWorker::JVGetTime();
        STGETSTATUS sts;
        sts.dwLastTime = m_dwLastTime;
        memcpy(sts.chGroup, chGroup, 4);
        sts.nYSTNO = nYSTNO;
        m_GetStatus.push_back(sts);
        return 3;
    }
    
    //???o—≠a∑μ?￥?Ω”?’∑??ò??∑￥?°￡¨?áπ?”–“a∏?∑μa?‘???￡¨‘ú?￠o￥Ω·?ˉ￡¨∑ò‘úo?–ˉμ?￥?
    int nType;
    DWORD dwTimeOld,dwTimeNow;
    dwTimeOld = CCWorker::JVGetTime();
    BOOL bRun = TRUE;
    
    int nRecvcount = 0;
    BYTE recvBuf[JVN_ASPACKDEFLEN] = {0};
    while(bRun)
    {
        int l=0;
        if ((l=CCChannel::receivefrom(socketTemp, (char *)&recvBuf, JVN_ASPACKDEFLEN, 0, &sockAddr, &nSockAddrLen, 1)) > 0)
        {//Ω”?’￡∫??–?(4)+≥§??(4)+?”??–?(1)+±??è∫≈(4)+‘??”??∫≈??(4)+÷???–≠“è∞ê±?∫≈(4)+??∑ò‘???(1)
            nType = 0;
            memcpy(&nType, &recvBuf[0], 4);
            if(nType == JVN_CMD_YSTCHECK)
            {//?’μΩ∑??ò??≥…π?????
                int nl = 0, nystno = 0, nv = 0; char chg[4]={0};
                memcpy(&nl, &recvBuf[4], 4);
                memcpy(chg, &recvBuf[9], 4);
                memcpy(&nystno, &recvBuf[13], 4);
                memcpy(&nv, &recvBuf[17], 4);
                if(recvBuf[8] == 1 && (nystno == nYSTNO))// && (strcmp(chg, chGroup) == 0)
                {//’??∑μ?a?∏￥
                    if(recvBuf[21] == 1)
                    {
                        shutdown(socketTemp,SD_BOTH);
                        closesocket(socketTemp);
                        
                        m_dwLastTime = CCWorker::JVGetTime();
                        STGETSTATUS sts;
                        sts.dwLastTime = m_dwLastTime;
                        memcpy(sts.chGroup, chGroup, 4);
                        sts.nYSTNO = nYSTNO;
                        m_GetStatus.push_back(sts);
                        return 1;//‘???
                    }
                    else
                    {
                        nRecvcount++;//≤a‘???
                    }
                }//≤a??≈‰≤a’??∑μ?a?∏￥
            }//?’μΩ∑??ò???‰à?????
            
            dwTimeNow = CCWorker::JVGetTime();
            if(nRecvcount>=nSerCount)
            {//“—?’μΩ?à??πaa?∏￥,?…“‘Ω·?ˉ
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 2;//≤a‘???
            }
            else if(dwTimeNow < dwTimeOld)
            {//o??±??–?
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 3;//≤è—???∞?
            }
            else if(dwTimeNow  > dwTimeOld + nTimeOut*1000)
            {//√a”–??μΩ??πaμ?a?∏￥￡¨μ′“—?≠≥¨?±
                if(nRecvcount <= 0)
                {
                    shutdown(socketTemp,SD_BOTH);
                    closesocket(socketTemp);
                    
                    m_dwLastTime = CCWorker::JVGetTime();
                    STGETSTATUS sts;
                    sts.dwLastTime = m_dwLastTime;
                    memcpy(sts.chGroup, chGroup, 4);
                    sts.nYSTNO = nYSTNO;
                    m_GetStatus.push_back(sts);
                    return 3;//“a∏?a?∏￥√a?’μΩ￡¨≤è—???∞?
                }
                
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 2;//?’μΩ?àa?∏￥μ′≤a‘???￡¨∑μa?≤a‘???
            }
        }
        else
        {//≈–??Ω”?’π?≥???∑ò≥¨?±
            dwTimeNow = CCWorker::JVGetTime();
            if(nRecvcount>=nSerCount)
            {//“—?’μΩ?à??πaa?∏￥,?…“‘Ω·?ˉ
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 2;//≤a‘???
            }
            else if(dwTimeNow < dwTimeOld)
            {//o??±??–?
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 3;//≤è—???∞?
            }
            else if(dwTimeNow  > dwTimeOld + nTimeOut*1000)
            {//√a”–??μΩ??πaμ?a?∏￥￡¨μ′“—?≠≥¨?±
                if(nRecvcount <= 0)
                {
                    shutdown(socketTemp,SD_BOTH);
                    closesocket(socketTemp);
                    
                    m_dwLastTime = CCWorker::JVGetTime();
                    STGETSTATUS sts;
                    sts.dwLastTime = m_dwLastTime;
                    memcpy(sts.chGroup, chGroup, 4);
                    sts.nYSTNO = nYSTNO;
                    m_GetStatus.push_back(sts);
                    return 3;//“a∏?a?∏￥√a?’μΩ￡¨≤è—???∞?
                }
                
                shutdown(socketTemp,SD_BOTH);
                closesocket(socketTemp);
                
                m_dwLastTime = CCWorker::JVGetTime();
                STGETSTATUS sts;
                sts.dwLastTime = m_dwLastTime;
                memcpy(sts.chGroup, chGroup, 4);
                sts.nYSTNO = nYSTNO;
                m_GetStatus.push_back(sts);
                return 2;//?’μΩ?àa?∏￥μ′≤a‘???￡¨∑μa?≤a‘???
            }
        }
    }
    
    shutdown(socketTemp,SD_BOTH);
    closesocket(socketTemp);
    
    m_dwLastTime = CCWorker::JVGetTime();
    STGETSTATUS sts;
    sts.dwLastTime = m_dwLastTime;
    memcpy(sts.chGroup, chGroup, 4);
    sts.nYSTNO = nYSTNO;
    m_GetStatus.push_back(sts);
    return 3;
}

/****************************************************************************
 *√?≥?  : jvs_sleep
 *π???  : π“?? ms
 *≤???  : ??
 *∑μa?÷μ: ??
 *?‰à?  : ??
 *****************************************************************************/
void CCWorker::jvc_sleep(unsigned long time)//π“??
{
    
#ifndef WIN32
    //     DWORD EEE =CCWorker::JVGetTime();
    struct timeval start;
    start.tv_sec = 0;
    start.tv_usec = time*1000;
    select(0,NULL,NULL,NULL,&start);
    
    //usleep(time*1000);
    //    DWORD DDD = CCWorker::JVGetTime();
    //    printf("intime: %d, sleepTime: %d\n",time,(DDD-EEE));
    
#else
    Sleep(time);
#endif
}

/****************************************************************************
 *√?≥?  : GetTime
 *π???  : aò?°μ±?∞?±o‰ ms
 *≤???  : ??
 *∑μa?÷μ: ??
 *?‰à?  : ??
 *****************************************************************************/
DWORD CCWorker::JVGetTime()
{
    DWORD dwresult=0;
#ifndef WIN32
    struct timeval start;
    gettimeofday(&start,NULL);
    
    dwresult = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
#else
    dwresult = GetTickCount();
#endif
    
    return dwresult;
}


/*a?μ?∫???*/
//”?÷????à??–≈?￥?¨∫???(?¨Ω”?￥?¨)
void CCWorker::ConnectChange(int nLocalChannel, BYTE uchType, char *pMsg, int nPWData,const char* pFile,const int nLine,const char* pFun)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctCC);
#else
    EnterCriticalSection(&m_ctCC);
#endif
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        int nMsgLen = 0;
        if(!pMsg)
            nMsgLen = 0;
        else
            nMsgLen = strlen(pMsg);
        BYTE* pBuff = new BYTE[20 + nMsgLen];//1 type 1 uType 4 nPWData 4 len
        BYTE bType = JVC_HELP_CONNSTATUS;//√??ó ?￥?¨
        int nDataLen = nMsgLen + 9;
        memcpy(pBuff,&bType,1);
        memcpy(&pBuff[1],&nDataLen,4);
        
        memcpy(&pBuff[5],&uchType,1);
        memcpy(&pBuff[6],&nPWData,4);
        memcpy(&pBuff[10],&nMsgLen,4);
        if(nMsgLen > 0)
            memcpy(&pBuff[14],pMsg,nMsgLen);
        
        //		send(nLocalChannel,(char* )pBuff,nMsgLen + 14,0);
        
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pBuff,nMsgLen + 14,1);
        
        delete []pBuff;
    }
    else
    {
        g_dbg.jvcout(OUT_ON_EVENT,pFile,nLine,pFun,"jvcConnectCallBack:LOCH_%d,uchType_%d,pMsg_%s",nLocalChannel,uchType,pMsg);
        if(m_pLanSerch)
        {
            m_pLanSerch->m_bPause[nLocalChannel] = FALSE;
        }
        m_pfConnect(nLocalChannel, uchType, pMsg, nPWData);
#ifdef MOBILE_CLIENT
        if(uchType == JVN_CCONNECTTYPE_CONNOK || uchType == JVN_CCONNECTTYPE_CONNERR)
        {
            DWORD endtime2 = JVGetTime();
            OutputDebug("connectSpendtime: %d\n",(JVGetTime() - m_dwConnectTime));
        }
#endif
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctCC);
#else
    LeaveCriticalSection(&m_ctCC);
#endif
}

//?μ?±o???￥??ì(?’μΩ?μ?±o???????)//‘?≥?a?∑≈∫???(?’μΩ‘?≥?a?∑≈????)
void CCWorker::NormalData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctND);
#else
    EnterCriticalSection(&m_ctND);
#endif
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        int nDataLen = nSize + 5;
        memcpy(&pData[1],&nDataLen,4);
        memcpy(&pData[5],&uchType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],pBuffer,nSize);
        //		send(nLocalChannel,(char* )pData,nSize + 10,0);
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 10,1);
        
        delete []pData;
        
    }
    else
    {//OutputDebug("nlocalchannel: %d, nsize: %d,uchType: %d\n",nLocalChannel,nSize,uchType);
        m_pfNormalData(nLocalChannel,uchType, pBuffer, nSize, nWidth, nHeight);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctND);
#else
    LeaveCriticalSection(&m_ctND);
#endif
}

//?o?òo?à?Ω·π?￥??ì∫???(?’μΩ?o?òo?à?Ω·π?)
void CCWorker::CheckResult(int nLocalChannel,BYTE *pBuffer, int nSize)
{
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        
        int nDataLen = nSize + 5;
        memcpy(&pData[1],&nDataLen,4);
        
        bType = JVN_RSP_CHECKDATA;//?ìo”0 ??”??‰à?????±￡≥÷“a÷?
        memcpy(&pData[5],&bType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],pBuffer,nSize);
        //		send(nLocalChannel,(char* )pData,nSize + 10,0);
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 10,1);
        
        delete []pData;
        
    }
    else
    {
        m_pfCheckResult(nLocalChannel,pBuffer, nSize);
    }
}

//”?“ù????∫???(‘?≥?”?“ù)
void CCWorker::ChatData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize)
{
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        int nDataLen = nSize + 5;
        memcpy(&pData[1],&nDataLen,4);
        
        memcpy(&pData[5],&uchType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],pBuffer,nSize);
        //		send(nLocalChannel,(char* )pData,nSize + 10,0);
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 10,1);
        
        delete []pData;
        
    }
    else
    {
        m_pfChatData( nLocalChannel,  uchType,  pBuffer,  nSize);
    }
}

//??±?????∫???(‘?≥???±?????)
void CCWorker::TextData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize)
{
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        int nDataLen = nSize + 5;
        memcpy(&pData[1],&nDataLen,4);
        
        memcpy(&pData[5],&uchType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],pBuffer,nSize);
        //		send(nLocalChannel,(char* )pData,nSize + 10,0);
        
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 10,1);
        delete []pData;
        
    }
    else
    {
        m_pfTextData( nLocalChannel,  uchType,  pBuffer,  nSize);
    }
}

//‘?≥???‘?
void CCWorker::DownLoad(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nFileLen)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctDL);
#else
    EnterCriticalSection(&m_ctDL);
#endif
    
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        int nDataLen = nSize + 5 + 4;
        memcpy(&pData[1],&nDataLen,4);
        
        memcpy(&pData[5],&uchType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],&nFileLen,4);
        memcpy(&pData[14],pBuffer,nSize);
        
        //	int mmm = send(nLocalChannel,(char* )pData,nSize + 14,0);
        
        //int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 14,1);
        
        //??????Ω?￥? —≠a∑∑￠à?
        {
            int nLen = nSize + 14;
            int ss = 0;
            int ssize = 0;
            DWORD dwStart = CCWorker::JVGetTime();
            DWORD dwCurrent = CCWorker::JVGetTime();
            while(ssize < nLen && (dwCurrent - 5000 <= dwStart))
            {
                dwCurrent = CCWorker::JVGetTime();
                
                ss = CCChannel::tcpsend(nLocalChannel,(char *)pData + ssize, nLen - ssize, 0);
                
                if(ss > 0)
                {
                    ssize += ss;
                }
                else if(ss == 0)
                {
                    CCWorker::jvc_sleep(1);
                    continue;
                }
                else
                {
                    closesocket(nLocalChannel);
                    
                    //OutputDebugString("“?≥￡????.....................\n");
                    break;
                }
            }
        }
        
        delete []pData;
        
    }
    else
    {
        m_pfDownLoad( nLocalChannel,  uchType,  pBuffer,  nSize, nFileLen);
    }
    
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ctDL);
#else
    LeaveCriticalSection(&m_ctDL);
#endif
}

//‘?≥?a?∑≈
void CCWorker::PlayData(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame, BYTE uchHelpType, BYTE *pHelpBuf, int nHelpSize)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctPD);
#else
    EnterCriticalSection(&m_ctPD);
#endif
    
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        //BYTE* pData = new BYTE[nSize + 20];
        BYTE* pData = new BYTE[nHelpSize + 10];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        //int nDataLen = nSize + 5;
        int nDataLen = nHelpSize + 5;
        memcpy(&pData[1],&nDataLen,4);
        
        //memcpy(&pData[5],&uchType,1);
        //memcpy(&pData[6],&nSize,4);
        //memcpy(&pData[10],pBuffer,nSize);
        //send(nLocalChannel,(char* )pData,nSize + 10,0);
        memcpy(&pData[5],&uchHelpType,1);
        memcpy(&pData[6],&nHelpSize,4);
        memcpy(&pData[10],pHelpBuf,nHelpSize);
        //		send(nLocalChannel,(char* )pData,nHelpSize + 10,0);
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nHelpSize + 10,1);
        
        delete []pData;
        
    }
    else
    {
        m_pfPlayData( nLocalChannel,  uchType,  pBuffer,  nSize, nWidth, nHeight, nTotalFrame);
    }
    
#ifndef WIN32
    pthread_mutex_unlock(&m_ctPD);
#else
    LeaveCriticalSection(&m_ctPD);
#endif
}

//a∫≥?Ωˉ??
void CCWorker::BufRate(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize, int nRate)
{
    if(m_pHelpCtrl && m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
    {
        BYTE* pData = new BYTE[nSize + 20];
        BYTE bType = JVC_HELP_DATA;//’?≥￡μ?÷????￥????
        memcpy(pData,&bType,1);
        int nDataLen = nSize + 9;
        memcpy(&pData[1],&nDataLen,4);
        
        uchType = JVN_DATA_RATE;
        memcpy(&pData[5],&uchType,1);
        memcpy(&pData[6],&nSize,4);
        memcpy(&pData[10],&nRate,4);
        memcpy(&pData[14],pBuffer,nSize);
        //		send(nLocalChannel,(char* )pData,nSize + 14,0);
        int nSend = CCChannel::tcpsend(nLocalChannel,(char* )pData,nSize + 14,1);
        
        delete []pData;
    }
    else
    {
        m_pfBufRate(nLocalChannel, uchType, pBuffer, nSize, nRate + 1);
    }
}

void CCWorker::EnableLog(bool bEnable)
{//bEnable=TRUE;
    m_Log.EnableLog(bEnable, m_chLocalPath);
    m_bNeedLog = bEnable;
}

void CCWorker::SetLanguage(int nLgType)
{
    m_nLanguage = nLgType;
    m_Log.m_nLanguage = nLgType;
}

bool CCWorker::SetWebName(char *pchDomainName,char *pchPathName)
{
    strcpy(m_strDomainName,pchDomainName);
    strcpy(m_strPathName,pchPathName);
    
    return true;
}










/****************************************************************************
 *√?≥?  : SetUpnpProc
 *π???  : upnp…?÷√??≥?
 *≤???  : ??
 *∑μa?÷μ: ??
 *?‰à?  : ”…”?∑￠?÷…?±∏μ??±o‰?ó≥§4s￡¨??≤a”∞??≥??oa?–???￡¨∑≈?????￠??≥?
 *****************************************************************************/
#ifdef WIN32
UINT WINAPI CCWorker::SetUpnpProc(LPVOID pParam)
#else
void* CCWorker::SetUpnpProc(void* pParam)
#endif
{
    CCWorker *pWorker = (CCWorker *)pParam;
    
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
    int ntmp = 0;
    srand((int)time(0));
    ntmp = random();
    if(0 == pWorker->m_pUpnpCtrl->AddPortMap(ntmp,pWorker->m_nLocalStartPort,"UDP"))
    {
        pWorker->m_nLocalPortWan = ntmp;
        //		OutputDebug("Set upnp %d.",pWorker->m_nLocalPortWan);
    }
    else
    {
        pWorker->m_nLocalPortWan = 0;
        //		OutputDebug("Set upnp err.");
    }
    
#ifndef WIN32
    return NULL;
#else
    return 0;
#endif
}

/****************************************************************************
 *√?≥?  : GetIPNatProc
 *π???  : ?úà?”–∑??ò??≤è—?±?μ?NATμ???≥?
 *≤???  : ??
 *∑μa?÷μ: ??
 *****************************************************************************/
#ifdef WIN32
UINT WINAPI CCWorker::GetIPNatProc(LPVOID pParam)
#else
void* CCWorker::GetIPNatProc(void* pParam)
#endif
{
    CCWorker *pWorker = (CCWorker *)pParam;
    
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
    
    DWORD dwStart = 0;
    DWORD dwEnd = CCWorker::JVGetTime();
    while(TRUE)
    {
#ifndef WIN32
        if(pWorker->m_bNatGetEnd)
        {
            break;
        }
#else
        if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hNatGetEndEvent, 0))
        {
            if(pWorker->m_hNatGetEndEvent > 0)
            {
                CloseHandle(pWorker->m_hNatGetEndEvent);
                pWorker->m_hNatGetEndEvent = 0;
            }
            
            break;
        }
#endif
        
        dwEnd = CCWorker::JVGetTime();
        if(dwEnd - dwStart > 30 * 1000)
        {
            dwStart = dwEnd;
            
            pWorker->GetLocalIPList();//∏?–?±?μ????ˉμ?÷∑
        }
        CCWorker::jvc_sleep(10);
    }
    
    
#ifndef WIN32
    return NULL;
#else
    return 0;
#endif
}

void CCWorker::GetLocalIPList()//??≥?μ?”√
{
    ::std::vector<NAT> IPList;//±?μ?μ?IP?–±ì
    //?ìo”∏?0 0÷??∞??
#ifdef _WIN32
    char hostname[128] = {0};
    if(gethostname(hostname,128) == 0)
    {
        struct hostent *pHost = gethostbyname(hostname);
        for (int i = 0; pHost != NULL && pHost->h_addr_list[i] != NULL; i++)
        {
            NAT nat;
            
            memcpy(&nat.ip,pHost->h_addr_list[i],4);
            nat.port = m_nLocalStartPort;
            
            //			OutputDebug("Add local ip %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
            IPList.push_back(nat);
        }
    }
#else
#ifdef MOBILE_CLIENT//?÷a˙??a??à
    char strIP[20] = {0};
    BYTE bIP[4] = {0};
    if(GetLocalMobileIP(strIP,bIP))
    {
        NAT nat;
        
        memcpy(nat.ip,bIP,4);
        nat.port = m_nLocalStartPort;
        
        //			OutputDebug("Add local ip %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
        IPList.push_back(nat);
    }
#else
    _Adapter info;
    memset(&info,0,sizeof(_Adapter));
    //linux ??
    SOCKADDR_IN addrA;//±?μ?μ?÷∑
    SOCKADDR_IN my_addr;
    struct ifreq ifr;
    int addrlen = sizeof(SOCKADDR_IN);
    
    SOCKET tsock = socket(AF_INET, SOCK_DGRAM,0);
    SOCKADDR_IN addrL;
    addrL.sin_addr.s_addr = htonl(INADDR_ANY);
    addrL.sin_family = AF_INET;
    addrL.sin_port = htons(0);
    //∞?????Ω”?÷
    bind(tsock, (SOCKADDR *)&addrL, sizeof(SOCKADDR));
    
    getsockname(tsock,(struct sockaddr *)&addrA,(socklen_t*)&addrlen);
    /**//* Get IP Address */
    strncpy(ifr.ifr_name, "eth0", IF_NAMESIZE);
    ifr.ifr_name[IFNAMSIZ-1]='\0';
    if (ioctl(tsock, SIOCGIFADDR, &ifr) < 0)
    {
        //error
    }
    memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
    sprintf(info.IP,"%s",inet_ntoa(my_addr.sin_addr));
    
    unsigned int n[4] = {0};
    sscanf(info.IP,"%d.%d.%d.%d",&n[0],&n[1],&n[2],&n[3]);
    
    closesocket(tsock);
    
    NAT nat;
    
    //	memcpy(nat.ip,n,4);
    nat.ip[0] = n[0];
    nat.ip[1] = n[1];
    nat.ip[2] = n[2];
    nat.ip[3] = n[3];
    
    nat.port = m_nLocalStartPort;
    
    IPList.push_back(nat);
    
#endif
#endif
    
    //	NAT nat;
    //	IPList.push_back(nat);
    
#ifndef WIN32
    pthread_mutex_lock(&m_ctlocalip);
#else
    EnterCriticalSection(&m_ctlocalip);
#endif
    m_LocalIPList.clear();
    int ncount = IPList.size();
    for(int i=0; i<ncount; i++)
    {
        m_LocalIPList.push_back(IPList[i]);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctlocalip);
#else
    LeaveCriticalSection(&m_ctlocalip);
#endif
    
    //	OutputDebug("Add local IP %d",m_NatListAB.size());
}

void CCWorker::GetLocalIP(NATList *pNatList)
{
#ifndef WIN32
    pthread_mutex_lock(&m_ctlocalip);
#else
    EnterCriticalSection(&m_ctlocalip);
#endif
    int ncount = m_LocalIPList.size();
    int i = 0;
    for(i=0; i<ncount; i++)
    {
        pNatList->push_back(m_LocalIPList[i]);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctlocalip);
#else
    LeaveCriticalSection(&m_ctlocalip);
#endif
}

/*
 int CCWorker::GetWANIPList(ServerList slisttmp,NATList *pNatList,int nTimeOut)
 {
	UDTSOCKET s = UDT::socket(AF_INET, SOCK_STREAM, 0);
	BOOL bReuse = TRUE;
	UDT::setsockopt(s, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
	//////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
	UDT::setsockopt(s, 0, UDT_MSS, &len1, sizeof(int));
	//////////////////////////////////////////////////////////////////////////
	UDT::bind(s, m_WorkerUDPSocket);
 
	//Ω′??Ω”?÷÷√??∑??????￡?Ω
	BOOL block = FALSE;
	UDT::setsockopt(s, 0, UDT_SNDSYN, &block, sizeof(BOOL));
	UDT::setsockopt(s, 0, UDT_RCVSYN, &block, sizeof(BOOL));
	LINGER linger;
	linger.l_onoff = 0;
	linger.l_linger = 0;
	UDT::setsockopt(s, 0, UDT_LINGER, &linger, sizeof(LINGER));
 
	int nRet = 0;
 //	OutputDebug("Start Get wan nat...");
 
	BYTE ip_list[1024] = {0};//–?“?≤è—? ?¨Ω”μ?∑??ò???–±ì
 
	int nc = slisttmp.size();
 
	for(int i=0; i<nc; i++)
	{
 unsigned short port = ntohs(slisttmp[i].addr.sin_port);
 memcpy(&ip_list[i * 6],&slisttmp[i].addr.sin_addr,4);
 memcpy(&ip_list[i * 6 + 4],&port,2);
	}
 
	int nret  = 0;
	nret = UDT::getlocalnat(s,ip_list,nc,nTimeOut);
 
 //	OutputDebug("recv nat %d",nret);
 
	//version 4 online 4 a_nat 16 b_nat 16
	SERVER_INFO stINFO;
	int nstlen = sizeof(SERVER_INFO);
	memset(&stINFO, 0, nstlen);
 
	//
	for(int k = 0;k < nret;k ++)
	{
 memcpy(&stINFO, &ip_list[nstlen * k], nstlen);
 
 NAT nat;
 memcpy(nat.ip, &stINFO.unAddr, 4);
 nat.port = stINFO.unPort;
 nat.nSVer = stINFO.nSVer;
 nat.unseraddr = stINFO.unSerAddr;
 
 //		OutputDebug("NAT port %d",port);
 //		OutputDebug("Add local NAT %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
 pNatList->push_back(nat);
 
 //∞—UPNP?ìo”…?
 if(m_nLocalPortWan > 0)
 {
 nat.port = m_nLocalPortWan;
 
 //			OutputDebug("Add local NAT %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
 pNatList->push_back(nat);
 }
	}
 
	UDT::close(s);
 
 //	OutputDebug("Get lcoal nat finish. %d",pNatList->size());
	return nRet;
 }
 */
/****************************************************************************
 *√?≥?  : GetNatADDR
 *π???  : aò?°±?μ????ˉμ?÷∑∫????ˉμ?÷∑
 *≤???  : ??
 *∑μa?÷μ: ??
 *?‰à?  : ??
 *****************************************************************************/
void CCWorker::GetNATADDR(NATList pNatListAll,NATList *pNatList, SOCKADDR_IN addrs, int &nSVer)//channelμ?”√
{
    if(pNatList == NULL)
    {
        return;
    }
    
    nSVer = 0;
    
    unsigned int unseraddr = 0;
    memcpy(&unseraddr, &addrs.sin_addr, 4);
    
    pNatList->clear();
    
#ifndef WIN32
    pthread_mutex_lock(&m_ctlocalip);
#else
    EnterCriticalSection(&m_ctlocalip);
#endif
    int ncount = m_LocalIPList.size();
    int i = 0;
    for(i=0; i<ncount; i++)
    {
        pNatList->push_back(m_LocalIPList[i]);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctlocalip);
#else
    LeaveCriticalSection(&m_ctlocalip);
#endif
    
    BOOL bfind = FALSE;
    ncount = pNatListAll.size();
    for(i=0; i<ncount; i++)
    {
        //////////////////////////////////////////////////////////////////////////
        bfind = FALSE;
        int ncn = pNatList->size();
        for(int ni = 0; ni<ncn; ni++)
        {
            if(pNatListAll[i].port == (*pNatList)[ni].port
               && pNatListAll[i].ip[0] == (*pNatList)[ni].ip[0]
               && pNatListAll[i].ip[1] == (*pNatList)[ni].ip[1]
               && pNatListAll[i].ip[2] == (*pNatList)[ni].ip[2]
               && pNatListAll[i].ip[3] == (*pNatList)[ni].ip[3])
            {//??≈‰￡¨’“μΩo??o,≤…?°????∫???‘?∑Ω?Ω
                bfind = TRUE;
                //OutputDebug("NAT exist %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
                break;
            }
        }
        
        if(!bfind)
        {
            //				OutputDebug("Add local NAT %d.%d.%d.%d:%d",nat.ip[0],nat.ip[1],nat.ip[2],nat.ip[3],nat.port);
            pNatList->push_back(pNatListAll[i]);
        }
        //////////////////////////////////////////////////////////////////////////
        
        if(pNatListAll[i].unseraddr == unseraddr && unseraddr > 0)
        {//’“μΩ’?∏?∑??ò??μ?”?￥?,’????…“‘”√μΩ∑??ò??μ?∞ê±?∫≈
            nSVer = pNatListAll[i].nSVer;
        }
    }
}

int CCWorker::AddFSIpSection(const IPSECTION * Ipsection ,int nSize, BOOL bEnablePing)//?ìo”￥?à—à?IP??;
{
    if (NULL==Ipsection||nSize<=0 )//√a”–????;
    {
        return -1;
    }
    return m_pLanSerch->AddFSIpSection(Ipsection,nSize,bEnablePing);
}


char* CCWorker::GetLocalMobileIP(char* pIP,BYTE *bIP)//±?∫??????÷a˙?π”√ aò?°μ±?∞IP￡¨÷a?°?ó∫?“a∏?μ?÷∑
{
    if(!pIP)
        return NULL;
#ifdef MOBILE_CLIENT//?÷a˙??a??à
#define BUFFERSIZE  4000
    int                 len, flags;
    char                buffer[BUFFERSIZE], *ptr, lastname[IFNAMSIZ], *cptr;
    struct ifconf       ifc;
    struct ifreq        *ifr, ifrcopy;
    struct sockaddr_in  *sin;
    
    int sockfd;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        perror("socket failed");
        return NULL;
    }
    
    ifc.ifc_len = BUFFERSIZE;
    ifc.ifc_buf = buffer;
    
    if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
    {
        close(sockfd);
        perror("ioctl error");
        return NULL;
    }
    
    lastname[0] = 0;
    
    for(ptr = buffer; ptr < buffer + ifc.ifc_len;)
    {
        ifr = (struct ifreq *)ptr;
        len = jvs_max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
        ptr += sizeof(ifr->ifr_name) + len;
        
        if (ifr->ifr_addr.sa_family != AF_INET)
        {
            continue;
        }
        
        if ((cptr = (char *)strchr(ifr->ifr_name, ':')) != NULL)
        {
            *cptr = 0;
        }
        
        if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)
        {
            continue;
        }
        
        memcpy(lastname, ifr->ifr_name, IFNAMSIZ);
        
        ifrcopy = *ifr;
        ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        if ((flags & IFF_UP) == 0)
        {
            continue;
        }
        
        sin = (struct sockaddr_in *)&ifr->ifr_addr;
        
        strcpy(pIP, inet_ntoa(sin->sin_addr));//aò?°?ó∫?“a∏?IP
    }
    
    close(sockfd);
    char str[100] = {0};
    strcpy(str,pIP);
    sscanf(str,"%d.%d.%d.%d",&bIP[0],&bIP[1],&bIP[2],&bIP[3]);
	//printf("GetLocalIP : %s\n",pIP);
#endif
    return pIP;
}



//aò?°o?à?∑??ò???–±ì
BOOL CCWorker::GetIndexServerList(char chGroup[4], ServerList &SList, int nWebIndex, int nLocalPort)
{
    SList.clear();
    
//     	STSERVER sts;
//     	sts.addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//     	sts.addr.sin_family = AF_INET;
//     	sts.addr.sin_port = htons(8633);
//     	sts.buseful = TRUE;
//     	sts.nver = 0;
//     	SList.push_back(sts);//o??o
//     	return TRUE;
    
    BOOL bfind = FALSE;
#ifndef WIN32
    pthread_mutex_lock(&m_ctGroup);
#else
    EnterCriticalSection(&m_ctGroup);
#endif
    int ngcount = m_IndexGroupList.size();
    for(int c=0; c<ngcount; c++)
    {
        if(0 == strcmp(chGroup, m_IndexGroupList[c].chgroup))
        {//??≈‰￡¨’“μΩo??o,≤…?°????∫???‘?∑Ω?Ω
            bfind = TRUE;
            break;
        }
    }
    if(!bfind)
    {
        //√?￥?‘à––≥?–ú?¨“a∏?±??è∫≈÷a??‘?“a￥?￡¨±?√??à∑—
        STGROUP stg;
        memcpy(stg.chgroup, chGroup, 4);
        m_IndexGroupList.push_back(stg);
    }
#ifndef WIN32
    pthread_mutex_unlock(&m_ctGroup);
#else
    LeaveCriticalSection(&m_ctGroup);
#endif
    
    std::vector<STSIP> IPList;
    if(bfind)
    {
        //??≥￠?‘±?μ?‘???￡¨‘?≥￠?‘??‘?
        if(!IndexServerList_Load(chGroup, IPList))
        {
            return FALSE;
            /*			if(!IndexServerList_Download(chGroup, IPList, nWebIndex, nLocalPort))
             {
             return FALSE;
             }
             else
             {
             IndexServerList_Save(chGroup, IPList);
             }*/
        }
    }
    else
    {
        BOOL bfindgroup = FALSE;
        for(int l=0; l<m_nGroupCount; l++)
        {
            if(strcmp(chGroup, m_chGroupList[l]) == 0)
            {
                bfindgroup = TRUE;
            }
        }
        
        if(!bfindgroup)
        {//±??è≤a‘?‘§…?μ?∑?????￡¨–?“?????‘?
            return FALSE;
            //??≥￠?‘??‘?￡¨‘?≥￠?‘±?μ?‘???
            /*		    if(!IndexServerList_Download(chGroup, IPList, nWebIndex, nLocalPort))
             {
             return FALSE;
             }
             else
             {
             IndexServerList_Save(chGroup, IPList);
             }*/
        }
        else
        {
            //??≥￠?‘±?μ?‘???￡¨‘?≥￠?‘??‘?
            if(!IndexServerList_Load(chGroup, IPList))
            {
                return FALSE;
                /*	if(!IndexServerList_Download(chGroup, IPList, nWebIndex, nLocalPort))
                 {
                 return FALSE;
                 }
                 else
                 {
                 IndexServerList_Save(chGroup, IPList);
                 }*/
            }
        }
    }
    
    //OutputDebug("o?à?∑??ò?? %s",chGroup);
    for(size_t i = 0; i < IPList.size(); ++i)
    {
        STSERVER sts;
#ifndef WIN32
        sts.addr.sin_addr.s_addr = inet_addr(IPList[i].chIP);
#else
        sts.addr.sin_addr.S_un.S_addr = inet_addr(IPList[i].chIP);
#endif
        sts.addr.sin_family = AF_INET;
        sts.addr.sin_port = htons(IPList[i].nPort);
        sts.buseful = TRUE;
        sts.nver = 0;
        SList.push_back(sts);
        
        //		OutputDebug("%d %s : %d",i + 1,IPList[i].chIP,IPList[i].nPort);
        
    }
    
    return TRUE;
}

BOOL CCWorker::IndexServerList_Download(char chGroup[4], std::vector<STSIP> &IPList, int nWebIndex, int nLocalPort)
{
    sockaddr_in sin;
    BOOL bFindSite=FALSE;
    char chSiteIP[30]={0};
    char recvBuf[1024] = {0};
    char chIPBuf[1024] = {0};
    
    char target[1024] = {0};
    char target_e[1024] = {0};
    int our_socket;
    struct sockaddr_in our_address;
    
    strcpy(target, "GET http://");
    strcpy(target_e, "GET ");
    if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
    {
        if(1 == nWebIndex)
        {
            strcat(target, JVN_WEBSITE1);
            //strcat(target_e, JVN_WEBSITE1);
        }
        else
        {
            strcat(target, JVN_WEBSITE2);
            //strcat(target_e, JVN_WEBSITE2);
        }
        strcat(target, JVN_WEBFOLDER);
        strcat(target, chGroup);
        strcat(target, JVN_YSTLIST_INDEX);
        strcat(target," HTTP/1.1\r\n");
        strcat(target,"Host:");
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e, JVN_WEBFOLDER);
        strcat(target_e, chGroup);
        strcat(target_e, JVN_YSTLIST_INDEX);
        strcat(target_e," HTTP/1.1\r\n");
        strcat(target_e,"Host:");
        //////////////////////////////////////////////////////////////////////////
        
        if(1 == nWebIndex)
        {
            strcat(target, JVN_WEBSITE1);
            strcat(target_e, JVN_WEBSITE1);
        }
        else
        {
            strcat(target, JVN_WEBSITE2);
            strcat(target_e, JVN_WEBSITE2);
        }
    }
    else
    {
        strcat(target, m_strDomainName);
        strcat(target, m_strPathName);
        
        strcat(target," HTTP/1.1\r\n");
        strcat(target,"Host:");
        
        strcat(target, m_strDomainName);
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e, m_strDomainName);
        strcat(target_e, m_strPathName);
        
        strcat(target_e," HTTP/1.1\r\n");
        strcat(target_e,"Host:");
        
        strcat(target_e, m_strDomainName);
        //////////////////////////////////////////////////////////////////////////
    }
    
    strcat(target,"\r\n");
    strcat(target,"Accept:*/*\r\n");
    //////////////////////////////////////////////////////////////////////////
    strcat(target_e,"\r\n");
    strcat(target_e,"Accept:*/*\r\n");
    strcat(target_e, JVN_AGENTINFO);
    //////////////////////////////////////////////////////////////////////////
    strcat(target,"Connection:Keep-Alive\r\n\r\n");
    strcat(target_e,"Connection:Keep-Alive\r\n\r\n");
    
    struct hostent * site=NULL;
    if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
    {
        if(1 == nWebIndex)
        {
            char severname[] = JVN_WEBSITE1;
            site = gethostbyname (severname);
            if(site == NULL)
            {
                CSDNSCtrl csdns;
                bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
            }
        }
        else
        {
            char severname[] = JVN_WEBSITE2;
            site = gethostbyname (severname);
            if(site == NULL)
            {
                CSDNSCtrl csdns;
                bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
            }
        }
    }
    else
    {
        site = gethostbyname (m_strDomainName);
        if(site == NULL)
        {
            CSDNSCtrl csdns;
            bFindSite = csdns.GetIPByDomain(m_strDomainName, chSiteIP);
        }
    }
    
    if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
        }
        
        return FALSE;
    }
    
    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
    
#ifndef WIN32
    sin.sin_addr.s_addr = INADDR_ANY;
#else
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
    
    sin.sin_family = AF_INET;
    sin.sin_port = htons(nLocalPort);
    if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
    }
    
    if(site == NULL && !bFindSite)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:aò?°”ú√?μ?÷∑??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:get hostsite failed.", __FILE__,__LINE__);
        }
        
        closesocket(our_socket);
        return FALSE;
    }
    
    memset(&our_address, 0, sizeof(struct sockaddr_in));
    our_address.sin_family = AF_INET;
    if(site != NULL)
    {
        memcpy (&our_address.sin_addr, site->h_addr_list[0], site->h_length);
    }
    else if(bFindSite)
    {
        our_address.sin_addr.s_addr = inet_addr(chSiteIP);
    }
    our_address.sin_port = htons(80);
    
    if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
        }
        
        closesocket(our_socket);
        return FALSE;
    }
    
    if(CCChannel::tcpsend(our_socket,target,strlen(target),3)<=0)
    {
        closesocket(our_socket);
        return FALSE;
    }
    
    int nLen = 0;
    int tryTimes=0;
    while(tryTimes<2)
    {
        int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
        if(rlen < 0 || (nLen+rlen)>1024)
        {
            break;
        }
        memcpy(&chIPBuf[nLen], recvBuf, rlen);
        nLen += rlen;
        tryTimes++;
    }
    
    closesocket(our_socket);
    our_socket = 0;
    
    if(nLen == 0)
    {//√a?’μΩ??∫?????￡¨∞￥–?????∞?÷?–?????“a￥?
        if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
            }
            
            return FALSE;
        }
        
        LINGER linger2;
        linger2.l_onoff = 1;
        linger2.l_linger = 0;
        setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger2, sizeof(LINGER));
        
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        
        sin.sin_family = AF_INET;
        sin.sin_port = htons(nLocalPort);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                return FALSE;
            }
        }
        
        if(connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
        
        if(CCChannel::tcpsend(our_socket,target_e,strlen(target_e),3)<=0)
        {
            closesocket(our_socket);
            return FALSE;
        }
        
        tryTimes=0;
        while(tryTimes<2)
        {
            int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
            if(rlen < 0 || (nLen+rlen)>1024)
            {
                break;
            }
            memcpy(&chIPBuf[nLen], recvBuf, rlen);
            nLen += rlen;
            tryTimes++;
        }
    }
    
    if(our_socket > 0)
    {
        closesocket(our_socket);
    }
    
    //Ω???μ?÷∑
    if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
    {
        char chIP[50] = {0};
        char chPort[50] = {0};
        //char chTmp[50]={0};
        int nStart = 0, nMid = 0, nEnd = 0;
        for(int i=0; i<nLen; i++)
        {
            memset(chIP, 0, 50);
            memset(chPort, 0, 50);
            if(chIPBuf[i] == 'I' && chIPBuf[i+1] == 'P' && chIPBuf[i+2] == ':')
            {//’“μΩ???o?a÷√
                nStart = i+3;
                nMid = nStart;
                nEnd = nMid;
                
                for(int j=nStart; j<nLen; j++)
                {
                    if(chIPBuf[j] == ':')
                    {//’“μΩ∑÷∏ù∑?,÷?￥àIP?…Ω?≥?
                        nMid = j+1;
                        nEnd = nMid;
                        
                        for(int k=nMid; k<nLen; k++)
                        {
                            if(chIPBuf[k] < '0' || chIPBuf[k] > '9')
                            {
                                nEnd = k;
                                break;
                            }
                            if(k==nLen-1)
                            {
                                nEnd = k+1;
                            }
                        }
                        break;
                    }
                }
                
                STSIP stIP;
                memset(stIP.chIP, 0, 16);
                
                memcpy(chIP, &chIPBuf[nStart], nMid-nStart-1);
                memcpy(stIP.chIP, &chIPBuf[nStart], nMid-nStart-1);//
                memcpy(chPort, &chIPBuf[nMid], nEnd-nMid);
                chIP[nMid-nStart-1] = ':';
                memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                chPort[nEnd-nMid]='\0';
                stIP.nPort = atoi(chPort);
                if(i>0 && chIPBuf[i-1] == 'D')
                {
                    stIP.nISP = JVNC_ISP_DX;
                }
                else
                {
                    stIP.nISP = JVNC_ISP_WT;//≤a??≈–??μ?μ??è??“a∏≈πè???ˉ??
                }
                
                if(stIP.nPort > 0)
                {
                    IPList.push_back(stIP);
                }
                
                i = nEnd+1;
            }
            else if(chIPBuf[i] == '\0')
            {
                break;
            }
        }
    }
    else//?π”√ –?”ú√??± –?“?’“μΩo”√?μ?–≈?￠Ω?√?
    {
        char chIP[50] = {0};
        char chPort[50] = {0};
        int nStart = 0, nMid = 0, nEnd = 0;
        int i=0;
        for(i=0; i<nLen; i++)
        {
            memset(chIP, 0, 50);
            memset(chPort, 0, 50);
            if(chIPBuf[i] == 'I' && chIPBuf[i+1] == 'P'
               && chIPBuf[i+2] == '_'
               && chIPBuf[i+3] == 'S' && chIPBuf[i+4] == 'T'
               && chIPBuf[i+5] == 'A' && chIPBuf[i+6] == 'R'
               && chIPBuf[i+7] == 'T')//IP_START:
            {//’“μΩ???o?a÷√
                nStart = i+9;
                memmove(chIPBuf,&chIPBuf[nStart],nLen-nStart);
                nLen-=nStart;
                chIPBuf[nLen] = '\0';
                MultiMaskRemove(chIPBuf,nLen);
                break;
            }
        }
        for(i=0; i<nLen; i++)
        {
            memset(chIP, 0, 50);
            memset(chPort, 0, 50);
            if(chIPBuf[i] == 'I' && chIPBuf[i+1] == 'P' && chIPBuf[i+2] == ':')
            {
                nStart = i + 3;
                nMid = nStart;
                nEnd = nMid;
                
                for(int j=nStart; j<nLen; j++)
                {
                    if(chIPBuf[j] == ':')
                    {//’“μΩ∑÷∏ù∑?,÷?￥àIP?…Ω?≥?
                        nMid = j+1;
                        nEnd = nMid;
                        
                        for(int k=nMid; k<nLen; k++)
                        {
                            if(chIPBuf[k] < '0' || chIPBuf[k] > '9')
                            {
                                nEnd = k;
                                break;
                            }
                            if(k==nLen-1)
                            {
                                nEnd = k+1;
                            }
                        }
                        break;
                    }
                }
                
                STSIP stIP;
                memset(stIP.chIP, 0, 16);
                
                memcpy(chIP, &chIPBuf[nStart], nMid-nStart-1);
                memcpy(stIP.chIP, &chIPBuf[nStart], nMid-nStart-1);//
                memcpy(chPort, &chIPBuf[nMid], nEnd-nMid);
                chIP[nMid-nStart-1] = ':';
                memcpy(&chIP[nMid-nStart], chPort, nEnd-nMid);
                chPort[nEnd-nMid]='\0';
                stIP.nPort = atoi(chPort);
                
                if(stIP.nPort > 0)
                {
                    IPList.push_back(stIP);
                }
                
                i = nEnd;
            }
            else if(chIPBuf[i] == '\0')
            {
                break;
            }
        }
    }
    
    return IPList.size() > 0;
}

BOOL CCWorker::IndexServerList_Load(char chGroup[4], std::vector<STSIP> &IPList)
{
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    char acBuff[MAX_PATH] = {0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//√a”–√??∑÷∏???∑??￡¨‘ú‘?dvr??≤…”√π????∑???ˉ≤a???μo?μ±?∞?∑??
        sprintf(acBuff, "%s/INDEX_%s.dat", "./data",chGroup);
    }
    else
    {//?π”√÷∏??μ??∑??
        sprintf(acBuff, "%s/INDEX_%s.dat", m_chLocalPath, chGroup);
    }
#else
    sprintf(acBuff, "%s\\INDEX_%s.dat", chCurPath, chGroup);
#endif
    
#ifdef MOBILE_CLIENT//?÷a˙??a??à￡¨￥”??￥ê???°
    
    char chread[10240]={0};
    int nread=0;
    char stName[200] = {0};
    sprintf(stName, "%s%s.dat","INDEX_",chGroup);
    nread = ReadMobileFile(stName,chread,10240);
    if(nread > 0)
    {
        int npostmp = 0;
        memset(acBuff, 0, MAX_PATH);
        while(MOGetLine(chread,nread,npostmp,acBuff))//Index
        {
            //∑??ò??μ?÷∑Ω?√?
            for (unsigned int m=0; m<strlen(acBuff); m++)
            {
                acBuff[m] ^= m;
            }
            STSIP stIP;
            memset(stIP.chIP, 0, 16);
            int i=0;
            char chPort[16] = {0};
            for(i=0; i<MAX_PATH; i++)
            {
                if(acBuff[i] == ':' || acBuff[i] == '\0')
                {
                    break;
                }
            }
            memcpy(stIP.chIP, acBuff, i);
            memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i-1);
            stIP.nPort = atoi(chPort);
            if(stIP.nPort > 0)
            {
                IPList.push_back(stIP);
            }
        }
    }
    
    //////////////////////////////////////////////////////////////////////////
#else//?‰à??è??￡¨￥”??o????°
    //’“μΩo??o,≤…?°????∫???‘?∑Ω?Ω
    ::std::string line;
    ::std::ifstream localfile(acBuff);
    while(::std::getline(localfile,line))
    {
        memset(acBuff, 0, MAX_PATH);
        line.copy(acBuff,MAX_PATH,0);
        //∑??ò??μ?÷∑Ω?√?
        for (unsigned int m=0; m<strlen(acBuff); m++)
        {
            acBuff[m] ^= m;
        }
        STSIP stIP;
        memset(stIP.chIP, 0, 16);
        int i=0;
        char chPort[16] = {0};
        for(i=0; i<MAX_PATH; i++)
        {
            if(acBuff[i] == ':' || acBuff[i] == '\0')
            {
                break;
            }
        }
        memcpy(stIP.chIP, acBuff, i);
        memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i+1);
        stIP.nPort = atoi(chPort);
        if(stIP.nPort > 0)
        {
            IPList.push_back(stIP);
        }
    }
#endif
    
    return IPList.size() > 0;
}
#ifndef WIN32
char* CCWorker::itoa(int num,char *str,int radix)
{     /* à?“?±ì*/
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned unum; /* ÷–o‰±‰?? */
    int i=0,j,k;
    /* ?∑??unumμ?÷μ */
    if(radix==10&&num<0) /* ??Ωˉ÷?∏∫?? */
    {
        unum=(unsigned)-num;
        str[i++]='-';
    }
    else unum=(unsigned)num; /* ?‰à??è?? */
    /* ??aa */
    do{
        str[i++]=index[unum%(unsigned)radix];
        unum/=radix;
    }while(unum);
    str[i]='\0';
    /* ?ê–ú */
    if(str[0]=='-') k=1; /* ??Ωˉ÷?∏∫?? */
    else k=0;
    char temp;
    for(j=k;j<=(i-1)/2;j++)
    {
        temp=str[j];
        str[j] = str[i-1+k-j];
        str[i-1+k-j] = temp;
    }
    return str;
}
#endif

BOOL CCWorker::IndexServerList_Save(char chGroup[4], std::vector<STSIP> &IPList)
{
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    char acBuff[MAX_PATH] = {0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//√a”–√??∑÷∏???∑??￡¨‘ú‘?dvr??≤…”√π????∑???ˉ≤a???μo?μ±?∞?∑??
        sprintf(acBuff, "%s/INDEX_%s.dat", "./data",chGroup);
    }
    else
    {//?π”√÷∏??μ??∑??
        sprintf(acBuff, "%s/INDEX_%s.dat", m_chLocalPath, chGroup);
    }
#else
    sprintf(acBuff, "%s\\INDEX_%s.dat", chCurPath, chGroup);
#endif
    char writeBuffer[1024] = {0};
    if(IPList.size() > 0)
    {
        char chIP[64] = {0};
        char chPort[16] = {0};
        
        for(size_t i = 0; i < IPList.size(); ++i)
        {
            memset(chIP, 0, sizeof(chIP));
            strcat(chIP, IPList[i].chIP);
            strcat(chIP, ":");
            itoa(IPList[i].nPort, chPort, 10);
            strcat(chIP, chPort);
            
            if(strlen(chIP) > 0)
            {
                //o”√?
                for(unsigned int m=0; m<strlen(chIP); m++)
                {
                    chIP[m] ^= m;
                }
                strcat(chIP, "\r\n");
                
                strcat(writeBuffer,chIP);
            }
        }
#ifdef MOBILE_CLIENT
        int writeLen =strlen(writeBuffer);
        //		m_pfWriteReadData(1,(unsigned char *)chGroup,"INDEX_",(unsigned char*)writeBuffer, &writeLen);//index write
        
        char stName[200] = {0};
        sprintf(stName, "%s%s.dat","INDEX_",chGroup);
        WriteMobileFile(stName,(char* )writeBuffer,writeLen);
//        WriteMobileFile(acBuff,(char* )writeBuffer,writeLen);
#else
        FILE *pfile = NULL;
        
        pfile = fopen(acBuff, "wb");
        
        if(pfile != NULL)
        {
            fwrite(writeBuffer,sizeof(char),strlen(writeBuffer),pfile);
            fclose(pfile);
        }
        
#endif
    }
    
    return TRUE;
}

void CCWorker::AddUdtRecv(SOCKET sSocket,int nLocalPort,sockaddr* addrRemote,char *pMsg,int nLen,BOOL bNewUDP)
{
    UDP_PACKAGE pkg = {0};
    
    DWORD dwTime = JVGetTime();
    
    //?????ì“‘?∞μ?
    BOOL bFind = FALSE;
    CLocker lock(m_ReceiveLock,__FILE__,__LINE__);
    for(::std::vector<UDP_PACKAGE >::iterator i = m_UdpDataList.begin(); i != m_UdpDataList.end();)
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
        
#ifdef WIN32
        if(pkg.sSocket == sSocket && pkg.nLocal == nLocalPort && pkg.nLen == nLen
           && AddrIsTheSame(addrRemote,(sockaddr *)&pkg.addrRemote) && stricmp(pkg.cData,pMsg) == 0)
#else
            if(pkg.sSocket == sSocket && pkg.nLocal == nLocalPort && pkg.nLen == nLen
               && AddrIsTheSame(addrRemote,(sockaddr *)&pkg.addrRemote) && strcasecmp(pkg.cData,pMsg) == 0)
#endif
            {
                bFind = TRUE;
                break;
            }
        if(dwTime - pkg.time > 30 * 1000)//?’μΩμ?????∞?“—?≠∫??√?à
        {
            m_UdpDataList.erase(i);
        }
        else
        {
            i ++;
        }
    }
    
    if(!bFind)
    {
        int nType = 0;
        memcpy(&nType,pMsg,4);
        //		OutputDebug("AddUdtRecv nLocalPort = %d,type %X",nLocalPort,nType);
        
        pkg.sSocket = sSocket;
        pkg.nLocal = nLocalPort;
        memcpy(&pkg.addrRemote,addrRemote,sizeof(sockaddr));
        memcpy(&pkg.cData,pMsg,nLen);
        pkg.nLen = nLen;
        pkg.time = dwTime;
        
        if(nLen == 20)
        {
            //			BOOL nType = 0;
            //			memcpy(&nType,pMsg,1);
            
            //			OutputDebug("recv UDP = 0X%X   %d",nType,nLen);
        }
        else
        {
            int nType = 0;
            memcpy(&nType,pMsg,4);
            
            //			OutputDebug("nLocalPort = %d recv UDP = 0X%X  Len = %d",nLocalPort,nType,nLen);
            if(nLen == 8)
            {
                if(nType == JVN_CMD_NEW_TRYTOUCH)
                {
                    //SOCKADDR_IN r = {0};
                    //memcpy(&r,addrRemote,sizeof(SOCKADDR_IN));
                    //OutputDebug("nLocalPort = %d recv UDP = 0X%X   %d  %s : %d",nLocalPort,nType,nLen,inet_ntoa(r.sin_addr),ntohs(r.sin_port));
                    
                    int nType = JVN_CMD_B_HANDSHAKE;//JVN_CMD_NEW_TRYTOUCH;
                    char data[10] = {0};
                    memcpy(&data[0],&nType,4);
                    memcpy(&data[4],&pMsg[4],4);
                    CCChannel::sendtoclientm(sSocket, (char*)data, 8, 0, (sockaddr *)addrRemote, sizeof(SOCKADDR_IN), 1);
                    //	CCChannel::sendtoclientm(sSocket, (char*)data, 8, 0, (sockaddr *)addrRemote, sizeof(SOCKADDR_IN), 1);
                    //	CCChannel::sendtoclientm(sSocket, (char*)data, 8, 0, (sockaddr *)addrRemote, sizeof(SOCKADDR_IN), 1);
                    //	CCChannel::sendtoclientm(sSocket, (char*)data, 8, 0, (sockaddr *)addrRemote, sizeof(SOCKADDR_IN), 1);
                    //	CCChannel::sendtoclientm(sSocket, (char*)data, 8, 0, (sockaddr *)addrRemote, sizeof(SOCKADDR_IN), 1);
                }
                else if(nType == JVN_CMD_A_HANDSHAKE)
                {
                    //SOCKADDR_IN r = {0};
                    //memcpy(&r,addrRemote,sizeof(SOCKADDR_IN));
                    //OutputDebug("nLocalPort = %d recv UDP = 0X%X   %d  %s : %d",nLocalPort,nType,nLen,inet_ntoa(r.sin_addr),ntohs(r.sin_port));
                }
                else if(nType == JVN_CMD_AB_HANDSHAKE)
                {
                    //SOCKADDR_IN r = {0};
                    //memcpy(&r,addrRemote,sizeof(SOCKADDR_IN));
                    //OutputDebug("nLocalPort = %d recv UDP = 0X%X   %d  %s : %d",nLocalPort,nType,nLen,inet_ntoa(r.sin_addr),ntohs(r.sin_port));
                    
                    //??÷??…“‘???¨Ω”
                    int nTy = JVN_CMD_TRYTOUCH;
                    memcpy(pkg.cData,&nTy,4);
                }
            }
        }
        m_UdpDataList.push_back(pkg);
    }
}

/*
 int CCWorker::SendUdpDataForMobile(SOCKET s,SOCKADDR_IN ServerSList,int nYSTNO,BYTE* pIP,int nSize)
 {
	//?úS∑￠à????? ??–?(4)+∫≈??(4)
	BYTE data[1024]={0};
	int nType = JVN_REQ_EXCONNA;//JVN_REQ_CONNA;
	
	memcpy(&data[4], &nYSTNO, 4);
	if(nSize > 0)
	{
 nType = JVN_REQ_EXCONNA2;
 memcpy(&data[8], pIP, nSize);
 nSize += 8;
	}
	else
	{
 nSize = 8;
	}
	memcpy(&data[0], &nType, 4);
	
	SOCKADDR_IN desAN = {0};
	memcpy(&desAN,&ServerSList,sizeof(SOCKADDR_IN));
	
	CCChannel::sendtoclientm(s,(char* )data,20,0,(sockaddr *)&desAN,sizeof(SOCKADDR),1);
	
	sockaddr_in sin = {0};
	memcpy(&sin,&desAN,sizeof(sockaddr));
	
	sin.sin_port = htons(8000);//–??ìo”8000?à??∑￠à?
	CCChannel::sendtoclientm(s,(char* )data,20,0,(sockaddr *)&sin,sizeof(SOCKADDR),1);
 
	return 0;
 }
 
 */

int CCWorker::SendUdpDataForMobile(SOCKET s,SOCKADDR_IN ServerSList,int nYSTNO,BYTE* pIP,int nSize)
{
    BYTE data[1024]={0};
    int nType = JVN_REQ_EXCONNA;//JVN_REQ_CONNA;
    
    memcpy(&data[4], &nYSTNO, 4);
    if(nSize > 0)
    {
        nType = JVN_REQ_EXCONNA2;
        memcpy(&data[8], pIP, nSize);
        nSize += 8;
    }
    else
    {
        nSize = 8;
    }
    memcpy(&data[0], &nType, 4);
    
    SOCKADDR_IN desAN = {0};
    memcpy(&desAN,&ServerSList,sizeof(SOCKADDR_IN));
    
    CCChannel::sendtoclientm(s,(char* )data,nSize,0,(sockaddr *)&desAN,sizeof(SOCKADDR),1);
    
    sockaddr_in sin = {0};
    memcpy(&sin,&desAN,sizeof(sockaddr));
    
    sin.sin_port = htons(8000);
    CCChannel::sendtoclientm(s,(char* )data,nSize,0,(sockaddr *)&sin,sizeof(SOCKADDR),1); 
    
    return 0; 
}


int CCWorker::SendUdpData(SOCKET s,ServerList ServerSList,int nYSTNO,BYTE* pIP,int nSize)
{
    //?úS∑￠à????? ??–?(4)+∫≈??(4)
    BYTE data[1024]={0};
    int nType = JVN_REQ_EXCONNA;//JVN_REQ_CONNA;
    
    memcpy(&data[4], &nYSTNO, 4);
    if(nSize > 0)
    {
        nType = JVN_REQ_EXCONNA2;
        memcpy(&data[8], pIP, nSize);
        nSize += 8;
    }
    else
    {
        nSize = 8;
    }
    memcpy(&data[0], &nType, 4);
    
    for(int i = 0;i < ServerSList.size();i ++)
    {
        SOCKADDR_IN desAN = {0};
        memcpy(&desAN,&ServerSList[i].addr,sizeof(SOCKADDR_IN));
        
        CCChannel::sendtoclientm(s,(char* )data,20,0,(sockaddr *)&desAN,sizeof(SOCKADDR),1);
        
        sockaddr_in sin = {0};
        memcpy(&sin,&desAN,sizeof(sockaddr));
        
        sin.sin_port = htons(8000);//–??ìo”8000?à??∑￠à?
        CCChannel::sendtoclientm(s,(char* )data,20,0,(sockaddr *)&sin,sizeof(SOCKADDR),1);
    }
    return 0;
}

int CCWorker::GetUdpData(SOCKET s,int nYSTNO,UDP_LIST* UdpList)
{
    int nCount = 0;
    UDP_PACKAGE pkg = {0};
    
    CLocker lock(m_ReceiveLock,__FILE__,__LINE__);
    
    for(::std::vector<UDP_PACKAGE >::iterator i = m_UdpDataList.begin(); i != m_UdpDataList.end();i ++)
    {
        int nSize = 0;//m_UdpDataList.size();
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
        if(pkg.sSocket == s)
        {
            int nType = 0;
            BYTE bType = 0;
            memcpy(&bType,pkg.cData,1);
            memcpy(&nType,pkg.cData,4);
            
            int nYST = 0;
            if(pkg.nLen == 8)//”–?…????￥ú?￥∞?
            {
                memcpy(&nYST,&pkg.cData[4],4);
                if(nType == JVN_CMD_TRYTOUCH && nYST == nYSTNO)
                {
                    if(nSize > 0)
                        UdpList->insert(m_UdpDataList.begin(),pkg);
                    else
                        UdpList->push_back(pkg);
                    nCount ++;
                    return nCount;
                }
            }
            else if(pkg.nLen == 13)//”–?…????￥ú?￥∞?
            {
                memcpy(&nYST,&pkg.cData[4],4);
                if(nType == JVN_CMD_TRYTOUCH && nYST == nYSTNO)
                {
                    if(nSize > 0)
                        UdpList->insert(m_UdpDataList.begin(),pkg);
                    else
                        UdpList->push_back(pkg);
                    nCount ++;
                    return nCount;
                }
            }
            else if(pkg.nLen == 20)//”–?…????￥ú?￥∞?
            {
                memcpy(&nYST,&pkg.cData[1],4);
                if(bType == JVN_CMD_TRYTOUCH && nYST == nYSTNO)
                {
                    if(nSize > 0)
                        UdpList->insert(m_UdpDataList.begin(),pkg);
                    else
                        UdpList->push_back(pkg);
                    nCount ++;
                    return nCount;
                }
                
            }
            else if(pkg.nLen == 22)//∑??ò??∑μa?
            {
                memcpy(&nYST,&pkg.cData[4],4);
                if(nType == JVN_CMD_YSTCHECK2 && nYST == 14)
                {
                    UdpList->push_back(pkg);
                    nCount ++;
                }
            }
            else if(pkg.nLen == 40 || pkg.nLen == 41)//∑??ò??∑μa?
            {
                memcpy(&nYST,&pkg.cData[4],4);
                if((nType == YST_A_NEW_ADDRESS || nType == JVN_REQ_NAT || nType == JVN_RSP_CONNA) && nYST == nYSTNO)
                {
                    UdpList->push_back(pkg);
                    nCount ++;
                }
            }
            
            if(nType == YST_A_NEW_ADDRESS)
            {
                SOCKADDR_IN m_addrAN,m_addrAL;
                memcpy(&m_addrAN,&pkg.cData[8],sizeof(SOCKADDR_IN));
                memcpy(&m_addrAL,&pkg.cData[8 + sizeof(SOCKADDR_IN)],sizeof(SOCKADDR_IN));
                m_addrAN.sin_family = AF_INET;
                m_addrAL.sin_family = AF_INET;
                
                //				char strServer[100] = {0};
                //				sprintf(strServer,"================recv new Nat %s:%d\n",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                //				OutputDebug(strServer);
                nCount ++;
            }
        }
    }
    //	OutputDebug("num = %d",nCount);
    return nCount;
}

int CCWorker::GetConnectInfo(char* pGroup,int nYST,SOCKET& s,char* pIP,int &nPort)
{
    if(m_Helper.GetLocalInfo(pGroup,nYST,pIP,nPort))//tcp?¨Ω”
        return 2;//?′≤???udp∫?≈–????∑òtcp
    
    if(m_Helper.GetOuterInfo(pGroup,nYST,s,pIP,nPort))//udp
        return 2;
    
    return 0;
}

void CCWorker::AddHelpConnect(char* pGroup,int nYST,SOCKET s, char *pIP, int nPort)
{
    if(m_pHelpCtrl)
        m_pHelpCtrl->AddRemoteConnect(pGroup,nYST,s,pIP,nPort);
}

void CCWorker::WriteMobileFile(char* strName,char* pData,int nLen)
{
    char strFileName[MAX_PATH] = {0};
    sprintf(strFileName,"%s/%s",m_chLocalPath,strName);
//    printf("WriteMobileFile url: %s\n",strFileName);
    FILE* file = fopen(strFileName, "w+");
    if(file)
    {
        //		fprintf(file,pData, pData);
        //		fflush(file);
        
        
        fwrite(pData,sizeof(char),nLen,file);
        //		printf("---------------%s---- nLen: %d---- write  data: %s,  \n",strFileName, nLen, pData);
        fclose(file);
    }
}


int CCWorker::ReadMobileFile(char* strName,char* pData,int nLen)
{
    char strFileName[MAX_PATH] = {0};
    sprintf(strFileName,"%s/%s",m_chLocalPath,strName);
//    printf("readMobileFile: %s\n",strFileName);
    FILE* file = fopen(strFileName, "r");
    
    if(file)
    {
        //		fprintf(file,pData, pData);
        //		fflush(file);
        int nRead = fread(pData,sizeof(char),nLen,file);
        //		printf("---------------%s---- nLen: %d---- read  data: %s,  \n",strFileName, nRead, pData);
        fclose(file);
        
        return nRead;
    }
    return -1;
}

BOOL CCWorker::GetDemoList(int nWebIndex)
{
    sockaddr_in sin;
    
    int nSockAddrLen = sizeof(SOCKADDR);
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    
    char strFileName[MAX_PATH] = {0};
    
    FILE *pfile=NULL;
    
    BOOL bFindSite = FALSE;
    char chSiteIP[30] = {0};
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//
        sprintf(strFileName, "%s/demo.dat", "./data");
    }
    else
    {//
        sprintf(strFileName, "%s/demo.dat", m_chLocalPath);
    }
#else
    sprintf(strFileName, "%s\\demo.dat", chCurPath);
#endif
    {
        
        char recvBuf[1024] = {0};
        char IPBuf[1024] = {0};
        char target[1024] = {0};
        char target_e[1024] = {0};
        int our_socket;
        struct sockaddr_in our_address;
        
        strcpy(target, "GET http://");
        strcpy(target_e, "GET ");
        if(strlen(m_strDomainName) == 0)//
        {
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
            }
            strcat(target, JVN_WEBFOLDER);
            strcat(target, "Demo.txt");
            strcat(target," HTTP/1.1\r\n");
            strcat(target,"Host:");
            //////////////////////////////////////////////////////////////////////////
            strcat(target_e, JVN_WEBFOLDER);
            strcat(target_e, "Demo.txt");
            strcat(target_e," HTTP/1.1\r\n");
            strcat(target_e,"Host:");
            //////////////////////////////////////////////////////////////////////////
            
            if(1 == nWebIndex)
            {
                strcat(target, JVN_WEBSITE1);
                strcat(target_e, JVN_WEBSITE1);
            }
            else
            {
                strcat(target, JVN_WEBSITE2);
                strcat(target_e, JVN_WEBSITE2);
            }
        }
        else
        {
            return FALSE;
        }
        
        strcat(target,"\r\n");
        strcat(target,"Accept:*/*\r\n");
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e,"\r\n");
        strcat(target_e,"Accept:*/*\r\n");
        strcat(target_e, JVN_AGENTINFO);
        //////////////////////////////////////////////////////////////////////////
        strcat(target,"Connection:Keep-Alive\r\n\r\n");
        strcat(target_e,"Connection:Keep-Alive\r\n\r\n");
        
        struct hostent * site=NULL;
        if(strlen(m_strDomainName) == 0)//
        {
            if(1 == nWebIndex)
            {
                char severname[] = JVN_WEBSITE1;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
                
            }
            else
            {
                char severname[] = JVN_WEBSITE2;
                site = gethostbyname (severname);
                if(site == NULL)
                {
                    CSDNSCtrl csdns;
                    bFindSite = csdns.GetIPByDomain(severname, chSiteIP);
                }
            }
        }
        else
        {
            return FALSE;//
        }
        
        if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
            }
            
            //return FALSE;
            goto READFILE;
        }
        
        LINGER linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
        
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                //return FALSE;
                goto READFILE;
                
            }
        }
        
        if(site == NULL && !bFindSite)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:aò?°”ú√?μ?÷∑??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:get hostsite failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            //return FALSE;
            goto READFILE;
        }
        
        memset(&our_address, 0, sizeof(struct sockaddr_in));
        our_address.sin_family = AF_INET;
        if(site != NULL)
        {
            memcpy (&our_address.sin_addr, site->h_addr_list[0], site->h_length);
        }
        else if(bFindSite)
        {
            our_address.sin_addr.s_addr = inet_addr(chSiteIP);
        }
        our_address.sin_port = htons(80);
        
        if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            //return FALSE;
            goto READFILE;
        }
        
        if(CCChannel::tcpsend(our_socket,target,strlen(target),3)<=0)
        {
            closesocket(our_socket);
            goto READFILE;
        }
        
        int nLen = 0;
        int tryTimes=0;
        while(tryTimes<2)
        {
            int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
            if(rlen < 0 || (nLen+rlen)>1024)
            {
                break;
            }
            memcpy(&IPBuf[nLen], recvBuf, rlen);
            nLen += rlen;
            tryTimes++;
        }
        
        closesocket(our_socket);
        our_socket = 0;
        
        if(nLen == 0)
        {
            if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
                }
                
                //return FALSE;
                goto READFILE;
            }
            
            LINGER linger2;
            linger2.l_onoff = 1;
            linger2.l_linger = 0;
            setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger2, sizeof(LINGER));
            
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
#ifndef WIN32
                sin.sin_addr.s_addr = INADDR_ANY;
#else
                sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
                
                sin.sin_family = AF_INET;
                sin.sin_port = htons(0);
                if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
                {
                    if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                    {
                        m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                    }
                    
                    closesocket(our_socket);
                    //return FALSE;
                    goto READFILE;
                    
                }
            }
            
            if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                //return FALSE;
                goto READFILE;
            }
            
            if(CCChannel::tcpsend(our_socket,target_e,strlen(target_e),3)<=0)
            {
                closesocket(our_socket);
                goto READFILE;
            }
            
            tryTimes=0;
            while(tryTimes<2)
            {
                int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,1);
                if(rlen < 0 || (nLen+rlen)>1024)
                {
                    break;
                }
                memcpy(&IPBuf[nLen], recvBuf, rlen);
                nLen += rlen;
                tryTimes++;
            }
        }
        
        if(our_socket > 0)
        {
            closesocket(our_socket);
        }
        
        if(strlen(m_strDomainName) == 0)
        {
            char strHead[] = {"DemoStart"};
            
            int start = 0;
            for(int i = 0;i < nLen;i ++)
            {
                if(IPBuf[i + 0] == strHead[0] &&
                   IPBuf[i + 1] == strHead[1] &&
                   IPBuf[i + 2] == strHead[2] &&
                   IPBuf[i + 3] == strHead[3] &&
                   IPBuf[i + 4] == strHead[4] &&
                   IPBuf[i + 5] == strHead[5] &&
                   IPBuf[i + 6] == strHead[6] &&
                   IPBuf[i + 7] == strHead[7] &&
                   IPBuf[i + 8] == strHead[8])
                {
                    start = (i + strlen(strHead));
                    break;
                }
            }
            if(start == 0)
            {
                goto READFILE;
            }
            int nNum = 0;
            memcpy(&nNum,&IPBuf[start + 0],4);
            int ver = 0;
            memcpy(&ver,&IPBuf[start + 4],4);
            
            start += 8;
            
            pfile = fopen(strFileName,"w");
            if(pfile)
            {
                fwrite(&nNum,sizeof(char),4,pfile);
                fwrite(&ver,sizeof(char),4,pfile);
                
                int j = 0;
                int i = 0;
                for(i = 0;i < nNum;i ++)
                {
                    int nL = 0;
                    char strYST[30] = {0};
                    int nChannelNum = 0;
                    int nLU = 0,nP = 0;
                    char strU[32] = {0},strP[32] = {0};
                    int nIP = 0;
                    char strIP[30] = {0};
                    int nPort = 0;
                    
                    memcpy(&nL,&IPBuf[start],4);
                    start += 4;
                    memcpy(strYST,&IPBuf[start],nL);
                    fwrite(&nL,sizeof(char),4,pfile);
                    fwrite(strYST,sizeof(char),nL,pfile);
                    
                    for(j = 0;j < nL;j ++)
                    {
                        strYST[j] ^= j + 10;
                    }
                    start += nL;
                    memcpy(&nChannelNum,&IPBuf[start],4);
                    fwrite(&nChannelNum,sizeof(char),4,pfile);
                    
                    start += 4;
                    memcpy(&nLU,&IPBuf[start],4);
                    start += 4;
                    memcpy(strU,&IPBuf[start],nLU);
                    fwrite(&nLU,sizeof(char),4,pfile);
                    fwrite(strU,sizeof(char),nLU,pfile);
                    
                    for(j = 0;j < nLU;j ++)
                    {
                        strU[j] ^= j + 10;
                    }
                    
                    
                    start += nLU;
                    memcpy(&nP,&IPBuf[start],4);
                    start += 4;
                    memcpy(strP,&IPBuf[start],nP);
                    fwrite(&nP,sizeof(char),4,pfile);
                    fwrite(strP,sizeof(char),nP,pfile);
                    
                    for(j = 0;j < nP;j ++)
                    {
                        strP[j] ^= j + 10;
                    }
                    
                    
                    start += nP;
                    memcpy(&nIP,&IPBuf[start],4);
                    start += 4;
                    memcpy(strIP,&IPBuf[start],nIP);
                    fwrite(&nIP,sizeof(char),4,pfile);
                    fwrite(strIP,sizeof(char),nIP,pfile);
                    
                    for(j = 0;j < nIP;j ++)
                    {
                        strIP[j] ^= j + 10;
                    }
                    
                    start += nIP;
                    memcpy(&nPort,&IPBuf[start],4);
                    fwrite(&nPort,sizeof(char),4,pfile);
                    
                    start += 4;
                    
                    OutputDebug("download YST %s channel = %d user = %s pwd = %s  %s : %d",strYST,nChannelNum,strU,strP,strIP,nPort);
                    
                    memcpy(&nL,&IPBuf[start],4);
                    if(nL <= 0)
                        break;
                }
                fclose(pfile);
            }
        }
        else//
        {
            return FALSE;
        }
    }
READFILE:
    LoadDemoFile();
    
    return TRUE;
}

void CCWorker::HelpRemove(char* pGroup,int nYST)
{
	if(m_pHelpCtrl)
	{
		if(m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
		{
//			CCHelpCtrlH* pHelp = (CCHelpCtrlH* )m_pHelpCtrl;
//
//			pHelp->HelpRemove(pGroup,nYST);
		}
		else if(m_pHelpCtrl->m_nHelpType == JVN_WHO_M)
		{
			CCHelpCtrlM* pHelp = (CCHelpCtrlM* )m_pHelpCtrl;

			pHelp->HelpRemove(pGroup,nYST);
		}
	}
}

int CCWorker::GetHelper(char* pGroup,int nYST,int *nCount)
{
	if(m_pHelpCtrl)
	{
		if(m_pHelpCtrl->m_nHelpType == JVN_WHO_H)
		{
//			CCHelpCtrlH* pHelp = (CCHelpCtrlH* )m_pHelpCtrl;
//
//			return pHelp->GetHelper(pGroup,nYST,nCount);
		}
		else if(m_pHelpCtrl->m_nHelpType == JVN_WHO_M)
		{
			CCHelpCtrlM* pHelp = (CCHelpCtrlM* )m_pHelpCtrl;

			return pHelp->GetHelper(pGroup,nYST,nCount);
		}
	}

	return -1;
}

int CCWorker::GetDemo(BYTE* pBuff,int nBuffSize)
{
    int nSize = m_DemoList.size();
    if(nSize * 9 > nBuffSize)
        return -1;
    
    if(nSize <= 0)
        return 0;
    
    DEMO_DATA info = {0};
    
    int j = 0;
    for(int i = 0;i < m_DemoList.size();i ++)
    {
        memcpy(&info,&m_DemoList[i],sizeof(DEMO_DATA));
        
        memcpy(&pBuff[j],info.chGroup,4);
        j += 4;
        memcpy(&pBuff[j],&info.nYST,4);
        j += 4;
        pBuff[j] = info.nChannelNum;
        j += 1;
    }
    
    return nSize;
}

BOOL CCWorker::YstIsDemo(char* pGroup,int nYST)
{
    char strYST[100] = {0},strYST2[100] = {0};
    sprintf(strYST,"%s%d",pGroup,nYST);
    
    DEMO_DATA info = {0};
    
    int j = 0;
    for(int i = 0;i < m_DemoList.size();i ++)
    {
        memcpy(&info,&m_DemoList[i],sizeof(DEMO_DATA));
        sprintf(strYST2,"%s%d",info.chGroup,info.nYST);
#ifndef WIN32
        if(strcasecmp(strYST,strYST2) == 0)
#else
            if(stricmp(strYST,strYST2) == 0)
#endif
            {
                writeLog("YstIsDemo  %s %d = %s: %d, line: %d",
                         pGroup,nYST,info.strIP,info.nPort,__LINE__);
                return TRUE;
            }
    }
    return FALSE;
}

BOOL CCWorker::GetDemoAddr(char* pGroup,int nYST,char* pIP,int &nPort,char *pUser,char* pPwd)
{
    char strYST[100] = {0},strYST2[100] = {0};
    sprintf(strYST,"%s%d",pGroup,nYST);
    
    DEMO_DATA info = {0};
    
    int j = 0;
    for(int i = 0;i < m_DemoList.size();i ++)
    {
        memcpy(&info,&m_DemoList[i],sizeof(DEMO_DATA));
        sprintf(strYST2,"%s%d",info.chGroup,info.nYST);
#ifndef WIN32
        if(strcasecmp(strYST,strYST2) == 0)
#else
            if(stricmp(strYST,strYST2) == 0)
#endif
            {
                strcpy(pIP,info.strIP);
                nPort = info.nPort;
                strcpy(pUser,info.strName);
                strcpy(pPwd,info.strPwd);
                
                return TRUE;
            }
    }
    return FALSE;
}

void CCWorker::LoadDemoFile()
{
    m_DemoList.clear();
    
    char strFileName[MAX_PATH] = {0};
    
    FILE *pfile=NULL;
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {
        sprintf(strFileName, "%s/demo.dat", "./data");
    }
    else
    {
        sprintf(strFileName, "%s/demo.dat", m_chLocalPath);
    }
#else
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    sprintf(strFileName, "%s\\demo.dat", chCurPath);
#endif
    pfile = fopen(strFileName,"r");
    if(pfile)
    {
        int nNum = 0,ver = 0;
        fread(&nNum,sizeof(char),4,pfile);
        fread(&ver,sizeof(char),4,pfile);
        
        int j = 0,i = 0;
        for(i = 0;i < nNum;i ++)
        {
            int nL = 0;
            char strYST[30] = {0};
            int nChannelNum = 0;
            int nLU = 0,nP = 0;
            char strU[32] = {0},strP[32] = {0};
            int nIP = 0;
            char strIP[30] = {0};
            int nPort = 0;
            
            fread(&nL,sizeof(char),4,pfile);
            fread(strYST,sizeof(char),nL,pfile);
            
            fread(&nChannelNum,sizeof(char),4,pfile);
            
            fread(&nLU,sizeof(char),4,pfile);
            fread(strU,sizeof(char),nLU,pfile);
            
            fread(&nP,sizeof(char),4,pfile);
            fread(strP,sizeof(char),nP,pfile);
            
            fread(&nIP,sizeof(char),4,pfile);
            fread(strIP,sizeof(char),nIP,pfile);
            
            fread(&nPort,sizeof(char),4,pfile);
            
            for(j = 0;j < nL;j ++)
            {
                strYST[j] ^= j + 10;
            }
            for(j = 0;j < nLU;j ++)
            {
                strU[j] ^= j + 10;
            }
            for(j = 0;j < nP;j ++)
            {
                strP[j] ^= j + 10;
            }
            for(j = 0;j < nIP;j ++)
            {
                strIP[j] ^= j + 10;
            }
            
            OutputDebug("Load YST %s channel = %d user = %s pwd = %s  %s : %d",strYST,nChannelNum,strU,strP,strIP,nPort);
            
            DEMO_DATA info = {0};
            info.chGroup[0] = strYST[0];
            char strY[10] = {0};
            strcpy(strY,&strYST[1]);
            info.nYST = atoi(strY);
            info.nChannelNum = nChannelNum;
            strcpy(info.strName,strU);
            strcpy(info.strPwd,strP);
            
            strcpy(info.strIP,strIP);
            info.nPort = nPort;
            m_DemoList.push_back(info);
        }
        fclose(pfile);
    }
    
}

//http://www.jovetech.com/down/yst/YST_server.txt
BOOL CCWorker::DownLoadFile(char *pServer,char* pPath, char *pFileName,char *pHeadFilter)
{
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    
    char strFileName[MAX_PATH] = {0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//
        sprintf(strFileName, "%s/%s", "./data",pFileName);
    }
    else
    {//
        sprintf(strFileName, "%s/%s", m_chLocalPath,pFileName);
    }
#else
    sprintf(strFileName, "%s\\%s", chCurPath,pFileName);
#endif
    
    char recvBuf[1024] = {0};
    char IPBuf[1024] = {0};
    char target[1024] = {0};
    char target_e[1024] = {0};
    int our_socket;
    struct sockaddr_in our_address;
    
    struct sockaddr_in sin;
    int len = sizeof(sin);
    
    BOOL bFindSite=FALSE;
    char chSiteIP[30]={0};
    
    strcpy(target, "GET http://");
    strcpy(target_e, "GET ");
    if(strlen(m_strDomainName) == 0)//?π”√π′à?μ?”ú√?
    {
        strcat(target, pServer);
        
        strcat(target, pPath);
        
        strcat(target," HTTP/1.1\r\n");
        strcat(target,"Host:");
        //////////////////////////////////////////////////////////////////////////
        strcat(target_e, pServer);
        strcat(target_e, pPath);
        
        strcat(target_e," HTTP/1.1\r\n");
        strcat(target_e,"Host:");
        //////////////////////////////////////////////////////////////////////////
        
        strcat(target, pServer);
        strcat(target_e, pServer);
    }
    else
    {
        return FALSE;
    }
    
    strcat(target,"\r\n");
    strcat(target,"Accept:*/*\r\n");
    //////////////////////////////////////////////////////////////////////////
    strcat(target_e,"\r\n");
    strcat(target_e,"Accept:*/*\r\n");
    strcat(target_e, JVN_AGENTINFO);
    //////////////////////////////////////////////////////////////////////////
    strcat(target,"Connection:Keep-Alive\r\n\r\n");
    strcat(target_e,"Connection:Keep-Alive\r\n\r\n");
    
    struct hostent * site=NULL;
    
    site = gethostbyname (pServer);
    if(site == NULL)
    {
        CSDNSCtrl csdns;
        bFindSite = csdns.GetIPByDomain(pServer, chSiteIP);
    }
    
    if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
        }
        
        return FALSE;
    }
    
    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
#ifndef WIN32
    sin.sin_addr.s_addr = INADDR_ANY;
#else
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
    }
    
    if(site == NULL && !bFindSite)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:aò?°”ú√?μ?÷∑??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:get hostsite failed.", __FILE__,__LINE__);
        }
        
        closesocket(our_socket);
        return FALSE;
    }
    
    memset(&our_address, 0, sizeof(struct sockaddr_in));
    our_address.sin_family = AF_INET;
    
    if(site != NULL)
    {
        memcpy (&our_address.sin_addr, site->h_addr_list[0], site->h_length);
    }
    else if(bFindSite)
    {
        our_address.sin_addr.s_addr = inet_addr(chSiteIP);
    }
    
    our_address.sin_port = htons(80);
    
    if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
        }
        
        closesocket(our_socket);
        return FALSE;
    }
    
    
    if(CCChannel::tcpsend(our_socket,target,strlen(target),3)<=0)
    {
        if(JVN_LANGUAGE_CHINESE == m_nLanguage)
        {
            m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:?ú∑??ò??∑￠à???????∞?", __FILE__,__LINE__);
        }
        else
        {
            m_Log.SetRunInfo(0,"get server address failed.Info:send data to server failed.", __FILE__,__LINE__);
        }
        closesocket(our_socket);
        return FALSE;
    }
    int nLen = 0;
    int tryTimes=0;
    while(tryTimes < 3)
    {
        int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,2);
        if(rlen <0 || (nLen+rlen)> 10 * 1024)
        {
            break;
        }
        memcpy(&IPBuf[nLen], recvBuf, rlen);
        nLen += rlen;
        tryTimes++;
    }
    closesocket(our_socket);
    our_socket = 0;
    
    if(nLen == 0)
    {//√a?’μΩ??∫?????￡¨∞￥–?????∞?÷?–?????“a￥?
        if ((our_socket = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?,￥￥Ω?web??Ω”?÷??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed,Info:create web sock failed.", __FILE__,__LINE__);
            }
            
            return FALSE;
        }
        
        LINGER linger2;
        linger2.l_onoff = 1;
        linger2.l_linger = 0;
        setsockopt(our_socket, SOL_SOCKET, SO_LINGER, (const char*)&linger2, sizeof(LINGER));
        
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
#ifndef WIN32
            sin.sin_addr.s_addr = INADDR_ANY;
#else
            sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            sin.sin_family = AF_INET;
            sin.sin_port = htons(0);
            if (bind(our_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_nLanguage)
                {
                    m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:∞???web??Ω”?÷??∞?", __FILE__,__LINE__);
                }
                else
                {
                    m_Log.SetRunInfo(0,"get server address failed.Info:bind web sock failed.", __FILE__,__LINE__);
                }
                
                closesocket(our_socket);
                return FALSE;
            }
        }
        
        if (connect(our_socket, (struct sockaddr *) & our_address, sizeof(our_address)) != 0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:web?¨Ω”??∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:web connect failed.", __FILE__,__LINE__);
            }
            
            closesocket(our_socket);
            return FALSE;
        }
        
        if(CCChannel::tcpsend(our_socket,target_e,strlen(target_e),3)<=0)
        {
            if(JVN_LANGUAGE_CHINESE == m_nLanguage)
            {
                m_Log.SetRunInfo(0,"aò?°∑??ò????∞?.‘≠“ú:?ú∑??ò??∑￠à???????∞?", __FILE__,__LINE__);
            }
            else
            {
                m_Log.SetRunInfo(0,"get server address failed.Info:send data to server failed.", __FILE__,__LINE__);
            }
            closesocket(our_socket);
            return FALSE;
        }
        tryTimes=0;
        while(tryTimes < 3)
        {
            int rlen = CCChannel::tcpreceive(our_socket,recvBuf,1024,2);
            if(rlen <0 || (nLen+rlen)>1024)
            {
                break;
            }
            memcpy(&IPBuf[nLen], recvBuf, rlen);
            nLen += rlen;
            tryTimes++;
        }
    }
    
    if(our_socket > 0)
    {
        closesocket(our_socket);
    }
    
    if(pHeadFilter)
    {
        char* p = strstr(IPBuf,pHeadFilter);
        int nL = nLen - (p - IPBuf);
        FILE *pfile = NULL;
        
        pfile = fopen(strFileName, "wb");
        
        if(pfile != NULL)
        {
            fwrite(p,sizeof(char),nL,pfile);
            fclose(pfile);
            return TRUE;
        }
        
    }
    else
    {
        FILE *pfile = NULL;
        
        pfile = fopen(strFileName, "wb");
        
        if(pfile != NULL)
        {
            fwrite(IPBuf,sizeof(char),nLen,pfile);
            fclose(pfile);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL CCWorker::ParseIndexFile()
{
    char chCurPath[MAX_PATH] = {0};
    GetCurrentPath(chCurPath);
    
    char strFileName[MAX_PATH] = {0};
    char strSaveFilePath[MAX_PATH] = {0};
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//
        sprintf(strFileName, "%s/server.txt", "./data");
        sprintf(strSaveFilePath, "%s/INDEX_", "./data");
    }
    else
    {//
        sprintf(strFileName, "%s/server.txt", m_chLocalPath);
        sprintf(strSaveFilePath, "%s/INDEX_", m_chLocalPath);
    }
#else
    sprintf(strFileName, "%s\\server.txt", chCurPath);
    sprintf(strSaveFilePath, "%s\\INDEX_", chCurPath);
#endif
    char strGroupNum[10] = {0};
    GetPrivateString("Group","Num",strGroupNum,strFileName);
    for(int i = 0;i < atoi(strGroupNum);i ++)
    {
        char strGroup[10] = {0};
        char strKey[20] = {0};
        sprintf(strKey,"Group%d",i);
        GetPrivateString("Group",strKey,strGroup,strFileName);
        
//        char strIndexNum[20] = {0};
//        GetPrivateString(strGroup,"Num",strIndexNum,strFileName);
        char strIndexNum[20] = {0};
        char strGroupKey[20] = {0};
        sprintf(strGroupKey,"%s_index",strGroup);
        GetPrivateString(strGroupKey,"Num",strIndexNum,strFileName);

        
        char strSaveFileName[MAX_PATH] = {0};
        sprintf(strSaveFileName,"%s%s.dat",strSaveFilePath,strGroup);
//        printf("ParseIndexFile bstrSaveFileName: %s\n",strSaveFileName);
        FILE *pfile = NULL;
        pfile = fopen(strSaveFileName, "wb");
        
        for(int j = 0;j < atoi(strIndexNum);j ++)
        {
            sprintf(strKey,"index%d",j);
            char strK[20] = {0};
            sprintf(strK,"%s_index",strGroup);
            char strIP[40] = {0};
            GetPrivateString(strK,strKey,strIP,strFileName);
            
            if(pfile != NULL)
            {
                strcat(strIP, "\r\n");
                int nL = strlen(strIP);
                
                fwrite(strIP,sizeof(char),nL,pfile);
            }
        }
        fclose(pfile);
    }
    
    return TRUE;
}

//void CCWorker::SearchCallBack(STLANSRESULT stLSResult)
//{
//	m_pfLANSData(stLSResult);
//
//	//m_Helper.RecvSearchDevice(stLSResult);
//}


void CCWorker::QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack)
{
    DEVICE_QUERY_DATA* pData = new DEVICE_QUERY_DATA;
    strcpy(pData->pGroup,pGroup);
    pData->nYST = nYST;
    pData->nTimeOut = nTimeOut;
    pData->callBack = callBack;
    pData->pWorker = this;
#ifdef WIN32
    UINT unTheadID;
    
    _beginthreadex(NULL, 0, QueryDeviceProc, (void *)pData, 0, &unTheadID);
    
#else
    pthread_t hSearch;
    pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
    pthread_create(&hSearch, pAttr, QueryDeviceProc, pData);
    
#endif
}



#ifdef WIN32
UINT WINAPI CCWorker::QueryDeviceProc(LPVOID pParam)
#else
void* CCWorker::QueryDeviceProc(void* pParam)
#endif
{
    DEVICE_QUERY_DATA *pData = (DEVICE_QUERY_DATA *)pParam;
    
    char pGroup[10] = {0};
    int nYST;
    int nTimeOut;
    FUNC_DEVICE_CALLBACK callBack;
    
    strcpy(pGroup,pData->pGroup);
    nYST = pData->nYST;
    nTimeOut = pData->nTimeOut;
    callBack = pData->callBack;
    
    CCWorker* pWorker = pData->pWorker;
    delete pData;
    
    NATList LocalIPList;//本地IP列表
    pWorker->GetLocalIP(&LocalIPList);
    
    int nIpNum = LocalIPList.size();
    
    BOOL bFind = FALSE;
    SOCKET *SocketLANS = new SOCKET[nIpNum];
    int i = 0;
    for(i = 0;i < nIpNum;i ++)
    {
        //套接字
        SocketLANS[i] = socket(AF_INET, SOCK_DGRAM,0);
        SOCKADDR_IN addrSrv;
        char strIP[30] = {0};
        sprintf(strIP,"%d.%d.%d.%d",
                LocalIPList[i].ip[0],LocalIPList[i].ip[1],LocalIPList[i].ip[2],LocalIPList[i].ip[3]);
#ifndef WIN32
        addrSrv.sin_addr.s_addr = inet_addr(strIP);
#else
        addrSrv.sin_addr.S_un.S_addr = inet_addr(strIP);
#endif
        addrSrv.sin_family = AF_INET;
        addrSrv.sin_port = htons(0);
        //绑定套接字
        bind(SocketLANS[i], (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
        
        // 有效SO_BROADCAST选项
        int nBroadcast = 1;
        ::setsockopt(SocketLANS[i], SOL_SOCKET, SO_BROADCAST, (char*)&nBroadcast, sizeof(int));
    }
    SOCKADDR_IN addrBcast;
    // 设置广播地址，这里的广播端口号
    addrBcast.sin_family = AF_INET;
    addrBcast.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
    
    addrBcast.sin_port = htons(6666);
    
    BYTE data[30] = {0};
    int nSLen = 0;
    int nSType = 0;
    int nType = JVN_REQ_LANSERCH;
    
    int m_nLANSPort = 0;
    int m_unLANSID = 1;
    int nVariety = 0;
    nSType = JVN_CMD_LANSALL;
    nSLen = 13;
    
    memcpy(&data[0], &nType, 4);
    
    memcpy(&data[4], &nSLen, 4);
    memcpy(&data[8], &m_nLANSPort, 4);
    memcpy(&data[12], &m_unLANSID, 4);
    memcpy(&data[16], &nSType, 4);
    data[20] = nVariety;
    
    char recBuf[1024] = {0};
    SOCKADDR_IN clientaddr = {0};
    int addrlen = sizeof(SOCKADDR_IN);
    
    DWORD dwStart = CCWorker::JVGetTime();
    DWORD dwEnd = CCWorker::JVGetTime();
    int nTimes = 10;
    while(dwEnd - dwStart < nTimeOut)
    {
        for(i = 0;i < nIpNum;i ++)
        {
            int ntmp = CCChannel::sendtoclient(SocketLANS[i],(char *)data,nSLen+8,0,(SOCKADDR *)&addrBcast, sizeof(SOCKADDR),1);
        }
        //OutputDebug("search....");
        CCWorker::jvc_sleep(30);
        
        for(int k = 0;k < nTimes;k ++)
        {
            for(i = 0;i < nIpNum;i ++)
            {
                int nRecvLen = CCChannel::receivefromm(SocketLANS[i],recBuf, 1024, 0, (SOCKADDR*)&clientaddr,&addrlen,100);
                if(nRecvLen > 0)
                {
                    memcpy(&nType, &recBuf[0], 4);
                    int nRLen = 0,unSerchID = 0;
                    if(nType == JVN_RSP_LANSERCH)
                    {
                        STLANSRESULT stLSResult = {0};
                        memcpy(&nRLen, &recBuf[4], 4);//长度(4)
                        memcpy(&unSerchID, &recBuf[8], 4);//搜索ID(4)
                        memcpy(&stLSResult.nCardType, &recBuf[12], 4);//卡系(4)
                        memcpy(&stLSResult.nYSTNO, &recBuf[16], 4);//云视通号(4)
                        memcpy(&stLSResult.nChannelCount, &recBuf[20], 4);//总通道数(4)
                        memcpy(&stLSResult.nClientPort, &recBuf[24], 4);//服务端口号(4)
                        memcpy(stLSResult.chGroup, &recBuf[28], 4);//编组号(4)
                        stLSResult.nVariety = recBuf[32];//产品种类(1)
                        int nNameLen = recBuf[33];//别名长度(1)
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
                        
                        
                        callBack(&stLSResult); 
                    }	
                } 
            }//end for 
            if(bFind) 
                break; 
        } 
        
        CCWorker::jvc_sleep(200); 
        dwEnd = CCWorker::JVGetTime(); 
    } 
    
    for(i = 0;i < nIpNum;i ++) 
    { 
        closesocket(SocketLANS[i]); 
    } 
    delete []SocketLANS; 
    
    
#ifndef WIN32 
    STLANSRESULT stLSResult = {0};
    memset(&stLSResult,0, sizeof(STLANSRESULT));
    stLSResult.bTimoOut = TRUE;
    callBack(&stLSResult);
    return NULL; 
#else 
    _endthread(); 
    return 0; 
#endif 
}


bool CCWorker::ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout)
{
	CLocker lock(m_RtmpLock);
	for(RTMP_LIST::iterator k = m_RtmpChannelList.begin(); k != m_RtmpChannelList.end(); ++ k)
	{
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )k->second;
		if(nLocalChannel == pRTMP->m_nLocalChannel)
		{
//			if(pRTMP->m_hRtmpClient != NULL)
//			{
//				return false;
//			}
//			else
			{
				return pRTMP->ConnectServer(nLocalChannel,strURL,rtmpConnectChange,rtmpNormalData,nTimeout);
			}
		}
	}
	CCRtmpChannel* pRTMP = new CCRtmpChannel;
	m_RtmpChannelList.insert(std::map<int,CCRtmpChannel* >::value_type(nLocalChannel, pRTMP));

	return pRTMP->ConnectServer(nLocalChannel,strURL,rtmpConnectChange,rtmpNormalData,nTimeout);
}

void CCWorker::ShutdownRTMP(int nLocalChannel)
{
	CLocker lock(m_RtmpLock);
	int nnn = m_RtmpChannelList.size();
	RTMP_LIST::iterator i = m_RtmpChannelList.find(nLocalChannel);
	
	if(i != m_RtmpChannelList.end())
	{
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )i->second;
		m_RtmpChannelList.erase(nLocalChannel);
		delete pRTMP;
	}
}

void CCWorker::ShutdownAllRTMP()
{
	CLocker lock(m_RtmpLock);
	for(RTMP_LIST::iterator k = m_RtmpChannelList.begin(); k != m_RtmpChannelList.end(); ++ k)
	{
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )k->second;
		
		delete pRTMP;
	}
	m_RtmpChannelList.clear();

}

void CCWorker::ClearHelpCache()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctYSTNO);
#else
	EnterCriticalSection(&m_ctYSTNO);
#endif
	m_stYSTCushion.clear();
	m_stYSTNOLIST.clear();
#ifndef WIN32
	pthread_mutex_unlock(&m_ctYSTNO);
#else
	LeaveCriticalSection(&m_ctYSTNO);
#endif

	if(m_pHelpCtrl != NULL)
	{
		m_pHelpCtrl->ClearCache();
	}
}

int CCWorker::SetMTU(int nMtu)
{
	JVC_MSS= nMtu;
	return 1;
}

int CCWorker::stopHelp()
{
    if(m_pHelpCtrl)
    {
        CCHelpCtrlM* pHelp = (CCHelpCtrlM* )m_pHelpCtrl;
        pHelp->StopHelp();
        
        delete pHelp;
        if(m_WorkerUDTSocket!=0)
        {
            UDT::close(m_WorkerUDTSocket);
            m_WorkerUDTSocket = 0;
        }
    }
    m_pHelpCtrl = NULL;
    return 1;
}


char* Get1IpByDomain(char* pServer,BOOL& bIsDomain)
{
    if (inet_addr(pServer) != INADDR_NONE)
    {
        bIsDomain = FALSE;
        return pServer;
    }
    static char strIP[20] = {0};
    
    hostent* answer = gethostbyname(pServer);
    if (answer == NULL)
    {
        return strIP;
    }
#ifndef WIN32
    {
        inet_ntop(AF_INET, answer->h_addr_list[0], strIP, sizeof(strIP));
    }
#else
    BYTE ip[4] = {0};
    memcpy(ip,answer->h_addr_list[0],4);
    
    sprintf(strIP,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
    
#endif
    bIsDomain = TRUE;
    return strIP;
}

int CCWorker::SetSelfServer(char* pGroup,char* pServer)
{
    writeLog("SetSelfServer pGroup:%s pServer:%s",pGroup?pGroup:"",pServer);
    if(pGroup&&strlen(pGroup)!=0)
    {
        OutputDebug("SetSelfServer %s  %s",pGroup,pServer);
        CSELF_DEFINE_SERVER server = {0};
        BOOL bIsDomain = FALSE;
        char strIP[20] = {0};
        strcpy(strIP,Get1IpByDomain(pServer,bIsDomain));
        if(bIsDomain)
        {
            server.bIsDomain = TRUE;
            strcpy(server.strDomain,pServer);
        }
        strcpy(server.strGroup,pGroup);
        strcpy(server.strIP,strIP);
        for(int i = 0;i < m_CServerPortList.size();i ++)
        {
#ifndef WIN32
            if(strcasecmp(m_CServerPortList[i].strGroup,pGroup) == 0)
#else
                if(stricmp(m_CServerPortList[i].strGroup,pGroup) == 0)
#endif
                {
                    server.nPort = m_CServerPortList[i].nServerPort;
                    break;
                }
            
        }
        
        if(server.nPort != 0)
        {
            m_CSelfDefineServer.push_back(server);
            return 1;
        }
    }
    else//设置全部组
    {
        OutputDebug("SetSelfServer All  %s",pServer);
        BOOL bIsDomain = FALSE;
        char strIP[20] = {0};
        strcpy(strIP,Get1IpByDomain(pServer,bIsDomain));
        for(int i = 0;i < m_CServerPortList.size();i ++)
        {
            CSELF_DEFINE_SERVER server = {0};
            if(bIsDomain)
            {
                server.bIsDomain = TRUE;
                strcpy(server.strDomain,pServer);
            }
            strcpy(server.strGroup,m_CServerPortList[i].strGroup);
            strcpy(server.strIP,strIP);
            server.nPort = m_CServerPortList[i].nServerPort;
            
            m_CSelfDefineServer.push_back(server);
        }
    }
    return 0;
}

int CCWorker::SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
{
    char strPath[MAX_PATH] = {0};
    sprintf(strPath,"%s%s%s",JVN_WEBFOLDER,pGroup,JVN_YSTLIST_ALL);
    
    char strError[1024] = {0};
    ServerList SList;
    if(!DownLoadFirst(pGroup, SList, 1, 0))
    {
        if(!DownLoadFirst(pGroup, SList, 2, 0))
        {
            return -2;
        }
    }
#ifdef _DEBUG
    STSERVER stserver;
#ifndef WIN32
    stserver.addr.sin_addr.s_addr = inet_addr("172.16.29.197");
#else
    stserver.addr.sin_addr.S_un.S_addr = inet_addr("172.16.29.197");
#endif
    stserver.addr.sin_family = AF_INET;
    stserver.addr.sin_port = htons(9010);
    stserver.buseful = TRUE;
    stserver.nver = 0;
    SList.push_back(stserver);//记录
#endif
    
    SOCKET sNatCheckSocket = socket(AF_INET, SOCK_DGRAM,0);
    SOCKADDR_IN addrL;
#ifndef WIN32
    addrL.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrL.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrL.sin_family = AF_INET;
    addrL.sin_port = htons(0);
    
    //绑定套接字
    int err = bind(sNatCheckSocket, (SOCKADDR *)&addrL, sizeof(SOCKADDR));
    if(err != 0)
    {
        closesocket(sNatCheckSocket);
        sNatCheckSocket = 0;
        return -3;
    }
    
    int nL = *nLen;
    char strSend[1024] = {0};
    char strRecv[1024] = {0};
    int nType = JVN_COMMAND_AB;//添加服务器
    
    memcpy(&strSend[0],&nType,4);
    memcpy(&strSend[4],&nYst,4);
    strSend[8] = 1;//1添加 2 删除
    memcpy(&strSend[9],&nL,4);
    memcpy(&strSend[13],pServer,nL);
    
    for(int i = 0;i < SList.size();i ++)
    {
        CCChannel::sendtoclientm(sNatCheckSocket, strSend, nL + 13, 0, (sockaddr *)&SList[i].addr, sizeof(SOCKADDR_IN),1);
    }
    
    DWORD d = CCWorker::JVGetTime();
    DWORD dwSt = d;
    SOCKADDR_IN natAddr = {0};
    BOOL bRet = FALSE;
    while(d < (dwSt + nTimeOut * 1000))
    {
        d = CCWorker::JVGetTime();
        int addrlen = sizeof(SOCKADDR_IN);
        int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
        
        if(nNatLen > 0)
        {
            int nT = 0;
            memcpy(&nT,&strRecv[0],4);
            if(nT == JVN_COMMAND_BA)//添加服务器
            {
                int nAdd = strRecv[4];
                if(nAdd == 1)//1添加
                {
                    int l = 0;
                    memcpy(&l,&strRecv[5],4);
                    if(l > 0 && l < 1000)
                    {
                        memcpy(pServer,&strRecv[9],l);
                        *nLen = l;
                        bRet = TRUE;
                        break;
                    }
                }
            }
            break;
        }
        CCWorker::jvc_sleep(10);
    }
    closesocket(sNatCheckSocket);
    sNatCheckSocket = 0;
    
    if(bRet)
        return 1;
    return 0;
}

int CCWorker::SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
{
    char strPath[MAX_PATH] = {0};
    sprintf(strPath,"%s%s%s",JVN_WEBFOLDER,pGroup,JVN_YSTLIST_ALL);
    
    char strError[1024] = {0};
    ServerList SList;
    if(!DownLoadFirst(pGroup, SList, 1, 0))
    {
        if(!DownLoadFirst(pGroup, SList, 2, 0))
        {
            return -2;
        }
    }
#ifdef _DEBUG
    STSERVER stserver;
#ifndef WIN32
    stserver.addr.sin_addr.s_addr = inet_addr("172.16.29.197");
#else
    stserver.addr.sin_addr.S_un.S_addr = inet_addr("172.16.29.197");
#endif
    stserver.addr.sin_family = AF_INET;
    stserver.addr.sin_port = htons(9010);
    stserver.buseful = TRUE;
    stserver.nver = 0;
    SList.push_back(stserver);//记录
#endif
    
    SOCKET sNatCheckSocket = socket(AF_INET, SOCK_DGRAM,0);
    SOCKADDR_IN addrL;
#ifndef WIN32
    addrL.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrL.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrL.sin_family = AF_INET;
    addrL.sin_port = htons(0);
    
    //绑定套接字
    int err = bind(sNatCheckSocket, (SOCKADDR *)&addrL, sizeof(SOCKADDR));
    if(err != 0)
    {
        closesocket(sNatCheckSocket);
        sNatCheckSocket = 0;
        return -3;
    }
    
    int nL = *nLen;
    char strSend[1024] = {0};
    char strRecv[1024] = {0};
    int nType = JVN_COMMAND_AB;//删除服务器
    
    memcpy(&strSend[0],&nType,4);
    memcpy(&strSend[4],&nYst,4);
    strSend[8] = 2;//1添加 2 删除
    memcpy(&strSend[9],&nL,4);
    memcpy(&strSend[13],pServer,nL);
    
    for(int i = 0;i < SList.size();i ++)
    {
        CCChannel::sendtoclientm(sNatCheckSocket, strSend, nL + 13, 0, (sockaddr *)&SList[i].addr, sizeof(SOCKADDR_IN),1);
    }
    
    DWORD d = CCWorker::JVGetTime();
    DWORD dwSt = d;
    SOCKADDR_IN natAddr = {0};
    BOOL bRet = FALSE;
    while(d < (dwSt + nTimeOut * 1000))
    {
        d = CCWorker::JVGetTime();
        int addrlen = sizeof(SOCKADDR_IN);
        int nNatLen = CCChannel::receivefromm(sNatCheckSocket,strRecv, 1024, 0, (SOCKADDR*)&natAddr,&addrlen,100);
        
        if(nNatLen > 0)
        {
            int nT = 0;
            memcpy(&nT,&strRecv[0],4);
            if(nT == JVN_COMMAND_BA)//删除服务器
            {
                int nAdd = strRecv[4];
                if(nAdd == 2)//2 删除
                {
                    int l = 0;
                    memcpy(&l,&strRecv[5],4);
                    if(l > 0 && l < 1000)
                    {
                        memcpy(pServer,&strRecv[9],l);
                        *nLen = l;
                        bRet = TRUE;
                        break;
                    }
                }
            }
            break;
        }
        CCWorker::jvc_sleep(10);
    }
    closesocket(sNatCheckSocket);
    sNatCheckSocket = 0;
    
    if(bRet)
        return 1;
    return 0;
}


void CCWorker::AddYstSvr(char* chGroup,SOCKADDR_IN addr)
{
	if(NULL == chGroup)
	{
		return;
	}
	STSERVER svraddr;
	svraddr.buseful = false;
	svraddr.nver = 0;
	memcpy(&svraddr.addr,&addr,sizeof(SOCKADDR_IN));
	int size = m_YstSvrList.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(chGroup,m_YstSvrList[i].chGroup) == 0)
		{
			int svrnum = m_YstSvrList[i].addrlist.size();
			if(svrnum == 0)
			{
				m_YstSvrList[i].addrlist.push_back(svraddr);
				return;
			}
			for(int j = 0; j < svrnum; j++)
			{
#ifdef WIN32
				if(m_YstSvrList[i].addrlist[j].addr.sin_addr.S_un.S_addr == svraddr.addr.sin_addr.S_un.S_addr)
#else 
				if(m_YstSvrList[i].addrlist[j].addr.sin_addr.s_addr== svraddr.addr.sin_addr.s_addr)
#endif
				{
					return;
				}
			}
			m_YstSvrList[i].addrlist.push_back(svraddr);
			return;
		}
	}
	CYstSvrList list;
	strcpy(list.chGroup,chGroup);
	list.addrlist.push_back(svraddr);
	m_YstSvrList.push_back(list);
}
void CCWorker::AddYstSvr(char* chGroup,STSERVER svraddr)
{
	if(NULL == chGroup)
	{
		return;
	}
	int size = m_YstSvrList.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(chGroup,m_YstSvrList[i].chGroup) == 0)
		{
			int svrnum = m_YstSvrList[i].addrlist.size();
			if(svrnum == 0)
			{
				m_YstSvrList[i].addrlist.push_back(svraddr);
				return;
			}
			for(int j = 0; j < svrnum; j++)
			{
#ifdef WIN32
				if(m_YstSvrList[i].addrlist[j].addr.sin_addr.S_un.S_addr == svraddr.addr.sin_addr.S_un.S_addr)
#else 
				if(m_YstSvrList[i].addrlist[j].addr.sin_addr.s_addr== svraddr.addr.sin_addr.s_addr)
#endif
				{
					return;
				}
			}
			m_YstSvrList[i].addrlist.push_back(svraddr);
			return;
		}
	}
	CYstSvrList list;
	strcpy(list.chGroup,chGroup);
	list.addrlist.push_back(svraddr);
	m_YstSvrList.push_back(list);
}

void CCWorker::ShowYstSvr(void)
{
	return;
	int size = m_YstSvrList.size();
	for(int i = 0; i < size; i++)
	{
		printf("%s:%d..............**********.........group %s svrlistsize:%d\n",__FILE__,__LINE__,m_YstSvrList[i].chGroup,m_YstSvrList[i].addrlist.size());
		for(int j = 0; j < m_YstSvrList[i].addrlist.size(); j++)
		{
			printf("group:%s, addr: %s\n",m_YstSvrList[i].chGroup,inet_ntoa(m_YstSvrList[i].addrlist[j].addr.sin_addr));
		}
	}
}
int CCWorker::GetGroupSvrListIndex(char* chGroup)
{
	if(NULL == chGroup)
	{
		return -1;
	}
	int size = m_YstSvrList.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(chGroup,m_YstSvrList[i].chGroup) == 0)
		{
			return i;
		}
	}
	CYstSvrList list;
	strcpy(list.chGroup,chGroup);
	list.addrlist.clear();
	m_YstSvrList.push_back(list);
	return size;
}
void CCWorker::GetGroupSvrList(char* chGroup,CYstSvrList &grouplist)
{
    if(NULL == chGroup)
    {
        return;
    }
    int size = m_YstSvrList.size();
    writeLog(".......................size %d line %d",size,__LINE__);
    if(size < 1)
    {
        int ret = ReadSerListInFile(chGroup,grouplist.addrlist);
        writeLog("..*********************..readfiler return:%d",ret);
        return;
    }
    
    for(int i = 0; i < size; i++)
    {
        if(strcmp(chGroup,m_YstSvrList[i].chGroup) == 0)
        {
            grouplist = m_YstSvrList[i];
            if(grouplist.addrlist.size() == 0)
            {
                int ret = ReadSerListInFile(chGroup,grouplist.addrlist);
                writeLog("..*********************..readfiler return:%d",ret);
            }
            return;
        }
    }
    
    if(grouplist.addrlist.size() < 1)
    {
        int ret = ReadSerListInFile(chGroup,grouplist.addrlist);
        writeLog("..*********************..readfiler return:%d",ret);
    }
}
int  CCWorker::ReadSerListInFile(char chGroup[4],ServerList &list)
{
    char acBuff[MAX_PATH] = {0};
    char chCurPath[MAX_PATH] = {0};
    int nSerCount = 0;
    GetCurrentPath(chCurPath);
    
#ifndef WIN32
    if(strlen(m_chLocalPath) <= 0)
    {//没有明确指定路径，则在dvr下采用固定路径而不是实际当前路径
        sprintf(acBuff, "%s/yst_%s.dat", chCurPath,chGroup);
    }
    else
    {//使用指定的路径
        sprintf(acBuff, "%s/yst_%s.dat", m_chLocalPath,chGroup);
    }
#else
    sprintf(acBuff, "%s\\yst_%s.dat", chCurPath,chGroup);
#endif
    
    writeLog("readserlist, acBuff:%s,m_chLocalPath:%s",acBuff,m_chLocalPath);
    
    char chread[10240]={0};
    int nread=0;
    char stName[200] = {0};
    sprintf(stName, "%s%s.dat","YST_",chGroup);
    
    STSERVER ser;
    ser.addr.sin_family = AF_INET;
    unsigned short port = 0;
    char chPort[16] = {0};
    
#ifdef MOBILE_CLIENT //手机客户端，从内存读取
    nread = ReadMobileFile(stName,chread,10240);
    //	m_pfWriteReadData(2,(unsigned char *)chGroup,"YST_",(unsigned char *)chread,&nread);//YST read 杩欓噷璋冪敤鍥炶皟鍑芥暟
    if(nread > 0)
    {
        int npostmp = 0;
        memset(acBuff, 0, MAX_PATH);
        while(MOGetLine(chread,nread,npostmp,acBuff))//YST
        {
            char chIP[20] = {0};
            //服务器地址解密
            for (unsigned int m=0; m<strlen(acBuff); m++)
            {
                acBuff[m] ^= m;
            }
            STSIP stIP;
            memset(stIP.chIP, 0, 16);
            int i=0;
            char chPort[16] = {0};
            for(i=0; i<MAX_PATH; i++)
            {
                if(acBuff[i] == ':' || acBuff[i] == '\0')
                {
                    break;
                }
            }
            //memcpy(stIP.chIP, acBuff, i);
            //memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i-1);
            if(i> 0)
            {
                memcpy(chIP, acBuff, i);
                memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i+1);
            }
            port = atoi(chPort);
            if(port > 0)
            {
#ifdef WIN32
                ser.addr.sin_addr.S_un.S_addr = inet_addr(chIP);
#else
                ser.addr.sin_addr.s_addr= inet_addr(chIP);
#endif
                ser.addr.sin_port = htons(port);
                list.push_back(ser);
                nSerCount++;
            }
        }
    }
    
#else//其他情况，从文件读取
    //找到记录,采取先读后下载方式
    ::std::string line;
    ::std::ifstream localfile(acBuff);
    while(::std::getline(localfile,line))
    {
        char chIP[20] = {0};
        port = 0;
        memset(acBuff, 0, MAX_PATH);
        line.copy(acBuff,MAX_PATH,0);
        //服务器地址解密
        for (unsigned int m=0; m<strlen(acBuff); m++)
        {
            acBuff[m] ^= m;
        }
        int i=0;
        
        for(i=0; i<MAX_PATH; i++)
        {
            if(acBuff[i] == ':' || acBuff[i] == '\0')
            {
                break;
            }
        }
        if(i > 0)
        {
            memcpy(chIP, acBuff, i);
            memcpy(chPort, &acBuff[i+1], strlen(acBuff)-i+1);
        }
        
        port = atoi(chPort);
        //outdbgs(0,".........................read ip:%s,port:%d,acbuff:%s,i:%d",chIP,port,acBuff,i);
        writeLog("...chIP:%s,port:%d",chIP,port);
        //writeLog("...acBuff:%s",acBuff);
        if(port > 0)
        {
#ifdef WIN32
            ser.addr.sin_addr.S_un.S_addr = inet_addr(chIP);
#else
            ser.addr.sin_addr.s_addr= inet_addr(chIP);
#endif
            ser.addr.sin_port = htons(port);
            list.push_back(ser);
            nSerCount++;
        }
    }
#endif
    return nSerCount;
}

BOOL CCWorker::StartBCSelfServer(int nLPort, int nServerPort)
{
    int nport = 0;
    int nsport = 0;
    if(nLPort >= 0)
    {
        nport = nLPort;
    }
    
    if(nServerPort > 0)
    {
        nsport = nServerPort;
    }
    
    if(m_pBCSelf != NULL)
    {
        return TRUE;
    }
    
    m_pBCSelf = new CCLanSerch(nport, nsport, this, JVC_BC_SELF);
    if(m_pBCSelf != NULL)
    {
        if(m_pBCSelf->m_bOK)
        {
            return TRUE;
        }
        delete m_pBCSelf;
        m_pBCSelf = NULL;
        return FALSE;
    }
    else
    {
        return FALSE;
    }
}

void CCWorker::StopBCSelfServer()
{
    if(m_pBCSelf != NULL)
    {
        delete m_pBCSelf;
    }
    m_pBCSelf = NULL;
}

BOOL CCWorker::DoBroadcastSelf(BYTE *pBuffer, int nSize, int nTimeOut)
{
    if(m_pBCSelf == NULL)
    {
        return FALSE;
    }
    return m_pBCSelf->Broadcast(0, pBuffer, nSize, nTimeOut);
}

BOOL CCWorker::DoSendSelfDataFromBC(BYTE *pBuffer, int nSize, char *pchDeviceIP, int nDestPort)
{
    if(m_pBCSelf == NULL)
    {
        return FALSE;
    }
    return m_pBCSelf->SendSelfDataFromBC(pBuffer, nSize, pchDeviceIP, nDestPort);
}
