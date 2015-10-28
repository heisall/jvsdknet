// CChannel.cpp: implementation of the CCChannel class.
//
//////////////////////////////////////////////////////////////////////

#include "CChannel.h"
#include "JVN_DBG.h"
#include "CWorker.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int JVC_MSS;
CCChannel::CCChannel()
{
}

CCChannel::CCChannel(STCONNECTINFO stConnectInfo, CCWorker *pWorker)
{
	m_TurnSvrAddrList.clear();
	pWorker->GetGroupSvrList(stConnectInfo.chGroup,m_GroupSvrList);

#ifndef WIN32
	//m_GroupSvrList.addrlist.clear();
	//STSERVER svr;
	//inet_aton("172.16.29.197", &svr.addr.sin_addr);
	//svr.addr.sin_port = htons(9010);
	//m_GroupSvrList.addrlist.push_back(svr);


	//inet_aton("172.16.29.191", &svr.addr.sin_addr);
	//svr.addr.sin_port = htons(9010);
	//m_GroupSvrList.addrlist.push_back(svr);
	//{
	//	for(int i = 0; i < m_GroupSvrList.addrlist.size(); i++)
	//	{
	//		printf("%s;%d........group:%s,ip:%s\n",__FILE__,__LINE__,m_GroupSvrList.chGroup,inet_ntoa(m_GroupSvrList.addrlist[i].addr.sin_addr));
	//	}

	//} //zhouhaotest
#endif

    m_nReconnectTimes = 0;
    m_bIsTurn = FALSE;
    m_dwLastUpdateTime = 0;
    
    m_nNatTypeA = NAT_TYPE_0_UNKNOWN;
    m_bDisConnectShow = FALSE;
    m_pOldChannel = NULL;
    
    m_pHelpConn = NULL;
    
    m_nOCount = 1;
    
    m_nProtocolType = stConnectInfo.nConnectType;
    m_bExit = FALSE;
    m_stConnInfo.nShow = 0;
    
    m_bAcceptChat = FALSE;
    m_bAcceptText = FALSE;
    m_bDAndP = FALSE;
    
    m_nChannel = stConnectInfo.nChannel; //0;
    m_nLocalChannel = stConnectInfo.nLocalChannel;
    m_nLocalStartPort = stConnectInfo.nLocalPort;
    m_pWorker = pWorker;
    m_bPass = FALSE;
    m_recvThreadExit = TRUE;//接收数据线程 线程退出标志
    m_connectThreadExit = FALSE;//连接线程退出标志
    
    m_nFYSTVER = 0;
    m_bOpenTurnAudio = FALSE;
    
    m_dwLastDataTime = CCWorker::JVGetTime();
    
#ifndef WIN32
    m_bEndPT = FALSE;
    m_bEndPTC = FALSE;
    m_bEndR = FALSE;
    m_bEndC = FALSE;
    //m_bEndTurnConn = FALSE;
    
    pthread_mutex_init(&m_ct, NULL); //初始化临界区
#else
    m_hStartEventPT = 0;
    m_hEndEventPT = 0;
    
    m_hStartEventPTC = 0;
    m_hEndEventPTC = 0;
    
    m_hStartEventR = 0;
    m_hEndEventR = 0;
    
    m_hStartEventC = 0;
    m_hEndEventC = 0;
    
    InitializeCriticalSection(&m_ct); //初始化临界区
#endif
    m_hPartnerThread = 0;
    m_hPTCThread = 0;
    m_hRecvThread = 0;
    m_hConnThread = 0;
    
    m_ServerSocket = 0;//对应服务的socket
    m_ListenSocket = 0;
    m_ListenSocketTCP = 0;
    
    m_SocketSTmp = 0;
    
    memset(&m_addrLastLTryAL, 0, sizeof(SOCKADDR_IN));
    
    memcpy(&m_stConnInfo, &stConnectInfo, sizeof(STCONNECTINFO));
    
    m_nTimeOutCount = 0;
    m_dwRUseTTime = 0;
    m_dwLastInfoTime = 0;
    m_nLastDownLoadTotalB = 0;
    m_nLastDownLoadTotalKB = 0;
    m_nLastDownLoadTotalMB = 0;
    m_nDownLoadTotalB = 0;
    m_nDownLoadTotalKB = 0;
    m_nDownLoadTotalMB = 0;
    m_nLPSAvage = 0;
    
    m_unBeginChunkID = 0;
    m_nChunkCount = 0;
    
    m_bLan2A = FALSE;
    
    m_bCache = FALSE;
    m_bTURN = FALSE;
    
    m_bJVP2P = TRUE;
    m_pBuffer = NULL;
    
    m_preadBuf = new BYTE[JVNC_DATABUFLEN];
    m_puchRecvBuf = new BYTE[JVNC_DATABUFLEN];
    m_pchPartnerInfo = NULL;
    m_nPartnerLen = 0;
    
    if(m_stConnInfo.nWhoAmI == JVN_WHO_H)
    {
        m_pchPartnerInfo = new char[JVNC_PTINFO_LEN];
    }
    
    m_nRecvPos = 0;
    m_nRecvPackLen = 0;
    
    m_nLinkID = 0;
    
    m_PartnerCtrl.m_pChannel = this;
    m_PartnerCtrl.m_pWorker = m_pWorker;
    
    m_bPassWordErr = FALSE;
    m_nStatus = NEW_IP;
    
    m_NatList.clear();
    
    m_sQueryIndexSocket = socket(AF_INET, SOCK_DGRAM,0);
    
    
    SOCKADDR_IN addrSrv;
#ifndef WIN32
    addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(0);
    int size = 1024*100;
    setsockopt(m_sQueryIndexSocket, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int));
    setsockopt(m_sQueryIndexSocket, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int));
    
//    setsockopt(<#int#>, <#int#>, <#int#>, <#const void *#>, <#socklen_t#>)
   int result =  bind(m_sQueryIndexSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
    OutputDebug("channel bind result: %d",result);
    //小数手没有开始工作，走本地基本流程
    StartConnThread();
}


CCChannel::~CCChannel()
{
    m_bOpenTurnAudio = FALSE;
    //OutputDebugString("~ccchannel...........\n");
    /*	if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
     {
     m_pWorker->WriteYSTNOInfo(m_stConnInfo.nYSTNO, m_SList, m_addrAN, -1);
     }
     */
    if(m_pOldChannel != NULL)
    {
        delete m_pOldChannel;
        m_pOldChannel = NULL;
    }
    
    m_bExit = TRUE;
    
#ifndef WIN32
    if (0 != m_hConnThread)
    {
        m_bEndC = TRUE;
        pthread_join(m_hConnThread, NULL);
        m_hConnThread = 0;
    }
    if (0 != m_hRecvThread)
    {
        m_bEndR = TRUE;
        pthread_join(m_hRecvThread, NULL);
        m_hRecvThread = 0;
    }
    if (0 != m_hPartnerThread)
    {
        m_bEndPT = TRUE;
        pthread_join(m_hPartnerThread, NULL);
        m_hPartnerThread = 0;
    }
    if (0 != m_hPTCThread)
    {
        m_bEndPTC = TRUE;
        pthread_join(m_hPTCThread, NULL);
        m_hPTCThread = 0;
    }
    
    m_PartnerCtrl.ClearPartner();
#else
    if(m_hEndEventC>0)
    {
        SetEvent(m_hEndEventC);
    }
    
    if(m_hEndEventR>0)
    {
        SetEvent(m_hEndEventR);
    }
    
    if(m_hEndEventPT>0)
    {
        SetEvent(m_hEndEventPT);
    }
    
    if(m_hEndEventPTC>0)
    {
        SetEvent(m_hEndEventPTC);
    }
    
    CCWorker::jvc_sleep(10);
    
    m_PartnerCtrl.ClearPartner();
    
    WaitThreadExit(m_hConnThread);
    WaitThreadExit(m_hRecvThread);
    WaitThreadExit(m_hPartnerThread);
    WaitThreadExit(m_hPTCThread);
    
    if(m_hEndEventC > 0)
    {
        CloseHandle(m_hEndEventC);
        m_hEndEventC = 0;
    }
    if(m_hStartEventC > 0)
    {
        CloseHandle(m_hStartEventC);
        m_hStartEventC = 0;
    }
    if(m_hEndEventR > 0)
    {
        CloseHandle(m_hEndEventR);
        m_hEndEventR = 0;
    }
    if(m_hEndEventPT > 0)
    {
        CloseHandle(m_hEndEventPT);
        m_hEndEventPT = 0;
    }
    if(m_hEndEventPTC > 0)
    {
        CloseHandle(m_hEndEventPTC);
        m_hEndEventPTC = 0;
    }
    
    if(m_hConnThread > 0)
    {
        CloseHandle(m_hConnThread);
        m_hConnThread = 0;
    }
    if(m_hRecvThread > 0)
    {
        CloseHandle(m_hRecvThread);
        m_hRecvThread = 0;
    }
    if(m_hPartnerThread > 0)
    {
        CloseHandle(m_hPartnerThread);
        m_hPartnerThread = 0;
    }
    if(m_hPTCThread > 0)
    {
        CloseHandle(m_hPTCThread);
        m_hPTCThread = 0;
    }
#endif
    
    if(m_ServerSocket > 0)
    {
        if(m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        else
        {
            closesocket(m_ServerSocket);
        }
    }
    m_ServerSocket = 0;
    
    if(m_ListenSocket > 0)
    {
        m_pWorker->pushtmpsock(m_ListenSocket);
    }
    m_ListenSocket = 0;
    
    if(m_ListenSocketTCP > 0)
    {
        closesocket(m_ListenSocketTCP);
    }
    m_ListenSocketTCP = 0;
    
    //这里考虑到udt公用套接字，这里不需要关闭，udt部分自会释放。这里不关闭，仅仅占用了一个套接字的编号.
    //可以有效避免断开后再次连接很近时，前次的号码再次被使用，结果udt释放较慢，是第二次的再次被释放造成的连接失败
    //	if(m_SocketSTmp > 0)
    //	{
    //		closesocket(m_SocketSTmp);
    //	}
    //	m_SocketSTmp = 0;
    
    //	m_PartnerCtrl.ClearPartner();
    
    if(m_pHelpConn != NULL)
    {
        delete m_pHelpConn;
        m_pHelpConn = NULL;
    }
    
    if(m_pBuffer != NULL)
    {
        delete m_pBuffer;
        m_pBuffer = NULL;
    }
    
    if(m_preadBuf != NULL)
    {
        delete[] m_preadBuf;
        m_preadBuf = NULL;
    }
    
    if(m_puchRecvBuf != NULL)
    {
        delete[] m_puchRecvBuf;
        m_puchRecvBuf = NULL;
    }
    if(m_pchPartnerInfo != NULL)
    {
        delete[] m_pchPartnerInfo;
        m_pchPartnerInfo = NULL;
    }
    
#ifndef WIN32
    pthread_mutex_destroy(&m_ct);
#else
    DeleteCriticalSection(&m_ct); //释放临界区
#endif
    closesocket(m_sQueryIndexSocket);
    m_sQueryIndexSocket = 0;
	m_SListTurn.clear();
}

void CCChannel::StartConnThread()
{
#ifndef WIN32
    pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    m_connectThreadExit = FALSE;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
    if (0 != pthread_create(&m_hConnThread, pAttr, ConnProc, this))
    {
        m_hConnThread = 0;
        m_connectThreadExit = TRUE;
#else
        //开启连接线程
        UINT unTheadID;
        m_hStartEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hConnThread = (HANDLE)_beginthreadex(NULL, 0, ConnProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventC);
        if (m_hConnThread == 0)//创建线程失败
        {
            //清理Conn线程
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
#endif
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 创建连接线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create connect thread failed.", __FILE__,__LINE__);
                }
            }
            
            m_nStatus = FAILD;
            
            if(m_pBuffer != NULL)
            {
                delete m_pBuffer;
                m_pBuffer = NULL;
            }
            
            if(m_preadBuf != NULL)
            {
                delete[] m_preadBuf;
                m_preadBuf = NULL;
            }
            
            if(m_puchRecvBuf != NULL)
            {
                delete[] m_puchRecvBuf;
                m_puchRecvBuf = NULL;
            }
        }
    }
    
    //新的云视通连接，判断从小助手切入还是从本地直接进行
    void CCChannel::DealNewYST(STCONNPROCP *pstConn)
    {
        if(m_stConnInfo.nWhoAmI == JVN_WHO_P || m_stConnInfo.nWhoAmI == JVN_WHO_M)
        {
            if(CheckNewHelp() > 0)//查询云世通号码对应的IP
                return;
            //客户端，需要优先从小助手连接
            
            if(m_pWorker->m_pHelpCtrl != NULL)
            {//提速模块已在工作
                //向小助手询问该号码的连接状态
                STVLINK stvlink;
                stvlink.nYSTNO = m_stConnInfo.nYSTNO;
                memcpy(&stvlink.chGroup, &m_stConnInfo.chGroup, 4);
                stvlink.nChannel = m_stConnInfo.nChannel;
                memcpy(&stvlink.chUserName, &m_stConnInfo.chPassName, strlen(m_stConnInfo.chPassName));
                memcpy(&stvlink.chPasswords, &m_stConnInfo.chPassWord, strlen(m_stConnInfo.chPassWord));
                int nret = m_pWorker->m_pHelpCtrl->SearchYSTNO(&stvlink);
                
                switch(nret)
                {
                    case JVN_HELPRET_LOCAL://有该号码信息 同内网(单机版时的外网直连) 客户端本地直连即可 地址
                    {
                        //                    OutputDebug("serchyst   local......\n");
                        memcpy(&m_stConnInfo.quickAddr, &stvlink.addrVirtual, sizeof(SOCKADDR_IN));
                        sprintf(m_stConnInfo.chServerIP, "%s", inet_ntoa(stvlink.addrVirtual.sin_addr));
                        m_stConnInfo.nServerPort = ntohs(stvlink.addrVirtual.sin_port);
                        
                        m_nStatus = NEW_VIRTUAL_IP;
                    }
                        break;
                    case JVN_HELPRET_WAN://有该号码信息 小助手外网直连 客户端从小助手连接及取数据即可
                    {
                        //OutputDebug("serchyst   wan......\n");
                        m_nStatus = NEW_HELP_CONN;
                    }
                        break;
                    case JVN_HELPRET_TURN://有该号码信息 转发 客户端本地直接转发即可
                    {
                        //OutputDebug("serchyst   turn......\n");
                        m_stConnInfo.nTURNType = JVN_ONLYTURN;
                        GetSerAndBegin(pstConn);
                    }
                        break;
                    case JVN_HELPRET_NULL://没有号码信息 客户端本地走基本流程
                    default://其它无效信息 客户端本地走基本流程
                    {
                        //OutputDebug("serchyst   null......\n");
                        GetSerAndBegin(pstConn);
                        
                    }
                        break;
                }
            }
            else
            {//提速模块没有工作，已没有提速可能，走本地原流程
                GetSerAndBegin(pstConn);//获取服务器地址，或是直接获取
                
            }
        }
        else if(m_stConnInfo.nWhoAmI == JVN_WHO_H)
        {//小助手，先检查是否有现成地址可用，有的话直接连接，没有的话走基本流程
            m_nStatus = NEW_VIRTUAL_IP;
            
        }
        else
        {//错误
            m_nStatus = FAILD;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "参数错误失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 参数错误失败", __FILE__,__LINE__);
            }
            else
            {
                char chMsg[] = "dataerr failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. dataerr", __FILE__,__LINE__);
            }
        }
    }
    
    //根据虚连接提供的ip，先进行直连 直连不通再转到基本流程
    void CCChannel::DealNewVirtualIP(STCONNPROCP *pstConn)
    {
        //连接ip地址 connectip
        m_bJVP2P = FALSE;
        m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
	 writeLog("DealNewVirtualIP ConnectIP...%d",__LINE__);
	 OutputDebug("DealNewVirtualIP ConnectIP...%d",__LINE__);
        if(ConnectIP())
        {//向对端发送有效性验证消息
            //检查对方是否支持tcp，支持的话改为tcp连接
            if((m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP) && m_stConnInfo.nWhoAmI != JVN_WHO_H && m_stConnInfo.nWhoAmI != JVN_WHO_M && m_nStatus != NEW_HAVEIP)
            {
                int nTCP = UDT::getpeertcp(m_ServerSocket);
                if(nTCP == 1)
                {//主控端支持tcp连接，关闭本地udp，进行tcp连接
                    m_nProtocolType = TYPE_PC_TCP;
                    writeLog("udp connect , to connect tcp.....");
                    OutputDebug("udp connect , to connect tcp.....");
                    m_nStatus = NEW_VTCP_IP;
                    m_dwStartTime = CCWorker::JVGetTime();
                    return;
                }
            }
            
            if(SendReCheck(FALSE))
            {//发送成功 等待验证结果
                m_nStatus = WAIT_IPRECHECK;
                m_dwStartTime = CCWorker::JVGetTime();
            }
            else
            {//发送失败 直接结束连接过程
                if(m_stConnInfo.nWhoAmI == JVN_WHO_P || m_stConnInfo.nWhoAmI == JVN_WHO_M)
                {//连接失败 如果是cv(小助手独立进程)或是手机(没小助手) 则继续本地连接
                    GetSerAndBegin(pstConn);
                }
                else
                {//连接失败 如果是小助手 则直接结束 这种情况应该极少见属于异常
                    m_nStatus = FAILD;
				 writeLog("new ip SendReCheck failed...%d",__LINE__);
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "连接失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 快速链接失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        char chMsg[] = "connect failed!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. quick connect failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                }
            }
        }
        else
        {//连接失败，直接结束连接过程
		if(m_nStatus == NEW_VIRTUAL_IP)
			{//如果使用的经验ip，应该按正常流程再进行而不是直接结束
	#ifdef MOBILE_CLIENT
            
/****************************************************************************/
                m_nReconnectTimes ++;
 //               m_nReconnectTimes =5;
                //从检索服务器上获取设备上线个数
                int nnnn = m_ISList.size();
                if(nnnn <= 0)
                {
                    if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 1, m_stConnInfo.nLocalPort))
                    {
                        if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 2, m_stConnInfo.nLocalPort))
                        {
                            
                        }else{
                             m_nStatus = WAIT_INDEXSER_REQ;
                            return;
                        }
                    }else{
                         m_nStatus = WAIT_INDEXSER_REQ;
                        return;
                    }
                }
/****************************************************************************/
//			GetSerAndBegin(pstConn);
//			m_nStatus = NEW_TURN;
			return;
	#endif

			}
            if(m_stConnInfo.nWhoAmI == JVN_WHO_P || m_stConnInfo.nWhoAmI == JVN_WHO_M)
            {//连接失败 如果是cv 则继续本地连接
                GetSerAndBegin(pstConn);
            }
            else
            {//连接失败 如果是小助手 则直接结束 这种情况应该极少见属于异常
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "连接失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 快速链接失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    char chMsg[] = "connect failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. quick connect failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
            }
        }
    }
    
    //通过小助手连接 连接不通再转到基本流程
    void CCChannel::DealNewHelpConn(STCONNPROCP *pstConn)
    {
        if(m_pWorker->m_pHelpCtrl != NULL)
        {
            if(m_pHelpConn != NULL)
            {
                delete m_pHelpConn;
                m_pHelpConn = NULL;
            }
            m_pHelpConn = new CCHelpConnCtrl();
            
            if(m_pHelpConn->ConnectYSTNO(m_stConnInfo))
            {//通过小助手连接正常进行
                m_dwStartTime = CCWorker::JVGetTime();
                m_nStatus = WAIT_HELP_RET;
            }
            else
            {//小助手连接失败，本地基本流程
                if(m_pHelpConn != NULL)
                {
                    delete m_pHelpConn;
                    m_pHelpConn = NULL;
                }
                GetSerAndBegin(pstConn);
            }
        }
        else
        {
            if(m_pHelpConn != NULL)
            {
                delete m_pHelpConn;
                m_pHelpConn = NULL;
            }
            GetSerAndBegin(pstConn);
        }
    }
    
    //等待小助手连接结果 失败的话分析结果 正常失败则退出 非正常失败则继续转到基本流程
    void CCChannel::DealWaitHelpRET(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 5000)//暂定小助手5秒内一定能连上 否则就不用小助手了
        {//超时等不明原因的失败 转到基本流程
            if(m_pHelpConn != NULL)
            {
                delete m_pHelpConn;
                m_pHelpConn = NULL;
            }
            
            GetSerAndBegin(pstConn);
            return;
        }
        
        if(m_pHelpConn != NULL)
        {
            int nSize = 0;
            STCONNBACK stconnback;
            int nret = m_pHelpConn->RecvConnResult(&stconnback);
            if(nret == 0)
            {//什么也没收到
                return;
            }
            else if(nret == 1)
            {//连接成功
                //			m_bJVP2P = stconnback.bJVP2P;//该信息助手没往回返 缺陷
                m_nStatus = HELP_OK;
                return;
            }
            else if(nret == 2)
            {//连接失败 正常失败(用户名密码错误，通道不存在，服务已停止等)
                //这里最好把密码错误等和连接超时区分开，便于密码错误时立即结束........
                if(m_pHelpConn != NULL)
                {
                    delete m_pHelpConn;
                    m_pHelpConn = NULL;
                }
                
                GetSerAndBegin(pstConn);
            }
            else
            {//错误
                if(m_pHelpConn != NULL)
                {
                    delete m_pHelpConn;
                    m_pHelpConn = NULL;
                }
                
                GetSerAndBegin(pstConn);
            }
        }
        else
        {
            if(m_pHelpConn != NULL)
            {
                delete m_pHelpConn;
                m_pHelpConn = NULL;
            }
            
            GetSerAndBegin(pstConn);
        }
    }
    
    //向服务器发送查询号码命令
    void CCChannel::DealWaitSerREQ(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        
        memset(pstConn->chdata, 0, 50);
        //发送：类型(4)+长度(4)+子类型(1)+编组号(4)+云视通号码(4)
        int nt = JVN_CMD_YSTCHECK;
        int nl = 9;
        memcpy(&pstConn->chdata[0], &nt, 4);
        memcpy(&pstConn->chdata[4], &nl, 4);
        pstConn->chdata[8] = 0;//查询请求
        memcpy(&pstConn->chdata[9], m_stConnInfo.chGroup, 4);
        memcpy(&pstConn->chdata[13], &m_stConnInfo.nYSTNO, 4);
        nl += 8;
        for(int i=0; i<ncount; i++)
        {
            sendtoclient(pstConn->udpsocktmp, (char*)pstConn->chdata, nl, 0, (sockaddr *)&m_SList[i].addr, sizeof(sockaddr_in), 1);
        }
        m_nStatus = WAIT_SER_RSP;//开始等待服务器响应
        m_dwStartTime = CCWorker::JVGetTime();
        pstConn->slisttmp.clear();
    }
    
    BOOL CCChannel::ServerIsExist(STCONNPROCP *pstConn,char* pServer)
    {
        int ncnew = pstConn->slisttmp.size();
        for(int j=0; j<ncnew; j++)
        {
            char chIP2[100] = {0};
            sprintf(chIP2, "%s", inet_ntoa(pstConn->slisttmp[j].addr.sin_addr));
            if(strcmp(pServer, chIP2) == 0)//已经返回结果
            {
                return TRUE;
            }
        }
        return FALSE;
    }
    
    //等待服务器结果 该过程最长允许2秒钟
    void CCChannel::DealWaitSerRSP(STCONNPROCP *pstConn)
    {
        if(m_nReconnectTimes < 3)
        {
            if(CheckNewHelp() > 0)
                return;
        }
        
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 2000 || (pstConn->slisttmp.size() == m_SList.size()) )//2000//2500
        {//等待超时或已接受全部应答
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
            
            //重新排序服务器
            char chIP1[30]={0};
            char chIP2[30]={0};
            int nc = m_SList.size();
            for(int i=0; i<nc; i++)
            {//将未返回的服务器放入最后
                sprintf(chIP1, "%s", inet_ntoa(m_SList[i].addr.sin_addr));
                BOOL bfind = FALSE;
                int ncnew = pstConn->slisttmp.size();
                for(int j=0; j<ncnew; j++)
                {
                    sprintf(chIP2, "%s", inet_ntoa(pstConn->slisttmp[j].addr.sin_addr));
                    if(strcmp(chIP1, chIP2) == 0)
                    {//该服务器已应答
                        bfind = TRUE;
                        break;
                    }
                }
                if(!bfind)
                {//该服务器没有应答，要么服务器有问题，要么服务器性能差
                    m_SList[i].buseful = TRUE;//FALSE;//注：由于目前服务器数目较少，没到分批上线的时候，所以也可以暂时每个服务器都尝试
                    pstConn->slisttmp.push_back(m_SList[i]);
                }
            }
            
            m_SList.clear();
            m_SList = pstConn->slisttmp;
			AddCSelfServer();
           m_SListTurn.clear();
            m_SListTurn = m_SList;
            m_nStatus = NEW_P2P;//已对服务器进行过筛选，开始正式连接
            
            if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
            {
                m_pWorker->WriteYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addressA, 0);
            }
            if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nTURNType == JVN_ONLYTURN)
            {
                m_nStatus = NEW_TURN;
                return;
            }
            DealMakeHole(pstConn);
            
            //取NAT
            //	    m_pWorker->GetWANIPList(m_SList,&m_NatList,1000);//更新本机外网地址
            //////////////////////////////////////////////////////////////////////////
            //char ch[100]={0};
            //for(i=0; i<m_SList.size(); i++)
            //{
            //	sprintf(ch,"%d**********IP:%s OnOff:%d \n",i,inet_ntoa(m_SList[i].addr.sin_addr), m_SList[i].buseful);
            //	OutputDebugString(ch);
            //}
            //////////////////////////////////////////////////////////////////////////
        }
        else
        {//接收数据
            //发送查询 接收服务器
            //接收数据
            memset(pstConn->chdata, 0, JVN_ASPACKDEFLEN);
            if(receivefrom(pstConn->udpsocktmp, (char *)pstConn->chdata, JVN_ASPACKDEFLEN, 0, &pstConn->sockAddr, &pstConn->nSockAddrLen, 1) > 0)
            {//接收：类型(4)+长度(4)+子类型(1)+编组号(4)+云视通号码(4)+主控协议版本号(4)+是否在线(1)
                int ntp = 0;
                memcpy(&ntp, &pstConn->chdata[0], 4);
                if(ntp == JVN_CMD_YSTCHECK)
                {//收到服务器成功数据
                    int nl = 0, nystno = 0, nv = 0; char chg[4]={0};
                    memcpy(&nl, &pstConn->chdata[4], 4);
                    memcpy(chg, &pstConn->chdata[9], 4);
                    memcpy(&nystno, &pstConn->chdata[13], 4);
                    memcpy(&nv, &pstConn->chdata[17], 4);
                    if(pstConn->chdata[8] == 1 && (nystno==m_stConnInfo.nYSTNO))// && (strcmp(chg, pWorker->m_stConnInfo.chGroup) == 0)
                    {//正确的回复
                        int nc = m_SList.size();
                        char chIP1[30]={0}, chIP2[30]={0};
                        sprintf(chIP1, "%s", inet_ntoa(((SOCKADDR_IN*)&pstConn->sockAddr)->sin_addr));
                        for(int i=0; i<nc; i++)
                        {
                            sprintf(chIP2, "%s", inet_ntoa(m_SList[i].addr.sin_addr));
                            if(strcmp(chIP1, chIP2)==0)
                            {//地址有效
                                if(!ServerIsExist(pstConn,chIP1))
                                {
                                    STSERVER stserver;
                                    memcpy(&stserver.addr, &pstConn->sockAddr, sizeof(SOCKADDR_IN));
                                    stserver.buseful = (pstConn->chdata[21] == 0)?FALSE:TRUE;//不在线的服务器不必使用
                                    stserver.nver = nv;
                                    pstConn->slisttmp.push_back(stserver);
                                    //////////////////////////////////////////////////////////////////////////
                                    //char ch[100]={0};
                                    //sprintf(ch,"SerRSP IP:%s ONOFF:%d\n",chIP1,stserver.buseful);
                                    //OutputDebugString(ch);
                                    //////////////////////////////////////////////////////////////////////////
                                    
                                    break;
                                }
                            }
                        }
                    }//不匹配不正确的回复
                }//收到服务器其他数据
            }//未接收到数据
        }
    }
    
    //去索引服务器查询号码在线服务器，发送请求包
    void CCChannel::DealWaitIndexSerREQ(STCONNPROCP *pstConn)
    {
        int ncount = m_ISList.size();
        writeLog("checksvrsize:%d",ncount);

        if(ncount == 0)//检索服务器没有下载到文件
        {
            m_SList.clear();
            if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 1, m_stConnInfo.nLocalPort))
            {
                if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 2, m_stConnInfo.nLocalPort))
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        //	char chMsg[] = "获取YST服务器信息失败!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST方式连接主控失败.原因:获取服务器失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        //	char chMsg[] = "get server address failed!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get server address failed.", __FILE__,__LINE__);
                    }
                    m_nStatus = WAIT_SER_REQ;
                    
                    return;
                }
            }
            
            if(m_SList.size() > 0)
            {
                m_SListTurn = m_SList;
                m_SListBak = m_SList;
                m_nStatus = WAIT_SER_REQ;
            }
            else
            {
                m_nStatus = WAIT_SER_REQ;
            }
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
            return;
        }
        
        //发送：类型(1)+数据区长度(4)+编组号(4)+云视通号码(4)
        memset(pstConn->chdata, 0, 50);
        pstConn->chdata[0] = JVN_REQ_QUERYYSTNUM;
        *(DWORD*)&(pstConn->chdata[1]) = htonl(8);
        memcpy(&(pstConn->chdata[5]), m_stConnInfo.chGroup, 4);
        *(DWORD*)&(pstConn->chdata[9]) = htonl(m_stConnInfo.nYSTNO);
        
        for(int i=0; i<ncount; i++)
        {
            sendtoclient(m_sQueryIndexSocket, (char*)pstConn->chdata, 13, 0, (sockaddr *)&m_ISList[i].addr, sizeof(sockaddr_in), 1);
            writeLog("send check onliesvradddr to [%s:%d]",inet_ntoa(m_ISList[i].addr.sin_addr),ntohs(m_ISList[i].addr.sin_port));

        }
        m_nStatus = WAIT_INDEXSER_RSP;//开始等待服务器响应
        m_dwStartTime = CCWorker::JVGetTime();
        pstConn->slisttmp.clear();
        m_SList.clear();
        
        OutputDebug("query index server...%d",ncount);

		memcpy(m_strRequestIndexData,pstConn->chdata,13);
		m_nRequestIndexTimes = 0;
		m_sRequestIndex = m_sQueryIndexSocket;
		m_dwSendIndexTime = m_dwStartTime;


}
//去索引服务器查询号码在线服务器，接收返回包
void CCChannel::DealWaitIndexSerRSP(STCONNPROCP *pstConn)
{
    if(m_nReconnectTimes < 3)
    {
        if(CheckNewHelp() > 0)
            return;
    }
        pstConn->dwendtime = CCWorker::JVGetTime();
        
	if(pstConn->dwendtime > m_dwStartTime + 3000)//未等到结果，等到一个结果就可以
        {
            //等待超时,进行服务器探测
            m_SList.clear();
            m_GroupSvrList.addrlist.clear();
            if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 1, m_stConnInfo.nLocalPort))
            {
                if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 2, m_stConnInfo.nLocalPort))
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        //	char chMsg[] = "获取YST服务器信息失败!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST方式连接主控失败.原因:获取服务器失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        //	char chMsg[] = "get server address failed!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get server address failed.", __FILE__,__LINE__);
                    }
                    m_nStatus = NEW_P2P;
                    if(JVN_ONLYTURN == m_stConnInfo.nTURNType || m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)//只转发
                    {
                        m_nStatus = NEW_TURN;
                    }
                    return;
                }
            }
            
            if(m_SList.size() > 0)
            {
				m_GroupSvrList.addrlist = m_SList;
#ifdef  MOBILE_CLIENT
				writeLog(" no rcv yst online svr num,and go REQ_DEB_PUBADDR ,listsize:%d!",m_SList.size());
                writeLog("...........................no rcv yst online svr num,and go REQ_DEB_PUBADDR ,listsize:%d!",m_SList.size());
				m_nStatus = REQ_DEV_PUBADDR;
#else
				m_SListTurn = m_SList;
				m_SListBak = m_SList;
				m_nStatus = WAIT_SER_REQ;
#endif			
            }
            else
            {
                m_nStatus = NEW_P2P;
                if(JVN_ONLYTURN == m_stConnInfo.nTURNType || m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)//只转发
                {
                    m_nStatus = NEW_TURN;
                    return;
                }
            }
        }
        else
        {//接收数据
            //发送查询 接收服务器
            //接收数据
            		OutputDebug(">>>>>>>>>>>");
            if(ReceiveIndexFromServer(pstConn,TRUE) >0)
            {
            }
			else
			{
				ReRequestIndexAddr();
			}
        }
    }
    
    void CCChannel::DealWaitNatSerREQ(STCONNPROCP *pstConn)
    {
        if(pstConn->udpsocktmp > 0)
        {
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
        }
        
        pstConn->udpsocktmp = socket(AF_INET, SOCK_DGRAM, 0);
        
        sockaddr_in sin = {0};
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        
        BOOL bReuse = TRUE;
        setsockopt(pstConn->udpsocktmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        
        bind(pstConn->udpsocktmp, (struct sockaddr*)&sin, sizeof(sin));
        
        char data[20] = {0};
        int nType = JVN_REQ_NATA_A;
        memcpy(&data[0],&nType,4);
        memcpy(&data[4],&m_stConnInfo.nYSTNO,4);
        STSERVER server = {0};
        int nServerNum = m_SList.size();
        for(int i = 0;i < nServerNum;i ++)
        {
            memcpy(&server,&m_SList[i],sizeof(STSERVER));
            CCChannel::sendtoclientm(pstConn->udpsocktmp,data,8,0,(sockaddr *)&server.addr,sizeof(server.addr),1);
            
            SOCKADDR_IN srv = {0};
            memcpy(&srv,&server.addr,sizeof(SOCKADDR_IN));
            
            //		OutputDebug("============== Query A type  %s : %d\n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
            
        }
        
        m_nStatus = WAIT_NAT_A;//等待主控类型的返回
        m_dwStartTime = CCWorker::JVGetTime();
        
    }
    
    void CCChannel::DealWaitNatSerRSP(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        
        if(pstConn->dwendtime > m_dwStartTime + 1500)//未等到结果，等到一个结果就可以
        {
            if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nTURNType == JVN_ONLYTURN)
            {
                m_nStatus = NEW_TURN;
                return;
            }
            m_nStatus = WAIT_MAKE_HOLE;//已对服务器进行过筛选，开始正式连接
            //此时可以打洞准备
            //		OutputDebug("Query time out");
            
            //运行到此处时一定是新的程序，加载了索引服务器的功能
            DealMakeHole(pstConn);
            return;
        }
        unsigned char chdata[JVN_ASPACKDEFLEN] = {0};
        SOCKADDR sockAddr = {0};
        int nSockAddrLen = sizeof(SOCKADDR);
        int nRecvSize = CCChannel::receivefromm(pstConn->udpsocktmp,(char* )chdata, JVN_ASPACKDEFLEN, 0, &sockAddr, &nSockAddrLen, 1);
        if(nRecvSize > 0)//5 or 11 = 4:type 1:NAT(4:ip 2:port)
        {
            int nType = 0;
            memcpy(&nType,chdata,4);
            if(JVN_REQ_NATA_A == nType)
            {
                m_nNatTypeA = chdata[4];
                
                OutputDebug("Get a type %d",m_nNatTypeA);
                if(m_nNatTypeA == NAT_TYPE_0_PUBLIC_IP || m_nNatTypeA == NAT_TYPE_1_FULL_CONE)//公网和全透明的NAT直接走udp
                {
                    //取得地址，直接
                    sprintf(m_stConnInfo.chServerIP, "%d.%d.%d.%d",chdata[5],chdata[6],chdata[7],chdata[8]);
                    memcpy(&m_stConnInfo.nServerPort,&chdata[9],2);
                    
//                    m_nStatus = NEW_HAVEIP;
                    m_nStatus = NEW_VIRTUAL_IP;
                    m_pWorker->m_Helper.AddOkYST(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,
                                                 m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                    return;
                }
                else
                {
                    if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nTURNType == JVN_ONLYTURN)
                    {
                        m_nStatus = NEW_TURN;
                        return;
                    }
                    //此时可以打洞准备
                    //				OutputDebug("查询检索服务器 结果...%d",nServerNum);
                    
                    m_nStatus = WAIT_MAKE_HOLE;//已对服务器进行过筛选，开始正式连接
                    //运行到此处时一定是新的程序，加载了索引服务器的功能
                    DealMakeHole(pstConn);
                    
                }
                
                return;
            }
        }
    }
    
    void CCChannel::DealMakeHole(STCONNPROCP *pstConn)
    {
        if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nTURNType == JVN_ONLYTURN)
        {
            m_nStatus = NEW_TURN;
            return;
        }
        if(!m_pWorker->m_MakeHoleGroup.AddConnect(this,m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,
                                                  m_stConnInfo.nChannel,m_stConnInfo.bLocalTry,m_SList,m_nNatTypeA,m_pWorker->m_nLocalPortWan,FALSE))
        {
            m_nStatus = NEW_P2P;
        }
        else
        {
            m_nStatus = WAIT_MAKE_HOLE;
        }
        //	m_nStatus = NEW_TURN;
    }
    
    //等待进行TCP直连
    void CCChannel::DealNewTCP(STCONNPROCP *pstConn)
    {
        int nOldProtocolType = m_nProtocolType;
        
        if(m_nProtocolType == TYPE_PC_UDP)
            m_nProtocolType = TYPE_PC_TCP;//强制改为tcp类型，意味着此次尝试失败则直接退出
        if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
            m_nProtocolType = TYPE_PC_TCP;//强制改为tcp类型，意味着此次尝试失败则直接退出
        
        if(!ConnectIPTCP())
        {
            //		OutputDebug("TCP fail");
            if(m_nStatus == NEW_HAVETCP || m_nStatus == NEW_HAVE_LOCAL)
            {//如果使用的经验ip，应该按正常流程再进行而不是直接结束
                m_nProtocolType = nOldProtocolType;
                
                if(m_nProtocolType == TYPE_PC_TCP)
                    m_nProtocolType = TYPE_PC_UDP;
                if(m_nProtocolType == TYPE_MO_TCP)
                    m_nProtocolType = TYPE_MO_UDP;
                
                //			OutputDebug("try UDP");
                m_nStatus = NEW_HAVEIP;//tcp不成功 试下udp
                
            }
            return;
        }
        /////////////////////////////////////////////////////////////////////////
        //先等待是否无通道服务
        int n = 0;
        char datatmp[100] = {0};
        DWORD dwTime = CCWorker::JVGetTime();
        DWORD dw = CCWorker::JVGetTime();
        while(dw - dwTime < 50)
        {
            dw = CCWorker::JVGetTime();
            n = tcpreceive2(m_ServerSocket,(char* )datatmp,100,10);
            if(n > 0)
            {
                break;
            }
        }
        if(JVN_RSP_NOSERVER == datatmp[0])
        {
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "无该通道服务!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "channel is not open!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 无通道服务", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. no channel service.", __FILE__,__LINE__);
            }
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        else if(JVN_RSP_OVERLIMIT == datatmp[0])
        {//超过主控最大连接限制
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "超过主控最大连接限制!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "client count limit!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 超过主控最大连接限制!", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. client count limit!", __FILE__,__LINE__);
            }
            
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        ///////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        if(SendReCheck(FALSE))
        {//发送成功 等待验证结果
            m_nStatus = WAIT_TCPIPRECHECK;
            m_dwStartTime = CCWorker::JVGetTime();
        }
        else
        {//发送失败 直接结束连接过程
            m_nStatus = FAILD;
		writeLog("SendReCheck failed...%d",__LINE__);
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "预验证失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 向主控发送预验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                char chMsg[] = "ReCheck failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. send repass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return;
        }
    }
    //等待进行IP直连
    void CCChannel::DealNewIP(STCONNPROCP *pstConn)
    {
        m_bJVP2P = FALSE;
        m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
	 writeLog("DealNewIP ConnectIP...%d",__LINE__);
		 OutputDebug("DealNewIP ConnectIP...%d",__LINE__);
        if(ConnectIP())
        {//向对端发送有效性验证消息
            //检查对方是否支持tcp，支持的话改为tcp连接
            if(m_nProtocolType != TYPE_3GMO_UDP && m_nProtocolType != TYPE_3GMOHOME_UDP)
            {
                if((m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP) && m_stConnInfo.nWhoAmI != JVN_WHO_H && m_stConnInfo.nWhoAmI != JVN_WHO_M && m_nStatus != NEW_HAVEIP)
                {//pc的udp连接 不是小助手 此时考虑使用tcp
                    int nTCP = UDT::getpeertcp(m_ServerSocket);
                    if(nTCP == 1)
                    {//主控端支持tcp连接，关闭本地udp，进行tcp连接
                        m_nStatus = NEW_TCP_IP;
                        writeLog("udp connect , to connect tcp.....");
                        OutputDebug("udp connect , to connect tcp.....");
                        m_dwStartTime = CCWorker::JVGetTime();
                        return;
                    }
                }
                
            }
            if(SendReCheck(FALSE))
            {//发送成功 等待验证结果
                m_nStatus = WAIT_IPRECHECK;
                m_dwStartTime = CCWorker::JVGetTime();
            }
            else
            {//发送失败 直接结束连接过程
                m_nStatus = FAILD;
			    writeLog("virtural SendReCheck failed...%d",__LINE__);
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "预验证失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 向主控发送预验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    char chMsg[] = "ReCheck failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. send repass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
            }
        }
        else
        {//连接失败，直接结束连接过程
            OutputDebug("连接失败，直接结束连接过程， m_nStatus： %d, line: %d",m_nStatus,__LINE__);
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "连接失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
                else
                {
                    char chMsg[] = "Connect failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    printf("connect failed  \n");
                }
            }
        
    }
    
    //等待IP连接有效性验证
    void CCChannel::DealWaitIPRECheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 3000)//3000)//6000)//2000
        {//等待超时，连接失败
            writeLog("recheck time out...%d",__LINE__);
            OutputDebug("recheck time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            //		OutputDebug("%d Recheck time out.",m_stConnInfo.nLocalChannel);
            if(!ConnectIP())
            {
                m_nStatus = FAILD;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "预验证失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 等待预验证数据失败", __FILE__,__LINE__);
                }
                else
                {
                    char chMsg[] = "ReCheck Failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. wait repass info failed", __FILE__,__LINE__);
                }
                return;
            }
            //////////////////////////////////////////////////////////////////////////
            //这一段，可以在新主控响应预验证不及时的情况下也能进行新协议的手机连接
            if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
            {
                int nTCP = UDT::getpeertcp(m_ServerSocket);
                if(nTCP == 1)
                {//主控端支持tcp连接，是比较新的主控程序
                    m_nStatus = WAIT_IP_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                }
                else
                {
                    int nver = UDT::getystverF(m_ServerSocket);//获取远端协议版本
                    if(nver >= JVN_YSTVER2)
                    {
                        m_nStatus = WAIT_IP_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    }
                    else
                    {
                        m_nStatus = WAIT_IP_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this);
                    }
                }
            }
            else
            {
                m_nStatus = WAIT_IP_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
            }
            //////////////////////////////////////////////////////////////////////////
            /*
             m_nStatus = WAIT_IP_CONNECTOLD;
             m_pOldChannel = new CCOldChannel(m_pWorker, this);
             */		if(m_pOldChannel == NULL)
             {
                 m_nStatus = FAILD;
                 
                 if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                 {
                     char chMsg[] = "预验证失败!";
                     m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                     m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 等待预验证数据失败", __FILE__,__LINE__);
                 }
                 else
                 {
                     char chMsg[] = "ReCheck Failed!";
                     m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                     m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. wait repass info failed", __FILE__,__LINE__);
                 }
             }
        }
        else
        {//接收数据
            int nver = 0;
            int nret = RecvReCheck(nver, pstConn->chAVersion);
            if(nret == 1)
            {//验证通过 对方是统一版SDK
                if(!m_bJVP2P || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
                {//不是多播方式，走原流程  手机连接全部走原流程身份验证协议按旧协议
                    m_nStatus = WAIT_IP_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    return;
                }
                
                if(SendPWCheck())
                {//发送身份验证消息成功 等待结果
                    m_nStatus = WAIT_IPPWCHECK;
                    m_dwStartTime = CCWorker::JVGetTime();
                }
                else
                {//发送身份验证消息失败 直接结束连接
                    m_nStatus = FAILD;
                    
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "身份验证失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 发送身份验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        char chMsg[] = "PassWord is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. send pass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                }
            }
            else if(nret == 0)
            {//预验证未通过 直接结束连接
                m_nStatus = FAILD;
                
                if(nver > 0)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "本地版本太低!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 本地版本太低,对方版本:", __FILE__,__LINE__,pstConn->chAVersion);
                    }
                    else
                    {
                        char chMsg[] = "Local Version too old!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. Local Version too old,Des:", __FILE__,__LINE__,pstConn->chAVersion);
                    }
                }
                else if(nver < 0)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "对方版本太低!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 对方版本太低,对方版本:", __FILE__,__LINE__,pstConn->chAVersion);
                    }
                    else
                    {
                        char chMsg[] = "Destination Version too old!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. Destination Version too old,Des:", __FILE__,__LINE__,pstConn->chAVersion);
                    }
                }
                else
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "预验证失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 预验证失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        char chMsg[] = "ReCheck Failed!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. repass failed.", __FILE__,__LINE__);
                    }
                }
            }
            else if(nret == 2)
            {//预验证未通过，对方是旧版主控
                m_nStatus = WAIT_IP_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
                if(m_pOldChannel == NULL)
                {
                    m_nStatus = FAILD;
                    
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "预验证失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 对方是旧主控", __FILE__,__LINE__);
                    }
                    else
                    {
                        char chMsg[] = "ReCheck Failed!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. old jdvr", __FILE__,__LINE__);
                    }
                }
            }
        }
    }
    
    //旧版尝试失败，调整主连接流程状态
    void CCChannel::DealWaitIPConnectOldF(STCONNPROCP *pstConn)
    {
        //	m_nProtocolType = m_stConnInfo.nConnectType;//这里直接结束了 不必再转状态
        m_nStatus = FAILD;
        
        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
        {
            char chMsg[] = "预验证失败!";
            if(!m_bShowInfo)
            {
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 等待预验证数据失败", __FILE__,__LINE__);
        }
        else
        {
            char chMsg[] = "ReCheck Failed!";
            if(!m_bShowInfo)
            {
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. wait repass info failed", __FILE__,__LINE__);
        }
    }
    
    //等待身份验证
    void CCChannel::DealWaitIPPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 3000)//5000)//2000
        {//等待超时，连接失败
            m_nStatus = FAILD;
            writeLog("pwd time out...%d",__LINE__);
            OutputDebug("pwd time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "身份验证超时!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 等待身份验证数据失败", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. wait pass info failed.", __FILE__,__LINE__);
            }
        }
        else
        {//接收数据
            int nPWData=0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//验证通过 连接成功建立
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrAL, sizeof(SOCKADDR_IN));
                memcpy(&m_addrConnectOK, &m_addrAL, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//身份验证未通过 直接结束连接
                m_nStatus = FAILD;
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                m_bPassWordErr = TRUE;
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "身份未通过验证!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 身份验证失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. pass failed.", __FILE__,__LINE__);
                }
            }
        }
    }
    
    //连接成功
    void CCChannel::DealHelpOK(STCONNPROCP *pstConn)
    {
        m_unBeginChunkID = 0;
        m_nChunkCount = 0;
        
        if(m_pBuffer != NULL)
        {
            delete m_pBuffer;
            m_pBuffer = NULL;
        }
        
        //	if(m_bJVP2P)//该标志不起作用
        {
            m_pchPartnerInfo = new char[JVNC_PTINFO_LEN];
        }
        
        if(!StartHelpWorkThread())
        {
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "开启工作线程失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            else
            {
                char chMsg[] = "Start work thread failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            
            m_nStatus = FAILD;
        }
        else
        {
            if(m_stConnInfo.nShow == JVN_CONNTYPE_TURN)
            {
                m_bTURN = TRUE;
                char chMsg[] = "(TURN)";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
			{
//				m_pWorker->WriteYSTNOInfo(m_stConnInfo.nYSTNO, m_SList, m_addressA, 1,FALSE);
				OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",m_nLocalChannel,m_stConnInfo.nYSTNO,__LINE__);
				m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
			}
		}
            else
            {
                char chMsg[] = "(HP2P)";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
                {
                    //				m_pWorker->WriteYSTNOInfo(m_stConnInfo.nYSTNO, m_SList, m_addressA, 1,FALSE);
				OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",m_nLocalChannel,m_stConnInfo.nYSTNO,__LINE__);
                    m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
                }
            }
            
            m_bPass = TRUE;//
        }
        
#ifdef WIN32
        //连接线程结束
        if(m_hEndEventC > 0)
        {
            CloseHandle(m_hEndEventC);
        }
        m_hEndEventC = 0;
#endif
    }
    
    //连接成功
    void CCChannel::DealOK(STCONNPROCP *pstConn)
    {
        //	pWorker->m_bPass = TRUE;
        m_unBeginChunkID = 0;
        m_nChunkCount = 0;
        
        if(m_pBuffer != NULL)
        {
            delete m_pBuffer;
            m_pBuffer = NULL;
        }
        
        m_nFYSTVER = UDT::getystverF(m_ServerSocket);//获取远端协议版本
        
        if(m_bJVP2P)
        {
            m_pBuffer = new CCMultiBufferCtrl((m_stConnInfo.nShow == JVN_CONNTYPE_TURN)?TRUE:FALSE);
            m_pBuffer->m_bLan2A = m_bLan2A;
        }
        else
        {//非多播方式全部走原流程 此处暂时走不到
            if(m_nFYSTVER >= JVN_YSTVER1)
            {//使用视频帧新协议，更好的播放控制
                m_pBuffer = new CCSingleBufferCtrl(m_nLocalChannel,(m_stConnInfo.nShow == JVN_CONNTYPE_TURN)?TRUE:FALSE);
            }
            else
            {
                m_pBuffer = new CCOldBufferCtrl(m_nLocalChannel,(m_stConnInfo.nShow == JVN_CONNTYPE_TURN)?TRUE:FALSE);
            }
        }
        
        m_pBuffer->m_pLog = &m_pWorker->m_Log;
        m_pBuffer->m_nChannel = m_nLocalChannel;
        if(!StartWorkThread())
        {
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "开启工作线程失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            else
            {
                char chMsg[] = "Start work thread failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            
            m_nStatus = FAILD;
        }
        else
        {
            if(m_stConnInfo.nShow == JVN_CONNTYPE_TURN)
            {
                m_bTURN = TRUE;
                char chMsg[] = "(TURN)";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                //OutputDebugString("channel connectok(turn)\n");
			if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)//删除正在连接到号码
			{
				OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",m_nLocalChannel,m_stConnInfo.nYSTNO,__LINE__);
				m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
			}
		}
            else
            {
                char chMsg[] = "(P2P)";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                //OutputDebugString("channel connectok(p2p)\n");
                if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
                {
                    m_pWorker->WriteYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addressA, 1,m_stConnInfo.sRandSocket);
				OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",m_nLocalChannel,m_stConnInfo.nYSTNO,__LINE__);
                    m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
                }
            }
            
            if(!m_bJVP2P)
            {//单播，沿用原处理流程
                //pWorker->m_bPass = TRUE;
                
                if(m_stConnInfo.isBeRequestVedio==TRUE)
                {
                    if(!SendData(JVN_CMD_VIDEO, NULL, 0))
                    {
                        if(!SendData(JVN_CMD_VIDEO, NULL, 0))
                        {
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "请求预览失败!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "首次请求录像失败.", __FILE__,__LINE__);
                            }
                            else
                            {
                                char chMsg[] = "Request video failed!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel,"first REQ video failed.",__FILE__,__LINE__);
                            }
                            m_nStatus = FAILD;
                            return;
                        }
                    }
                }
                m_bPass = TRUE;//
            }
            else
            {//多播方式采用新流程
                SOCKADDR_IN addrLocal;//本地地址
                int namelen = sizeof(SOCKADDR_IN);
                char chIP[16]={0};
                int nPort = 0;
                
                if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                {
#ifndef WIN32
                    getsockname(m_pWorker->m_WorkerTCPSocket, (sockaddr *)&addrLocal, (socklen_t *)&namelen);//m_ServerSocket
#else
                    getsockname(m_pWorker->m_WorkerTCPSocket, (sockaddr *)&addrLocal, &namelen);//m_ServerSocket
#endif
                    
                    //	addrLocal.sin_port = htons(addrLocal.sin_port);//(m_nLocalStartPort);
                }
                else
                {
                    UDT::getsockname(m_ServerSocket, (SOCKADDR*)&addrLocal, &namelen);
                }
                //向主控更新本地地址
                
#ifndef WIN32
                addrLocal.sin_addr.s_addr = inet_addr(pstConn->szIPAddress);
#else
                addrLocal.sin_addr.S_un.S_addr = inet_addr(pstConn->szIPAddress);
#endif
                
                sprintf(chIP,"%s",inet_ntoa(addrLocal.sin_addr));
                nPort = ntohs(addrLocal.sin_port);
                
                memcpy(&m_puchRecvBuf[0], chIP, 16);
                memcpy(&m_puchRecvBuf[16], &nPort, 4);
                SendData(JVN_CMD_LADDR, m_puchRecvBuf, 20);
                
                //向主控请求伙伴列表
                if(!SendData(JVN_CMD_PLIST, NULL, 0))
                {
                    if(!SendData(JVN_CMD_PLIST, NULL, 0))
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            char chMsg[] = "请求伙伴列表失败!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "请求伙伴列表失败.", __FILE__,__LINE__);
                        }
                        else
                        {
                            char chMsg[] = "Request partner list failed!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel,"REQ partnerlist failed.",__FILE__,__LINE__);
                        }
                        m_nStatus = FAILD;
                        return;
                    }
                }
                
                m_bPass = TRUE;
                
                SendData(JVN_CMD_VIDEO, NULL, 0);//?
            }
        }
        
#ifdef WIN32
        //连接线程结束
        if(m_hEndEventC > 0)
        {
            CloseHandle(m_hEndEventC);
        }
        m_hEndEventC = 0;
#endif
    }
    
    //连接失败
    void CCChannel::DealFAILD(STCONNPROCP *pstConn)
    {
        if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
        {
            m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
        }
        
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            
            //结束线程
            if(m_ServerSocket > 0)
            {
                closesocket(m_ServerSocket);
            }
            m_ServerSocket = 0;
        }
        else
        {
            //结束线程
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
        }
        
        if(m_ListenSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ListenSocket);
        }
        m_ListenSocket = 0;
        
        if(m_ListenSocketTCP > 0)
        {
            closesocket(m_ListenSocketTCP);
        }
        m_ListenSocketTCP = 0;
        m_PartnerCtrl.ClearPartner();
        
#ifdef WIN32
        if(m_hEndEventC > 0)
        {
            CloseHandle(m_hEndEventC);
            m_hEndEventC = 0;
        }
#endif
    }
    
    //开始进行云视通号码方式连接
    void CCChannel::DealNEWP2P(STCONNPROCP *pstConn)
    {
        m_bJVP2P = FALSE;
        
        m_stConnInfo.nShow = JVN_CONNTYPE_P2P;
        
        BOOL bfind = FALSE;
        if(JVN_ONLYTURN != m_stConnInfo.nTURNType)
        {//只进行转发链接时，不需要进行直连相关处理
            int ncount = m_SList.size();
            if(m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)
            {//如果是3G手机连接，p2p尝试最多允许2次
                ncount = min(ncount, 2);
            }
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {//开始尝试请求主控地址
                    if(SendSP2P(m_SList[i].addr, i, pstConn->chError))
                    {//向服务器发送请求成功
                        m_nStatus = WAIT_S;
                        memset(&m_addrAN,0,sizeof(m_addrAN));
                        
                        m_dwStartTime = CCWorker::JVGetTime();
                        bfind = TRUE;
                        break;
                    }
                    else
                    {//向服务器发送请求失败 尝试下一个服务器
                        m_SList[i].buseful = FALSE;
                    }
                }
            }
        }
        if(!bfind)
        {//已没有可以尝试直连的服务器
            //判断是否需要TURN连接
            if(m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)//是否是手机直接转发
            {
                m_nStatus = NEW_TURN;
                return;
            }
            if(JVN_NOTURN == m_stConnInfo.nTURNType || (m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP))
            {//禁用转发;tcp连接方式由于直连成功率低，暂不允许转发，否则会增加转发服务器负担
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    if(!m_bShowInfo)
                    {
                        if(m_bPassWordErr)
                        {
                            char chMsg[] = "身份未通过验证!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                        else
                        {
                            char chMsg[] = "连接超时!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                    }
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(P2P)方式连接主控失败.原因分别为:", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    if(!m_bShowInfo)
                    {
                        if(m_bPassWordErr)
                        {
                            char chMsg[] = "password is wrong!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                        else
                        {
                            char chMsg[] = "connect timeout!";
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                    }
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(P2P) connect failed.Infos:", __FILE__,__LINE__,pstConn->chError);
                }
            }
            else
            {//开始尝试TURN连接
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(P2P)方式连接主控失败,尝试转发.原因分别为:", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(P2P) connect failed,try TURN type.Infos:", __FILE__,__LINE__,pstConn->chError);
                }
                
                memset(pstConn->chError, 0, 20480);
                
                m_SList.clear();
				AddCSelfServer();
                m_SList = m_SListTurn;
                
                //清理连接残余，重新开始
                if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                {//TCP连接 此处暂时是走不到的.......
                    //结束线程
                    if(m_ServerSocket > 0)
                    {
                        closesocket(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                }
                else
                {
                    //结束线程
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                }
                
                if(m_SocketSTmp > 0)
                {
                    shutdown(m_SocketSTmp, SD_BOTH);
                    closesocket(m_SocketSTmp);
                }
                m_SocketSTmp = 0;
                m_nStatus = NEW_TURN;
            }
        }
    }
    
    //等待服务器响应P2P连接请求
    void CCChannel::DealWaitS(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 2500)//5000
        {//等待超时，连接失败
            m_nStatus = NEW_P2P;
            
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    m_SList[i].buseful = FALSE;
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH] = {0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]等待服务器数据超时>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]wait info from S timeout>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    break;
                }
            }
        }
        else
        {//接收数据
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    int nret = RecvS(m_SList[i].addr, i, pstConn->chError);
                    switch(nret)
                    {//收到服务器回复
                        case JVN_RSP_CONNA://取得地址 开始连接
                        {
                            //先进行几次预打洞 需要2s的时间
                            PrePunch(m_pWorker->m_WorkerUDPSocket, m_addrAN);//先向对方打洞，给对方包进来创造前提
                            //	Punch(m_stConnInfo.nYSTNO, m_pWorker->m_WorkerUDPSocket,&m_addrAN);
                            
                            m_dwStartTime = CCWorker::JVGetTime();
                            if(m_stConnInfo.bLocalTry)
                            {
                                char chIP1[30]={0};
                                char chIP2[30]={0};
                                sprintf(chIP1, "%s:%d", inet_ntoa(m_addrLastLTryAL.sin_addr),ntohs(m_addrLastLTryAL.sin_port));
                                sprintf(chIP2, "%s:%d", inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
                                
                                if(strcmp(chIP1, chIP2))
                                {//不一致，该地址没尝试过
                                    m_nStatus = NEW_P2P_L;//进行内网探测
                                }
                                else
                                {
                                    //打洞
                                    //////////////////////////////////////////////////////////////////////////
                                    UDTSOCKET udts = UDT::socket(AF_INET, SOCK_STREAM, 0);
                                    BOOL bReuse = TRUE;
                                    UDT::setsockopt(udts, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                                    //////////////////////////////////////////////////////////////////////////
                                    int len1 = JVC_MSS;
                                    UDT::setsockopt(udts, 0, UDT_MSS, &len1, sizeof(int));
                                    //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                                    len1=1500*1024;
                                    UDT::setsockopt(udts, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                                    
                                    len1=1000*1024;
                                    UDT::setsockopt(udts, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                                    
                                    if(0!=UDT::bind(udts, m_pWorker->m_WorkerUDPSocket)){
                                    OutputDebug("************************************Channel  line: %d",__LINE__);
                                    }
                                    
                                    //将套接字置为非阻塞模式
                                    BOOL block = FALSE;
                                    UDT::setsockopt(udts, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                                    UDT::setsockopt(udts, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                                    LINGER linger;
                                    linger.l_onoff = 0;
                                    linger.l_linger = 0;
                                    UDT::setsockopt(udts, 0, UDT_LINGER, &linger, sizeof(LINGER));
                                    
                                    //								UDT::ystpunch(udts, &m_addrAN, m_stConnInfo.nYSTNO, 500);
                                    m_pWorker->pushtmpsock(udts);
                                    //////////////////////////////////////////////////////////////////////////
                                    
                                    m_nStatus = NEW_P2P_N;//不需要重复对同一地址进行内网探测
                                }
                            }
                            else
                            {
                                //打洞
                                //////////////////////////////////////////////////////////////////////////
                                UDTSOCKET udts = UDT::socket(AF_INET, SOCK_STREAM, 0);
                                BOOL bReuse = TRUE;
                                UDT::setsockopt(udts, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                                //////////////////////////////////////////////////////////////////////////
								int len1 = JVC_MSS;
                                UDT::setsockopt(udts, 0, UDT_MSS, &len1, sizeof(int));
                                //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                                len1=1500*1024;
                                UDT::setsockopt(udts, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                                
                                len1=1000*1024;
                                UDT::setsockopt(udts, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                                if(0!=UDT::bind(udts, m_pWorker->m_WorkerUDPSocket)){
                                    OutputDebug("************************************Channel  line: %d",__LINE__);
                                }
                                
                                //将套接字置为非阻塞模式
                                BOOL block = FALSE;
                                UDT::setsockopt(udts, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                                UDT::setsockopt(udts, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                                LINGER linger;
                                linger.l_onoff = 0;
                                linger.l_linger = 0;
                                UDT::setsockopt(udts, 0, UDT_LINGER, &linger, sizeof(LINGER));
                                
                                //							UDT::ystpunch(udts, &m_addrAN, m_stConnInfo.nYSTNO, 500);
                                m_pWorker->pushtmpsock(udts);
                                //////////////////////////////////////////////////////////////////////////
                                
                                m_nStatus = NEW_P2P_N;//进行外网连接
                            }
                        }
                            break;
                        case JVN_RSP_CONNAF://取地址失败 尝试其他服务器
                            m_SList[i].buseful = FALSE;
                            m_SListTurn[i].buseful = FALSE;//..未上线的服务器也不需再尝试转发
                            m_nStatus = NEW_P2P;
                            m_dwStartTime = CCWorker::JVGetTime();
                            
                            break;
                        case JVN_CMD_TRYTOUCH://收到主控打洞包 为防止外网连接失败直接连接(可能会漏掉少数内网连接的机会)
                            m_nStatus = NEW_P2P_N;
                            m_dwStartTime = CCWorker::JVGetTime();
                            break;
                        default://未收到信息或收到其他错误信息 继续接收
                            break;
                    }
                    
                    break;//找到对应服务器，跳出此次接收
                }
            }
        }
    }
    
    //连接中途如果发现对方是支持tcp的，则改为tcp通过此函数去连接
    void CCChannel::DealNEWTCPIP(STCONNPROCP *pstConn)
    {
        SendData(JVN_CMD_DISCONN, NULL, 0);
        
        CCWorker::jvc_sleep(1);//可尽量让对方收到断开消息
        SendData(JVN_CMD_DISCONN, NULL, 0);
        
        m_nProtocolType = TYPE_PC_TCP;//强制改为pctcp类型，意味着此次尝试失败则直接退出
        
        OutputDebug("%d----------%d",m_nLocalChannel,__LINE__);
        if(!ConnectIPTCP())
        {
            m_nStatus = FAILD;
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "tcp连接失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. tcp尝试失败", __FILE__,__LINE__);
            }
            else
            {
                char chMsg[] = "TCPLink Failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. TCPLink failed", __FILE__,__LINE__);
            }
            return;
        }
        /////////////////////////////////////////////////////////////////////////
        //先等待是否无通道服务
        int n = 0;
        char datatmp[100] = {0};
        DWORD dwTime = CCWorker::JVGetTime();
        DWORD dw = CCWorker::JVGetTime();
        while(dw - dwTime < 50)
        {
            dw = CCWorker::JVGetTime();
            n = tcpreceive2(m_ServerSocket,(char* )datatmp,100,10);
            if(n > 0)
            {
                break;
            }
        }
        if(JVN_RSP_NOSERVER == datatmp[0])
        {
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "无该通道服务!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "channel is not open!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 无通道服务", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. no channel service.", __FILE__,__LINE__);
            }
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        else if(JVN_RSP_OVERLIMIT == datatmp[0])
        {//超过主控最大连接限制
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "超过主控最大连接限制!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "client count limit!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 超过主控最大连接限制!", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. client count limit!", __FILE__,__LINE__);
            }
            
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        ///////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        if(SendReCheck(FALSE))
        {//发送成功 等待验证结果
            m_nStatus = WAIT_TCPIPRECHECK;
            m_dwStartTime = CCWorker::JVGetTime();
        }
        else
        {//发送失败 直接结束连接过程
            m_nStatus = FAILD;
            writeLog("SendReCheck failed...%d",__LINE__);
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "预验证失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 向主控发送预验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                char chMsg[] = "ReCheck failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. send repass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return;
        }
        //////////////////////////////////////////////////////////////////////////
        
        /*
         m_nStatus = WAIT_IP_CONNECTOLD;
         m_pOldChannel = new CCOldChannel(m_pWorker, this);
         if(m_pOldChannel == NULL)
         {
         m_nStatus = FAILD;
         
         if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
         {
         char chMsg[] = "TCPLink失败!";
         m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. TCPLink失败", __FILE__,__LINE__);
         }
         else
         {
         char chMsg[] = "TCPLink Failed!";
         m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. TCPLink failed", __FILE__,__LINE__);
         }
         }
         */
    }
    void CCChannel::DealNEWVTCPIP(STCONNPROCP *pstConn)
    {
        SendData(JVN_CMD_DISCONN, NULL, 0);
        OutputDebug("%d----------%d",m_nLocalChannel,__LINE__);
        if(!ConnectIPTCP())
        {
            if(m_stConnInfo.nWhoAmI == JVN_WHO_P || m_stConnInfo.nWhoAmI == JVN_WHO_M)
            {//连接失败 如果是cv 则继续本地连接
                GetSerAndBegin(pstConn);
            }
            else
            {//连接失败 如果是小助手 则直接结束 这种情况应该极少见属于异常
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "tcp连接失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 快速链接失败 tcp", __FILE__,__LINE__);
                }
                else
                {
                    char chMsg[] = "connect failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. quick connect failed. TCPLink.", __FILE__,__LINE__);
                }
            }
            
            return;
        }
        ///////////////////////////////////////////////////
        //先等待是否无通道服务
        int n = 0;
        char datatmp[100] = {0};
        DWORD dwTime = CCWorker::JVGetTime();
        DWORD dw = CCWorker::JVGetTime();
        while(dw - dwTime < 50)
        {
            dw = CCWorker::JVGetTime();
            n = tcpreceive2(m_ServerSocket,(char* )datatmp,100,10);
            if(n > 0)
            {
                break;
            }
        }
        if(JVN_RSP_NOSERVER == datatmp[0])
        {
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "无该通道服务!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "channel is not open!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 无通道服务", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. no channel service.", __FILE__,__LINE__);
            }
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        else if(JVN_RSP_OVERLIMIT == datatmp[0])
        {//超过主控最大连接限制
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "超过主控最大连接限制!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "client count limit!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 超过主控最大连接限制!", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. client count limit!", __FILE__,__LINE__);
            }
            
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        ////////////////////////////////////////////////////////////////////////
        m_nStatus = WAIT_IP_CONNECTOLD;
        m_pOldChannel = new CCOldChannel(m_pWorker, this);
        if(m_pOldChannel == NULL)
        {
            m_nStatus = FAILD;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "TCPLink失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. TCPLink失败", __FILE__,__LINE__);
            }
            else
            {
                char chMsg[] = "TCPLink Failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. TCPLink failed", __FILE__,__LINE__);
            }
        }
    }
    
    //内网探测过程中发现对方支持tcp，则改为tcp通过该函数去连
    void CCChannel::DealNEWTCPP2PL(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;//找到当前服务器，后续有用
            }
        }
        
        SendData(JVN_CMD_DISCONN, NULL, 0);
        
        CCWorker::jvc_sleep(1);//可尽量让对方收到断开消息
        SendData(JVN_CMD_DISCONN, NULL, 0);
        
        int ntypetmp = m_nProtocolType;//这里暂存一下，便于尝试失败后恢复原流程继续向下走
        m_nProtocolType = TYPE_PC_TCP;//这里的临时修改是为了ConnectLocalTry里顺利走向tcp
        
        if(!ConnectLocalTry(i, pstConn->chError))
        {//连接失败，直接结束连接过程
            m_nProtocolType = ntypetmp;//TYPE_PC_UDP;
            
            m_nStatus = NEW_P2P_N;
            return;
        }
        
        /////////////////////////////////////////////////////////////////////////
        //先等待是否无通道服务
        int n = 0;
        char datatmp[100] = {0};
        DWORD dwTime = CCWorker::JVGetTime();
        DWORD dw = CCWorker::JVGetTime();
        while(dw - dwTime < 50)
        {
            dw = CCWorker::JVGetTime();
            n = tcpreceive2(m_ServerSocket,(char* )datatmp,100,10);
            if(n > 0)
            {
                break;
            }
        }
        if(JVN_RSP_NOSERVER == datatmp[0])
        {
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "无该通道服务!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "channel is not open!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 无通道服务", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. no channel service.", __FILE__,__LINE__);
            }
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        else if(JVN_RSP_OVERLIMIT == datatmp[0])
        {//超过主控最大连接限制
            m_nStatus = FAILD;
            m_bPassWordErr = FALSE;
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "超过主控最大连接限制!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                else
                {
                    char chMsg[] = "client count limit!";
                    if(!m_bShowInfo)
                    {
                        m_bShowInfo = TRUE;
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 超过主控最大连接限制!", __FILE__,__LINE__);
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. client count limit!", __FILE__,__LINE__);
            }
            
            closesocket(m_ServerSocket);
            m_ServerSocket = 0;
            return;
        }
        ///////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        if(SendReCheck(FALSE))
        {//发送成功 等待验证结果
            m_nStatus = WAIT_TCPIPRECHECK;//注：这里严格来说应该走LRecheck,失败的话继续去连接外网。但状态切换有点麻烦，暂时处理为失败的话直接结束。
            m_dwStartTime = CCWorker::JVGetTime();
        }
        else
        {//发送失败 直接结束连接过程
            //注：这里严格来说应该继续去连接外网。但状态切换有点麻烦，暂时处理为直接结束。
            
            m_nStatus = FAILD;
            writeLog("SendReCheck failed...%d",__LINE__);
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "预验证失败!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP连接主控失败. 向主控发送预验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                char chMsg[] = "ReCheck failed!";
                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "IP connect failed. send repass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return;
        }
        //////////////////////////////////////////////////////////////////////////
        /*
         m_nStatus = WAIT_LW_CONNECTOLD;
         m_pOldChannel = new CCOldChannel(m_pWorker, this);
         if(m_pOldChannel == NULL)
         {
         m_nStatus = NEW_P2P_N;
         
         if(m_pWorker->m_bNeedLog)
         {
         char chMsg[MAX_PATH]={0};
         if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
         {
         sprintf(chMsg,"<[S%d]内网探测失败. TCPLink失败>**",i);
         strcat(pstConn->chError, chMsg);
         }
         else
         {
         sprintf(chMsg,"<[S%d]LocalTry failed. TCPLink failed>**",i);
         strcat(pstConn->chError, chMsg);
         }
         }
         }
         */
    }
    
    //需要内网探测
    void CCChannel::DealNEWP2PL(STCONNPROCP *pstConn)
    {
        m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;//找到对应服务器，后续有用
            }
        }
        
        if(ConnectLocalTry(i, pstConn->chError))
        {//向对端发送有效性验证消息
            //检查对方是否支持tcp，支持的话改为tcp连接
            if((m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP) && m_stConnInfo.nWhoAmI != JVN_WHO_H && m_stConnInfo.nWhoAmI != JVN_WHO_M && m_nStatus != NEW_HAVEIP)
            {
                BYTE uchTCP = UDT::getpeertcp(m_ServerSocket);
                if(uchTCP == 1)
                {//主控端支持tcp连接，关闭本地udp，进行tcp连接
                    //m_nProtocolType = TYPE_PC_TCP;//注：这里只有临时改为pctcp,接下来才会执行tcp连接 注2：这个修改放到DealNewTCPP2PL里了
                    
                    m_nStatus = NEW_TCP_P2PL;
                    m_dwStartTime = CCWorker::JVGetTime();
                    return;
                }
            }
            
            if(SendReCheck(TRUE))
            {//发送成功 等待验证结果
                m_nStatus = WAIT_LRECHECK;
                m_dwStartTime = CCWorker::JVGetTime();
            }
            else
            {//发送失败 开始外网连接
                m_nStatus = NEW_P2P_N;
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[3*MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]内网探测失败. 向主控发送预验证数据失败 详细:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pstConn->chError,chMsg);
                        strcat(pstConn->chError,">**");
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]LocalTry connect failed. send repass info failed. Info:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pstConn->chError,chMsg);
                        strcat(pstConn->chError,">**");
                    }
                }
            }
        }
        else
        {//连接失败，直接结束连接过程
            m_nStatus = NEW_P2P_N;
        }
    }
    
    //等待L(内网探测)连接有效性验证
    void CCChannel::DealWaitLRECheck(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;//找到当前服务器，后续有用
            }
        }
        
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 1000)//3000)//1000
        {//等待超时，连接失败 开始外网连接
            //这里重新进行连接，一些旧主控可能因为与验证信息的干扰无法继续，重新连接可兼容的更好，只是牺牲些时间
            writeLog("lrecheck time out...%d",__LINE__);
            OutputDebug("lrecheck time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            if(!ConnectLocalTry(i, pstConn->chError))
            {//连接失败，直接结束连接过程
                m_nStatus = NEW_P2P_N;
                return;
            }
            
            //////////////////////////////////////////////////////////////////////////
            //这一段，可以在新主控响应预验证不及时的情况下也能进行新协议的手机连接
            if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
            {
                int nTCP = UDT::getpeertcp(m_ServerSocket);
                if(nTCP == 1)
                {//主控端支持tcp连接，是比较新的主控程序
                    m_nStatus = WAIT_LW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                }
                else
                {
                    int nver = UDT::getystverF(m_ServerSocket);//获取远端协议版本
                    if(nver >= JVN_YSTVER2)
                    {
                        m_nStatus = WAIT_LW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    }
                    else
                    {
                        m_nStatus = WAIT_LW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this);
                    }
                }
            }
            else
            {
                m_nStatus = WAIT_LW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
            }
            //////////////////////////////////////////////////////////////////////////
            //		m_nStatus = WAIT_LW_CONNECTOLD;
            //		m_pOldChannel = new CCOldChannel(m_pWorker, this);
            if(m_pOldChannel == NULL)
            {
                m_nStatus = NEW_P2P_N;
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]内网探测失败. 等待预验证数据超时>**",i);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]LocalTry failed. wait repass info failed>**",i);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
        else
        {//接收数据
            int nver = 0;
            int nret = RecvReCheck(nver, pstConn->chAVersion);
            if(nret == 1)
            {//验证通过
                if(!m_bJVP2P || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
                {//不是多播方式，走原流程
                    m_nStatus = WAIT_LW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    return;
                }
                
                if(SendPWCheck())
                {//发送身份验证消息成功 等待结果
                    m_nStatus = WAIT_LPWCHECK;
                    m_dwStartTime = CCWorker::JVGetTime();
                }
                else
                {//发送身份验证消息失败 开始外网连接
                    m_nStatus = NEW_P2P_N;
                    
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[3*MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
            else if(nret == 0)
            {//预验证未通过 开始外网连接
                m_nStatus = NEW_P2P_N;
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(nver > 0)
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 本地版本太低,对方版本:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. Local Version too old,Des:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    else if(nver < 0)
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 对方版本太低,对方版本:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. Destination Version too old,Des:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    else
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 预验证失败>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. repass failed.>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
            else if(nret == 2)
            {//预验证未通过，对方是旧版主控
                m_nStatus = WAIT_LW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
                if(m_pOldChannel == NULL)
                {
                    m_nStatus = NEW_P2P_N;
                    
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 等待预验证数据超时>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. wait repass info failed>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
        }
    }
    
    //旧版尝试失败，调整主连接流程状态
    void CCChannel::DealWaitLWConnectOldF(STCONNPROCP *pstConn)
    {
        if(m_nProtocolType == TYPE_PC_TCP)
        {
            if(m_ServerSocket > 0)
            {
                closesocket(m_ServerSocket);
            }
            m_ServerSocket = 0;
        }
        m_nProtocolType = m_stConnInfo.nConnectType;
        m_nStatus = NEW_P2P_N;
        
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;
            }
        }
        
        if(m_pWorker->m_bNeedLog)
        {
            char chMsg[MAX_PATH]={0};
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                sprintf(chMsg,"<[S%d]内网探测失败. 等待预验证数据超时>**",i);
                strcat(pstConn->chError, chMsg);
            }
            else
            {
                sprintf(chMsg,"<[S%d]LocalTry failed. wait repass info failed>**",i);
                strcat(pstConn->chError, chMsg);
            }
        }
    }
    
    //等待身份验证
    void CCChannel::DealWaitLPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 4000)//1000
        {//等待超时，结束连接
            m_nStatus = NEW_P2P;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            m_stConnInfo.nTURNType = JVN_NOTURN;//不再进行转发链接
            writeLog("pwd time out...%d",__LINE__);
            OutputDebug("pwd time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "身份验证超时!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]内网探测失败. 等待身份验证数据超时",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]LocalTry failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//接收数据
            int nPWData = 0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//验证通过 连接成功建立
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrAL, sizeof(SOCKADDR_IN));
                memcpy(&m_addrConnectOK, &m_addrAL, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//身份验证未通过 直接结束连接
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_P2P;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                m_stConnInfo.nTURNType = JVN_NOTURN;//不再进行转发链接
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "身份未通过验证!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]内网探测失败. 身份验证失败",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]LocalTry failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //需要外网直连
    void CCChannel::DealNEWP2PN(STCONNPROCP *pstConn)
    {
        BOOL bfind = FALSE;
        int ncount = m_SList.size();
        for(int i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                bfind = TRUE;
                if(ConnectNet(i, pstConn->chError))
                {//连接成功则发送有效性验证消息并置为WAIT_NRECHECK
                    if(SendReCheck(TRUE))
                    {//发送成功 等待验证结果
                        m_nStatus = WAIT_NRECHECK;
                        m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送失败 尝试其他服务器
                        m_nStatus = NEW_P2P;
                        
                        m_SList[i].buseful = FALSE;
                        
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]外网连接失败. 向主控发送预验证数据失败 详细:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError,chMsg);
                                strcat(pstConn->chError,">**");
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]Net connect failed. send repass info failed. Info:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError,chMsg);
                                strcat(pstConn->chError,">**");
                            }
                        }
                    }
                }
                else
                {//连接失败则预测性连接
                    if(ConnectNetTry(m_SList[i].addr, i, pstConn->chError))
                    {
                        if(SendReCheck(TRUE))
                        {//发送成功 等待验证结果
                            m_nStatus = WAIT_NRECHECK;
                            m_dwStartTime = CCWorker::JVGetTime();
                        }
                        else
                        {//发送失败 尝试其他服务器
                            m_nStatus = NEW_P2P;
                            
                            m_SList[i].buseful = FALSE;
                            
                            if(m_pWorker->m_bNeedLog)
                            {
                                char chMsg[3*MAX_PATH]={0};
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    sprintf(chMsg,"<[S%d]外网连接失败. 向主控发送预验证数据失败 详细:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                    strcat(pstConn->chError,chMsg);
                                    strcat(pstConn->chError,">**");
                                }
                                else
                                {
                                    sprintf(chMsg,"<[S%d]Net connect failed. send repass info failed. Info:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                    strcat(pstConn->chError,chMsg);
                                    strcat(pstConn->chError,">**");
                                }
                            }
                        }
                        break;
                    }
                    else
                    {//失败 尝试下一个服务器
                        m_nStatus = NEW_P2P;
                        m_SList[i].buseful = FALSE;
                    }
                }
                
                break;
            }
        }
        
        if(!bfind)
        {
            m_nStatus = NEW_P2P;
        }
    }
    
    //等待N连接有效性验证
    void CCChannel::DealWaitNReCheck(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;
            }
        }
        
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 3000)//5000)
        {//等待超时，连接失败 尝试其他服务器
            //////////////////////////////////////////////////////////////////////////
            writeLog("nrecheck time out...%d",__LINE__);
            OutputDebug("nrecheck time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            if(!ConnectNet(i, pstConn->chError))
            {//连接失败则预测性连接
                m_nStatus = NEW_P2P;
                m_SList[i].buseful = FALSE;
                return;
            }
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            //这一段，可以在新主控响应预验证不及时的情况下也能进行新协议的手机连接
            if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
            {
                int nTCP = UDT::getpeertcp(m_ServerSocket);
                if(nTCP == 1)
                {//主控端支持tcp连接，是比较新的主控程序
                    m_nStatus = WAIT_NW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                }
                else
                {
                    int nver = UDT::getystverF(m_ServerSocket);//获取远端协议版本
                    if(nver >= JVN_YSTVER2)
                    {
                        m_nStatus = WAIT_NW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    }
                    else
                    {
                        m_nStatus = WAIT_NW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this);
                    }
                }
            }
            else
            {
                m_nStatus = WAIT_NW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
            }
            //////////////////////////////////////////////////////////////////////////
            
            //		m_nStatus = WAIT_NW_CONNECTOLD;
            //		m_pOldChannel = new CCOldChannel(m_pWorker, this);
            if(m_pOldChannel == NULL)
            {
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "预验证失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    
                    char chinfo[MAX_PATH]={0};
                    sprintf(chinfo,"<[S%d]外网直连失败. 等待预验证数据超时>**",i);
                    strcat(pstConn->chError, chinfo);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. ", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    char chMsg[] = "ReCheck Failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    
                    char chinfo[MAX_PATH]={0};
                    sprintf(chinfo,"<[S%d]Net Connect failed. wait repass info failed>**",i);
                    strcat(pstConn->chError, chinfo);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. ", __FILE__,__LINE__,pstConn->chError);
                }
            }
            /*
             if(pWorker->m_pWorker->m_bNeedLog)
             {
             char chMsg[MAX_PATH]={0};
             if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
             {
             sprintf(chMsg,"<[S%d]外网直连失败. 等待预验证数据超时>**",i);
             strcat(chError, chMsg);
             }
             else
             {
             sprintf(chMsg,"<[S%d]Net Connect failed. wait repass info failed>**",i);
             strcat(chError, chMsg);
             }
             }
             */
        }
        else
        {//接收数据
            int nver = 0;
            int nret = RecvReCheck(nver, pstConn->chAVersion);
            if(nret == 1)
            {//验证通过
                if(!m_bJVP2P || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
                {//不是多播方式，走原流程
                    m_nStatus = WAIT_NW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    return;
                }
                
                if(SendPWCheck())
                {//发送身份验证消息成功 等待结果
                    m_nStatus = WAIT_NPWCHECK;
                    m_dwStartTime = CCWorker::JVGetTime();
                }
                else
                {//发送身份验证消息失败 尝试其他服务器
                    m_nStatus = NEW_P2P;
                    
                    if(i>=0)
                    {
                        m_SList[i].buseful = FALSE;
                    }
                    
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[3*MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]外网直连失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]Net Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
            else if(nret == 0)
            {//预验证未通过 尝试其他服务器
                m_nStatus = NEW_P2P;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                m_stConnInfo.nTURNType = JVN_NOTURN;//不再进行转发链接
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    m_bShowInfo = TRUE;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        if(nver > 0)
                        {
                            char chMsg[] = "本地版本太低!";//Local Version too old!
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "外网直连失败. 本地版本太低,对方版本:", __FILE__,__LINE__,pstConn->chAVersion);
                        }
                        else if(nver < 0)
                        {
                            char chMsg[] = "对方版本太低!";//Destination Version too old!
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "外网直连失败. 对方版本太低,对方版本:", __FILE__,__LINE__,pstConn->chAVersion);
                        }
                        else
                        {
                            char chMsg[] = "预验证失败!";//ReCheck Faild
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "外网直连失败. 预验证失败", __FILE__,__LINE__);
                        }
                    }
                    else
                    {
                        if(nver > 0)
                        {
                            char chMsg[] = "Local Version too old!";//本地版本太低!
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Net connect failed. Local Version too old,Des:", __FILE__,__LINE__,pstConn->chAVersion);
                        }
                        else if(nver < 0)
                        {
                            char chMsg[] = "Destination Version too old!";//对方版本太低!
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Net connect failed. Destination Version too old,Des:", __FILE__,__LINE__,pstConn->chAVersion);
                        }
                        else
                        {
                            char chMsg[] = "ReCheck Failed!";//预验证失败
                            m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Net connect failed. repass failed.", __FILE__,__LINE__);
                        }
                    }
                }
            }
            else if(nret == 2)
            {//预验证未通过，对方是旧版主控
                m_nStatus = WAIT_NW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
                if(m_pOldChannel == NULL)
                {
                    m_nStatus = FAILD;
                    
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "预验证失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        
                        char chinfo[MAX_PATH]={0};
                        sprintf(chinfo,"<[S%d]外网直连失败. 等待预验证数据超时>**",i);
                        strcat(pstConn->chError, chinfo);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. ", __FILE__,__LINE__,pstConn->chError);
                    }
                    else
                    {
                        char chMsg[] = "ReCheck Failed!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        
                        char chinfo[MAX_PATH]={0};
                        sprintf(chinfo,"<[S%d]Net Connect failed. wait repass info failed>**",i);
                        strcat(pstConn->chError, chinfo);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. ", __FILE__,__LINE__,pstConn->chError);
                    }
                }
            }
        }
    }
    
    //旧版尝试失败，调整主连接流程状态
    void CCChannel::DealWaitNWConnectOldF(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;
            }
        }
        
        m_nProtocolType = m_stConnInfo.nConnectType;
        m_nStatus = NEW_P2P;
        if(i>=0)
        {
            m_SList[i].buseful = FALSE;
        }
        
        if(m_pWorker->m_bNeedLog)
        {
            char chMsg[MAX_PATH]={0};
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                sprintf(chMsg,"<[S%d]外网直连失败. 等待预验证数据超时>**",i);
                strcat(pstConn->chError, chMsg);
            }
            else
            {
                sprintf(chMsg,"<[S%d]Net Connect failed. wait repass info failed>**",i);
                strcat(pstConn->chError, chMsg);
            }
        }
    }
    
    //等待身份验证
    void CCChannel::DealWaitNPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 10000)
        {//等待超时，结束连接
            m_nStatus = NEW_P2P;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            m_stConnInfo.nTURNType = JVN_NOTURN;//不再进行转发链接
            writeLog("pwd time out...%d",__LINE__);
            OutputDebug("pwd time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "身份验证超时!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]外网直连失败. 等待身份验证数据超时",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]Net Connect failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//接收数据
            int nPWData=0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//验证通过 连接成功建立
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrAN, sizeof(SOCKADDR_IN));
                memcpy(&m_addrConnectOK, &m_addrAN, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//身份验证未通过 直接结束连接
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_P2P;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                m_stConnInfo.nTURNType = JVN_NOTURN;//不再进行转发链接
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    m_bShowInfo = TRUE;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "身份未通过验证!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]外网直连失败. 身份验证失败",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]Net Connect failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //等待进行转发连接
    void CCChannel::DealNEWTURN(STCONNPROCP *pstConn)
    {
        OutputDebug("ReceiveIndexFromServer  1");
        ReceiveIndexFromServer(pstConn,FALSE);
        m_stConnInfo.nShow = JVN_CONNTYPE_TURN;
        
        BOOL bfind = FALSE;
        if(JVN_NOTURN != m_stConnInfo.nTURNType)
        {
            int ncount = m_SList.size();
            OutputDebug("DealNEWTURN  1 count: %d",ncount);
            writeLog(".................m_SList.size();  count:%d",ncount);
			for(int i=0; i<ncount; i++)
			{
				if(m_SList[i].buseful)
				{//开始尝试请求主控地址
	//				m_SList[i].addr.sin_addr.s_addr = inet_addr("60.217.227.239");
	//				m_SList[i].addr.sin_port = htons(9110);
					if(SendSTURN(m_SList[i].addr, i, pstConn->chError))
					{//向服务器发送请求成功
						m_nStatus = WAIT_TURN;
						m_nRequestTurnTimes = 0;
						m_dwStartTime = CCWorker::JVGetTime();
						m_dwSendTurnTime = m_dwStartTime;
						bfind = TRUE;
							OutputDebug("请求转发地址.....%d",i);
							writeLog("请求转发地址.....%d, requestServerIp: %s",i,inet_ntoa(m_SList[i].addr.sin_addr),ntohs(m_SList[i].addr.sin_port));
							break;
						}
						else
						{//向服务器发送请求失败 尝试下一个服务器
							m_SList[i].buseful = FALSE;
						}
					}
			}
        }
        if(!bfind || m_bPassWordErr)
        {	
#ifdef MOBILE_CLIENT
          //  g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,__FUNCTION__,"JOV lib nRecvLen:%d, type:0x%x!\n", nRecvLen, nType);
			writeLog("...........******........no find online svr return, online svrnum:%d,totao:%d pword %d",m_SList.size(),m_GroupSvrList.addrlist.size(),m_bPassWordErr);
			if((m_SList.size() > 0) && (!m_bPassWordErr))
			{
				if(m_GroupSvrList.addrlist.size() == 0)
				{
					m_pWorker->GetGroupSvrList(m_stConnInfo.chGroup,m_GroupSvrList);
				}
				if(m_GroupSvrList.addrlist.size() > 0)
				{
				
					if(SelectAliveSvrList(m_SList) > 0)
					{
						g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,"",".......selec alivesvrlist success !");
						for (int i = 0; i < m_SList.size(); i++)
						{
							g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,"","..........onliesvr [%s:%d]",inet_ntoa(m_SList[i].addr.sin_addr),ntohs(m_SList[i].addr.sin_port));
						}
						m_nStatus = REQ_FORWARD_TURNADDR;
						return;
					}
					g_dbg.jvcout(OUT_ON_EVENT,__FILE__,__LINE__,"",".............selec alivesvrlist error !");
				}
			}
#endif
			OutputDebug("request turn ip failed");
            writeLog("request turn ip failed m_nStatus = FAILD;");
            m_nStatus = FAILD;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                if(!m_bShowInfo)
                {
                    if(m_bPassWordErr)
                    {
                        char chMsg[] = "身份未通过验证!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "连接超时!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    
                }
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(TURN)方式连接主控失败.原因分别为:", __FILE__,__LINE__,pstConn->chError);
            }
            else
            {
                if(!m_bShowInfo)
                {
                    if(m_bPassWordErr)
                    {
                        char chMsg[] = "password is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "connect timeout!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "YST(TURN) connect failed.Infos:", __FILE__,__LINE__,pstConn->chError);
            }
        }
    }
    
    //等待转发地址
    void CCChannel::DealWaitTURN(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
	if(pstConn->dwendtime > m_dwStartTime + 1500*2)//3000)//3000
        {//等待超时，连接失败 尝试其他服务器
            m_nStatus = NEW_TURN;
            char str[50] = {0};
            sprintf(str,"wait Turn timeout line %d\r\n",__LINE__);
            OutputDebug(str);
            writeLog(str);
            
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
            }
            
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    m_SList[i].buseful = FALSE;
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH] = {0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]等待服务器数据TS超时>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]wait info TS from S timeout>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    break;
                }
            }
        }
        else
        {//接收数据
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    int nret = RecvSTURN(i, pstConn->chError);
                    switch(nret)
                    {//收到服务器回复
                        case JVN_CMD_CONNS2://取得地址 开始连接
                        {
                            m_nStatus = RECV_TURN;
                            
                            m_dwStartTime = CCWorker::JVGetTime();
                            OutputDebug("请求转发地址.....%d 成功",i);
                            writeLog("请求转发地址.....%d 成功",i);
					}

					break;
				case JVN_RSP_CONNAF://取地址失败 尝试其他服务器
					m_SList[i].buseful = FALSE;
					m_nStatus = NEW_TURN;
					m_dwStartTime = CCWorker::JVGetTime();
                            OutputDebug("请求转发地址.....%d 失败",i);
                            writeLog("请求转发地址.....%d 失败",i);
					break;
				case -1://没有收到数据重新请求下
					{
						DWORD dwT = CCWorker::JVGetTime();
						if(dwT - m_dwStartTime > 100)
						{
							ReRequestTurnAddr();
						}
						break;
					}
				case JVN_CMD_TRYTOUCH://收到打洞包 直接连接
				default://未收到信息或收到其他错误信息 继续接收
                            OutputDebug("未收到信息或收到其他错误信息 继续接收",i);
                            writeLog("未收到信息或收到其他错误信息 继续接收",i);
                            break;
                    }
                    break;
                }
            }
        }
    }
    
    //取得转发地址
    void CCChannel::DealRecvTURN(STCONNPROCP *pstConn)
    {
        BOOL bfind=FALSE;
        int ncount = m_SList.size();
        for(int i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                bfind = TRUE;
                if(ConnectTURN(i, pstConn->chError))
                {//向对端发送有效性验证消息
                    if(SendReCheck(TRUE))
                    {//发送成功 等待验证结果
                        m_nStatus = WAIT_TSRECHECK;
                        m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送失败 尝试其他服务器
                        m_nStatus = NEW_TURN;
                        
                        m_SList[i].buseful = FALSE;
                        
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]TURN连接失败. 向主控发送预验证数据失败 详细:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError,chMsg);
                                strcat(pstConn->chError,">**");
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]TURN failed. send repass info failed. Info:%s",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError,chMsg);
                                strcat(pstConn->chError,">**");
                            }
                        }
                    }
                }
                else
                {//连接失败，尝试其他服务器
                    m_nStatus = NEW_TURN;
                    m_SList[i].buseful = FALSE;
                }
                
                break;
            }
        }
        if(!bfind)
        {
            m_nStatus = NEW_TURN;
        }
    }
    
    //等待TURN连接有效性验证
    void CCChannel::DealWaitTSReCheck(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;
            }
        }
        
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 4000)//5000)
        {//等待超时，连接失败 尝试其他服务器
            //////////////////////////////////////////////////////////////////////////
            writeLog("tsrecheck time out...%d",__LINE__);
            OutputDebug("tsrecheck time out...%d",__LINE__);
            SendData(JVN_CMD_DISCONN, NULL, 0);
            if(!ConnectTURN(i, pstConn->chError))
            {//连接失败，尝试其他服务器
                m_nStatus = NEW_TURN;
                m_SList[i].buseful = FALSE;
            }
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            //这一段，可以在新主控响应预验证不及时的情况下也能进行新协议的手机连接
            if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
            {
                int nTCP = UDT::getpeertcp(m_ServerSocket);
                if(nTCP == 1)
                {//主控端支持tcp连接，是比较新的主控程序
                    m_nStatus = WAIT_TSW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                }
                else
                {
                    int nver = UDT::getystverF(m_ServerSocket);//获取远端协议版本
                    if(nver >= JVN_YSTVER2)
                    {
                        m_nStatus = WAIT_TSW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    }
                    else
                    {
                        m_nStatus = WAIT_TSW_CONNECTOLD;
                        m_pOldChannel = new CCOldChannel(m_pWorker, this);
                    }
                }
            }
            else
            {
                m_nStatus = WAIT_TSW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
            }
            //////////////////////////////////////////////////////////////////////////
            //		m_nStatus = WAIT_TSW_CONNECTOLD;
            //		m_pOldChannel = new CCOldChannel(m_pWorker, this);
            if(m_pOldChannel == NULL)
            {
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "预验证失败!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    
                    char chinfo[MAX_PATH]={0};
                    sprintf(chinfo,"<[S%d]TURN连接失败. 等待预验证数据超时>**",i);
                    strcat(pstConn->chError, chinfo);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. ", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    char chMsg[] = "ReCheck Failed!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                    
                    char chinfo[MAX_PATH]={0};
                    sprintf(chinfo,"<[S%d]TURN Connect failed. wait repass info failed>**",i);
                    strcat(pstConn->chError, chinfo);
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. ", __FILE__,__LINE__,pstConn->chError);
                }
            }
            /*
             if(pWorker->m_pWorker->m_bNeedLog)
             {
             char chMsg[MAX_PATH]={0};
             if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
             {
             sprintf(chMsg,"<[S%d]TURN连接失败. 等待预验证数据超时>**",i);
             strcat(chError, chMsg);
             }
             else
             {
             sprintf(chMsg,"<[S%d]TURN Connect failed. wait repass info failed>**",i);
             strcat(chError, chMsg);
             }
             }
             */
        }
        else
        {//接收数据
            int nver = 0;
            int nret = RecvReCheck(nver, pstConn->chAVersion);
            if(nret == 1)
            {//验证通过
                if(!m_bJVP2P || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
                {//不是多播方式，走原流程
                    m_nStatus = WAIT_TSW_CONNECTOLD;
                    m_pOldChannel = new CCOldChannel(m_pWorker, this, TRUE);
                    return;
                }
                
                if(SendPWCheck())
                {//发送身份验证消息成功 等待结果
                    m_nStatus = WAIT_TSPWCHECK;
                    m_dwStartTime = CCWorker::JVGetTime();
                }
                else
                {//发送身份验证消息失败 尝试其他服务器
                    m_nStatus = NEW_TURN;
                    
                    if(i>=0)
                    {
                        m_SList[i].buseful = FALSE;
                    }
                    
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[3*MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]TURN连接失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]TURN Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
            else if(nret == 0)
            {//预验证未通过 尝试其他服务器
                m_nStatus = NEW_TURN;
                
                if(i>=0)
                {
                    m_SList[i].buseful = FALSE;
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(nver > 0)
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]TURN连接失败. 本地版本太低,对方版本:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]TURN Connect failed. Local Version too old,Des:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    else if(nver < 0)
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]TURN连接失败. 对方版本太低,对方版本:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]TURN Connect failed. Destination Version too old,Des:%s>**",i,pstConn->chAVersion);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    else
                    {
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]TURN连接失败. 预验证失败>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]TURN Connect failed. repass failed.>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                }
            }
            else if(nret == 2)
            {//预验证未通过，对方是旧版主控
                m_nStatus = WAIT_TSW_CONNECTOLD;
                m_pOldChannel = new CCOldChannel(m_pWorker, this);
                if(m_pOldChannel == NULL)
                {
                    m_nStatus = FAILD;
                    
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "预验证失败!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        
                        char chinfo[MAX_PATH]={0};
                        sprintf(chinfo,"<[S%d]TURN连接失败. 等待预验证数据超时>**",i);
                        strcat(pstConn->chError, chinfo);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. ", __FILE__,__LINE__,pstConn->chError);
                    }
                    else
                    {
                        char chMsg[] = "ReCheck Failed!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        
                        char chinfo[MAX_PATH]={0};
                        sprintf(chinfo,"<[S%d]TURN Connect failed. wait repass info failed>**",i);
                        strcat(pstConn->chError, chinfo);
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. ", __FILE__,__LINE__,pstConn->chError);
                    }
                }
            }
        }
    }
    
    //旧版尝试失败，调整主连接流程状态
    void CCChannel::DealWaitTSWConnectOldF(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;
            }
        }
        
        m_nStatus = NEW_TURN;
        if(i>=0)
        {
            m_SList[i].buseful = FALSE;
        }
        
        if(m_pWorker->m_bNeedLog)
        {
            char chMsg[MAX_PATH]={0};
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                sprintf(chMsg,"<[S%d]TURN连接失败. 等待预验证数据超时>**",i);
                strcat(pstConn->chError, chMsg);
            }
            else
            {
                sprintf(chMsg,"<[S%d]TURN Connect failed. wait repass info failed>**",i);
                strcat(pstConn->chError, chMsg);
            }
        }
    }
    
    //等待身份验证
    void CCChannel::DealWaitTSPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 5000)
        {//等待超时，结束连接
            m_nStatus = NEW_TURN;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            writeLog("pwd time out...%d",__LINE__);
            OutputDebug("pwd time out...%d",__LINE__);

            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "身份验证超时!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]TURN连接失败. 等待身份验证数据超时",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]TURN Connect failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//接收数据
            int nPWData=0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//验证通过 连接成功建立
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrTS, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//身份验证未通过 直接结束连接
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_TURN;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    m_bShowInfo = TRUE;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "身份未通过验证!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]TURN连接失败. 身份验证失败",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]TURN Connect failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //获取服务器地址，或是直接获取
    void CCChannel::GetSerAndBegin(STCONNPROCP *pstConn)
    {
        if(CheckNewHelp() > 0)
            return;
        DWORD dwend = 0;
        DWORD dwbegin = CCWorker::JVGetTime();
        if(m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, 0) > 0)//有通道正在连接该号码，应该等待对方的有效结果
        {
#ifdef MOBILE_CLIENT
            while(!m_bExit)
#else
                while(TRUE)
#endif
                {
                    if(CheckNewHelp() > 0)
                        return;
                    
                    ServerList slist;
                    char chIPA[16]={0};
                    int nport = 0;
                    SOCKET s = 0;
                    int nret = m_pWorker->GetYSTNOInfo(m_stConnInfo.chGroup, m_stConnInfo.nYSTNO, slist, m_addrAL, chIPA, nport,s);
                    if(nret != 1)// && nret != 2)
                    {
                        dwend = CCWorker::JVGetTime();
                        if(dwend < dwbegin || dwend > dwbegin + 15 * 1000)
                        {
                            break;
                        }
                        CCWorker::jvc_sleep(50);
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
        }
	OutputDebug("++++++++++++++++++++++++++++++channel localchannel: %d, yst: %d, line: %d\n",m_nLocalChannel,m_stConnInfo.nYSTNO,__LINE__);
        m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, 1);
        
        ServerList slist;
        char chIPA[16]={0};
        int nport = 0;
        SOCKET s = 0;
        int nret = m_pWorker->GetYSTNOInfo(m_stConnInfo.chGroup, m_stConnInfo.nYSTNO, slist, m_addrAL, chIPA, nport,s);
        if(nret == 1)
        {//已连接过，直接连接对方ip
            m_SList = slist;
			AddCSelfServer();
           m_SListTurn = m_SList;
            m_SListBak = m_SList;
            
            m_nStatus = NEW_VIRTUAL_IP;
            
            sprintf(m_stConnInfo.chServerIP, "%s", inet_ntoa(m_addrAL.sin_addr));
            m_stConnInfo.nServerPort = ntohs(m_addrAL.sin_port);
            m_stConnInfo.sRandSocket = s;
            
            //char ch[100]={0};
            //sprintf(ch,"%s:%d===============%s:%d\n",pWorker->m_stConnInfo.chServerIP,pWorker->m_stConnInfo.nServerPort,chIPA,nport);
            //OutputDebugString(ch);
        }
        else if(nret == 2)
        {//服务器地址可以直接使用
            m_SList = slist;
			AddCSelfServer();
            m_SListTurn = m_SList;
            m_SListBak = m_SList;
            
            m_nStatus = NEW_P2P;//直接进行连接
            
            if(JVN_ONLYTURN == m_stConnInfo.nTURNType || m_stConnInfo.nConnectType == TYPE_MO_UDP ||m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)//只转发
            {
                m_nStatus = NEW_TURN;
#ifdef MOBILE_CLIENT
                m_nStatus = WAIT_INDEXSER_REQ;
#endif
                m_nStatus = WAIT_INDEXSER_REQ;
                int nnnn = m_ISList.size();
                if(m_ISList.size() <= 0)
                {
                    if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 1, m_stConnInfo.nLocalPort))
                    {
                        if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 2, m_stConnInfo.nLocalPort))
                        {
                            ;
                        }
                    }
                }
                return;
            }
        }
        else
        {
            //全新流程
            if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 1, m_stConnInfo.nLocalPort))
            {
                if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 2, m_stConnInfo.nLocalPort))
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        //	char chMsg[] = "获取YST服务器信息失败!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST方式连接主控失败.原因:获取索引服务器失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        //	char chMsg[] = "get server address failed!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get index server address failed.", __FILE__,__LINE__);
                    }
                }
            }
            
            m_SList.clear();
            m_SListTurn.clear();
            m_SListBak.clear();
            
            m_nStatus = WAIT_INDEXSER_REQ;//以云视通方式连接时，优先进行服务期查询，提高服务器利用效率
            
            //初始化临时套接字用于号码服务器查询
            pstConn->udpsocktmp = socket(AF_INET, SOCK_DGRAM, 0);
            if (pstConn->udpsocktmp > 0)
            {
#ifndef WIN32
                pstConn->sin.sin_addr.s_addr = INADDR_ANY;
#else
                pstConn->sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
                pstConn->sin.sin_family = AF_INET;
                pstConn->sin.sin_port = htons(0);
                
                BOOL bReuse = TRUE;
                setsockopt(pstConn->udpsocktmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                
                if (bind(pstConn->udpsocktmp, (struct sockaddr*)&pstConn->sin, sizeof(pstConn->sin)) < 0)
                {
                    shutdown(pstConn->udpsocktmp,SD_BOTH);
                    closesocket(pstConn->udpsocktmp);
                    pstConn->udpsocktmp = 0;
                    if(JVN_ONLYTURN == m_stConnInfo.nTURNType)//只转发
                    {
                        m_nStatus = NEW_TURN;
                        return;
                    }
                    else if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP)
                    {
                        m_nStatus = NEW_TURN;
                    }
                    else
                    {
                        m_nStatus = NEW_P2P;//出错，直接进行连接
                    }
                }
            }
            else
            {
                if(JVN_ONLYTURN == m_stConnInfo.nTURNType)//只转发
                {
                    m_nStatus = NEW_TURN;
                    return;
                }
                else if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP)
                {
                    m_nStatus = NEW_TURN;
                }
                else
                {
                    m_nStatus = NEW_P2P;//出错，直接进行连接
                }
            }
        }
    }
    
    
#ifndef WIN32
    void* CCChannel::ConnProc(void* pParam)
#else
    UINT WINAPI CCChannel::ConnProc(LPVOID pParam)
#endif
    {
        CCChannel *pWorker = (CCChannel *)pParam;
        
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
        
        ::WaitForSingleObject(pWorker->m_hStartEventC, INFINITE);
        if(pWorker->m_hStartEventC > 0)
        {
            CloseHandle(pWorker->m_hStartEventC);
        }
        pWorker->m_hStartEventC = 0;
#endif
        
        STCONNPROCP stConnP;
        
        stConnP.udpsocktmp=0;
        //	stConnP.sin;
        stConnP.nSockAddrLen = sizeof(SOCKADDR_IN);
        stConnP.slisttmp.clear();
        
	//判断连接类型
	if(pWorker->m_stConnInfo.bYST)
	{//云视通方式连接
		memset(&pWorker->m_addrANOLD, 0, sizeof(SOCKADDR_IN));
		pWorker->m_nStatus = NEW_YST;

		if(pWorker->m_pWorker->YstIsDemo(pWorker->m_stConnInfo.chGroup,pWorker->m_stConnInfo.nYSTNO))
		{
			if(pWorker->m_pWorker->GetDemoAddr(pWorker->m_stConnInfo.chGroup,pWorker->m_stConnInfo.nYSTNO,
				pWorker->m_stConnInfo.chServerIP,pWorker->m_stConnInfo.nServerPort,
				pWorker->m_stConnInfo.chPassName,pWorker->m_stConnInfo.chPassWord
				))
			{
				pWorker->m_nStatus = NEW_VIRTUAL_IP;
			}
		}
	}
	else
	{//IP直连
		pWorker->m_nStatus = NEW_IP;
        if (pWorker->m_stConnInfo.nOnlyTCP==1) {
            pWorker->m_nStatus = NEW_HAVETCP;
        }
	}
        
        stConnP.dwendtime = 0;
        memset(stConnP.chAVersion, 0, MAX_PATH);
        memset(stConnP.chError, 0, 20480);
        
        //0版本兼容 >0本地版本太低 <0对方版本太低
        
        pWorker->m_bShowInfo = FALSE;//用于避免云视通连接时重复提示
        //////////////////////////////////////////////////////////////////////////
        
        memset(stConnP.szIPAddress, 0, 100);
#ifndef MOBILE_CLIENT
        NATList m_LocalIPList;
        pWorker->m_pWorker->GetLocalIP(&m_LocalIPList);
        
        if(m_LocalIPList.size() > 0)
        {
            sprintf(stConnP.szIPAddress,"%d.%d.%d.%d",m_LocalIPList[0].ip[0],m_LocalIPList[0].ip[1],m_LocalIPList[0].ip[2],m_LocalIPList[0].ip[3]);
        }
#endif
        //////////////////////////////////////////////////////////////////////////
        
        /*
         一.IP直连
         按新协议发起连接->发送与验证消息->收到与验证消息->身份验证->成功或失败
         ->未收到与验证消息->按旧协议发送身份验证消息->成功或失败
         
         二.号码连接
         内网探测 直连 转发
         */
        int nlast = 0;
        while(TRUE)
        {
#ifndef WIN32
            if(pWorker->m_bExit || pWorker->m_bEndC)
            {
                break;
            }
#else
            if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventC, 0))
            {
                if(pWorker->m_hEndEventC > 0)
                {
                    CloseHandle(pWorker->m_hEndEventC);
                    pWorker->m_hEndEventC = 0;
                }
                
                break;
            }
#endif
//            if(pWorker->ConnectTURN(0, "")){
//                if(pWorker->SendReCheck(TRUE))
//                {//发送成功 等待验证结果
//                    pWorker->m_nStatus = WAIT_TSRECHECK;
//                    pWorker->m_dwStartTime = CCWorker::JVGetTime();
//                }
//            }else{
//                OutputDebug("connect faild");
//                return 0;
//            }
            
            //		OutputDebug("pWorker->m_nStatus = %d",pWorker->m_nStatus);
            if(nlast != pWorker->m_nStatus)
            {
                
                writeLog("............................... status: %d, line: %d",pWorker->m_nStatus,__LINE__);
            }
            nlast = pWorker->m_nStatus;
            switch(pWorker->m_nStatus)
            {
                case NEW_YST://新的云视通号码连接
                {
                    pWorker->DealNewYST(&stConnP);
                }
                    break;
                case NEW_HAVE_LOCAL://内网已经有扫描到的地址
                {
                    OutputDebug("case NEW_HAVE_LOCAL  %line: %d",__LINE__);
                    writeLog("case NEW_HAVE_LOCAL  %line: %d",__LINE__);
                    printf("connect ip: line: %d \n",__LINE__);
                    pWorker->DealNewIP(&stConnP);
                }
                    break;
                case NEW_VIRTUAL_IP://根据虚连接提供的ip，先进行直连
                {
                    pWorker->DealNewVirtualIP(&stConnP);
                    
                }
                    break;
                case NEW_HELP_CONN://从小助手连接和取数据
                {
                    pWorker->DealNewHelpConn(&stConnP);
                    
                }
                    break;
                case WAIT_HELP_RET://从小助手等待连接结果
                {
                    pWorker->DealWaitHelpRET(&stConnP);
                    
                }
                    break;
                case WAIT_SER_REQ://向服务器发送查询号码命令
                {
                    pWorker->DealWaitSerREQ(&stConnP);
                }
                    break;
                case WAIT_SER_RSP://等待服务器结果 该过程最长允许2秒钟
                {//根据返回情况对服务器进行优劣和有效性判断
                    
                    pWorker->DealWaitSerRSP(&stConnP);
                }
                    break;
                case WAIT_INDEXSER_REQ://向索引服务器发送查询号码命令
                {
                    pWorker->DealWaitIndexSerREQ(&stConnP);
                }
                    break;
                case WAIT_INDEXSER_RSP://等待服务器结果 该过程最长允许2秒钟
                {
                    pWorker->DealWaitIndexSerRSP(&stConnP);
                }
                    break;
                case WAIT_CHECK_A_NAT://等待主控NAT类型
                {
                    pWorker->DealWaitNatSerREQ(&stConnP);
                }
                    break;
				case REQ_DEV_PUBADDR: // 请求设备公网地址
				{
					pWorker->DealWaitReqDevPubAddr(&stConnP);
				}
				break;
                case WAIT_NAT_A://查询主控NAT类型
                {
                    pWorker->DealWaitNatSerRSP(&stConnP);
                }
                    break;
                case WAIT_MAKE_HOLE://等待连接过程，一直在等待
                {
                    CCWorker::jvc_sleep(2);
                    //				OutputDebug(".........");
                }
                    break;
                case NEW_IP://开始进行IP直连
                {
                    printf("connect ip: line: %d \n",__LINE__);
                    pWorker->DealNewIP(&stConnP);
                    printf("connect ip: line: %d \n",__LINE__);
                }
                    break;
                case NEW_HAVEIP://开始进行IP直连
                {printf("connect ip: line: %d \n",__LINE__);
                    pWorker->DealNewIP(&stConnP);
                }
                    break;
                case NEW_HAVETCP://开始进行TCP直连
                {
                    pWorker->DealNewTCP(&stConnP);
                }
                    break;
                case NEW_TCP_IP:
                {
                    pWorker->DealNEWTCPIP(&stConnP);
                }
                    break;
                case NEW_VTCP_IP:
                {
                    pWorker->DealNEWVTCPIP(&stConnP);
                }
                    break;
                case WAIT_IPRECHECK://等待IP连接有效性验证，预验证，2.0版本协议
                case WAIT_TCPIPRECHECK:
                {
                    //有预验证响应，说明是新主控，按正常流程进行
                    //没有预验证响应，或是明确收到旧版通知，说明是旧主控，按旧协议进行
                    pWorker->DealWaitIPRECheck(&stConnP);
                }
                    break;
                case WAIT_IP_CONNECTOLD_F://旧版尝试失败，调整主连接流程状态
                {//旧协议尝试失败，说明主控确定无法建立连接
                    
                    pWorker->DealWaitIPConnectOldF(&stConnP);
                }
                    break;
                case WAIT_IPPWCHECK://等待身份验证
                {//预验证通过后需要主控的身份确认，在此处等待
                    
                    pWorker->DealWaitIPPWCheck(&stConnP);
                }
                    break;
                case OK_OLD://旧版尝试连接成功
                {
                    pWorker->m_nStatus = FAILD;//新协议连接失败，但旧协议连接成功
                    pWorker->m_connectThreadExit = TRUE;
                     printf("connect failed-----------------------line: %d\n",__LINE__);
#ifdef WIN32
                    //连接线程结束
                    if(pWorker->m_hEndEventC > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventC);
                    }
                    pWorker->m_hEndEventC = 0;
                    return 0;
#else
                    return NULL;
#endif
                }
                    break;
                case OK://连接成功
                {//创建其他工作线程
                    pWorker->DealOK(&stConnP);
                    pWorker->m_connectThreadExit = TRUE;
#ifdef WIN32
                    return 0;
#else
                    return NULL;
#endif
                }
                    break;
                case HELP_OK://通过小助手连接成功
                {//创建其他工作线程
                    pWorker->DealHelpOK(&stConnP);
                    pWorker->m_connectThreadExit = TRUE;
                    
                    return 0;
                }
                    break;
                case FAILD://连接失败
                {
                    pWorker->DealFAILD(&stConnP);
                    pWorker->m_connectThreadExit = TRUE;
                    printf("connect failed-----------------------line: %d\n",__LINE__);
#ifdef WIN32
                    return 0;
#else
                    return NULL;
#endif
                }
                    break;
                case NEW_P2P://等待进行外网连接
                {
                    pWorker->DealNEWP2P(&stConnP);
                }
                    break;
                case WAIT_S://等待服务器响应
                {
                    pWorker->DealWaitS(&stConnP);
                }
                    break;
                case NEW_P2P_L://需要内网探测
                {//直连内网IP，成功则发送有效性验证消息并置为WAIT_LRECHECK;失败置为NEW_P2P_N开始外网连接
                    
                    pWorker->DealNEWP2PL(&stConnP);
                }
                    break;
                case NEW_TCP_P2PL://需要内网探测
                {//直连内网IP，成功则发送有效性验证消息并置为WAIT_LRECHECK;失败置为NEW_P2P_N开始外网连接
                    
                    pWorker->DealNEWTCPP2PL(&stConnP);
                }
                    break;
                case WAIT_LRECHECK://等待L连接有效性验证
                case WAIT_LTCPRECHECK:
                {//通过则置为OK_LRECHECK,没通过则置为NEW_P2P_N开始外网连接
                    
                    pWorker->DealWaitLRECheck(&stConnP);
                }
                    break;
                case WAIT_LW_CONNECTOLD_F://旧版尝试失败，调整主连接流程状态
                {
                    pWorker->DealWaitLWConnectOldF(&stConnP);
                }
                    break;
                case WAIT_LPWCHECK://等待身份验证
                {//验证成功置为OK, 否则置为FAILD
                    
                    pWorker->DealWaitLPWCheck(&stConnP);
                }
                    break;
                case NEW_P2P_N://需要外网直连
                {
                    pWorker->DealNEWP2PN(&stConnP);
                }
                    break;
                case WAIT_NRECHECK://等待N连接有效性验证
                {//发送身份验证消息，发送成功置为WAIT_NPWCHECK;否则置为NEW_P2P开始
                    pWorker->DealWaitNReCheck(&stConnP);
                }
                    break;
                case WAIT_NW_CONNECTOLD_F://旧版尝试失败，调整主连接流程状态
                {
                    pWorker->DealWaitNWConnectOldF(&stConnP);
                }
                    break;
                case WAIT_NPWCHECK://等待身份验证
                {
                    pWorker->DealWaitNPWCheck(&stConnP);
                }
                    break;
                    
                case NEW_TURN://等待进行转发连接
			{
                    pWorker->DealNEWTURN(&stConnP);
                }
                    break;
                case WAIT_TURN://等待转发地址
                {
                    pWorker->DealWaitTURN(&stConnP);
                }
                    break;
                case RECV_TURN://取得转发地址
                {
                    pWorker->DealRecvTURN(&stConnP);
                }
                    break;
				case RECV_TURNLIST:
				{
					pWorker->DealRecvTURNLIST(&stConnP);
				}
					break;
                case WAIT_TSRECHECK://等待TURN连接有效性验证
                {
                    pWorker->DealWaitTSReCheck(&stConnP);
                }
                    break;
				case REQ_FORWARD_TURNADDR: // 请求云视通服务器转发请求turn地址
				{
					pWorker->ReqTurnAddrViaSvr();
				}
					break;
                case WAIT_TSW_CONNECTOLD_F://旧版尝试失败，调整主连接流程状态
                {
                    pWorker->DealWaitTSWConnectOldF(&stConnP);
                }
                    break;
                case WAIT_TSPWCHECK://等待身份验证
                {
                    pWorker->DealWaitTSPWCheck(&stConnP);
                }
                    break;
                case WAIT_IP_CONNECTOLD:
                case WAIT_LW_CONNECTOLD:
                case WAIT_NW_CONNECTOLD:
                case WAIT_TSW_CONNECTOLD:
                {//正在尝试用旧版方式连接，看是否是在连接旧版主控，返回结果前连接线程暂不退出
                    
                    CCWorker::jvc_sleep(2);
                }
                    break;
                default:
                    break;
            }
            
            CCWorker::jvc_sleep(10);
        }
        
        pWorker->m_connectThreadExit = TRUE;
        printf("connproc thread exit line: %d\n",__LINE__);
#ifdef WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    BOOL CCChannel::ConnectIP()
    {
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            return ConnectIPTCP();
        }
        
        if(m_ListenSocketTCP > 0)
        {
            closesocket(m_ListenSocketTCP);
        }
        m_ListenSocketTCP = 0;
        
        m_PartnerCtrl.ClearPartner();
        
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        m_stConnInfo.nShow = 0;
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	   int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
        
#ifdef MOBILE_CLIENT
	   len1 = JVC_MSS;
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        
        if(m_stConnInfo.sRandSocket == 0)
        {
            if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
            {//绑定到指定端口失败，改为绑定到随机端口
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                
                return FALSE;
            }
        }
        else
        {
            //		OutputDebug("Bind 2 %d",__LINE__);
            if (UDT::ERROR == UDT::bind(m_ServerSocket, m_stConnInfo.sRandSocket))
            {//绑定到指定端口失败，改为绑定到随机端口
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                
                return FALSE;
            }
        }
        
        
        SOCKADDR_IN addrA;
#ifndef WIN32
        addrA.sin_addr.s_addr = inet_addr(m_stConnInfo.chServerIP);
#else
        addrA.sin_addr.S_un.S_addr = inet_addr(m_stConnInfo.chServerIP);
#endif
        addrA.sin_family = AF_INET;
        addrA.sin_port = htons(m_stConnInfo.nServerPort);
        
        memcpy(&m_addrAL, &addrA, sizeof(SOCKADDR_IN));
        
        char ch[1000]={0};
        char ch1[1000]={0};
        char ch2[1000]={0};
        sprintf(ch,"IP:%s:%d",inet_ntoa(addrA.sin_addr), ntohs(addrA.sin_port));
        sprintf(ch1,"IP:%s:%d",inet_ntoa(m_addrAL.sin_addr), ntohs(m_addrAL.sin_port));
        

        
        //将套接字置为非阻塞模式
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        
        STJUDTCONN stcon;
        stcon.u = m_ServerSocket;
        stcon.name = (SOCKADDR *)&addrA;
        stcon.namelen = sizeof(SOCKADDR);
        stcon.nChannelID = m_stConnInfo.nChannel;
        stcon.nLVer_new = JVN_YSTVER;
        stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
        stcon.nMinTime = 2000;
        if(((m_nProtocolType == TYPE_PC_UDP) && m_stConnInfo.nWhoAmI != JVN_WHO_H && m_nStatus != NEW_HAVEIP)
           || ((m_nProtocolType == TYPE_3GMOHOME_UDP) && m_nStatus != NEW_VIRTUAL_IP))
        {
            stcon.uchLLTCP = 1;
        }
        if(m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
            stcon.uchLLTCP = 0;
        
        SOCKADDR_IN srv = {0};
        char strServer[100] = {0};
        memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
        
        	sprintf(strServer,"connectIP  connecting a  %d %s:%d  m_ServerSocket = %d line %d\n",m_stConnInfo.nLocalChannel,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
        	OutputDebug(strServer);
        writeLog(strServer);
        if(UDT::ERROR == UDT::connect(stcon))//1500
        {
            sprintf(ch2,"err:%s",(char *)UDT::getlasterror().getErrorMessage());
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 连接主控失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. connect op. failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            writeLog("************************ end connectIP connect failed %s %d = %s: %d, line: %d",
                     m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__);
		OutputDebug("connect err. %d  %d\t%d",m_stConnInfo.nLocalChannel,__LINE__,m_ServerSocket);
		return FALSE;
	}
	else
	{
            writeLog("************************ end connectIP connect ok %s %d = %s: %d, line: %d",
                 m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__);
		OutputDebug("connect ok. %d  %d\t%d",m_stConnInfo.nLocalChannel,__LINE__,m_ServerSocket);
		m_pWorker->AddHelpConnect(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_pWorker->m_WorkerUDPSocket,inet_ntoa(addrA.sin_addr),ntohs(addrA.sin_port));
		return TRUE;
	}
}

BOOL CCChannel::SendReCheck(BOOL bYST)
{//类型(1)+长度(4)+是否号码连接(1)+云视通号码(4)+是否作为高速缓存(1)+是否提速连接(1)
	int nLen = 7;
	BYTE data[20]={0};
	data[0] = JVN_REQ_RECHECK;
	memcpy(&data[1], &nLen, 4);
	if(bYST)
	{
		data[5] = 1;
		memcpy(&data[6], &m_stConnInfo.nYSTNO, 4);
	}
	if(m_stConnInfo.bCache)
	{
		data[10] = 1;
		m_bCache = TRUE;
	}
	else
	{
		m_bCache = FALSE;
	}

	if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
	{//TCP连接
		if(m_nProtocolType == TYPE_PC_TCP)
		{//pc连接时发送预验证信息
			if(0 >= tcpsenddata(m_ServerSocket, (char *)data, 12, TRUE))
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
	}
	else
	{//UDP连接
		//if(0 >= UDT::send(m_ServerSocket, (char *)data, 12, 0))
		if(0 >= udpsenddata(m_ServerSocket, (char *)data, 12, TRUE))
		{
			return FALSE;
		}
		else
		{
			OutputDebug("%d send recheck ok.",m_stConnInfo.nLocalChannel);
			return TRUE;
		}
	}

	return FALSE;
}

int CCChannel::RecvReCheck(int &nver, char chAVersion[MAX_PATH])
{
//	OutputDebug("RecvReCheck..........");
	if(m_nProtocolType != TYPE_PC_UDP && m_nProtocolType != TYPE_PC_TCP && m_nProtocolType != TYPE_MO_UDP && m_nProtocolType != TYPE_3GMO_UDP && m_nProtocolType != TYPE_3GMOHOME_UDP)
	{//手机TCP连接不需要预验证
		return -1;
	}
	short sVer1=0;
	short sVer2=0;
	short sVer3=0;
	short sVer4=0;
	int rs = 0;
	int rsize = 0;
	int nLen = 0;
	if(m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
	{//UDP连接
		m_nFYSTVER = UDT::getystverF(m_ServerSocket);//获取远端协议版本
		if(m_nFYSTVER >= JVN_YSTVER4)
		{//支持msg
			rs = UDT::recvmsg(m_ServerSocket, (char *)m_puchRecvBuf, JVNC_DATABUFLEN);
			//rs = 0;
			if(0 < rs)
			{//收到数据
				nLen=-1;
                writeLog("......................m_puchRecvBuf[0] %x, line: %d",m_puchRecvBuf[0],__LINE__);
				if(m_puchRecvBuf[0] == JVN_RSP_RECHECK)
				{//预验证 类型(1) + 长度(4) + 是否通过(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2) + LinkID(4) + 是否多播(1) + IsLan2A(1)
					rsize = 0;
					rs = 0;
					memcpy(&nLen, &m_puchRecvBuf[1], 4);
                    writeLog("......................nLen %d, line: %d",nLen,__LINE__);
					if(nLen == 15)
					{
						rsize = 0;
						rs = 0;

						memcpy(&sVer1, &m_puchRecvBuf[6], 2);
						memcpy(&sVer2, &m_puchRecvBuf[8], 2);
						memcpy(&sVer3, &m_puchRecvBuf[10], 2);
						memcpy(&sVer4, &m_puchRecvBuf[12], 2);
						memcpy(&m_nLinkID, &m_puchRecvBuf[14], 4);

						m_bJVP2P = (m_puchRecvBuf[18]==1)?TRUE:FALSE;
						m_bLan2A = (m_puchRecvBuf[19] == 1)?TRUE:FALSE;
						if(m_puchRecvBuf[18] != 0 && m_puchRecvBuf[18] != 1)
						{
                             writeLog("......................retrun , line: %d",__LINE__);
							return 0;
						}

						sprintf(chAVersion, "%d.%d.%d.%d", sVer1, sVer2, sVer3, sVer4);
						nver = CheckVersion(sVer1, sVer2, sVer3, sVer4);
						if(nver != 0)
						{
							//						return 0;
							//////////////////////////////////////////////////////////////////////////
							if(m_bJVP2P)
							{//设备端开启了多播，必须严格按照版本进行兼容
                                writeLog("......................retrun , line: %d",__LINE__);
								return 0;
							}
							//没开启多播，都是去走旧流程，返回合适的结果即可
							//////////////////////////////////////////////////////////////////////////
						}

						if(m_puchRecvBuf[5] == 1)
						{
                            writeLog("......................retrun , line: %d",__LINE__);
							return 1;
						}
						else
						{
                            writeLog("......................retrun , line: %d",__LINE__);
							return 0;
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_OVERLIMIT)
				{//超过最大连接数
					rsize = 0;
					rs = 0;

					memcpy(&nLen, &m_puchRecvBuf[1], 4);
					if(nLen == 0)
					{
						if(m_nStatus != WAIT_LRECHECK)
						{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
							m_bPass = FALSE;
							m_nStatus = FAILD;
							if(m_pWorker != NULL)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									char chMsg[] = "超过主控最大连接限制!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
								else
								{
									char chMsg[] = "client count limit!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}

							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_NOSERVER)
				{//无通道服务
					rsize = 0;
					rs = 0;

					memcpy(&nLen, &m_puchRecvBuf[1], 4);
					if(nLen == 0)
					{
						//					if(m_nStatus != WAIT_LRECHECK)//此处连接旧主控实惠短暂影响连接，暂时屏蔽
						{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
							m_bPass = FALSE;
							m_nStatus = FAILD;
							if(m_pWorker != NULL)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									char chMsg[] = "无该通道服务!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
								else
								{
									char chMsg[] = "channel is not open!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_INVALIDTYPE)
				{//连接类型无效
					rsize = 0;
					rs = 0;

					memcpy(&nLen, &m_puchRecvBuf[1], 4);
					if(nLen == 0)
					{
						m_bPass = FALSE;
						m_nStatus = FAILD;
						if(m_pWorker != NULL)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								char chMsg[] = "连接类型无效!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
							else
							{
								char chMsg[] = "connect type invalid!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_OLD)
				{//预验证 类型(1) + 长度(4) + 子类型(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2)
					rsize = 0;
					rs = 0;

					memcpy(&nLen, &m_puchRecvBuf[1], 4);
					if(nLen >0 && nLen < 1024)
					{
						rsize = 0;
						rs = 0;

						if(m_puchRecvBuf[5] == OLD_RSP_IMOLD)
						{
							memcpy(&sVer1, &m_puchRecvBuf[6], 2);
							memcpy(&sVer2, &m_puchRecvBuf[8], 2);
							memcpy(&sVer3, &m_puchRecvBuf[10], 2);
							memcpy(&sVer4, &m_puchRecvBuf[12], 2);

							return 2;
						}
					}
				}
			}
		}
		else
		{
			if(0 < (rs = UDT::recv(m_ServerSocket, (char *)m_puchRecvBuf, 1, 0)))
			{//收到数据
				nLen=-1;
                writeLog("......................m_puchRecvBuf[0] %x,line: %d",m_puchRecvBuf[0],__LINE__);
				if(m_puchRecvBuf[0] == JVN_RSP_RECHECK)
				{//预验证 类型(1) + 长度(4) + 是否通过(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2) + LinkID(4) + 是否多播(1) + IsLan2A(1)
					rsize = 0;
					rs = 0;
					while (rsize < 4)
					{
						if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
						{
                            writeLog("......................retrun , line: %d",__LINE__);
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&nLen, m_puchRecvBuf, 4);
					if(nLen == 15)
					{
						rsize = 0;
						rs = 0;
						while (rsize < 15)
						{
							if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 17 - rsize, 0)))
							{
                                writeLog("......................retrun , line: %d",__LINE__);
								return -1;
							}
							else if(rs == 0)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}

							rsize += rs;
						}

						memcpy(&sVer1, &m_puchRecvBuf[1], 2);
						memcpy(&sVer2, &m_puchRecvBuf[3], 2);
						memcpy(&sVer3, &m_puchRecvBuf[5], 2);
						memcpy(&sVer4, &m_puchRecvBuf[7], 2);
						memcpy(&m_nLinkID, &m_puchRecvBuf[9], 4);

						m_bJVP2P = (m_puchRecvBuf[13]==1)?TRUE:FALSE;
						m_bLan2A = (m_puchRecvBuf[14] == 1)?TRUE:FALSE;
						if(m_puchRecvBuf[13] != 0 && m_puchRecvBuf[13] != 1)
						{
							writeLog("......................retrun , line: %d",__LINE__);
                            return 0;
						}

						sprintf(chAVersion, "%d.%d.%d.%d", sVer1, sVer2, sVer3, sVer4);
						nver = CheckVersion(sVer1, sVer2, sVer3, sVer4);
						if(nver != 0)
						{
							//						return 0;
							//////////////////////////////////////////////////////////////////////////
							if(m_bJVP2P)
							{//设备端开启了多播，必须严格按照版本进行兼容
								writeLog("......................retrun , line: %d",__LINE__);
                                return 0;
							}
							//没开启多播，都是去走旧流程，返回合适的结果即可
							//////////////////////////////////////////////////////////////////////////
						}

						if(m_puchRecvBuf[0] == 1)
						{
							writeLog("......................retrun , line: %d",__LINE__);
                            return 1;
						}
						else
						{
							writeLog("......................retrun , line: %d",__LINE__);
                            return 0;
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_OVERLIMIT)
				{//超过最大连接数
					rsize = 0;
					rs = 0;
					while (rsize < 4)
					{
						if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
						{
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&nLen, m_puchRecvBuf, 4);
					if(nLen == 0)
					{
						if(m_nStatus != WAIT_LRECHECK)
						{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
							m_bPass = FALSE;
							m_nStatus = FAILD;
							if(m_pWorker != NULL)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									char chMsg[] = "超过主控最大连接限制!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
								else
								{
									char chMsg[] = "client count limit!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}

							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_NOSERVER)
				{//无通道服务
					rsize = 0;
					rs = 0;
					while (rsize < 4)
					{
						if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
						{
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&nLen, m_puchRecvBuf, 4);
					if(nLen == 0)
					{
						//					if(m_nStatus != WAIT_LRECHECK)//此处连接旧主控实惠短暂影响连接，暂时屏蔽
						{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
							m_bPass = FALSE;
							m_nStatus = FAILD;
							if(m_pWorker != NULL)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									char chMsg[] = "无该通道服务!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
								else
								{
									char chMsg[] = "channel is not open!";
									m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
								}
							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_INVALIDTYPE)
				{//连接类型无效
					rsize = 0;
					rs = 0;
					while (rsize < 4)
					{
						if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
						{
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&nLen, m_puchRecvBuf, 4);
					if(nLen == 0)
					{
						m_bPass = FALSE;
						m_nStatus = FAILD;
						if(m_pWorker != NULL)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								char chMsg[] = "连接类型无效!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
							else
							{
								char chMsg[] = "connect type invalid!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
						}
					}
				}
				else if(m_puchRecvBuf[0] == JVN_RSP_OLD)
				{//预验证 类型(1) + 长度(4) + 子类型(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2)
					rsize = 0;
					rs = 0;
					while (rsize < 4)
					{
						if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
						{
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&nLen, m_puchRecvBuf, 4);
					if(nLen >0 && nLen < 1024)
					{
						rsize = 0;
						rs = 0;
						while (rsize < nLen)
						{
							if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], nLen - rsize, 0)))
							{
								return -1;
							}
							else if(rs == 0)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}

							rsize += rs;
						}

						if(m_puchRecvBuf[0] == OLD_RSP_IMOLD)
						{
							memcpy(&sVer1, &m_puchRecvBuf[1], 2);
							memcpy(&sVer2, &m_puchRecvBuf[3], 2);
							memcpy(&sVer3, &m_puchRecvBuf[5], 2);
							memcpy(&sVer4, &m_puchRecvBuf[7], 2);

							return 2;
						}
					}
				}
			}
		}
	}
	else
	{//TCP连接
		if(0 < (rs = tcpreceive(m_ServerSocket, (char *)m_puchRecvBuf, 1, 1)))
		{//收到数据
			nLen=-1;
//			if(m_puchRecvBuf[0] != JVN_CMD_KEEPLIVE && m_puchRecvBuf[0] != 0)
//			{
//				int lll=0;
//			}
			if(m_puchRecvBuf[0] == JVN_RSP_RECHECK)
			{//预验证 类型(1) + 长度(4) + 是否通过(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2) + LinkID(4) + 是否多播(1) + IsLan2A(1)
				rsize = 0;
				rs = 0;
				while (rsize < 4)
				{
					if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 1)))
					{
						return -1;
					}
					else if(rs == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}

					rsize += rs;
				}

				memcpy(&nLen, m_puchRecvBuf, 4);
				if(nLen == 15)
				{
					rsize = 0;
					rs = 0;
					while (rsize < 15)
					{
						if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 17 - rsize, 1)))
						{
							return -1;
						}
						else if(rs == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}

						rsize += rs;
					}

					memcpy(&sVer1, &m_puchRecvBuf[1], 2);
					memcpy(&sVer2, &m_puchRecvBuf[3], 2);
					memcpy(&sVer3, &m_puchRecvBuf[5], 2);
					memcpy(&sVer4, &m_puchRecvBuf[7], 2);
					memcpy(&m_nLinkID, &m_puchRecvBuf[9], 4);

					m_bJVP2P = (m_puchRecvBuf[13]==1)?TRUE:FALSE;
					m_bLan2A = (m_puchRecvBuf[14] == 1)?TRUE:FALSE;
					if(m_puchRecvBuf[13] != 0 && m_puchRecvBuf[13] != 1)
					{
						return 0;
					}

					sprintf(chAVersion, "%d.%d.%d.%d", sVer1, sVer2, sVer3, sVer4);
					nver = CheckVersion(sVer1, sVer2, sVer3, sVer4);
					if(nver != 0)
					{
//						return 0;
						//////////////////////////////////////////////////////////////////////////
						if(m_bJVP2P)
						{//设备端开启了多播，必须严格按照版本进行兼容
							return 0;
						}
						//没开启多播，都是去走旧流程，返回合适的结果即可
						//////////////////////////////////////////////////////////////////////////
					}

					if(m_puchRecvBuf[0] == 1)
					{
						return 1;
					}
					else
					{
						return 0;
					}
				}
			}
			else if(m_puchRecvBuf[0] == JVN_RSP_OVERLIMIT)
			{//超过最大连接数
				rsize = 0;
				rs = 0;
				while (rsize < 4)
				{
					if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 1)))
					{
						return -1;
					}
					else if(rs == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}

					rsize += rs;
				}

				memcpy(&nLen, m_puchRecvBuf, 4);
				if(nLen == 0)
				{
					if(m_nStatus != WAIT_LRECHECK)
					{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
						m_bPass = FALSE;
						m_nStatus = FAILD;
						if(m_pWorker != NULL)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								char chMsg[] = "超过主控最大连接限制!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
							else
							{
								char chMsg[] = "client count limit!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}

						}
					}
				}
			}
			else if(m_puchRecvBuf[0] == JVN_RSP_NOSERVER)
			{//无通道服务
				rsize = 0;
				rs = 0;
				while (rsize < 4)
				{
					if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 1)))
					{
						return -1;
					}
					else if(rs == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}

					rsize += rs;
				}

				memcpy(&nLen, m_puchRecvBuf, 4);
				if(nLen == 0)
				{
					if(m_nStatus != WAIT_LRECHECK)
					{//内网探测中这种提示忽略，继续进行外网尝试，其他情况可以直接返回连接失败了
						m_bPass = FALSE;
						m_nStatus = FAILD;
						if(m_pWorker != NULL)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								char chMsg[] = "无该通道服务!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}
							else
							{
								char chMsg[] = "channel is not open!";
								m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
							}

						}
					}
				}
			}
			else if(m_puchRecvBuf[0] == JVN_RSP_INVALIDTYPE)
			{//连接类型无效
				rsize = 0;
				rs = 0;
				while (rsize < 4)
				{
					if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 1)))
					{
						return -1;
					}
					else if(rs == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}

					rsize += rs;
				}

				memcpy(&nLen, m_puchRecvBuf, 4);
				if(nLen == 0)
				{
					m_bPass = FALSE;
					m_nStatus = FAILD;
					if(m_pWorker != NULL)
					{
						if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
						{
							char chMsg[] = "连接类型无效!";
							m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
						}
						else
						{
							char chMsg[] = "connect type invalid!";
							m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
						}
					}
				}
			}
		}
	}

	return -1;
}

BOOL CCChannel::SendPWCheck()
{//类型(1)+长度(4)+用户名长(4)+密码长(4)+用户名+密码
	int nNLen = strlen(m_stConnInfo.chPassName);
	if(nNLen > MAX_PATH)
	{
		nNLen = 0;
	}
	int nWLen = strlen(m_stConnInfo.chPassWord);
	if(nWLen > MAX_PATH)
	{
		nWLen = 0;
	}

	BYTE data[3*MAX_PATH]={0};
	data[0] = JVN_REQ_CHECKPASS;
	memcpy(&data[1], &nNLen, 4);
	memcpy(&data[5], &nWLen, 4);
	if(nNLen > 0 && nNLen < MAX_PATH)
	{
		memcpy(&data[9], m_stConnInfo.chPassName, nNLen);
	}
	if(nWLen > 0 && nWLen < MAX_PATH)
	{
		memcpy(&data[9+nNLen], m_stConnInfo.chPassWord, nWLen);
	}
         OutputDebug("----------------name: %s, pwd: %s, line: %d, function: %s",m_stConnInfo.chPassName,m_stConnInfo.chPassWord,__LINE__,__FUNCTION__);
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            if(0 >= tcpsenddata(m_ServerSocket, (char *)data, nNLen + nWLen + 9, TRUE))
            {
                return FALSE;
            }
            else
            {
                //			OutputDebug("send pwd");
                return TRUE;
            }
        }
        else
        {//UDP连接
            if(m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP)
            {//手机连接新主控时需要告知对方这是手机连接
                char chtmp[10]={0};
                chtmp[0] = JVN_CMD_MOTYPE;
                udpsenddata(m_ServerSocket, chtmp, 5, TRUE);
            }
            
            if(0 >= udpsenddata(m_ServerSocket, (char *)data, nNLen + nWLen + 9, TRUE))
            {
                return FALSE;
            }
            else
            {
                //			OutputDebug("send pwd");
                return TRUE;
            }
        }
    }
    
    int CCChannel::RecvPWCheck(int &nPWData)
    {
        int rs = 0;
        int rsize = 0;
        int nLen = 0;
        nPWData = 0;
        
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            if(0 < (rs = tcpreceive(m_ServerSocket, (char *)m_puchRecvBuf, 1, 1)))
            {//收到数据
                nLen=-1;
                if(m_puchRecvBuf[0] == JVN_RSP_CHECKPASS)
                {//身份验证 类型(1) + 长度(4) + 是否通过(1) + [附加值(4)]
                    rsize = 0;
                    rs = 0;
                    while (rsize < 4)
                    {
                        if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 1)))
                        {
                            return -1;
                        }
                        else if(rs == 0)
                        {
                            CCWorker::jvc_sleep(1);
                            continue;
                        }
                        
                        rsize += rs;
                    }
                    
                    memcpy(&nLen, m_puchRecvBuf, 4);
                    if(nLen == 1 || nLen == 5)
                    {
                        rsize = 0;
                        rs = 0;
                        while (rsize < nLen)
                        {
                            if(0 > (rs = tcpreceive(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], nLen - rsize, 1)))
                            {
                                return -1;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        if(nLen == 5)
                        {//新协议 带附加值
                            memcpy(&nPWData, &m_puchRecvBuf[1], 4);
                        }
                        
                        if(m_puchRecvBuf[0] == 1)
                        {
                            return 1;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                }
            }
        }
        else
        {//UDP连接
            m_nFYSTVER = UDT::getystverF(m_ServerSocket);//获取远端协议版本
            if(m_nFYSTVER >= JVN_YSTVER4)
            {//支持msg
                if(0 < (rs = UDT::recvmsg(m_ServerSocket, (char *)m_puchRecvBuf, JVNC_DATABUFLEN)))
                {//收到数据
                    nLen=-1;
                    if(m_puchRecvBuf[0] == JVN_RSP_CHECKPASS)
                    {//身份验证 类型(1) + 长度(4) + 是否通过(1) + [附加值(4)]
                        rsize = 0;
                        rs = 0;
                        
                        memcpy(&nLen, &m_puchRecvBuf[1], 4);
                        if(nLen == 1 || nLen == 5)
                        {
                            rsize = 0;
                            rs = 0;
                            
                            if(nLen == 5)
                            {//新协议 带附加值
                                memcpy(&nPWData, &m_puchRecvBuf[6], 4);
                            }
                            
                            if(m_puchRecvBuf[5] == 1)
                            {
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                    }
                }
            }
            else
            {
                if(0 < (rs = UDT::recv(m_ServerSocket, (char *)m_puchRecvBuf, 1, 0)))
                {//收到数据
                    nLen=-1;
                    if(m_puchRecvBuf[0] == JVN_RSP_CHECKPASS)
                    {//身份验证 类型(1) + 长度(4) + 是否通过(1) + [附加值(4)]
                        rsize = 0;
                        rs = 0;
                        while (rsize < 4)
                        {
                            if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], 4 - rsize, 0)))
                            {
                                return -1;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        memcpy(&nLen, m_puchRecvBuf, 4);
                        if(nLen == 1 || nLen == 5)
                        {
                            rsize = 0;
                            rs = 0;
                            while (rsize < nLen)
                            {
                                if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&m_puchRecvBuf[rsize], nLen - rsize, 0)))
                                {
                                    return -1;
                                }
                                else if(rs == 0)
                                {
                                    CCWorker::jvc_sleep(1);
                                    continue;
                                }
                                
                                rsize += rs;
                            }
                            
                            if(nLen == 5)
                            {//新协议 带附加值
                                memcpy(&nPWData, &m_puchRecvBuf[1], 4);
                            }
                            
                            if(m_puchRecvBuf[0] == 1)
                            {
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        
        return -1;
    }
    
    BOOL CCChannel::StartWorkThread()
    {
#ifndef WIN32
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            pthread_attr_t attr;
            pthread_attr_t *pAttr = &attr;
            unsigned long size = LINUX_THREAD_STACK_SIZE;
            size_t stacksize = size;
            pthread_attr_init(pAttr);
            if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
            {
                pAttr = NULL;
            }
            
            if (0 != pthread_create(&m_hRecvThread, pAttr, RecvProcTCP, this))
            {
                m_hRecvThread = 0;
                if(m_pWorker != NULL)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建接收线程失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create receive thread failed.", __FILE__,__LINE__);
                    }
                }
                
                return FALSE;
            }
        }
        else
        {
            pthread_attr_t attr;
            pthread_attr_t *pAttr = &attr;
            unsigned long size = LINUX_THREAD_STACK_SIZE;
            size_t stacksize = size;
            pthread_attr_init(pAttr);
            if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
            {
                pAttr = NULL;
            }
            m_recvThreadExit = FALSE;
            if (0 != pthread_create(&m_hRecvThread, pAttr, RecvProc, this))
            {
                m_recvThreadExit=TRUE;
                m_hRecvThread = 0;
                if(m_pWorker != NULL)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建接收线程失败", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create receive thread failed.", __FILE__,__LINE__);
                    }
                }
                
                return FALSE;
            }
        }
        
#else
        //启动接收线程
        UINT unTheadID;
        //创建本地监听线程
        m_hStartEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvProcTCP, (void *)this, 0, &unTheadID);
        }
        else
        {
            m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvProc, (void *)this, 0, &unTheadID);
        }
        SetEvent(m_hStartEventR);
        if (m_hRecvThread == 0)//创建线程失败
        {
            //清理Recv线程
            if(m_hStartEventR > 0)
            {
                CloseHandle(m_hStartEventR);
                m_hStartEventR = 0;
            }
            
            if(m_hEndEventR > 0)
            {
                CloseHandle(m_hEndEventR);
                m_hEndEventR = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建接收线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create receive thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
#endif
        
        if(!m_bJVP2P)
        {//主控不支持多播 无需伙伴收发线程
            return TRUE;
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
        if (0 != pthread_create(&m_hPartnerThread, pAttr, PartnerProc, this))
        {
            //清理PT线程
            m_hPartnerThread = 0;
            
            //清理Recv线程
            if (0 != m_hRecvThread)
            {
                m_bEndR = TRUE;
                pthread_join(m_hRecvThread, NULL);
                m_hRecvThread = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 创建伙伴下载线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create pt thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
        if (0 != pthread_create(&m_hPTCThread, pAttr, PTConnectProc, this))
        {
            //清理PTC线程
            m_hPTCThread = 0;
            
            //清理Recv线程
            if (0 != m_hRecvThread)
            {
                m_bEndR = TRUE;
                pthread_join(m_hRecvThread, NULL);
                m_hRecvThread = 0;
            }
            
            //清理Partner线程
            if (0 != m_hPartnerThread)
            {
                m_bEndPT = TRUE;
                pthread_join(m_hPartnerThread, NULL);
                m_hPartnerThread = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 创建伙伴连接线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create ptc thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
#else
        //创建本地伙伴处理线程
        m_hStartEventPT = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventPT = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hPartnerThread = (HANDLE)_beginthreadex(NULL, 0, PartnerProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventPT);
        if (m_hPartnerThread == 0)//创建线程失败
        {
            //清理RecvP线程
            if(m_hStartEventPT > 0)
            {
                CloseHandle(m_hStartEventPT);
                m_hStartEventPT = 0;
            }
            if(m_hEndEventPT > 0)
            {
                CloseHandle(m_hEndEventPT);
                m_hEndEventPT = 0;
            }
            
            //清理Recv线程
            if(m_hEndEventR > 0)
            {
                SetEvent(m_hEndEventR);
                CCWorker::jvc_sleep(10);
                WaitThreadExit(m_hRecvThread);
                
                if(m_hRecvThread > 0)
                {
                    CloseHandle(m_hRecvThread);
                    m_hRecvThread = 0;
                }
                
                CloseHandle(m_hEndEventR);
                m_hEndEventR = 0;
            }
            if(m_hStartEventR > 0)
            {
                CloseHandle(m_hStartEventR);
                m_hStartEventR = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 创建伙伴下载线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create pt thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
        
        //创建本地伙伴连接线程
        m_hStartEventPTC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventPTC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hPTCThread = (HANDLE)_beginthreadex(NULL, 0, PTConnectProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventPTC);
        if (m_hPTCThread == 0)//创建线程失败
        {
            //清理PTC线程
            if(m_hStartEventPTC > 0)
            {
                CloseHandle(m_hStartEventPTC);
                m_hStartEventPTC = 0;
            }
            if(m_hEndEventPTC > 0)
            {
                CloseHandle(m_hEndEventPTC);
                m_hEndEventPTC = 0;
            }
            
            //清理Recv线程
            if(m_hEndEventR > 0)
            {
                SetEvent(m_hEndEventR);
                CCWorker::jvc_sleep(10);
                WaitThreadExit(m_hRecvThread);
                
                if(m_hRecvThread > 0)
                {
                    CloseHandle(m_hRecvThread);
                    m_hRecvThread = 0;
                }
                
                CloseHandle(m_hEndEventR);
                m_hEndEventR = 0;
            }
            if(m_hStartEventR > 0)
            {
                CloseHandle(m_hStartEventR);
                m_hStartEventR = 0;
            }
            
            //清理Partner线程
            if(m_hEndEventPT > 0)
            {
                SetEvent(m_hEndEventPT);
                CCWorker::jvc_sleep(10);
                WaitThreadExit(m_hPartnerThread);
                
                if(m_hPartnerThread > 0)
                {
                    CloseHandle(m_hPartnerThread);
                    m_hPartnerThread = 0;
                }
                
                CloseHandle(m_hEndEventPT);
                m_hEndEventPT = 0;
            }
            if(m_hStartEventPT > 0)
            {
                CloseHandle(m_hStartEventPT);
                m_hStartEventPT = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 创建伙伴连接线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create ptc thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
#endif
        
        return TRUE;
    }
    
    BOOL CCChannel::StartHelpWorkThread()
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
        
        if (0 != pthread_create(&m_hRecvThread, pAttr, RecvHelpProc, this))
        {
            m_hRecvThread = 0;
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建接收线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create receive thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
#else
        //启动接收线程
        UINT unTheadID;
        //创建本地监听线程
        m_hStartEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvHelpProc, (void *)this, 0, &unTheadID);
        
        SetEvent(m_hStartEventR);
        if (m_hRecvThread == 0)//创建线程失败
        {
            //清理Recv线程
            if(m_hStartEventR > 0)
            {
                CloseHandle(m_hStartEventR);
                m_hStartEventR = 0;
            }
            
            if(m_hEndEventR > 0)
            {
                CloseHandle(m_hEndEventR);
                m_hEndEventR = 0;
            }
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建接收线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create receive thread failed.", __FILE__,__LINE__);
                }
            }
            
            return FALSE;
        }
#endif
        
        return TRUE;
    }
    
    BOOL CCChannel::SendSP2P(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            return SendSP2PTCP(addrs, nIndex, pchError);
        }
        return TRUE;
    }
    
    int CCChannel::RecvS(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            return RecvSTCP(nIndex, pchError);
        }
        
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
        int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
        {//绑定到指定端口失败，改为绑定到随机端口
            OutputDebug("************************************Channel  line: %d",__LINE__);
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return FALSE;
        }
        
        //将套接字置为非阻塞模式
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        
        //////////////////////////////////////////////////////////////////////////
        //获取当前客户端在内网和外网所表现出来的所有地址
        int nSVer = 0;
        m_NATListTEMP.clear();
        m_pWorker->GetNATADDR(m_NatList, &m_NATListTEMP, addrs, nSVer);
        //////////////////////////////////////////////////////////////////////////
        
        char chdata[1024]={0};
        //构造NAT列表
        BYTE strIP[1024]  = {0};
        int maxIP = m_NATListTEMP.size() > 20 ?20:m_NATListTEMP.size();
        int nSize = maxIP * 6;
        for(int k = 0;k < maxIP;k ++)
        {
            memcpy(&strIP[k * 6 + 0],m_NATListTEMP[k].ip,4);
            memcpy(&strIP[k * 6 + 4],&m_NATListTEMP[k].port,2);
        }
        
        if(nSVer < 20336)
        {//主控端太旧，不支持新数据，采用原流程
            nSize = 0;
        }
#ifndef MOBILE_CLIENT
        for(int m = 0;m < 3;m ++)
        {
            m_pWorker->SendUdpData(m_pWorker->m_WorkerUDPSocket,m_SList,m_stConnInfo.nYSTNO,strIP,nSize);
#else
            for(int m = 0;m < m_SList.size();m ++)
            {
                m_pWorker->SendUdpDataForMobile(m_pWorker->m_WorkerUDPSocket,m_SList[m].addr,m_stConnInfo.nYSTNO,strIP,nSize);
                
#endif
                CCWorker::jvc_sleep(10);
                
                UDP_LIST list;
                int nret = m_pWorker->GetUdpData(m_pWorker->m_WorkerUDPSocket,m_stConnInfo.nYSTNO,&list);
                if(nret > 0)
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
                            if(m_ServerSocket > 0)
                            {
                                m_pWorker->pushtmpsock(m_ServerSocket);
                                m_ServerSocket = 0;
                            }
                            
                            m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                            
                            BOOL bReuse = TRUE;
                            UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                            //////////////////////////////////////////////////////////////////////////
                            int len1 = JVC_MSS;
                            UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                            //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                            len1=1500*1024;
                            UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                            
                            len1=1000*1024;
                            UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                            if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
                            {OutputDebug("************************************Channel  line: %d",__LINE__);
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                }
                                m_ServerSocket = 0;
                                
                                if(m_pWorker->m_bNeedLog)
                                {
                                    char chMsg[MAX_PATH]={0};
                                    sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex);
                                    strcat(pchError, chMsg);
                                    strcat(pchError, UDT::getlasterror().getErrorMessage());
                                    strcat(pchError, ">**");
                                }
                                
                                return JVN_RSP_CONNAF;
                            }
                            //将套接字置为非阻塞模式
                            BOOL block = FALSE;
                            UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                            UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                            LINGER linger;
                            linger.l_onoff = 0;
                            linger.l_linger = 0;
                            UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
                            return JVN_CMD_TRYTOUCH;
                        }
                        else if(nType == YST_A_NEW_ADDRESS)
                        {
                            memcpy(&m_addrAN, &pkg.cData[8], sizeof(SOCKADDR_IN));
                            m_addrAN.sin_family = AF_INET;
                            
                            memcpy(&m_addrAL, &pkg.cData[8 + sizeof(SOCKADDR_IN)], sizeof(SOCKADDR_IN));
                            m_addrAL.sin_family = AF_INET;
                            
                            return JVN_RSP_CONNA;
                        }
                        else if(nType == JVN_RSP_CONNA)
                        {
                            memcpy(&m_addrANOLD, &pkg.cData[8], sizeof(SOCKADDR_IN));
                            m_addrANOLD.sin_family = AF_INET;
                            
                            DWORD dwCurr = CCWorker::JVGetTime();
                            if(dwCurr > m_dwStartTime + 500)//超时 连接原NAT
                            {
                                return JVN_RSP_CONNA;
                            }
                            return YST_A_NEW_ADDRESS;//没超时 外部还会再次等新nat地址
                        }
                    }
                }
                
            }
            
            return -1;//JVN_RSP_CONNAF;
        }
        
        BOOL CCChannel::ConnectLocalTry(int nIndex, char *pchError)
        {
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP连接
                //创建套接字
                SOCKET Sockettmp = socket(AF_INET, SOCK_STREAM,0);
                
                if(m_SocketSTmp > 0)
                {
                    closesocket(m_SocketSTmp);
                }
                m_SocketSTmp = 0;
                
                if(m_ServerSocket > 0)
                {
                    if((m_stConnInfo.nConnectType == TYPE_PC_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP) && m_nProtocolType == TYPE_PC_TCP)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    else
                    {
                        closesocket(m_ServerSocket);
                    }
                }
                m_ServerSocket = 0;
                m_ServerSocket = Sockettmp;
                
                SOCKADDR_IN addrSrv;
#ifndef WIN32
                addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
                addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
                addrSrv.sin_family = AF_INET;
                addrSrv.sin_port = htons(0);//(m_nLocalStartPort);
                
                int nReuse = 1;
                setsockopt(Sockettmp, SOL_SOCKET, SO_REUSEADDR, (char *)&nReuse, sizeof(int));
                
                //绑定套接字
#ifndef WIN32
                if(bind(m_ServerSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)) < 0)
#else
                    if(SOCKET_ERROR == bind(m_ServerSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)))
#endif
                    {
#ifdef WIN32
                        int kkk=WSAGetLastError();
#endif
                        closesocket(m_ServerSocket);
                        m_ServerSocket = 0;
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[MAX_PATH]={0};
#ifndef WIN32
                            sprintf(chMsg,"<[S%d]bind sock failed,e:%d",nIndex, errno);
#else
                            sprintf(chMsg,"<[S%d]bind sock failed,e:%d",nIndex, kkk);
#endif
                            strcat(pchError, chMsg);
                            strcat(pchError, ">**");
                        }
                        
                        return JVN_RSP_CONNAF;
                    }
                
                //将套接字置为非阻塞模式
                int iSockStatus = 0;
#ifndef WIN32
                int flags=0;
                flags = fcntl(m_ServerSocket, F_GETFL, 0);
                fcntl(m_ServerSocket, F_SETFL, flags | O_NONBLOCK);
#else
                unsigned long ulBlock = 1;
                iSockStatus = ioctlsocket(m_ServerSocket, FIONBIO, (unsigned long*)&ulBlock);
#endif
                
                
                //将套接字置为不等待未处理完的数据
                LINGER linger;
                linger.l_onoff = 1;//0;
                linger.l_linger = 0;
                iSockStatus = setsockopt(m_ServerSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
                
                int nSetSize = 128 * 1024;
                setsockopt(m_ServerSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
                nSetSize = 128 * 1024;
                setsockopt(m_ServerSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));
                
                if(0 != connectnb(m_ServerSocket,(SOCKADDR *)&m_addrAL, sizeof(SOCKADDR), 10))//3))
                {
                    if(m_pWorker->m_bNeedLog)
                    {
#ifndef WIN32
                        int kkk = errno;
#else
                        int kkk=WSAGetLastError();
#endif
                        
                        char chMsg[3*MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 连接主控失败 详细e:%d",nIndex,kkk);
                            strcat(pchError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. connect op. failed. e:%d",nIndex,kkk);
                            strcat(pchError, chMsg);
                        }
                    }
                    
                    return FALSE;
                }
                else
                {
                    //tcp连接首先发送通道号等信息
                    //初步连接成功 发送 通道号(4)+连接类型(4) 信息
                    BYTE datatmp[3*MAX_PATH]={0};
                    memcpy(&datatmp[0], &m_stConnInfo.nChannel, 4);
                    memcpy(&datatmp[4], &m_nProtocolType, 4);
                    
                    tcpsenddata(m_ServerSocket, (char *)datatmp, 8, TRUE);
                    
                    return TRUE;
                }
            }
            else
            {//UDP连接
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                BOOL bReuse = TRUE;
                UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                //////////////////////////////////////////////////////////////////////////
		int len1 = JVC_MSS;
                UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                len1=1500*1024;
                UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                
                len1=1000*1024;
                UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
                {//绑定到指定端口失败，改为绑定到随机端口
                    OutputDebug("************************************Channel  line: %d",__LINE__);
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                    
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    
                    return FALSE;
                }
                
                //将套接字置为非阻塞模式
                BOOL block = FALSE;
                UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                LINGER linger;
                linger.l_onoff = 0;
                linger.l_linger = 0;
                UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
                
                STJUDTCONN stcon;
                stcon.u = m_ServerSocket;
                stcon.name = (SOCKADDR *)&m_addrAL;
                stcon.namelen = sizeof(SOCKADDR);
                stcon.nChannelID = m_stConnInfo.nChannel;
                stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
                memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
                stcon.nLVer_new = JVN_YSTVER;
                stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
                stcon.nMinTime = 500;
                if((m_nProtocolType == TYPE_PC_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP) && m_stConnInfo.nWhoAmI != JVN_WHO_H && m_nStatus != NEW_HAVEIP && m_stConnInfo.nWhoAmI != JVN_WHO_M)
                {
                    stcon.uchLLTCP = 1;
                }
                //		SOCKADDR_IN srv = {0};
                //		char strServer[100] = {0};
                //		memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
                
                //		sprintf(strServer,"connecting a %s:%d  m_ServerSocket = %d line %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
                //		OutputDebug(strServer);
                if(UDT::ERROR == UDT::connect(stcon))//1000
                {
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[3*MAX_PATH]={0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]内网探测失败. 连接主控失败 详细:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pchError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]LocalTry failed. connect op. failed. INFO:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                            strcat(pchError, chMsg);
                        }
                    }
                    
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                    
                    memcpy(&m_addrLastLTryAL, &m_addrAL, sizeof(SOCKADDR_IN));
                    
                    //////////////////////////////////////////////////////////////////////////
                    //char ch[100]={0};
                    //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
                    //sprintf(ch,"AN:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                    //////////////////////////////////////////////////////////////////////////
                    
                    return FALSE;
                }
                else
                {
                    //			OutputDebug("connect ok. %d  %d\t%d",m_stConnInfo.nLocalChannel,__LINE__,m_ServerSocket);
                    m_pWorker->AddHelpConnect(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_pWorker->m_WorkerUDPSocket,inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
                    return TRUE;
                }
            }
        }
        
        BOOL CCChannel::ConnectNet(int nIndex, char *pchError)
        {
            if(m_pWorker != NULL)
            {//测试提示
                m_pWorker->NormalData(m_nLocalChannel,0x20,(BYTE*)&m_addrAN, sizeof(SOCKADDR_IN), 0,0);
            }
            
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP连接
                return ConnectNetTCP(nIndex, pchError);
            }
            
            //////////////////////////////////////////////////////////////////////////
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
            BOOL bReuse = TRUE;
            UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
            //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
            UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
            //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
            len1=1500*1024;
            UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
            
            len1=1000*1024;
            UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
            if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
            {//绑定到指定端口失败，改为绑定到随机端口
                OutputDebug("************************************Channel  line: %d",__LINE__);
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                
                return FALSE;
            }
            
            //将套接字置为非阻塞模式
            BOOL block = FALSE;
            UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
            UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
            LINGER linger;
            linger.l_onoff = 0;
            linger.l_linger = 0;
            UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            //char ch[100]={0};
            //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
            //////////////////////////////////////////////////////////////////////////
            
            STJUDTCONN stcon;
            stcon.u = m_ServerSocket;
            stcon.name = (SOCKADDR *)&m_addrAN;
            stcon.namelen = sizeof(SOCKADDR);
            stcon.nChannelID = m_stConnInfo.nChannel;
            stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
            memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
            stcon.nLVer_new = JVN_YSTVER;
            stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
            stcon.nMinTime = 2000;
            
            SOCKADDR_IN srv = {0};
            char strServer[100] = {0};
            memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
            
            	sprintf(strServer,"connectNet connecting a %s:%d  m_ServerSocket = %d line %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
            	OutputDebug(strServer);
            writeLog(strServer);
            if(UDT::ERROR == UDT::connect(stcon))//3000
            {
                if(m_addrANOLD.sin_family != 0)
                {
                    stcon.name = (SOCKADDR *)&m_addrANOLD;
                    //	SOCKADDR_IN srv = {0};
                    //	char strServer[100] = {0};
                    memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
                    
                    //	sprintf(strServer,"connecting a %s:%d  m_ServerSocket = %d line %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
                    //			OutputDebug(strServer);
                    if(UDT::ERROR == UDT::connect(stcon))
                    {
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            sprintf(chMsg,"<[S%d]Net connect failed,Info:%s>**",nIndex, UDT::getlasterror().getErrorMessage());
                            strcat(pchError, chMsg);
                        }
                        writeLog("**************************connectnet connect failed %s %d = %s: %d, line: %d",
                                 m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__);
                        return FALSE;
                    }
                    m_pWorker->AddHelpConnect(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_pWorker->m_WorkerUDPSocket,inet_ntoa(m_addrANOLD.sin_addr),ntohs(m_addrANOLD.sin_port));
                }
                else
                {
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[3*MAX_PATH]={0};
                        sprintf(chMsg,"<[S%d]Net connect failed,Info:%s>**",nIndex, UDT::getlasterror().getErrorMessage());
                        strcat(pchError, chMsg);
                    }
                    writeLog("**************************connectnet connect failed %s %d = %s: %d, line: %d",
                             m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__);
                    return FALSE;
                }
            }
            else
            {
                m_pWorker->AddHelpConnect(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_pWorker->m_WorkerUDPSocket,inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
            }
            
            //	OutputDebug("connect ok. %d  %d\t%d",m_stConnInfo.nLocalChannel,__LINE__,m_ServerSocket);
            writeLog("**************************connectnet connect ok %s %d = %s: %d, line: %d",
                     m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),__LINE__);
            return TRUE;
        }
        
        BOOL CCChannel::ConnectNetTry(SOCKADDR_IN addrs, int nIndex, char *pchError)
        {
            //预测部分做的不好 暂时停用
            //////////////////////////////////////////////////////////////////////////
            return FALSE;
            //////////////////////////////////////////////////////////////////////////
            
            return TRUE;
        }
        
        BOOL CCChannel::SendSTURN(SOCKADDR_IN addrs, int nIndex, char *pchError)
        {
            
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP连接
                return FALSE;//SendSTURNTCP(addrs, nIndex, pchError);
            }
            
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            //创建临时UDP套接字
            SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
            
            SOCKADDR_IN addrSrv;
#ifndef WIN32
            addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
            addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
            addrSrv.sin_family = AF_INET;
            addrSrv.sin_port = htons(0);//htons(m_nLocalStartPort);
            
            BOOL bReuse = TRUE;
            setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
//            int size = 1024*100;
//            setsockopt(stmp, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int));
//            setsockopt(stmp, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int));
            //绑定套接字
           int bindResult =  bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
            OutputDebug("SendSTURN bind result: %d",bindResult);
            //	BOOL bReuse = TRUE;
            //	setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
            
            //向服务器请求TS地址
            //向S发送请求
            BYTE data[JVN_ASPACKDEFLEN]={0};
            int nType = JVN_REQ_S2;
            if(m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
            {//3G手机连接采用独立的流程
                nType = JVN_REQ_MOS2;
            }
            
            memcpy(&data[0], &nType, 4);
            memcpy(&data[4], &m_stConnInfo.nYSTNO, 4);
            data[8] = (m_stConnInfo.nTURNType == JVN_ONLYTURN)?1:0;
            data[9] = m_stConnInfo.nVIP;
            char str[50] = {0};
            sprintf(str,"%d Req Turn %s:%d line %d\r\n",data[9],inet_ntoa(addrs.sin_addr),ntohs(addrs.sin_port),__LINE__);
            
            OutputDebug(str);
            writeLog(str);
	memcpy(&m_RequestTurnAddr,&addrs,sizeof(SOCKADDR));
	memcpy(m_strRequestTurnData,data,10);
	m_sRequestTurn = stmp;

	if(sendtoclient(stmp, (char *)data, 10, 0, (SOCKADDR *)&(addrs), sizeof(SOCKADDR),2) > 0)
	{
		if(m_SocketSTmp > 0)
		{
			closesocket(m_SocketSTmp);
		}
		m_SocketSTmp = stmp;
		return TRUE;
	}
	else
	{
	#ifdef WIN32
		if(WSAGetLastError() == 10038)
		{
			if(stmp > 0)
			{
				closesocket(stmp);
			}
			if(m_SocketSTmp > 0)
			{
				closesocket(m_SocketSTmp);
			}
			stmp = 0;
			m_SocketSTmp = 0;

			if(SendSTURN(addrs, nIndex, pchError))//由于套接字连续开启关闭及重用上还有问题，此处多执行为了提高成功率，不过有风险
			{
				return TRUE;
			}
		}
	#endif

		char chMsg[MAX_PATH]={0};
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			sprintf(chMsg,"<[S%d]向服务器发送数据TS失败>**",nIndex);
			strcat(pchError, chMsg);
		}
		else
		{
			sprintf(chMsg,"<[S%d]send data TS to server failed>**",nIndex);
			strcat(pchError, chMsg);
		}

		if(stmp > 0)
		{
			closesocket(stmp);
		}
		if(m_SocketSTmp > 0)
		{
			closesocket(m_SocketSTmp);
		}
		stmp = 0;
		m_SocketSTmp = 0;

		return FALSE;
	}
}


int CCChannel::RecvSTURN(int nIndex, char *pchError)
{
	if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
	{//TCP连接
		return -1;//RecvSTCP();
	}

	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = JVN_CMD_CONNS2;
	memset(data, 0, JVN_ASPACKDEFLEN);
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	int nrecvlen=0;
	if((nrecvlen = receivefromm(m_SocketSTmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
	{
		memcpy(&nType, &data[0], 4);
		if(nType == JVN_CMD_CONNS2)
		{//收到服务器成功数据
			memcpy(&m_addrTS, &data[8], sizeof(SOCKADDR_IN));
			//////////////////////////////////////////////////////////////////////////
//			char ch[100]={0};
//			sprintf(ch,"TS:%s:%d\n",inet_ntoa(m_addrTS.sin_addr),ntohs(m_addrTS.sin_port));
#ifdef WIN32
                    //		OutputDebugString(ch);
#endif
                    
                    //////////////////////////////////////////////////////////////////////////
                    return JVN_CMD_CONNS2;
                }
                else if(nType == JVN_RSP_CONNAF)
                {//收到服务器失败数据
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]服务器返回失败 详细:",nIndex);
                        strcat(pchError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]server return failed info:",nIndex);
                        strcat(pchError, chMsg);
                    }
                    memset(chMsg, 0, MAX_PATH);
                    sprintf(chMsg,"%d",m_stConnInfo.nYSTNO);
                    strcat(pchError, (char *)&data[4]);
                    strcat(pchError, " YSTNO:");
                    strcat(pchError, chMsg);
                    strcat(pchError, ">**");
                    
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                    
                    //此处特殊处理，防止资源不释放
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                    
                    return JVN_RSP_CONNAF;
                }
                else if(nType == JVN_CMD_TRYTOUCH)
                {//收到主控打洞包,不再等服务器消息，直接连接
                    if(nrecvlen != 8)
                    {//打洞包长度无效，无法判断是不是有效的打洞包，舍弃
                        return -2;
                    }
                    else
                    {
                        int nyst=0;
                        memcpy(&nyst, &data[4], 4);
                        if(nyst != m_stConnInfo.nYSTNO)
                        {//打洞包不是来自需要的主控，舍弃
                            return -2;
                        }
                    }
                    
                    memcpy(&m_addrAN, &addrtemp, sizeof(SOCKADDR_IN));
                    
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                        m_ServerSocket = 0;
                    }
                    
                    m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                    
                    BOOL bReuse = TRUE;
                    UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                    //////////////////////////////////////////////////////////////////////////
                    int len1 = JVC_MSS;
                    UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                    //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                    len1=1500*1024;
                    UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                    
                    len1=1000*1024;
                    UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                    if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
                    {OutputDebug("************************************Channel  line: %d",__LINE__);
                        if(m_SocketSTmp > 0)
                        {
                            closesocket(m_SocketSTmp);
                        }
                        m_SocketSTmp = 0;
                        
                        if(m_ServerSocket > 0)
                        {
                            m_pWorker->pushtmpsock(m_ServerSocket);
                        }
                        m_ServerSocket = 0;
                        
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[MAX_PATH]={0};
                            sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex);
                            strcat(pchError, chMsg);
                            strcat(pchError, UDT::getlasterror().getErrorMessage());
                            strcat(pchError, ">**");
                        }
                        
                        return JVN_RSP_CONNAF;
                    }
                    //将套接字置为非阻塞模式
                    BOOL block = FALSE;
                    UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                    UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                    LINGER linger;
                    linger.l_onoff = 0;
                    linger.l_linger = 0;
                    UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
                    return JVN_CMD_TRYTOUCH;
                }
                else
                {//其它无效数据
                    return -2;
                }
            }
            else
            {
                return -1;
            }
        }
        
        
        BOOL CCChannel::ConnectTURN(int nIndex, char *pchError)
        {
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP连接
                return FALSE;//ConnectTURNTCP();
            }
            
            UDTSOCKET SocketTmp = UDT::socket(AF_INET, SOCK_STREAM, 0);
            
            BOOL bReuse = TRUE;
            UDT::setsockopt(SocketTmp, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
            //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
            UDT::setsockopt(SocketTmp, 0, UDT_MSS, &len1, sizeof(int));
            //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
            len1=1500*1024;
            UDT::setsockopt(SocketTmp, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
            
            len1=1000*1024;
            UDT::setsockopt(SocketTmp, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
            if (UDT::ERROR == UDT::bind(SocketTmp, m_pWorker->m_WorkerUDPSocket))
            {OutputDebug("************************************Channel  line: %d",__LINE__);
                if(m_SocketSTmp > 0)
                {
                    closesocket(m_SocketSTmp);
                }
                m_SocketSTmp = 0;
                
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(SocketTmp > 0)
                {
                    m_pWorker->pushtmpsock(SocketTmp);
                }
                SocketTmp = 0;
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex);
                    strcat(pchError, chMsg);
                    strcat(pchError, UDT::getlasterror().getErrorMessage());
                    strcat(pchError, ">**");
                }
                
                return FALSE;
            }
            
            //将套接字置为非阻塞模式
            BOOL block = FALSE;
            UDT::setsockopt(SocketTmp, 0, UDT_SNDSYN, &block, sizeof(BOOL));
            UDT::setsockopt(SocketTmp, 0, UDT_RCVSYN, &block, sizeof(BOOL));
            LINGER linger;
            linger.l_onoff = 0;
            linger.l_linger = 0;
            UDT::setsockopt(SocketTmp, 0, UDT_LINGER, &linger, sizeof(LINGER));
            
//	m_addrTS.sin_addr.s_addr = inet_addr("14.17.120.226");
//	m_addrTS.sin_port =htons(9320);
            
            STJUDTCONN stcon;
            stcon.u = SocketTmp;
            stcon.name = (SOCKADDR *)&m_addrTS;
            stcon.namelen = sizeof(SOCKADDR);
            stcon.nChannelID = m_stConnInfo.nChannel;
            memcpy(stcon.chGroup, m_stConnInfo.chGroup, 4);
            stcon.nYSTNO = m_stConnInfo.nYSTNO;
            stcon.nLVer_new = JVN_YSTVER;
            stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
            stcon.nMinTime = 3000;
            
            SOCKADDR_IN srv = {0};
            char strServer[100] = {0};
            memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
                         sprintf(strServer,"ConnectTURN connecting a %s:%d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port));
            OutputDebug(strServer);
            writeLog(strServer);
            if(UDT::ERROR == UDT::connect(stcon))
            {
                if(m_SocketSTmp > 0)
                {
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                }
                
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(SocketTmp > 0)
                {
                    m_pWorker->pushtmpsock(SocketTmp);
                }
                SocketTmp = 0;
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[3*MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]TURN连接失败.详细:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pchError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]TURN Connect failed. INFO:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pchError, chMsg);
                    }
                }
                writeLog("************************ end connectTurn connect failed.");
                OutputDebug("************************ end connectTurn connect failed.");
                return FALSE;
            }
            else
            {
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);//在这里清理会造成收不到帧头从而连接成功无图像
                }
                m_ServerSocket = SocketTmp;
                OutputDebug("************************ end connectTurn connect ok.");
                writeLog("************************ end connectTurn connect ok.");
                m_bIsTurn = TRUE;
                
                return TRUE;
            }
        }
        
        //tcp直连，无论是直接拿ip去连还是中途改为tcp去连，最终都要使用该函数去执行
        BOOL CCChannel::ConnectIPTCP()
        {
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            
            if(m_ListenSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ListenSocket);
            }
            m_ListenSocket = 0;
            
            if(m_ListenSocketTCP > 0)
            {
                closesocket(m_ListenSocketTCP);
            }
            m_ListenSocketTCP = 0;
            
            m_PartnerCtrl.ClearPartner();
            
            if(m_ServerSocket > 0)
            {
                if((m_stConnInfo.nConnectType == TYPE_PC_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP) && m_nProtocolType == TYPE_PC_TCP)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                else
                {
                    closesocket(m_ServerSocket);
                }
            }
            m_ServerSocket = 0;
            
            //创建临时UDP套接字
            m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            
            SOCKADDR_IN addrSrv;
#ifndef WIN32
            addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
            addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
            addrSrv.sin_family = AF_INET;
            addrSrv.sin_port = htons(0);//(m_nLocalStartPort);//绑定到指定起始端口
            
            BOOL bReuse = TRUE;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
            
            //绑定套接字
            int err = bind(m_ServerSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
            if(err != 0)
            {
                char ch[1000]={0};
                sprintf(ch,"%d",m_nLocalStartPort);
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.指定端口可能被占，改用随机端口 本次弃用端口:", __FILE__,__LINE__,ch);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed.old port is invalid，use new prot. old port:", __FILE__,__LINE__,ch);
                }
                
#ifndef WIN32
                addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
                addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
                addrSrv.sin_family = AF_INET;
                addrSrv.sin_port = htons(0);//绑定到随机端口
                
                bind(m_ServerSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
            }
            
            m_stConnInfo.nShow = 0;
            
            SOCKADDR_IN addrA;
#ifndef WIN32
            addrA.sin_addr.s_addr = inet_addr(m_stConnInfo.chServerIP);
#else
            addrA.sin_addr.S_un.S_addr = inet_addr(m_stConnInfo.chServerIP);
#endif
            addrA.sin_family = AF_INET;
            addrA.sin_port = htons(m_stConnInfo.nServerPort);
            
            memcpy(&m_addrAL, &addrA, sizeof(SOCKADDR_IN));
            
            //将套接字置为非阻塞模式
            int iSockStatus = 0;
#ifndef WIN32
            int flags=0;
            if ((flags = fcntl(m_ServerSocket, F_GETFL, 0)) < 0)
            {
                if(m_ServerSocket > 0)
                {
                    closesocket(m_ServerSocket);
                }
                m_ServerSocket = 0;
                return FALSE;
            }
            
            if (fcntl(m_ServerSocket, F_SETFL, flags | O_NONBLOCK) < 0)
#else
                unsigned long ulBlock = 1;
            iSockStatus = ioctlsocket(m_ServerSocket, FIONBIO, (unsigned long*)&ulBlock);
            if (SOCKET_ERROR == iSockStatus)
#endif
            {
                if(m_ServerSocket > 0)
                {
                    closesocket(m_ServerSocket);
                }
                m_ServerSocket = 0;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.设置非阻塞失败.", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. set noblock failed.", __FILE__,__LINE__);
                }
                
                return FALSE;
            }
            
            //将套接字置为不等待未处理完的数据
            LINGER linger;
            linger.l_onoff = 0;
            linger.l_linger = 0;
            iSockStatus = setsockopt(m_ServerSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
#ifndef WIN32
            if (iSockStatus < 0)
#else
                if (SOCKET_ERROR == iSockStatus)
#endif
                {
                    if(m_ServerSocket > 0)
                    {
                        closesocket(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败.设置非阻塞失败.", __FILE__,__LINE__);
                    }
                    else
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. set noblock failed.", __FILE__,__LINE__);
                    }
                    return FALSE;
                }
            
            int nSetSize = 128 * 1024;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
            nSetSize = 128 * 1024;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));
            
            writeLog("connect tcp %s : %d",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                OutputDebug("connect tcp %s : %d",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
            if(0 != connectnb(m_ServerSocket,(SOCKADDR *)&addrA, sizeof(SOCKADDR), 3))
            {
                OutputDebug("connect tcp %s : %d error",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                writeLog("connect tcp %s : %d error",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                if(m_ServerSocket > 0)
                {
                    closesocket(m_ServerSocket);
                }
                m_ServerSocket = 0;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 连接主控失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect failed. connect op. failed. INFO:", __FILE__,__LINE__);
                }
                
                return FALSE;
            }
            else
            {
                //tcp连接首先发送通道号等信息
                //初步连接成功 发送 通道号(4)+连接类型(4) 信息
                BYTE datatmp[3*MAX_PATH]={0};
                memcpy(&datatmp[0], &m_stConnInfo.nChannel, 4);
                memcpy(&datatmp[4], &m_nProtocolType, 4);
                
                tcpsenddata(m_ServerSocket, (char *)datatmp, 8, TRUE);
                OutputDebug("connect tcp %s : %d ok",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                writeLog("connect tcp %s : %d ok",m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);

                return TRUE;
            }
        }
        
        BOOL CCChannel::SendSP2PTCP(SOCKADDR_IN addrs, int nIndex, char *pchError)
        {
            if(m_ServerSocket > 0)
            {
                closesocket(m_ServerSocket);
            }
            m_ServerSocket = 0;
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            //创建套接字
            m_SocketSTmp = socket(AF_INET, SOCK_STREAM,0);
            
            SOCKADDR_IN addrSrv;
#ifndef WIN32
            addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
            addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
            
            addrSrv.sin_family = AF_INET;
            addrSrv.sin_port = htons(m_nLocalStartPort);
            
            int nReuse = 1;
            setsockopt(m_SocketSTmp, SOL_SOCKET, SO_REUSEADDR, (char *)&nReuse, sizeof(int));
            
            //绑定套接字
#ifndef WIN32
            if(bind(m_SocketSTmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)) < 0)
#else
                if(SOCKET_ERROR == bind(m_SocketSTmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)))
#endif
                {
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        strcat(pchError,  "YST连接主控失败.设置非阻塞失败>**");
                    }
                    else
                    {
                        strcat(pchError, "YST connect failed. set noblock failed>**");
                    }
                    
                    return FALSE;
                }
            
            //将套接字置为非阻塞模式
            
            int iSockStatus = 0;
#ifndef WIN32
            int flags=0;
            if ((flags = fcntl(m_SocketSTmp, F_GETFL, 0)) < 0)
            {
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
                return FALSE;
            }
            
            if (fcntl(m_SocketSTmp, F_SETFL, flags | O_NONBLOCK) < 0)
#else
                unsigned long ulBlock = 1;
            iSockStatus = ioctlsocket(m_SocketSTmp, FIONBIO, (unsigned long*)&ulBlock);
            if (SOCKET_ERROR == iSockStatus)
#endif
            {
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    strcat(pchError, "YST连接主控失败.设置非阻塞失败>**");
                }
                else
                {
                    strcat(pchError, "YST connect failed. set noblock failed>**");
                }
                
                return FALSE;
            }
            //将套接字置为不等待未处理完的数据
            LINGER linger;
            linger.l_onoff = 1;//0;
            linger.l_linger = 0;
            iSockStatus = setsockopt(m_SocketSTmp, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
#ifndef WIN32
            if (iSockStatus < 0)
#else
                if (SOCKET_ERROR == iSockStatus)
#endif
                {
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        strcat(pchError, "YST连接主控失败.设置非阻塞失败>**");
                    }
                    else
                    {
                        strcat(pchError, "YST connect failed. set noblock failed>**");
                    }
                    return FALSE;
                }
            
            if(0 == connectnb(m_SocketSTmp,(SOCKADDR *)&addrs, sizeof(SOCKADDR), 3))
            {
                //向服务器请求A地址
                //向S发送请求
                BYTE data[JVN_ASPACKDEFLEN]={0};
                int nType = JVN_REQ_CONNA;
                memcpy(&data[0], &nType, 4);
                memcpy(&data[4], &m_stConnInfo.nYSTNO, 4);
                
                if(tcpsenddata(m_SocketSTmp, (char *)data, 8, TRUE) == 8)
                {
                    return TRUE;
                }
                else
                {
#ifndef WIN32
                    int kkk = errno;
#else
                    int kkk = WSAGetLastError();
#endif
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]向服务器发送数据A失败(e:%d)>**",nIndex,kkk);
                        strcat(pchError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]send data A to server failed(e:%d)>**",nIndex,kkk);
                        strcat(pchError, chMsg);
                    }
                    
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                    
                    return FALSE;
                }
            }
            else
            {
#ifndef WIN32
                int kkk = errno;
#else
                int kkk = WSAGetLastError();
#endif
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]向服务器发送数据A失败(e:%d)>**",nIndex,kkk);
                    strcat(pchError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]send data A to server failed(e:%d)>**",nIndex,kkk);
                    strcat(pchError, chMsg);
                }
                
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
                
                return FALSE;
            }
        }
        
        int CCChannel::RecvSTCP(int nIndex, char *pchError)
        {
            BYTE data[JVN_ASPACKDEFLEN]={0};
            int nType = JVN_REQ_CONNA;
            memset(data, 0, JVN_ASPACKDEFLEN);
            int nrecvlen=0;
            
            //	int naddrlen = sizeof(SOCKADDR);
            if((nrecvlen = tcpreceive(m_SocketSTmp, (char *)data, JVN_ASPACKDEFLEN, 1)) > 0)
            {
                memcpy(&nType, &data[0], 4);
                if(nType == JVN_RSP_CONNA)
                {//收到服务器成功数据
                    int nTCPPort=0;
                    memcpy(&nTCPPort, &data[4], 4);
                    char chIPA[20]={0};
                    char chIPAL[20]={0};
                    memcpy(chIPA, &data[8],20);
                    memcpy(chIPAL, &data[28],20);
                    
#ifndef WIN32
                    m_addrAN.sin_addr.s_addr = inet_addr(chIPA);
#else
                    m_addrAN.sin_addr.S_un.S_addr = inet_addr(chIPA);
#endif
                    m_addrAN.sin_family = AF_INET;
                    m_addrAN.sin_port = htons(nTCPPort);
                    
#ifndef WIN32
                    m_addrAL.sin_addr.s_addr = inet_addr(chIPAL);
#else
                    m_addrAL.sin_addr.S_un.S_addr = inet_addr(chIPAL);
#endif
                    m_addrAL.sin_family = AF_INET;
                    m_addrAL.sin_port = htons(nTCPPort);
                    
                    if(m_SocketSTmp > 0)
                    {
                        closesocket(m_SocketSTmp);
                        m_SocketSTmp = 0;
                    }
                    
                    return JVN_RSP_CONNA;
                }
                else if(nType == JVN_RSP_CONNAF)
                {//收到服务器失败数据
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]服务器返回失败 详细:",nIndex);
                        strcat(pchError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]server return failed info:",nIndex);
                        strcat(pchError, chMsg);
                    }
                    memset(chMsg, 0, MAX_PATH);
                    sprintf(chMsg,"%d",m_stConnInfo.nYSTNO);
                    strcat(pchError, (char *)&data[4]);
                    strcat(pchError, " YSTNO:");
                    strcat(pchError, chMsg);
                    strcat(pchError, ">**");
                    
                    closesocket(m_SocketSTmp);
                    m_SocketSTmp = 0;
                    
                    return JVN_RSP_CONNAF;
                }
                else
                {//其它无效数据
                    char chMsg[MAX_PATH]={0};
                    char chType[10]={0};
                    sprintf(chType,"%X",nType);
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]收到的数据类型有误 错误值:0x%s>**",nIndex,chType);
                        strcat(pchError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]data type received from server is wrong, datatype:0x%s>**",nIndex,chType);
                        strcat(pchError, chMsg);
                    }
                    
                    return -2;
                }
            }
            else
            {
                return -1;
            }
        }
        
        BOOL CCChannel::ConnectNetTCP(int nIndex, char *pchError)
        {
            if(m_pWorker != NULL)
            {//测试提示
                m_pWorker->NormalData(m_nLocalChannel,0x20,(BYTE*)&m_addrAN, sizeof(SOCKADDR_IN), 0,0);
            }
            
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            
            if(m_ServerSocket > 0)
            {
                closesocket(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            //////////////////////////////////////////////////////////////////////////
            //创建套接字
            m_ServerSocket = socket(AF_INET, SOCK_STREAM,0);
            
            SOCKADDR_IN addrSrv;
#ifndef WIN32
            addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
            addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
            addrSrv.sin_family = AF_INET;
            addrSrv.sin_port = htons(m_nLocalStartPort);
            
            int nReuse = TRUE;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nReuse, sizeof(int));
            
            //绑定套接字
            if(bind(m_ServerSocket, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)) < 0)
            {
#ifndef WIN32
                int kkk = errno;
#else
                int kkk = WSAGetLastError();
#endif
                closesocket(m_ServerSocket);
                m_ServerSocket = 0;
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    sprintf(chMsg,"<[S%d]bind sock failed,e:%d",nIndex, kkk);
                    strcat(pchError, chMsg);
                    strcat(pchError, ">**");
                }
                
                return JVN_RSP_CONNAF;
            }
            
            //将套接字置为非阻塞模式
            
            int iSockStatus = 0;
#ifndef WIN32
            int flags=0;
            fcntl(m_ServerSocket, F_GETFL, 0);
            fcntl(m_ServerSocket, F_SETFL, flags | O_NONBLOCK);
#else
            unsigned long ulBlock = 1;
            iSockStatus = ioctlsocket(m_ServerSocket, FIONBIO, (unsigned long*)&ulBlock);
#endif
            
            //将套接字置为不等待未处理完的数据
            LINGER linger;
            linger.l_onoff = 1;//0;
            linger.l_linger = 0;
            iSockStatus = setsockopt(m_ServerSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
            
            int nSetSize = 128 * 1024;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
            nSetSize = 128 * 1024;
            setsockopt(m_ServerSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));
            
            if(0 != connectnb(m_ServerSocket,(SOCKADDR *)&m_addrAN, sizeof(SOCKADDR), 3))
            {
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[3*MAX_PATH]={0};
#ifndef WIN32
                    sprintf(chMsg,"<[S%d]Net connect failed,Info:%d>**",nIndex, errno);
#else
                    sprintf(chMsg,"<[S%d]Net connect failed,Info:%d>**",nIndex, WSAGetLastError());
#endif
                    strcat(pchError, chMsg);
                }
                
                if(m_ServerSocket > 0)
                {
                    closesocket(m_ServerSocket);
                }
                m_ServerSocket = 0;
                return FALSE;
            }
            
            
            //tcp连接首先发送通道号等信息
            //初步连接成功 发送 通道号(4)+连接类型(4) 信息
            BYTE datatmp[3*MAX_PATH]={0};
            memcpy(&datatmp[0], &m_stConnInfo.nChannel, 4);
            memcpy(&datatmp[4], &m_nProtocolType, 4);
            
            tcpsenddata(m_ServerSocket, (char *)datatmp, 8, TRUE);
            return TRUE;
        }
        
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //版本兼容性验证
        int CCChannel::CheckVersion(short sV1, short sV2, short sV3, short sV4)
        {
            if(sV1 > m_pWorker->m_stVersion.sver1)
            {
                return 1;
            }
            else if(sV1 < m_pWorker->m_stVersion.sver1)
            {
                return -1;
            }
            else
            {
                if(sV2 > m_pWorker->m_stVersion.sver2)
                {
                    return 1;
                }
                else if(sV2 < m_pWorker->m_stVersion.sver2)
                {
                    return -1;
                }
                else
                {
                    if(sV3 > m_pWorker->m_stVersion.sver3)
                    {
                        return 1;
                    }
                    else if(sV3 < m_pWorker->m_stVersion.sver3)
                    {
                        return -1;
                    }
                    else
                    {
                        if(sV4 > m_pWorker->m_stVersion.sver4)
                        {
                            return 1;
                        }
                        else if(sV4 < m_pWorker->m_stVersion.sver4)
                        {
                            return -1;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                }
            }
        }
        
        void CCChannel::NatTry2Partner(char chIP[16], int nPort)
        {
#ifndef WIN32
            pthread_mutex_lock(&m_ct);
#else
            EnterCriticalSection(&m_ct);
#endif
            if(m_ServerSocket > 0)
            {
                BYTE data[10]={0};
                //data[0] = JVN_CMD_TRY;
                int ntmp = JVN_CMD_TRYTOUCH;
                memcpy(&data[0], &ntmp, 4);
                memcpy(&data[4], &m_stConnInfo.nYSTNO, 4);
                SOCKADDR_IN addr;
#ifndef WIN32
                addr.sin_addr.s_addr = inet_addr(chIP);
#else
                addr.sin_addr.S_un.S_addr = inet_addr(chIP);
#endif
                addr.sin_family = AF_INET;
                addr.sin_port = htons(nPort);
                
                sendtoclient(m_pWorker->m_WorkerUDPSocket, (char *)&data, 8, 0, (SOCKADDR *)&(addr), sizeof(SOCKADDR),0);//1
            }
#ifndef WIN32
            pthread_mutex_unlock(&m_ct);
#else
            LeaveCriticalSection(&m_ct);
#endif
        }
        
        BOOL CCChannel::SendHelpData(BYTE uchType, BYTE *pBuffer,int nSize)
        {
#ifndef WIN32
            pthread_mutex_lock(&m_ct);
#else
            EnterCriticalSection(&m_ct);
#endif
            
            {
                switch(uchType)
                {
                    case JVN_REQ_CHECK://请求录像检索
                    {
                        if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
                        {
                            BYTE data[10 + 2*JVN_ABCHECKPLEN]={0};
                            data[0] = JVC_HELP_CVDATA;
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                            m_pHelpConn->SendToHelp(data, 10 + nSize);
                        }
                    }
                        break;
                    case JVN_CMD_LADDR://本地地址
                    case JVN_REQ_DOWNLOAD://请求录像下载
                    case JVN_REQ_PLAY://请求远程回放
                    {
                        //OutputDebugString("req play.......\n");
                        if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
                        {
                            BYTE data[5 + MAX_PATH]={0};
                            data[0] = JVC_HELP_CVDATA;
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                            
                            int nLen = nSize + 10;
                            int ss=0;
                            int ssize=0;
                            while(ssize < nLen)
                            {
                                ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                                
                                if(0 < ss)
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
                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    else
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    //////////////////////////////////////////////////////////////////////////
                                    if(m_ServerSocket > 0)
                                    {
                                        m_pWorker->pushtmpsock(m_ServerSocket);
                                        m_ServerSocket = 0;
                                    }
                                    //////////////////////////////////////////////////////////////////////////
                                    
#ifndef WIN32
                                    pthread_mutex_unlock(&m_ct);
#else
                                    LeaveCriticalSection(&m_ct);
#endif
                                    
                                    return FALSE;
                                }
                            }
                            
                            //char ch[100]={0};
                            //sprintf(ch,"req play.......1  %d\n",nLen);
                            //OutputDebugString(ch);
                            if(uchType == JVN_REQ_DOWNLOAD || uchType == JVN_REQ_PLAY)
                            {
                                m_bDAndP = TRUE;
                                if(m_pBuffer != NULL)
                                {
                                    m_pBuffer->ClearBuffer();
                                }
                            }
                        }
                    }
                        break;
                        
                    case JVN_CMD_PLAYSEEK://播放定位
                    case JVN_CMD_YTCTRL://云台控制
                    {
                        if(pBuffer != NULL && nSize == 4)
                        {
                            BYTE data[20]={0};
                            data[0] = JVC_HELP_CVDATA;
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                            m_pHelpConn->SendToHelp(data, 14);
                        }
                    }
                        break;
                    case JVN_RSP_CHATACCEPT://同意语音
                        m_bAcceptChat = TRUE;
                        goto DOSEND;
                    case JVN_RSP_TEXTACCEPT://同意文本
                        m_bAcceptText = TRUE;
                        goto DOSEND;
                    case JVN_REQ_CHAT://请求语音聊天
                    case JVN_REQ_TEXT://请求文本聊天
                    case JVN_CMD_DISCONN://断开连接
                    case JVN_CMD_PLAYSTOP://暂停播放
                    case JVN_CMD_DOWNLOADSTOP://停止下载数据
                    case JVN_CMD_CHATSTOP://停止语音聊天
                    case JVN_CMD_TEXTSTOP://停止文本聊天
                    case JVN_CMD_VIDEO://请求实时监控数据
                    case JVN_CMD_VIDEOPAUSE://请求实时监控数据
                    case JVN_CMD_PLAYUP://快进
                    case JVN_CMD_PLAYDOWN://慢放
                    case JVN_CMD_PLAYDEF://原速播放
                    case JVN_CMD_PLAYPAUSE://暂停播放
                    case JVN_CMD_PLAYGOON://继续播放
                    case JVN_DATA_DANDP://下载回放确认
                    case JVN_CMD_PLIST://请求伙伴列表
                    {
                    DOSEND:
                        int nLen = 10;
                        BYTE data[10]={0};
                        data[0] = JVC_HELP_CVDATA;
                        int nl = 5;
                        memcpy(&data[1], &nl, 4);
                        
                        data[5] = uchType;
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                            
                            if(0 < ss)
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
                                
                                char chtmp[20]={0};
                                sprintf(chtmp, "type:%X", uchType);
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 成功关闭套接字", __FILE__,__LINE__,chtmp);
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed close socket successed", __FILE__,__LINE__,chtmp);
                                }
                                
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                        
                        if(uchType == JVN_CMD_DOWNLOADSTOP || uchType == JVN_CMD_PLAYSTOP)
                        {
                            m_bDAndP = FALSE;
                            
                            //char ch[100]={0};
                            //sprintf(ch,"req play stop.......%d\n",nLen);
                            //OutputDebugString(ch);
                        }
                    }
                        break;
                    case JVN_DATA_OK://视频帧确认
                    {
                        if(m_pHelpConn != NULL)
                        {
                            break;
                        }
                    }
                        break;
                    case JVN_RSP_TEXTDATA://文本数据
                    {
                        if(m_bAcceptText)
                        {
                            if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                            {
                                int nLen = 5+nSize+5;
                                BYTE data[JVN_BAPACKDEFLEN]={0};
                                data[0] = JVC_HELP_CVDATA;
                                int nl = nSize + 5;
                                memcpy(&data[1], &nl, 4);
                                
                                data[5] = uchType;
                                memcpy(&data[6], &nSize, 4);
                                memcpy(&data[10], pBuffer, nSize);
                                
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
                                    ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                                    
                                    if(0 < ss)
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
                                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        //////////////////////////////////////////////////////////////////////////
                                        if(m_ServerSocket > 0)
                                        {
                                            m_pWorker->pushtmpsock(m_ServerSocket);
                                            m_ServerSocket = 0;
                                        }
                                        //////////////////////////////////////////////////////////////////////////
                                        
#ifndef WIN32
                                        pthread_mutex_unlock(&m_ct);
#else
                                        LeaveCriticalSection(&m_ct);
#endif
                                        
                                        return FALSE;
                                    }
                                }
                            }
                        }
                    }
                        break;
                    case JVN_RSP_CHATDATA://语音数据
                    {
                        if(m_bAcceptChat)
                        {
                            if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                            {
                                int nLen = 5+nSize+5;
                                BYTE data[JVN_BAPACKDEFLEN]={0};
                                data[0] = JVC_HELP_CVDATA;
                                int nl = nSize + 5;
                                memcpy(&data[1], &nl, 4);
                                
                                data[5] = uchType;
                                memcpy(&data[6], &nSize, 4);
                                memcpy(&data[10], pBuffer, nSize);
                                
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
                                    ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                                    
                                    //							if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, 1400), 0)))
                                    if(0 < ss)
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
                                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        //////////////////////////////////////////////////////////////////////////
                                        if(m_ServerSocket > 0)
                                        {
                                            m_pWorker->pushtmpsock(m_ServerSocket);
                                            m_ServerSocket = 0;
                                        }
                                        //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                        pthread_mutex_unlock(&m_ct);
#else
                                        LeaveCriticalSection(&m_ct);
#endif
                                        
                                        return FALSE;
                                    }
                                }
                            }
                        }
                    }
                        break;
                    case JVN_CMD_ONLYI://只要关键帧
                    case JVN_CMD_FULL://回复满帧发送
                    {
                        int nLen = 5+nSize+5;
                        BYTE data[JVN_BAPACKDEFLEN]={0};
                        data[0] = JVC_HELP_CVDATA;
                        
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            nLen = 5+nSize+5;
                            
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                        }
                        else
                        {
                            nLen = 5+5;
                            
                            int nl = 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                        }
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                            
                            if(0 < ss)
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                    }
                        break;
                    case JVN_REQ_RATE:
                    {
                        int nLen = 5+nSize+5;
                        BYTE data[JVN_BAPACKDEFLEN]={0};
                        data[0] = JVC_HELP_CVDATA;
                        
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            nLen = 5+nSize+5;
                            
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                        }
                        else
                        {
                            break;
                        }
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                            
                            if(0 < ss)
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                    }
                        break;
                    case JVN_DATA_O://自定义数据
                    {
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            int nLen = 5+nSize+5;
                            BYTE data[JVN_BAPACKDEFLEN]={0};
                            data[0] = JVC_HELP_CVDATA;
                            int nl = nSize + 5;
                            memcpy(&data[1], &nl, 4);
                            
                            data[5] = uchType;
                            memcpy(&data[6], &nSize, 4);
                            memcpy(&data[10], pBuffer, nSize);
                            
                            int ss=0;
                            int ssize=0;
                            while(ssize < nLen)
                            {
                                ss = m_pHelpConn->SendToHelp(data + ssize, nLen - ssize);
                                
                                if(0 < ss)
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
                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    else
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    
                                    //////////////////////////////////////////////////////////////////////////
                                    if(m_ServerSocket > 0)
                                    {
                                        m_pWorker->pushtmpsock(m_ServerSocket);
                                        m_ServerSocket = 0;
                                    }
                                    //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                    pthread_mutex_unlock(&m_ct);
#else
                                    LeaveCriticalSection(&m_ct);
#endif
                                    
                                    return FALSE;
                                }
                            }
                        }
                    }
                        break;
                    default:
                        break;
                }
            }
            
#ifndef WIN32
            pthread_mutex_unlock(&m_ct);
#else
            LeaveCriticalSection(&m_ct);
#endif
            
            return TRUE;
        }
        
        BOOL CCChannel::SendData(BYTE uchType, BYTE *pBuffer,int nSize)
        {
            if(m_pOldChannel != NULL)
            {
                return m_pOldChannel->SendData(uchType, pBuffer, nSize);
            }
            
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP
                return SendDataTCP(uchType, pBuffer,nSize);
            }
            
            if(m_pHelpConn != NULL)
            {
                return SendHelpData(uchType, pBuffer, nSize);
            }
#ifndef WIN32
            pthread_mutex_lock(&m_ct);
#else
            EnterCriticalSection(&m_ct);
#endif
            if(m_ServerSocket > 0)
            {
               BYTE data[5 + 2*JVN_BAPACKDEFLEN]={0};
                memset(data, 0, sizeof(data));
                switch(uchType)
                {
                    case JVN_CMD_KEEPLIVE://回复心跳
                    {
                        //////////////////////////////////////////////////////////////////////////
                        int ss=0;
                        int ssize=0;
                        int nLen = nSize;
                        while(ssize < nLen)
                        {
                            if(0 < (ss = UDT::send(m_ServerSocket, (char *)pBuffer + ssize, nLen - ssize, 0)))
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
                                
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                        //////////////////////////////////////////////////////////////////////////
                    }
                        break;
                    case JVN_REQ_CHECK://请求录像检索
                    {
                        if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
                        {
                            //BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
                            data[0] = uchType;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            UDT::send(m_ServerSocket, (char *)data,5 + nSize, 0);
                        }
                    }
                        break;
                    case JVN_CMD_LADDR://本地地址
                    case JVN_REQ_DOWNLOAD://请求录像下载
                    case JVN_REQ_PLAY://请求远程回放
                    {
                        if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
                        {
                            //BYTE data[5 + MAX_PATH]={0};
                            data[0] = uchType;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            
                            int nLen = nSize + 5;
                            int ss=0;
                            int ssize=0;
                            while(ssize < nLen)
                            {
                                if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    else
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    //////////////////////////////////////////////////////////////////////////
                                    if(m_ServerSocket > 0)
                                    {
                                        m_pWorker->pushtmpsock(m_ServerSocket);
                                        m_ServerSocket = 0;
                                    }
                                    //////////////////////////////////////////////////////////////////////////
                                    
#ifndef WIN32
                                    pthread_mutex_unlock(&m_ct);
#else
                                    LeaveCriticalSection(&m_ct);
#endif
                                    
                                    return FALSE;
                                }
                            }
                            if(uchType == JVN_REQ_DOWNLOAD || uchType == JVN_REQ_PLAY)
                            {
                                m_bDAndP = TRUE;
                                if(m_pBuffer != NULL)
                                {
                                    m_pBuffer->ClearBuffer();
                                }
                            }
                        }
                    }
                        break;
                        
                    case JVN_CMD_PLAYSEEK://播放定位
                    case JVN_CMD_YTCTRL://云台控制
                    {
                        if(pBuffer != NULL && nSize == 4)
                        {
                            //BYTE data[9]={0};
                            data[0] = uchType;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            UDT::send(m_ServerSocket, (char *)data,9, 0);
                        }
                    }
                        break;
                    case JVN_RSP_CHATACCEPT://同意语音
                        m_bAcceptChat = TRUE;
                        goto DOSEND;
                    case JVN_RSP_TEXTACCEPT://同意文本
                        m_bAcceptText = TRUE;
                        goto DOSEND;
                    case JVN_REQ_CHAT://请求语音聊天
                    case JVN_REQ_TEXT://请求文本聊天
                    case JVN_CMD_DISCONN://断开连接
                    case JVN_CMD_PLAYSTOP://暂停播放
                    case JVN_CMD_DOWNLOADSTOP://停止下载数据
                    case JVN_CMD_CHATSTOP://停止语音聊天
                    case JVN_CMD_TEXTSTOP://停止文本聊天
                    case JVN_CMD_VIDEO://请求实时监控数据
                    case JVN_CMD_VIDEOPAUSE://请求实时监控数据
                    case JVN_CMD_PLAYUP://快进
                    case JVN_CMD_PLAYDOWN://慢放
                    case JVN_CMD_PLAYDEF://原速播放
                    case JVN_CMD_PLAYPAUSE://暂停播放
                    case JVN_CMD_PLAYGOON://继续播放
                    case JVN_DATA_DANDP://下载回放确认
                    case JVN_CMD_PLIST://请求伙伴列表
                    {
                    DOSEND:
                        int nLen = 5;
                        //BYTE data[5]={0};
                        data[0] = uchType;
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
                                
                                char chtmp[20]={0};
                                sprintf(chtmp, "type:%X", uchType);
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 成功关闭套接字", __FILE__,__LINE__,chtmp);
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed close socket successed", __FILE__,__LINE__,chtmp);
                                }
                                
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                        
                        if(uchType == JVN_CMD_DOWNLOADSTOP || uchType == JVN_CMD_PLAYSTOP)
                        {
                            m_bDAndP = FALSE;
                        }
                    }
                        break;
                    case JVN_DATA_OK://视频帧确认
                    {
                        if(nSize > 10)
                        {
                            nSize = 10;
                        }
                        
                        int nLen = 5+nSize;
                        
                        //BYTE data[15]={0};
                        data[0] = uchType;
                        memcpy(&data[1], &nSize, 4);
                        if(pBuffer != NULL)
                        {
                            memcpy(&data[5], pBuffer, nSize);
                        }
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                    }
                        break;
                    case JVN_RSP_TEXTDATA://文本数据
                    {
                        if(m_bAcceptText)
                        {
                            if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                            {
                                int nLen = 5+nSize;
                                //BYTE data[JVN_BAPACKDEFLEN]={0};
                                data[0] = uchType;
                                memcpy(&data[1], &nSize, 4);
                                memcpy(&data[5], pBuffer, nSize);
                                
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
                                    if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        //////////////////////////////////////////////////////////////////////////
                                        if(m_ServerSocket > 0)
                                        {
                                            m_pWorker->pushtmpsock(m_ServerSocket);
                                            m_ServerSocket = 0;
                                        }
                                        //////////////////////////////////////////////////////////////////////////
                                        
#ifndef WIN32
                                        pthread_mutex_unlock(&m_ct);
#else
                                        LeaveCriticalSection(&m_ct);
#endif
                                        
                                        return FALSE;
                                    }
                                }
                            }
                        }
                    }
                        break;
                    case JVN_RSP_CHATDATA://语音数据
                    {
                        if(m_bAcceptChat)
                        {
                            if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                            {
                                int nLen = 5+nSize;
                                //BYTE data[JVN_BAPACKDEFLEN]={0};
                                data[0] = uchType;
                                memcpy(&data[1], &nSize, 4);
                                memcpy(&data[5], pBuffer, nSize);
                                
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
                                    if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, 20000), 0)))
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
                                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        //////////////////////////////////////////////////////////////////////////
                                        if(m_ServerSocket > 0)
                                        {
                                            m_pWorker->pushtmpsock(m_ServerSocket);
                                            m_ServerSocket = 0;
                                        }
                                        //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                        pthread_mutex_unlock(&m_ct);
#else
                                        LeaveCriticalSection(&m_ct);
#endif
                                        
                                        return FALSE;
                                    }
                                }
                            }
                        }
                    }
                        break;
                    case JVN_CMD_ONLYI://只要关键帧
                    case JVN_CMD_FULL://回复满帧发送
                    case JVN_CMD_ALLAUDIO://请求音频全转发
                    {
                        int nLen = 5+nSize;
                        //BYTE data[JVN_BAPACKDEFLEN]={0};
                        data[0] = uchType;
                        
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            nLen = 5+nSize;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            
                        }
                        else
                        {
                            nLen = 5;
                        }
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                    }
                        break;
                    case JVN_REQ_RATE:
                    {
                        int nLen = 5+nSize;
                        //BYTE data[JVN_BAPACKDEFLEN]={0};
                        data[0] = uchType;
                        
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            nLen = 5+nSize;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            
                        }
                        else
                        {
                            break;
                        }
                        
                        int ss=0;
                        int ssize=0;
                        while(ssize < nLen)
                        {
                            if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
                                //////////////////////////////////////////////////////////////////////////
                                if(m_ServerSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                    m_ServerSocket = 0;
                                }
                                //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                pthread_mutex_unlock(&m_ct);
#else
                                LeaveCriticalSection(&m_ct);
#endif
                                
                                return FALSE;
                            }
                        }
                    }
                        break;
                    case JVN_DATA_O://自定义数据
                    {
                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                        {
                            int nLen = 5+nSize;
                            //BYTE data[JVN_BAPACKDEFLEN]={0};
                            data[0] = uchType;
                            memcpy(&data[1], &nSize, 4);
                            memcpy(&data[5], pBuffer, nSize);
                            
                            int ss=0;
                            int ssize=0;
                            while(ssize < nLen)
                            {
                                if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    else
                                    {
                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                    }
                                    
                                    //////////////////////////////////////////////////////////////////////////
                                    if(m_ServerSocket > 0)
                                    {
                                        m_pWorker->pushtmpsock(m_ServerSocket);
                                        m_ServerSocket = 0;
                                    }
                                    //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
                                    pthread_mutex_unlock(&m_ct);
#else
                                    LeaveCriticalSection(&m_ct);
#endif
                                    
                                    return FALSE;
                                }
                            }
                        }
                    }
                        break;
                    default:
                        break;
                }
            }
            
#ifndef WIN32
            pthread_mutex_unlock(&m_ct);
#else
            LeaveCriticalSection(&m_ct);
#endif
            
            return TRUE;
        }
        
        int CCChannel::SendCMD(BYTE uchType, BYTE* pBuffer, int nSize)
        {
            if(uchType == JVN_CMD_ONLYI || uchType == JVN_CMD_FULL)
            {
                if(m_nFYSTVER >= JVN_YSTVER2)
                {//对方支持关键帧切换
                    if(SendData(uchType, pBuffer, nSize))
                    {
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 2;
                }
            }
            else if(uchType == JVN_CMD_ALLAUDIO)
            {//其他消息暂不支持
                if(m_nFYSTVER >= JVN_YSTVER3 && m_bTURN)
                {//对方支持关键帧切换
                    if(SendData(uchType, pBuffer, nSize))
                    {
                        m_bOpenTurnAudio = TRUE;
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 2;
                }
            }
            else
            {//其他消息暂不支持
            }
            
            return 0;
        }
        
        BOOL CCChannel::DisConnect()
        {
            //////////////////////////////////////////////////////////////////////////
            if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
            {
                m_pWorker->YSTNOCushion(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, -1);
            }
            //////////////////////////////////////////////////////////////////////////
            
            m_bDisConnectShow = TRUE;
            if(m_pOldChannel != NULL)
            {
                return m_pOldChannel->DisConnect();
            }
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            //	Sleep(1);
            
            if(m_pHelpConn != NULL)
            {
                m_pHelpConn->DisConnectYSTNO();
            }
            
#ifdef MOBILE_CLIENT
            if (!m_connectThreadExit) 
            {
                m_bExit = TRUE;
                m_bEndR = TRUE;
                while (TRUE) 
                {
                    if (m_connectThreadExit) 
                    {
                        break;
                    }
//                    printf("run here  channel discconect  line: %d\n",__LINE__);
                    CCWorker::jvc_sleep(100);
                }
            }
            else
            {
                 printf("disconnect Channel m_recvThreadExit and m_playProExit\n");
            }
#else
            m_bExit = TRUE;
#endif
            
#ifndef WIN32
            if (0 != m_hConnThread)
            {
                m_bEndC = TRUE;
                pthread_join(m_hConnThread, NULL);
                m_hConnThread = 0;
            }
            if (0 != m_hRecvThread)
            {
                m_bEndR = TRUE;
                pthread_join(m_hRecvThread, NULL);
                m_hRecvThread = 0;
            }
            if (0 != m_hPartnerThread)
            {
                m_bEndPT = TRUE;
                pthread_join(m_hPartnerThread, NULL);
                m_hPartnerThread = 0;
            }
            if (0 != m_hPTCThread)
            {
                m_bEndPTC = TRUE;
                pthread_join(m_hPTCThread, NULL);
                m_hPTCThread = 0;
            }
#else
            if(m_hEndEventC>0)
            {
                SetEvent(m_hEndEventC);
            }
            if(m_hEndEventR>0)
            {
                SetEvent(m_hEndEventR);
            }
            if(m_hEndEventPT>0)
            {
                SetEvent(m_hEndEventPT);
            }
            if(m_hEndEventPTC>0)
            {
                SetEvent(m_hEndEventPTC);
            }
            
#endif
            
            CCWorker::jvc_sleep(10);
            //	WaitThreadExit(m_hConnThread);
            //  WaitThreadExit(m_hRecvThread);
            //	WaitThreadExit(m_hSendThread);
            //	WaitThreadExit(m_hRecvPThread);
            
            //	if(m_pHelpConn != NULL)
            //	{
            //		delete m_pHelpConn;
            //		m_pHelpConn = NULL;
            //	}
            
            if(m_ServerSocket > 0)
            {
                if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                {//TCP
                    closesocket(m_ServerSocket);
                }
                else
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
            }
            m_ServerSocket = 0;
            
            if(m_ListenSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ListenSocket);
            }
            m_ListenSocket = 0;
            
            if(m_ListenSocketTCP > 0)
            {
                closesocket(m_ListenSocketTCP);
            }
            m_ListenSocketTCP = 0;
            
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            
            m_PartnerCtrl.DisConnectPartners();
            
            m_PartnerCtrl.ClearPartner();
            
            return TRUE;
        }
        
#ifndef WIN32
        void* CCChannel::PartnerProc(void* pParam)
#else
        UINT WINAPI CCChannel::PartnerProc(LPVOID pParam)
#endif
        {
            CCChannel *pWorker = (CCChannel *)pParam;
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
            
            ::WaitForSingleObject(pWorker->m_hStartEventPT, INFINITE);
            if(pWorker->m_hStartEventPT > 0)
            {
                CloseHandle(pWorker->m_hStartEventPT);
                pWorker->m_hStartEventPT = 0;
            }
#endif
            
            //接收相关.............................
            BYTE *puchBuf = new BYTE[JVNC_DATABUFLEN];//200*1024
            int nSize = 0;
            DWORD dwstarttime = CCWorker::JVGetTime();
            DWORD dwstartBM = dwstarttime;
            DWORD dwstartLINK = dwstarttime;
            DWORD dwstartReq = dwstarttime;
            //	DWORD dwlastGT = dwstarttime;
            DWORD dwlastPlay = dwstarttime;
            DWORD dwlastproxy = dwstarttime;
            DWORD dwlastTURNC = dwstarttime;
            
            DWORD dwendtime = 0;
            
            STREQS STPullReqs;
            STPullReqs.reserve(5000);
            int nReqCount = 0;
            
            int nBufferRate = -1;
            BYTE uchTypeRead = 0;
            int nLenRead = 0;
            
            while(TRUE)
            {
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndPT)
                {
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventPT, 0))
                {
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
                    
                    break;
                }
#endif
                
                //Sleep(10);
                //continue;
                
                //		addrlen = sizeof(SOCKADDR_IN);
                //------------------------------------------------------------------------------
                //等待接受伙伴的UDP连接 因为改为单端口，移到外层进行
                //------------------------------------------------------------------------------
                //------------------------------------------------------------------------------
                //等待接受伙伴的TCP连接 因为改为单端口，移到外层进行
                //------------------------------------------------------------------------------
                
                //Sleep(10);
                //continue;
                
                //------------------------------------------------------------------------------
                //正常情况下，会不间断的收到主控的BM，如果超过30S没收到，可能与主控间出了问题，断开
                //------------------------------------------------------------------------------
                dwendtime = CCWorker::JVGetTime();
                if(dwendtime > pWorker->m_dwLastDataTime + PT_TIME_NODATA)
                {
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "长时间收不到主控BM.退出. ", __FILE__,__LINE__);
                    }
                    else
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "can not recv A BM.return.", __FILE__,__LINE__);
                    }
                    
                    pWorker->m_bExit = TRUE;
#ifdef WIN32
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
#endif
                    break;
                }
                
                
                //------------------------------------------------------------------------------
                //接收伙伴间的交互数据
                //------------------------------------------------------------------------------
#ifndef WIN32
                if(!pWorker->m_PartnerCtrl.RecvFromPartners(pWorker->m_bExit, 0))
                {
                    break;
                }
#else
                if(!pWorker->m_PartnerCtrl.RecvFromPartners(pWorker->m_bExit, pWorker->m_hEndEventPT))
                {
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
                    
                    break;
                }
#endif
                
                //------------------------------------------------------------------------------
                //检查伙伴连接情况，处理伙伴间的连接
                //------------------------------------------------------------------------------
                dwendtime = CCWorker::JVGetTime();
                if(dwendtime > JVN_TIME_CHECKLINK + dwstartLINK)
                {
                    dwstartLINK= dwendtime;
                    pWorker->m_PartnerCtrl.CheckPartnerLinks(pWorker->m_bExit);
                }
                
                //Sleep(10);
                //continue;
                
                if(pWorker->m_bCache)
                {
                    //------------------------------------------------------------------------------
                    //重置二级代理节点
                    //------------------------------------------------------------------------------
                    dwendtime = CCWorker::JVGetTime();
                    if(dwendtime > 30000 + dwlastproxy)
                    {
                        dwlastproxy= dwendtime;
                        pWorker->m_PartnerCtrl.ResetProxy2();
                    }
                }
                
                //------------------------------------------------------------------------------
                //检查播放效果如何，是否需要提速
                //------------------------------------------------------------------------------
                if(!pWorker->m_bTURN && pWorker->m_bPass && (dwendtime > JVN_TIME_TURNC + dwlastTURNC))
                {
                    dwlastTURNC = dwendtime;
                    if(pWorker->m_pBuffer != NULL)
                    {
                        if(pWorker->m_pBuffer->WaitHighFrequency())
                        {//效果很差 需要提速
                            pWorker->m_PartnerCtrl.AddTurnCachPartner();
                        }
                    }
                }
                
                //------------------------------------------------------------------------------
                //定时20ms从CSBufferCtrl读取BM(BM有变化时才发送)，根据需要并向所有已连接伙伴发送BM；
                //------------------------------------------------------------------------------
                dwendtime = CCWorker::JVGetTime();
                int nSENDBM = pWorker->m_bCache?JVN_TIME_SENDBMC:JVN_TIME_SENDBM;
                if(dwendtime > nSENDBM + dwstartBM && !pWorker->m_bDAndP)//40
                {
                    dwstartBM= dwendtime;
                    if(pWorker->m_pBuffer != NULL)
                    {
                        memset(puchBuf, 0, JVNC_DATABUFLEN);//200*1024
                        if(pWorker->m_pBuffer->GetBM(puchBuf, nSize))
                        {//BM有变化 可以发送给伙伴
#ifndef WIN32
                            if(!pWorker->m_PartnerCtrl.SendBM2Partners(puchBuf, nSize, pWorker->m_bExit, 0))
                            {
                                pWorker->m_bExit = TRUE;
                                
                                break;
                            }
#else
                            if(!pWorker->m_PartnerCtrl.SendBM2Partners(puchBuf, nSize, pWorker->m_bExit, pWorker->m_hEndEventPT))
                            {
                                pWorker->m_bExit = TRUE;
                                
                                if(pWorker->m_hEndEventPT > 0)
                                {
                                    CloseHandle(pWorker->m_hEndEventPT);
                                    pWorker->m_hEndEventPT = 0;
                                }
                                break;
                            }
#endif
                        }
                    }
                }
                
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndPT)
                {
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventPT, 0))
                {
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
                    
                    break;
                }
#endif
                
                
                //Sleep(10);
                //continue;
                //------------------------------------------------------------------------------
                //检查缓存中的待收数据片，向最优的节点发送数据片请求，更新数据源记录，计时开始并做标记，
                //------------------------------------------------------------------------------
                if(!pWorker->m_bDAndP)
                {//BM方式时主动请求(Normal方式时则是直接接收主控发来的数据)
                    dwendtime = CCWorker::JVGetTime();
                    if(pWorker->m_bPass && pWorker->m_pBuffer != NULL && (dwendtime > JVN_TIME_REQBMD + dwstartReq))//40
                    {
                        dwstartReq= dwendtime;
                        nReqCount = 0;
                        if(pWorker->m_pBuffer->ReadChunkLocalNeed(STPullReqs, nReqCount))
                        {
                            pWorker->m_PartnerCtrl.SendBMDREQ2Partner(STPullReqs, nReqCount, pWorker->m_bExit);
                        }
                    }
                }
                
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndPT)
                {
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventPT, 0))
                {
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
                    
                    break;
                }
#endif
                
                //------------------------------------------------------------------------------
                //检查待发送数据片
                //------------------------------------------------------------------------------
                if(!pWorker->m_bDAndP && pWorker->m_bPass)
                {//BM方式时主动请求(Normal方式时则是直接接收主控发来的数据)
                    pWorker->m_PartnerCtrl.SendBMD();
                }
                
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndPT)
                {
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventPT, 0))
                {
                    if(pWorker->m_hEndEventPT > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPT);
                        pWorker->m_hEndEventPT = 0;
                    }
                    
                    break;
                }
#endif
                
                //------------------------------------------------------------------------------
                //播放
                //------------------------------------------------------------------------------
                if(dwendtime > JVN_TIME_PLAY + dwlastPlay  && pWorker->m_pBuffer != NULL && !pWorker->m_bDAndP)
                {
                    dwlastPlay = dwendtime;
                    nBufferRate = -1;
                    uchTypeRead = 0;
                    if(pWorker->m_pBuffer->ReadPlayBuffer(uchTypeRead, puchBuf, nLenRead, nBufferRate))
                    {
                        if(pWorker->m_nLastRate != 100)
                        {
                            pWorker->m_nLastRate = 100;
                            pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, 100);
                        }
                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, puchBuf, nLenRead, 0,0);
                    }
                    else
                    {//播放失败 检查是否需要提示缓冲进度
                        if(pWorker->m_bJVP2P && nBufferRate >= 0 && nBufferRate <= 100)
                        {
                            pWorker->m_nLastRate = nBufferRate;
                            //提示缓冲信息
                            pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, nBufferRate);
                            //if(pWorker->m_nLocalChannel == 2)
                            //{
                            //	OutputDebugString("buf........\n");
                            //}
                        }
                    }
                }
                
                CCWorker::jvc_sleep(2);
            }
            
            if(puchBuf != NULL)
            {
                delete[] puchBuf;
                puchBuf = NULL;
            }
            
            //	if(pSTPullReqs != NULL)
            //	{
            //		delete[] pSTPullReqs;
            //		pSTPullReqs = NULL;
            //	}
            STPullReqs.clear();
            
            //结束线程
            if(pWorker->m_ServerSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ServerSocket);
            }
            pWorker->m_ServerSocket = 0;
            
            if(pWorker->m_ListenSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ListenSocket);
            }
            pWorker->m_ListenSocket = 0;
            
            if(pWorker->m_SocketSTmp > 0)
            {
                closesocket(pWorker->m_SocketSTmp);
            }
            pWorker->m_SocketSTmp = 0;
            
            if(pWorker->m_ListenSocketTCP > 0)
            {
                closesocket(pWorker->m_ListenSocketTCP);
                pWorker->m_ListenSocketTCP = 0;
                
            }
            pWorker->m_ListenSocketTCP = 0;
            pWorker->m_PartnerCtrl.ClearPartner();
            
            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "伙伴处理线程正常退出. ", __FILE__,__LINE__);
            }
            else
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "partner thread stop sucessed", __FILE__,__LINE__);
            }
            
#ifdef WIN32
            return 0;
#else
            return NULL;
#endif
        }
        
#ifndef WIN32
        void* CCChannel::PTConnectProc(void* pParam)
#else
        UINT WINAPI CCChannel::PTConnectProc(LPVOID pParam)
#endif
        {
            CCChannel *pWorker = (CCChannel *)pParam;
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
            
            ::WaitForSingleObject(pWorker->m_hStartEventPTC, INFINITE);
            if(pWorker->m_hStartEventPTC > 0)
            {
                CloseHandle(pWorker->m_hStartEventPTC);
                pWorker->m_hStartEventPTC = 0;
            }
#endif
            
            DWORD dwstarttime = CCWorker::JVGetTime();
            //	DWORD dwstartCLINK = dwstarttime;
            DWORD dwstartLINK = dwstarttime;
            DWORD dwlastGT = dwstarttime;
            
            DWORD dwendtime = 0;
            
            while(TRUE)
            {
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndPTC)
                {
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventPTC, 0))
                {
                    if(pWorker->m_hEndEventPTC > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventPTC);
                        pWorker->m_hEndEventPTC = 0;
                    }
                    
                    break;
                }
#endif
                
                //------------------------------------------------------------------------------
                //检查伙伴连接情况，确定是否需要追加链接，加入待连列表
                //------------------------------------------------------------------------------
                /*		dwendtime = CCWorker::JVGetTime();
                 if(dwendtime > JVN_TIME_CHECKLINK + dwstartCLINK)
                 {
                 dwstartCLINK= dwendtime;
                 pWorker->m_PartnerCtrl.CheckPartnerLinks(pWorker->m_bExit);
                 }
                 */
                //------------------------------------------------------------------------------
                //检查伙伴连接情况，进行伙伴间的连接
                //------------------------------------------------------------------------------
                dwendtime = CCWorker::JVGetTime();
                if(dwendtime > JVN_TIME_LINK + dwstartLINK)
                {
                    dwstartLINK= dwendtime;
                    pWorker->m_PartnerCtrl.PartnerLink(pWorker->m_bExit);
                }
                
                //------------------------------------------------------------------------------
                //检查无效伙伴及时清理
                //------------------------------------------------------------------------------
                dwendtime = CCWorker::JVGetTime();
                if(pWorker->m_bPass && (dwendtime > JVN_TIME_GT + dwlastGT))
                {
                    dwlastGT= dwendtime;
                    pWorker->m_PartnerCtrl.CheckGarbage();
                }
                
                CCWorker::jvc_sleep(5);
            }
            
            //结束线程
            if(pWorker->m_ServerSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ServerSocket);
            }
            pWorker->m_ServerSocket = 0;
            
            if(pWorker->m_ListenSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ListenSocket);
            }
            pWorker->m_ListenSocket = 0;
            
            if(pWorker->m_ListenSocketTCP > 0)
            {
                closesocket(pWorker->m_ListenSocketTCP);
                pWorker->m_ListenSocketTCP = 0;
            }
            pWorker->m_ListenSocketTCP = 0;
            pWorker->m_PartnerCtrl.ClearPartner();
            
            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "伙伴连接线程正常退出. ", __FILE__,__LINE__);
            }
            else
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "ptc thread stop sucessed", __FILE__,__LINE__);
            }
            
#ifdef WIN32
            return 0;
#else
            return NULL;
#endif
        }
        
        
        BOOL CCChannel::ParseMsg(BYTE *chMAP)
        {
            if(m_nRecvPos < 5)
            {
                return FALSE;//过小 判断不出是什么包
            }
            
            char chLIP[16]={0};
            int nLPort=0;
            
            BYTE uchType=m_puchRecvBuf[0];
            int nLen=0;
            unsigned int unChunkID = 0;
            int nChunkCount=0;
            memcpy(&nLen, &m_puchRecvBuf[1], 4);
            
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                case JVN_DATA_A://音频
                case JVN_DATA_S://帧尺寸
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头数据
                    //--------------------------------------------------------2.0.0.1
                case JVN_CMD_KEEPLIVE://心跳检测
                case JVN_CMD_PLIST://伙伴列表
                case JVN_CMD_BM://BM消息
                case JVN_CMD_ADDR://伙伴内外网地址
                case JVN_CMD_TRY://向某个地址打洞
                case JVN_RSP_BMD://某数据片数据
                case JVN_RSP_BMDNULL://请求某数据片失败
                case JVN_RSP_BMDBUSY://对方忙碌，从其他数据源索取或重新索取
                case JVN_CMD_CMD://特殊命令
                    //--------------------------------------------------------2.0.0.1
                case JVN_RSP_NOSERVER://无该通道服务
                case JVN_RSP_INVALIDTYPE://连接类型无效
                case JVN_RSP_OVERLIMIT://超过最大连接数目
                case JVN_REQ_CHAT://请求语音聊天
                case JVN_REQ_TEXT://请求文本聊天
                case JVN_RSP_CHATACCEPT://同意语音请求
                case JVN_RSP_TEXTACCEPT://同意文本请求
                case JVN_CMD_CHATSTOP://停止语音
                case JVN_CMD_TEXTSTOP://停止文本
                case JVN_RSP_CHECKDATA://检索结果
                case JVN_RSP_CHECKOVER://检索完成(无检索内容)
                case JVN_RSP_CHATDATA://语音数据
                case JVN_RSP_TEXTDATA://文本数据
                case JVN_RSP_DOWNLOADDATA://下载数据
                case JVN_RSP_DOWNLOADE://下载数据失败
                case JVN_RSP_DOWNLOADOVER://下载数据完成
                case JVN_RSP_PLAYDATA://回放数据
                case JVN_RSP_PLAYOVER://回放完成
                case JVN_RSP_PLAYE://回放失败
                case JVN_RSP_DLTIMEOUT://下载超时
                case JVN_RSP_PLTIMEOUT://回放超时
                case JVN_CMD_DISCONN://服务端断开连接
                case JVN_CMD_FRAMETIME://帧间隔
                case JVN_DATA_SPEED://获取主控码率
                    break;
                default:
                {//错误类型，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVNC_DATABUFLEN-1);
                    m_nRecvPos -= 1;
                    //char ch[100];
                    //sprintf(ch,"parsemsg type err1 recvpos:%d\n",m_nRecvPos);
                    //OutputDebugString(ch);
                    return TRUE;
                }
            }
            
            if(nLen < 0 || nLen >= JVNC_DATABUFLEN)
            {//错误长度，丢弃该部分数据
                m_bError = TRUE;
                memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                m_nRecvPos -= 5;
                //OutputDebugString("parsemsg len err\n");
                return TRUE;
            }
            
            if(m_nRecvPos < nLen+5)
            {//数据未收完整
                return FALSE;
            }
            //OutputDebugString("parsemsg ok\n");
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        int nFrameIndex = 0;
                        unsigned int unIID=0;
                        unsigned int unFTime = 0;
                        short int snFID = 0;
                        if(m_nFYSTVER >= JVN_YSTVER1)
                        {//类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                            memcpy(&unIID, &m_puchRecvBuf[5], 4);
                            memcpy(&snFID, &m_puchRecvBuf[9], 2);
                            memcpy(&unFTime, &m_puchRecvBuf[11], 4);
                            if(uchType == JVN_DATA_I)
                            {//收到I视频帧，确认
                                m_bError = FALSE;
                                m_nOCount = 1;
                                SendData(JVN_DATA_OK, &m_puchRecvBuf[5], 6);
                            }
                            else
                            {
                                m_nOCount++;
                                if(m_nOCount%JVNC_ABFRAMERET == 0)
                                {
                                    SendData(JVN_DATA_OK, &m_puchRecvBuf[5], 6);
                                }
                            }
                        }
                        else
                        {//类型+长度+nframeindex(4) +数据区
                            /*判断收到的帧编号，I帧确认，一个序列内每10帧一个确认*/
                            memcpy(&nFrameIndex, &m_puchRecvBuf[5], 4);
                            if(uchType == JVN_DATA_I)
                            {//收到I视频帧，确认
                                m_bError = FALSE;
                                m_nOCount = 1;
                                SendData(JVN_DATA_OK, &m_puchRecvBuf[5], 4);
                            }
                            else
                            {
                                m_nOCount++;
                                if(m_nOCount%JVNC_ABFRAMERET == 0)
                                {
                                    SendData(JVN_DATA_OK, &m_puchRecvBuf[5], 4);
                                }
                            }
                            m_dTimeUsed = CCWorker::JVGetTime() - m_dBeginTime;
                        }
                        
                        //if(m_nLocalChannel == 2)
                        //{
                        //char ch[100]={0};
                        //sprintf(ch,"type:%d...........%d.%d\n",uchType,nFrameIndex>>8, nFrameIndex & 0xFF);
                        //OutputDebugString(ch);
                        //}
                        //计时
                        m_dBeginTime = CCWorker::JVGetTime();
                        
                        if(m_pWorker != NULL && !m_bError)
                        {
                            if(m_nFYSTVER >= JVN_YSTVER1)
                            {//类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                                //写入缓冲
                                m_pBuffer->WriteBuffer(uchType, &m_puchRecvBuf[15], nLen-10, unIID, (int)snFID, unFTime);
                            }
                            else
                            {//类型+长度+nframeindex(4) +数据区
                                m_puchRecvBuf[8] = nFrameIndex & 0xFF;
                                //写入缓冲
                                m_pBuffer->WriteBuffer(uchType, &m_puchRecvBuf[8], nLen+1, m_dTimeUsed);
                            }
                            //OutputDebugString("1-------------\n");
                        }
                        
                        m_nDownLoadTotalB += nLen+5+4;
                        m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                        m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                        m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                        m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                        
                        if(m_nFYSTVER < JVN_YSTVER1)
                        {//类型+长度+nframeindex(4) +数据区
                            nLen += 4;
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_DATA_A://音频
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头
                {
                    if(m_pWorker != NULL)
                    {
                        if(uchType == JVN_DATA_A && m_bTURN && m_bOpenTurnAudio && m_nFYSTVER >= JVN_YSTVER3)
                        {
                            //新协议，支持帧时间戳，播放控制更优，写入缓冲区
                            //类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                            unsigned int unIID=0;
                            unsigned int unFTime = 0;
                            short int snFID = 0;
                            
                            memcpy(&unIID, &m_puchRecvBuf[5], 4);
                            memcpy(&snFID, &m_puchRecvBuf[9], 2);
                            memcpy(&unFTime, &m_puchRecvBuf[11], 4);
                            
                            m_pBuffer->WriteBuffer(uchType, &m_puchRecvBuf[15], nLen-10, unIID, (int)snFID, unFTime);
                        }
                        else
                        {
                            if(uchType == JVN_DATA_HEAD && m_pBuffer != NULL)
                            {//收到解码头清空数据
                                m_pBuffer->ClearBuffer();
                            }
                            m_pWorker->NormalData(m_nLocalChannel,uchType, &m_puchRecvBuf[5], nLen, 0,0);
                        }
                    }
                    
                    m_nDownLoadTotalB += nLen+5;
                    m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                    m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                    m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                    m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                }
                    break;
                case JVN_CMD_FRAMETIME://帧间隔
                case JVN_DATA_S://帧尺寸
                {
                    int nWidth = 0, nHeight = 0;
                    
                    if(nLen == 8)
                    {
                        memcpy(&nWidth, &m_puchRecvBuf[5], 4);
                        memcpy(&nHeight, &m_puchRecvBuf[9], 4);
                        
                        if(uchType == JVN_DATA_S)
                        {
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->NormalData(m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
                            }
                        }
                        else
                        {
                            if(m_pBuffer != NULL)
                            {
                                m_pBuffer->m_nFrameTime = nWidth;
                                m_pBuffer->m_nFrames = nHeight>0?nHeight:50;
                                
                                m_pBuffer->m_nFrameTime = jvs_max(m_pBuffer->m_nFrameTime, 0);
                                m_pBuffer->m_nFrameTime = jvs_min(m_pBuffer->m_nFrameTime, 10000);
                            }
                        }
                        
                        break;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                    //---------------------------------------------------v2.0.0.1
                case JVN_CMD_KEEPLIVE://心跳检测
                {//类型(1) + 长度(4) + 时间戳(4)
                    if(nLen == 4)
                    {
                        //					DWORD dwt = 0;
                        //					memcpy(&dwt, &m_puchRecvBuf[5], 4);
                        SendData(JVN_CMD_KEEPLIVE, m_puchRecvBuf, 9);
                    }
                }
                    break;
                case JVN_CMD_PLIST://伙伴列表
                {//类型(1)+长度(4)+伙伴总数(4)+IsLan2A(1)+ [linkID(4)+IsSuper(1)+IsCache(1)+IsLan2A(1)+IsLan2P(1)+ADDR(4)] ....
                    int npnum = 0;
                    memcpy(&npnum, &m_puchRecvBuf[5], 4);
                    if(npnum >= 0 && npnum <= 1000)
                    {
                        //					int ntmp=0;
                        STPTLI pttmp;
                        PartnerIDList idlist;
                        int naddlen = sizeof(SOCKADDR_IN);
                        idlist.reserve(npnum+1);
                        idlist.clear();
                        
                        for(int i=0; i<npnum; i++)
                        {
                            memcpy(&pttmp.nLinkID, &m_puchRecvBuf[i*(8+naddlen) + 9], 4);
                            pttmp.bIsSuper = (m_puchRecvBuf[i*(8+naddlen) + 13] == 1)?TRUE:FALSE;
                            pttmp.bIsCache = (m_puchRecvBuf[i*(8+naddlen) + 14] == 1)?TRUE:FALSE;
                            pttmp.bIsLan2A = (m_puchRecvBuf[i*(8+naddlen) + 15] == 1)?TRUE:FALSE;
                            pttmp.bIsLan2B = (m_puchRecvBuf[i*(8+naddlen) + 16] == 1)?TRUE:FALSE;
                            pttmp.bIsTC = FALSE;
                            memcpy(&pttmp.sAddr, &m_puchRecvBuf[i*(8+naddlen) + 17], naddlen);
                            idlist.push_back(pttmp);
                        }
                        
                        if(m_bJVP2P)
                        {
                            m_PartnerCtrl.SetPartnerList(idlist);
                            m_pBuffer->m_nClientCount = npnum+1;
                        }
                    }
                    
                    m_nDownLoadTotalB += nLen+5;
                    m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                    m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                    m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                    m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                }
                    break;
                case JVN_CMD_BM://BM消息
                {//类型(1)+总长度(4)+CHUNKID(4)+总帧数(4)+最新数据块的时间戳(4)+次新数据块时间戳(4)+次新数据块时间戳(4)
                    DWORD dwCTime[10] = {0};
                    unChunkID = 0;
                    nChunkCount = 0;
                    memcpy(&unChunkID, &m_puchRecvBuf[5], 4);
                    memcpy(&nChunkCount, &m_puchRecvBuf[9], 4);
                    for(int i=0; i<10; i++)
                    {
                        memcpy(&dwCTime[i], &m_puchRecvBuf[13+i*4], 4);
                    }
                    
                    //char chMsg[100]={0};
                    //sprintf(chMsg, "A %d %d**************************BM\n", unChunkID, nChunkCount);
                    //OutputDebugString(chMsg);
                    //m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "", __FILE__,__LINE__,chMsg);
                    
                    SetBM(unChunkID, nChunkCount, dwCTime);
                    
                    m_nDownLoadTotalB += nLen+5;
                    m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                    m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                    m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                    m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                    
                    m_dwLastDataTime = CCWorker::JVGetTime();
                }
                    break;
                case JVN_RSP_BMD://某数据片数据
                {//数据类型(1)+数据长度(4)+ 数据块头(？)+数据片内容(?)
                    STCHUNKHEAD stchead;
                    memcpy(&stchead, &m_puchRecvBuf[5], sizeof(STCHUNKHEAD));
                    
                    //if(m_nLocalChannel == 2)
                    //{
                    //char chMsg[100]={0};
                    //sprintf(chMsg, "A.%d len:%d**************************Data\r\n", stchead.unChunkID, stchead.lWriteOffset);
                    //OutputDebugString(chMsg);
                    //}
                    
                    if(stchead.lWriteOffset == (nLen - sizeof(STCHUNKHEAD)))
                    {
                        if(m_pBuffer != NULL)
                        {
                            //更新本地缓存 若本地没有该bm 则丢弃
                            m_pBuffer->WriteBuffer(stchead, &m_puchRecvBuf[5+sizeof(STCHUNKHEAD)]);
                        }
                    }
                    else
                    {
                        int lll = nLen - sizeof(STCHUNKHEAD);
                    }
                    
                    m_nDownLoadTotalB += nLen+5;
                    m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                    m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                    m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                    m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                }
                    break;
                case JVN_RSP_BMDNULL://请求某数据片失败
                {//数据类型(1)+数据长度(4)+chunkid(4)
                    if(nLen == 4)
                    {
                        unChunkID = 0;
                        
                        memcpy(&unChunkID, &m_puchRecvBuf[5], 4);
                        
                        //char chMsg[100]={0};
                        //sprintf(chMsg, "A.%d**************************P NULL\r\n", unChunkID);
                        //OutputDebugString(chMsg);
                        //更新当前伙伴参数
                        m_PartnerCtrl.SetReqStartTime(TRUE,unChunkID,0);
                        
                        m_nDownLoadTotalB += nLen+5;
                        m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                        m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                        m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                        m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_BMDBUSY://对方忙碌，从其他数据源索取或重新索取
                {//数据类型(1)+数据长度(4)+chunkid(4)
                    if(nLen == 4)
                    {
                        unChunkID = 0;
                        
                        memcpy(&unChunkID, &m_puchRecvBuf[5], 4);
                        
                        //char chMsg[100]={0};
                        //sprintf(chMsg, "A.%d**************************P BUSY\r\n", unChunkID);
                        //OutputDebugString(chMsg);
                        //更新当前伙伴参数
                        m_PartnerCtrl.SetReqStartTime(TRUE,unChunkID,0);
                        
                        m_nDownLoadTotalB += nLen+5;
                        m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
                        m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
                        m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
                        m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //					OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_CMD_TRY://向某个地址打洞
                {//类型(1) + 长度(4) + 内网IP(16) + 内网端口(4)
                    //OutputDebugString("CMD_TRY............................\n");
                    if(nLen == 20)
                    {
                        memcpy(chLIP, &m_puchRecvBuf[5], 16);
                        memcpy(&nLPort, &m_puchRecvBuf[21], 4);
                        
                        //char ch[1000];
                        //sprintf(ch,"try.............. %s:%d\n",chLIP, nLPort);
                        //OutputDebugString(ch);
                        NatTry2Partner(chLIP, nLPort);
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_CMD_CMD://特殊命令
                {
                    if(nLen == 1)
                    {
                        BYTE uchcmd = m_puchRecvBuf[5];
                        if(uchcmd == CMD_TYPE_CLEARBUFFER)
                        {//清空本地缓存
                            if(m_bJVP2P)
                            {//仅在多播方式有效
                                m_pBuffer->ClearBuffer();
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                    //---------------------------------------------------v2.0.0.1
                case JVN_RSP_NOSERVER://无该通道服务
                {
                    if(nLen == 0)
                    {
                        m_bPass = FALSE;
                        if(m_pWorker != NULL && !m_bDisConnectShow)
                        {
                            m_bDisConnectShow = TRUE;
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "无该通道服务!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "channel is not open!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        
                        if(m_SocketSTmp > 0)
                        {
                            closesocket(m_SocketSTmp);
                        }
                        m_SocketSTmp = 0;
                        
                        if(m_ListenSocket > 0)
                        {
                            m_pWorker->pushtmpsock(m_ListenSocket);
                        }
                        m_ListenSocket = 0;
                        
                        if(m_ListenSocketTCP > 0)
                        {
                            closesocket(m_ListenSocketTCP);
                        }
                        m_ListenSocketTCP = 0;
                        m_PartnerCtrl.ClearPartner();
                        
                        //结束线程
                        if(m_ServerSocket > 0)
                        {
                            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                            {//TCP
                                closesocket(m_ServerSocket);
                            }
                            else
                            {
                                m_pWorker->pushtmpsock(m_ServerSocket);
                            }
                        }
                        m_ServerSocket = 0;
                        
                        m_nStatus = FAILD;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_INVALIDTYPE://连接类型无效
                {
                    if(nLen == 0)
                    {
                        m_bPass = FALSE;
                        if(m_pWorker != NULL && !m_bDisConnectShow)
                        {
                            m_bDisConnectShow = TRUE;
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "连接类型无效!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "connect type invalid!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        if(m_SocketSTmp > 0)
                        {
                            closesocket(m_SocketSTmp);
                        }
                        m_SocketSTmp = 0;
                        
                        if(m_ListenSocket > 0)
                        {
                            m_pWorker->pushtmpsock(m_ListenSocket);
                        }
                        m_ListenSocket = 0;
                        
                        if(m_ListenSocketTCP > 0)
                        {
                            closesocket(m_ListenSocketTCP);
                        }
                        m_ListenSocketTCP = 0;
                        m_PartnerCtrl.ClearPartner();
                        
                        //结束线程
                        if(m_ServerSocket > 0)
                        {
                            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                            {//TCP
                                closesocket(m_ServerSocket);
                            }
                            else
                            {
                                m_pWorker->pushtmpsock(m_ServerSocket);
                            }
                        }
                        m_ServerSocket = 0;
                        
                        m_nStatus = FAILD;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_OVERLIMIT://超过最大连接数目
                {
                    if(nLen == 0)
                    {
                        m_bPass = FALSE;
                        if(m_pWorker != NULL && !m_bDisConnectShow)
                        {
                            m_bDisConnectShow = TRUE;
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "超过主控最大连接限制!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "client count limit!";
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        if(m_SocketSTmp > 0)
                        {
                            closesocket(m_SocketSTmp);
                        }
                        m_SocketSTmp = 0;
                        
                        if(m_ListenSocket > 0)
                        {
                            m_pWorker->pushtmpsock(m_ListenSocket);
                        }
                        m_ListenSocket = 0;
                        
                        if(m_ListenSocketTCP > 0)
                        {
                            closesocket(m_ListenSocketTCP);
                        }
                        m_ListenSocketTCP = 0;
                        m_PartnerCtrl.ClearPartner();
                        
                        //结束线程
                        if(m_ServerSocket > 0)
                        {
                            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                            {//TCP
                                closesocket(m_ServerSocket);
                            }
                            else
                            {
                                m_pWorker->pushtmpsock(m_ServerSocket);
                            }
                        }
                        m_ServerSocket = 0;
                        
                        m_nStatus = FAILD;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_REQ_CHAT://请求语音聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->ChatData(m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_REQ_TEXT://请求文本聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->TextData(m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_CHATACCEPT://同意语音请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->ChatData(m_nLocalChannel,uchType, NULL, 0);
                        }
                        m_bAcceptChat = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_TEXTACCEPT://同意文本请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->TextData(m_nLocalChannel,uchType, NULL, 0);
                        }
                        m_bAcceptText = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_CMD_CHATSTOP://停止语音
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->ChatData(m_nLocalChannel,uchType, NULL, 0);
                        }
                        m_bAcceptChat = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_CMD_TEXTSTOP://停止文本
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->TextData(m_nLocalChannel,uchType, NULL, 0);
                        }
                        m_bAcceptText = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    break;
                case JVN_RSP_CHECKDATA://检索结果
                case JVN_RSP_CHATDATA://语音数据
                case JVN_RSP_TEXTDATA://文本数据
                {
                    if(uchType == JVN_RSP_CHATDATA && m_bAcceptChat)
                    {
                        /*回调函数语音*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->ChatData(m_nLocalChannel,uchType, &m_puchRecvBuf[5], nLen);
                        }
                    }
                    else if(uchType == JVN_RSP_TEXTDATA && m_bAcceptText)
                    {
                        /*回调函数文本*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->TextData(m_nLocalChannel,uchType, &m_puchRecvBuf[5], nLen);
                        }
                    }
                    else if(uchType == JVN_RSP_CHECKDATA)
                    {
                        /*回调函数检索结果*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->CheckResult(m_nLocalChannel, &m_puchRecvBuf[5], nLen);
                        }
                    }
                }
                    break;
                case JVN_RSP_CHECKOVER://检索完成(无检索内容)
                case JVN_RSP_DOWNLOADE://下载数据失败
                case JVN_RSP_DOWNLOADOVER://下载数据完成
                case JVN_RSP_PLAYE://下载数据失败
                case JVN_RSP_DLTIMEOUT://下载超时
                case JVN_RSP_PLTIMEOUT://回放超时
                case JVN_RSP_PLAYOVER://回放完成
                case JVN_CMD_DISCONN://服务端断开连接
                {
                    if(nLen == 0)
                    {
                        if(m_bDAndP)
                        {
                            m_bDAndP = FALSE;
                            if(m_pBuffer != NULL)
                            {
                                m_pBuffer->ClearBuffer();
                            }
                        }
                        
                        if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
                        {
                            /*回调函数下载*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->DownLoad(m_nLocalChannel,uchType, NULL, 0, 0);
                            }
                        }
                        else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
                        {
                            /*回调函数回放*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->PlayData(m_nLocalChannel,uchType, NULL, 0, 0,0,0, 0,NULL,0);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKOVER)
                        {
                            //OutputDebugString("parsemsg jvn_rsp_checkover\n");
                            /*回调函数检索结果*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->CheckResult(m_nLocalChannel,NULL, 0);
                            }
                        }
                        else if(uchType == JVN_CMD_DISCONN)
                        {
                            //OutputDebugString("dis................\n");
                            m_bPass = FALSE;
                            if(m_pWorker != NULL && !m_bDisConnectShow)
                            {
                                m_bDisConnectShow = TRUE;
                                m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                            //进行断开确认处理
                            ProcessDisConnect();
                            
                            if(m_SocketSTmp > 0)
                            {
                                closesocket(m_SocketSTmp);
                            }
                            m_SocketSTmp = 0;
                            
                            if(m_ListenSocket > 0)
                            {
                                m_pWorker->pushtmpsock(m_ListenSocket);
                            }
                            m_ListenSocket = 0;
                            
                            if(m_ListenSocketTCP > 0)
                            {
                                closesocket(m_ListenSocketTCP);
                            }
                            m_ListenSocketTCP = 0;
                            m_PartnerCtrl.ClearPartner();
                            
                            //结束线程
                            if(m_ServerSocket > 0)
                            {
                                if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                                {//TCP
                                    closesocket(m_ServerSocket);
                                }
                                else
                                {
                                    m_pWorker->pushtmpsock(m_ServerSocket);
                                }
                            }
                            m_ServerSocket = 0;
                            
                            m_nStatus = FAILD;
                            return 0;
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                        m_nRecvPos -= 5;
                        //					OutputDebugString("parsemsg len err\n");
                        return TRUE;
                    }
                }
                    
                    break;
                case JVN_RSP_DOWNLOADDATA://下载数据
                {
                    int nFileLen = -1;
                    
                    memcpy(&nFileLen, &m_puchRecvBuf[5], 4);
                    
                    //确认
                    SendData(JVN_DATA_DANDP, NULL, 0);
                    
                    /*回调函数下载*/
                    if(m_pWorker != NULL)
                    {
                        m_pWorker->DownLoad(m_nLocalChannel,uchType, &m_puchRecvBuf[9], nLen-4, nFileLen);
                    }
                }
                    break;
                case JVN_RSP_PLAYDATA://回放数据
                {
                    if(!m_bDAndP)
                    {
                        break;//只有在回放下载中才接收回放下载数据
                    }
                    
                    //确认
                    SendData(JVN_DATA_DANDP, NULL, 0);
                    
                    if(m_puchRecvBuf[5] == JVN_DATA_S)
                    {
                        int nWidth = 0, nHeight = 0, nTotalFrames = 0;
                        memcpy(&nWidth, &m_puchRecvBuf[10], 4);
                        memcpy(&nHeight, &m_puchRecvBuf[14], 4);
                        memcpy(&nTotalFrames, &m_puchRecvBuf[18], 4);
                        /*回调函数回放*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[5], NULL, 0, nWidth,nHeight,nTotalFrames, 0,NULL,0);
                        }
                    }
                    else if(m_puchRecvBuf[5] == JVN_DATA_O)
                    {
                        int nHeadLen = 0;
                        memcpy(&nHeadLen, &m_puchRecvBuf[6], 4);
                        /*回调函数回放*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[5], &m_puchRecvBuf[10], nHeadLen, 0,0,0, 0,NULL,0);
                        }
                    }
                    else
                    {
                        int nFrameLen;
                        memcpy(&nFrameLen, &m_puchRecvBuf[6], 4);
                        /*回调函数回放*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[5], &m_puchRecvBuf[18], nFrameLen, 0,0,0, 0,NULL,0);
                        }
                    }
                }
                    break;
                case JVN_DATA_SPEED:
                {
                    if(m_pWorker != NULL)
                    {
                        m_pWorker->NormalData(m_nLocalChannel,uchType, &m_puchRecvBuf[5], 4, 0, 0);
                    }
                }
                    break;
                default:
                {//错误类型，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVNC_DATABUFLEN-1);
                    m_nRecvPos -= 1;
                    //char ch[100];
                    //sprintf(ch,"parsemsg type err2 recvpos:%d\n",m_nRecvPos);
                    //OutputDebugString(ch);
                    return TRUE;
                }
            }
            
            if(m_nRecvPos >= nLen+5)
            {//删除解析完的数据
                memmove(m_puchRecvBuf, m_puchRecvBuf + nLen+5, m_nRecvPos-nLen-5);
                m_nRecvPos -= (nLen+5);
            }
            else
            {
                m_nRecvPos = 0;
            }
            
            return TRUE;
        }
        
        BOOL CCChannel::ParseHelpMsg(BYTE *chMAP)
        {
            if(m_nRecvPos < 5)
            {
                return FALSE;//过小 判断不出是什么包
            }
            
            char chLIP[16]={0};
            int nLPort=0;
            
            BYTE uchHType=m_puchRecvBuf[0];
            int nHLen=0;
            memcpy(&nHLen, &m_puchRecvBuf[1], 4);
            
            if(uchHType == JVC_HELP_DATA)
            {
                if(nHLen < 0 || nHLen >= JVNC_DATABUFLEN)
                {//错误长度，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                    m_nRecvPos -= 5;
                    //OutputDebugString("parsemsg len err\n");
                    return TRUE;
                }
                
                BYTE uchType=m_puchRecvBuf[5];
                int nLen=0;
                unsigned int unChunkID = 0;
                int nChunkCount=0;
                memcpy(&nLen, &m_puchRecvBuf[6], 4);
                
                switch(uchType)
                {
                    case JVN_DATA_B://视频B帧
                    case JVN_DATA_P://视频P帧
                    case JVN_DATA_I://视频I帧
                    case JVN_DATA_SKIP://视频S帧
                    case JVN_DATA_A://音频
                    case JVN_DATA_S://帧尺寸
                    case JVN_DATA_O://自定义数据
                    case JVN_DATA_HEAD://解码头数据
                    case JVN_RSP_NOSERVER://无该通道服务
                    case JVN_RSP_INVALIDTYPE://连接类型无效
                    case JVN_RSP_OVERLIMIT://超过最大连接数目
                    case JVN_REQ_CHAT://请求语音聊天
                    case JVN_REQ_TEXT://请求文本聊天
                    case JVN_RSP_CHATACCEPT://同意语音请求
                    case JVN_RSP_TEXTACCEPT://同意文本请求
                    case JVN_CMD_CHATSTOP://停止语音
                    case JVN_CMD_TEXTSTOP://停止文本
                    case JVN_RSP_CHECKDATA://检索结果
                    case JVN_RSP_CHECKOVER://检索完成(无检索内容)
                    case JVN_RSP_CHATDATA://语音数据
                    case JVN_RSP_TEXTDATA://文本数据
                    case JVN_RSP_DOWNLOADDATA://下载数据
                    case JVN_RSP_DOWNLOADE://下载数据失败
                    case JVN_RSP_DOWNLOADOVER://下载数据完成
                    case JVN_RSP_PLAYDATA://回放数据
                    case JVN_RSP_PLAYOVER://回放完成
                    case JVN_RSP_PLAYE://回放失败
                    case JVN_RSP_DLTIMEOUT://下载超时
                    case JVN_RSP_PLTIMEOUT://回放超时
                    case JVN_CMD_DISCONN://服务端断开连接
                    case JVN_CMD_FRAMETIME://帧间隔
                    case JVN_DATA_RATE://多播缓冲进度
                    case JVN_DATA_SPEED:
                        break;
                    default:
                    {//错误类型，丢弃该部分数据
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVNC_DATABUFLEN-1);
                        m_nRecvPos -= 1;
                        //char ch[100];
                        //sprintf(ch,"parsemsg type err1 recvpos:%d\n",m_nRecvPos);
                        //OutputDebugString(ch);
                        return TRUE;
                    }
                }
                
                if(nLen < 0 || nLen >= JVNC_DATABUFLEN)
                {//错误长度，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                    m_nRecvPos -= 5;
                    //OutputDebugString("parsemsg len err\n");
                    return TRUE;
                }
                
                if(m_nRecvPos < nLen+10)
                {//数据未收完整
                    return FALSE;
                }
                //OutputDebugString("parsemsg ok\n");
                //char ch[100]={0};
                //sprintf(ch,"%X............%d\n",uchType,nLen);
                //OutputDebugString(ch);
                switch(uchType)
                {
                    case JVN_DATA_B://视频B帧
                    case JVN_DATA_P://视频P帧
                    case JVN_DATA_I://视频I帧
                    case JVN_DATA_SKIP://视频S帧
                    case JVN_DATA_A://音频
                    case JVN_DATA_O://自定义数据
                    case JVN_DATA_HEAD://解码头
                    {//类型(1) + 长度(4) + 数据(?)
                        if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                        {
                            if(m_pWorker != NULL)
                            {
                                if(uchType == JVN_DATA_HEAD && m_pBuffer != NULL)
                                {
                                    m_pBuffer->ClearBuffer();
                                }
                                m_pWorker->NormalData(m_nLocalChannel,uchType, &m_puchRecvBuf[10], nLen, 0,0);
                            }
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_CMD_FRAMETIME://帧间隔
                    case JVN_DATA_S://帧尺寸
                    {
                        int nWidth = 0, nHeight = 0;
                        
                        if(nLen == 8)
                        {
                            memcpy(&nWidth, &m_puchRecvBuf[10], 4);
                            memcpy(&nHeight, &m_puchRecvBuf[14], 4);
                            
                            if(uchType == JVN_DATA_S)
                            {
                                if(m_pWorker != NULL)
                                {
                                    m_pWorker->NormalData(m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
                                }
                            }
                            else
                            {
                                if(m_pBuffer != NULL)
                                {
                                    m_pBuffer->m_nFrameTime = nWidth;
                                    m_pBuffer->m_nFrames = nHeight>0?nHeight:50;
                                    
                                    m_pBuffer->m_nFrameTime = jvs_max(m_pBuffer->m_nFrameTime, 0);
                                    m_pBuffer->m_nFrameTime = jvs_min(m_pBuffer->m_nFrameTime, 10000);
                                }
                            }
                            
                            break;
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_REQ_CHAT://请求语音聊天
                    {
                        if(nLen == 0)
                        {
                            /*调用回调函数，*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->ChatData(m_nLocalChannel, uchType, NULL, 0);
                            }
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_REQ_TEXT://请求文本聊天
                    {
                        if(nLen == 0)
                        {
                            /*调用回调函数，*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->TextData(m_nLocalChannel, uchType, NULL, 0);
                            }
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_RSP_CHATACCEPT://同意语音请求
                    {
                        if(nLen == 0)
                        {
                            /*回调函数通知是否同意语音请求*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->ChatData(m_nLocalChannel,uchType, NULL, 0);
                            }
                            m_bAcceptChat = TRUE;
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_RSP_TEXTACCEPT://同意文本请求
                    {
                        if(nLen == 0)
                        {
                            /*回调函数通知是否同意语音请求*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->TextData(m_nLocalChannel,uchType, NULL, 0);
                            }
                            m_bAcceptText = TRUE;
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_CMD_CHATSTOP://停止语音
                    {
                        if(nLen == 0)
                        {
                            /*回调函数通知是否同意语音请求*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->ChatData(m_nLocalChannel,uchType, NULL, 0);
                            }
                            m_bAcceptChat = FALSE;
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_CMD_TEXTSTOP://停止文本
                    {
                        if(nLen == 0)
                        {
                            /*回调函数通知是否同意语音请求*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->TextData(m_nLocalChannel,uchType, NULL, 0);
                            }
                            m_bAcceptText = FALSE;
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        break;
                    case JVN_RSP_CHECKDATA://检索结果
                    case JVN_RSP_CHATDATA://语音数据
                    case JVN_RSP_TEXTDATA://文本数据
                    {
                        if(uchType == JVN_RSP_CHATDATA && m_bAcceptChat)
                        {
                            /*回调函数语音*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->ChatData(m_nLocalChannel,uchType, &m_puchRecvBuf[10], nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_TEXTDATA && m_bAcceptText)
                        {
                            /*回调函数文本*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->TextData(m_nLocalChannel,uchType, &m_puchRecvBuf[10], nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKDATA)
                        {
                            /*回调函数检索结果*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->CheckResult(m_nLocalChannel, &m_puchRecvBuf[10], nLen);
                            }
                        }
                    }
                        break;
                    case JVN_RSP_CHECKOVER://检索完成(无检索内容)
                    case JVN_RSP_DOWNLOADE://下载数据失败
                    case JVN_RSP_DOWNLOADOVER://下载数据完成
                    case JVN_RSP_PLAYE://下载数据失败
                    case JVN_RSP_DLTIMEOUT://下载超时
                    case JVN_RSP_PLTIMEOUT://回放超时
                    case JVN_RSP_PLAYOVER://回放完成
                    case JVN_CMD_DISCONN://服务端断开连接
                    {
                        if(nLen == 0)
                        {
                            if(m_bDAndP)
                            {
                                m_bDAndP = FALSE;
                                if(m_pBuffer != NULL)
                                {
                                    m_pBuffer->ClearBuffer();
                                }
                            }
                            
                            if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
                            {
                                /*回调函数下载*/
                                if(m_pWorker != NULL)
                                {
                                    m_pWorker->DownLoad(m_nLocalChannel,uchType, NULL, 0, 0);
                                }
                            }
                            else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
                            {
                                /*回调函数回放*/
                                if(m_pWorker != NULL)
                                {
                                    m_pWorker->PlayData(m_nLocalChannel,uchType, NULL, 0, 0,0,0, 0,NULL,0);
                                }
                            }
                            else if(uchType == JVN_RSP_CHECKOVER)
                            {
                                //OutputDebugString("parsemsg jvn_rsp_checkover\n");
                                /*回调函数检索结果*/
                                if(m_pWorker != NULL)
                                {
                                    m_pWorker->CheckResult(m_nLocalChannel,NULL, 0);
                                }
                            }
                            else if(uchType == JVN_CMD_DISCONN)
                            {
                                //OutputDebugString("dis................\n");
                                m_bPass = FALSE;
                                if(m_pWorker != NULL && !m_bDisConnectShow)
                                {
                                    m_bDisConnectShow = TRUE;
                                    m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                
                                //进行断开确认处理
                                ProcessDisConnect();
                                
                                if(m_SocketSTmp > 0)
                                {
                                    closesocket(m_SocketSTmp);
                                }
                                m_SocketSTmp = 0;
                                
                                if(m_ListenSocket > 0)
                                {
                                    m_pWorker->pushtmpsock(m_ListenSocket);
                                }
                                m_ListenSocket = 0;
                                
                                if(m_ListenSocketTCP > 0)
                                {
                                    closesocket(m_ListenSocketTCP);
                                }
                                m_ListenSocketTCP = 0;
                                m_PartnerCtrl.ClearPartner();
                                
                                //结束线程
                                if(m_ServerSocket > 0)
                                {
                                    if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                                    {//TCP
                                        closesocket(m_ServerSocket);
                                    }
                                    else
                                    {
                                        m_pWorker->pushtmpsock(m_ServerSocket);
                                    }
                                }
                                m_ServerSocket = 0;
                                
                                m_nStatus = FAILD;
                                return 0;
                            }
                        }
                        else
                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                            m_bError = TRUE;
                            memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                            m_nRecvPos -= 5;
                            //					OutputDebugString("parsemsg len err\n");
                            return TRUE;
                        }
                    }
                        
                        break;
                    case JVN_RSP_DOWNLOADDATA://下载数据
                    {
                        int nFileLen = -1;
                        
                        memcpy(&nFileLen, &m_puchRecvBuf[10], 4);
                        
                        //确认
                        SendData(JVN_DATA_DANDP, NULL, 0);
                        
                        /*回调函数下载*/
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->DownLoad(m_nLocalChannel,uchType, &m_puchRecvBuf[14], nLen, nFileLen);
                        }
                    }
                        break;
                    case JVN_RSP_PLAYDATA://回放数据
                    {
                        if(!m_bDAndP)
                        {
                            break;//只有在回放下载中才接收回放下载数据
                        }
                        
                        
                        //确认
                        //	SendData(JVN_DATA_DANDP, NULL, 0);
                        
                        if(m_puchRecvBuf[10] == JVN_DATA_S)
                        {
                            int nWidth = 0, nHeight = 0, nTotalFrames = 0;
                            memcpy(&nWidth, &m_puchRecvBuf[14], 4);
                            memcpy(&nHeight, &m_puchRecvBuf[19], 4);
                            memcpy(&nTotalFrames, &m_puchRecvBuf[23], 4);
                            /*回调函数回放*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[10], NULL, 0, nWidth,nHeight,nTotalFrames, 0,NULL,0);
                            }
                        }
                        else if(m_puchRecvBuf[10] == JVN_DATA_O)
                        {
                            int nHeadLen = 0;
                            memcpy(&nHeadLen, &m_puchRecvBuf[11], 4);
                            /*回调函数回放*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[10], &m_puchRecvBuf[15], nHeadLen, 0,0,0, 0,NULL,0);
                            }
                        }
                        else
                        {
                            int nFrameLen;
                            memcpy(&nFrameLen, &m_puchRecvBuf[11], 4);
                            /*回调函数回放*/
                            if(m_pWorker != NULL)
                            {
                                m_pWorker->PlayData(m_nLocalChannel,m_puchRecvBuf[10], &m_puchRecvBuf[23], nFrameLen, 0,0,0, 0,NULL,0);
                            }
                        }
                    }
                        break;
                    case JVN_DATA_RATE:
                    {
                        BYTE pBuffer[1024] = {0};
                        int nSize, nRate;
                        memcpy(&nSize,&m_puchRecvBuf[6],4);
                        memcpy(&nRate,&m_puchRecvBuf[10],4);
                        memcpy(pBuffer,&m_puchRecvBuf[14],nSize);
                        m_pWorker->BufRate(m_nLocalChannel, uchType, pBuffer, nSize, nRate);
                    }
                        break;
                    case JVN_DATA_SPEED:
                    {
                        if(m_pWorker != NULL)
                        {
                            m_pWorker->NormalData(m_nLocalChannel,uchType, &m_puchRecvBuf[10], 4, 0, 0);
                        }
                    }
                        break;
                    default:
                    {//错误类型，丢弃该部分数据
                        m_bError = TRUE;
                        memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVNC_DATABUFLEN-1);
                        m_nRecvPos -= 1;
                        //char ch[100];
                        //sprintf(ch,"parsemsg type err2 recvpos:%d\n",m_nRecvPos);
                        //OutputDebugString(ch);
                        return TRUE;
                    }
                }
                
                if(m_nRecvPos >= nHLen+5)
                {//删除解析完的数据
                    memmove(m_puchRecvBuf, m_puchRecvBuf + nHLen+5, m_nRecvPos-nHLen-5);
                    m_nRecvPos -= (nHLen+5);
                }
                else
                {
                    m_nRecvPos = 0;
                }
            }
            else if(uchHType == JVC_HELP_KEEP)
            {
                m_dwLastCommTime = CCWorker::JVGetTime();
                //memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                //m_nRecvPos -= 5;
                //////////////////////////////////////////////////////////////////////////
                if(nHLen < 0 || nHLen >= JVNC_DATABUFLEN)
                {//错误长度，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                    m_nRecvPos -= 5;
                    //OutputDebugString("parsemsg len err\n");
                    return TRUE;
                }
                
                BYTE uchType=m_puchRecvBuf[5];
                int nLen=0;
                
                memcpy(&nLen, &m_puchRecvBuf[6], 4);
                if(m_nRecvPos < nLen+10)
                {//数据未收完整
                    return FALSE;
                }
                
                if(m_pchPartnerInfo != NULL && nLen < JVNC_PTINFO_LEN && nLen >= 0)
                {
                    m_nPartnerLen = nLen;
                    memcpy(m_pchPartnerInfo, &m_puchRecvBuf[10], m_nPartnerLen);
                }
                
                //		OutputDebug("recv :%d",m_nPartnerLen);
                memmove(m_puchRecvBuf, m_puchRecvBuf + nHLen+5, m_nRecvPos-nHLen-5);
                m_nRecvPos -= (nHLen+5);
                
                return TRUE;
                //////////////////////////////////////////////////////////////////////////
            }
            else if(uchHType == JVC_HELP_CONNSTATUS)
            {//连接状态变更........
                if(nHLen < 0 || nHLen >= JVNC_DATABUFLEN)
                {//错误长度，丢弃该部分数据
                    m_bError = TRUE;
                    memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVNC_DATABUFLEN-5);
                    m_nRecvPos -= 5;
                    //OutputDebugString("parsemsg len err\n");
                    return TRUE;
                }
                
                //.............解析连接状态
                BYTE bType = m_puchRecvBuf[5];
                int nPWData = 0;
                memcpy(&nPWData,&m_puchRecvBuf[6],4);
                int nMsgLen = 0;
                memcpy(&nMsgLen,&m_puchRecvBuf[10],4);
                char pMsg[100] = {0};
                if(nMsgLen > 0)
                {
                    memcpy(pMsg,&m_puchRecvBuf[14],nMsgLen);
                }
                if(!m_bDisConnectShow)
                {
                    m_pWorker->ConnectChange(m_nLocalChannel,bType,pMsg,nPWData,__FILE__,__LINE__,__FUNCTION__);
                }
                m_bDisConnectShow = TRUE;
                
                if(m_nRecvPos >= nHLen+5)
                {//删除解析完的数据
                    memmove(m_puchRecvBuf, m_puchRecvBuf + nHLen+5, m_nRecvPos-nHLen-5);
                    m_nRecvPos -= (nHLen+5);
                }
                else
                {
                    m_nRecvPos = 0;
                }
                if(bType == JVN_CCONNECTTYPE_SSTOP || bType == JVN_CCONNECTTYPE_DISCONNE)
                {//设备端主动断开或异常断开 清理连结
                    m_bPass = FALSE;
                    
                    if(m_SocketSTmp > 0)
                    {
                        closesocket(m_SocketSTmp);
                    }
                    m_SocketSTmp = 0;
                    
                    if(m_ListenSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ListenSocket);
                    }
                    m_ListenSocket = 0;
                    
                    if(m_ListenSocketTCP > 0)
                    {
                        closesocket(m_ListenSocketTCP);
                    }
                    m_ListenSocketTCP = 0;
                    
                    //结束线程
                    if(m_ServerSocket > 0)
                    {
                        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
                        {//TCP
                            closesocket(m_ServerSocket);
                        }
                        else
                        {
                            m_pWorker->pushtmpsock(m_ServerSocket);
                        }
                    }
                    m_ServerSocket = 0;
                    
                    m_nStatus = FAILD;
                }
            }
            else
            {//错误类型，丢弃该部分数据
                m_bError = TRUE;
                memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVNC_DATABUFLEN-1);
                m_nRecvPos -= 1;
                //char ch[100];
                //sprintf(ch,"parsemsg type err2 recvpos:%d\n",m_nRecvPos);
                //OutputDebugString(ch);
                return TRUE;
            }
            
            
            return TRUE;
        }
        
#ifndef WIN32
        void* CCChannel::RecvProc(void* pParam)
#else
        UINT WINAPI CCChannel::RecvProc(LPVOID pParam)
#endif
        {
            CCChannel *pWorker = (CCChannel *)pParam;
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
            
            ::WaitForSingleObject(pWorker->m_hStartEventR, INFINITE);
            if(pWorker->m_hStartEventR > 0)
            {
                CloseHandle(pWorker->m_hStartEventR);
                pWorker->m_hStartEventR = 0;
            }
#endif
            
            BYTE chMAP[1000]={0};
            
            int rs = 0;
            int nLenRead=-1;
            BYTE uchTypeRead=0;
            int nBufferRate = -1;
            //	DWORD dwstartReq = CCWorker::JVGetTime();
            //	DWORD dwendtime = 0;
            //	int nReqCount = 0;
            //	STREQ *pSTPullReqs = new STREQ[5000];
            STREQS STPullReqs;
            STPullReqs.reserve(5000);
            
            pWorker->m_dBeginTime = CCWorker::JVGetTime();//视频数据计时
            pWorker->m_dTimeUsed = pWorker->m_dBeginTime;
            pWorker->m_bError = FALSE;
            //	OutputDebug("thread start %d\n\n\n",pWorker->m_stConnInfo.nLocalChannel);
            while(TRUE)
            {
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndR)
                {
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_ServerSocket <= 0 && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                {
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_hEndEventR > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventR);
                        pWorker->m_hEndEventR = 0;
                    }
                    
                    if(pWorker->m_ServerSocket <= 0 && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#endif
                
                if(!pWorker->m_bPass)
                {
                    CCWorker::jvc_sleep(10);
                    continue;
                }
                
                if(!pWorker->m_bDAndP)
                {
                    /*读缓冲区,显示*/
                    nBufferRate = -1;
                    uchTypeRead = 0;
                    if(pWorker->m_pBuffer != NULL && pWorker->m_pBuffer->ReadPlayBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead, nBufferRate))
                    {
                        if(pWorker->m_nLastRate != 100)
                        {
                            pWorker->m_nLastRate = 100;
                            pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, 100);
                        }
                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
                        
                        if(uchTypeRead == JVN_DATA_I)
                        {
                            pWorker->UpdateYSTNOList();
                        }
                        
                    }
                    else
                    {//播放失败 检查是否需要提示缓冲进度
                        if(pWorker->m_bJVP2P && nBufferRate >= 0 && nBufferRate <= 100)
                        {
                            pWorker->m_nLastRate = nBufferRate;
                            //提示缓冲信息
                            pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, nBufferRate);
                            
                            //if(pWorker->m_nLocalChannel == 2)
                            //{
                            //	OutputDebugString("buf........\n");
                            //}
                        }
                    }
                }
                
                if(JVNC_DATABUFLEN - pWorker->m_nRecvPos < 300*1024)
                {//剩余空间不多，避免接收不完整(这里暂假定发送的数据都小于300K)
                    CCWorker::jvc_sleep(2);
                    continue;
                }
                
                if (UDT::ERROR == (rs = UDT::recvmsg(pWorker->m_ServerSocket, (char *)&pWorker->m_puchRecvBuf[pWorker->m_nRecvPos], JVNC_DATABUFLEN - pWorker->m_nRecvPos)))
                {//出错
#ifndef WIN32
                    if(pWorker->m_bExit || pWorker->m_bEndR)
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        break;
                    }
#else
                    if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        if(pWorker->m_hEndEventR > 0)
                        {
                            CloseHandle(pWorker->m_hEndEventR);
                            pWorker->m_hEndEventR = 0;
                        }
                        
                        break;
                    }
#endif
                    
                    if(pWorker->m_pWorker != NULL && !pWorker->m_bDisConnectShow)
                    {
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    
                    break;
                }
                else if(rs == 0)
                {//未收到任何数据
                    CCWorker::jvc_sleep(2);
                    continue;
                }
                
                pWorker->m_nRecvPos += rs;
                
                while(TRUE)
                {
                    if(!pWorker->ParseMsg(chMAP))
                    {//解析不出完整数据 停止解析 继续接收
                        break;
                    }
                }
                //不补救效果也不错，暂不进行补救
                /*		if(!pWorker->m_bJVP2P && !pWorker->m_bDAndP)
                 {//普通模式时也进行补救请求
                 //------------------------------------------------------------------------------
                 //检查缓存中的待收数据片有无空缺，向主控发送数据片请求，更新数据源记录，计时开始并做标记，
                 //------------------------------------------------------------------------------
                 dwendtime = CCWorker::JVGetTime();
                 if(pWorker->m_bPass && pWorker->m_pBuffer != NULL && (dwendtime > 500 + dwstartReq))//40
                 {
                 dwstartReq= dwendtime;
                 nReqCount = 0;
                 if(pWorker->m_pBuffer->ReadChunkLocalNeed(STPullReqs, nReqCount))
                 {
                 pWorker->m_PartnerCtrl.SendBMDREQ2Partner(STPullReqs, nReqCount, pWorker->m_bExit);
                 }
                 }
                 }
                 */
            }
            
            //	OutputDebug("thread end %d\n\n\n",pWorker->m_stConnInfo.nLocalChannel);
            if(pWorker->m_ListenSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ListenSocket);
            }
            pWorker->m_ListenSocket = 0;
            
            if(pWorker->m_ListenSocketTCP > 0)
            {
                closesocket(pWorker->m_ListenSocketTCP);
            }
            pWorker->m_ListenSocketTCP = 0;
            
            //结束线程
            if(pWorker->m_ServerSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ServerSocket);
            }
            pWorker->m_ServerSocket = 0;
            
            if(pWorker->m_SocketSTmp > 0)
            {
                closesocket(pWorker->m_SocketSTmp);
            }
            pWorker->m_SocketSTmp = 0;
            
            //	pWorker->m_PartnerCtrl.ClearPartner();//
            
            pWorker->m_nStatus = FAILD;
            
#ifdef WIN32
            if(pWorker->m_hEndEventR > 0)
            {
                CloseHandle(pWorker->m_hEndEventR);
                pWorker->m_hEndEventR = 0;
            }
#endif
            
            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收线程正常退出. ", __FILE__,__LINE__);
            }
            else
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread stop sucessed", __FILE__,__LINE__);
            }
            
#ifdef WIN32
            return 0;
#else
            return NULL;
#endif
        }
        
#ifndef WIN32
        void* CCChannel::RecvHelpProc(void* pParam)
#else
        UINT WINAPI CCChannel::RecvHelpProc(LPVOID pParam)
#endif
        {
            CCChannel *pWorker = (CCChannel *)pParam;
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
            
            ::WaitForSingleObject(pWorker->m_hStartEventR, INFINITE);
            if(pWorker->m_hStartEventR > 0)
            {
                CloseHandle(pWorker->m_hStartEventR);
                pWorker->m_hStartEventR = 0;
            }
#endif
            
            BYTE chMAP[1000]={0};
            
            int rs = 0;
            int nLenRead=-1;
            BYTE uchTypeRead=0;
            int nBufferRate = -1;
            
            pWorker->m_dBeginTime = CCWorker::JVGetTime();//视频数据计时
            pWorker->m_dTimeUsed = pWorker->m_dBeginTime;
            pWorker->m_bError = FALSE;
            
            pWorker->m_dwLastCommTime = CCWorker::JVGetTime();
            DWORD dwRecvTime = CCWorker::JVGetTime();
            
            while(TRUE)
            {
#ifndef WIN32
                if((pWorker->m_pHelpConn != NULL && pWorker->m_pHelpConn->m_tcpsock <= 0) || pWorker->m_bExit || pWorker->m_bEndR || CCWorker::JVGetTime() > dwRecvTime + 5000)
                {
                    if(CCWorker::JVGetTime() > dwRecvTime + 5000)
                    {
                        closesocket(pWorker->m_pHelpConn->m_tcpsock);
                        pWorker->m_pHelpConn->m_tcpsock = 0;
                    }
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_pHelpConn->m_tcpsock <= 0 && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#else
                if((pWorker->m_pHelpConn != NULL && pWorker->m_pHelpConn->m_tcpsock <= 0) || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0) || CCWorker::JVGetTime() > dwRecvTime + 5000)
                {
                    if(CCWorker::JVGetTime() > dwRecvTime + 5000)
                    {
                        closesocket(pWorker->m_pHelpConn->m_tcpsock);
                        pWorker->m_pHelpConn->m_tcpsock = 0;
                    }
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_hEndEventR > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventR);
                        pWorker->m_hEndEventR = 0;
                    }
                    
                    if((pWorker->m_pHelpConn != NULL && pWorker->m_pHelpConn->m_tcpsock <= 0) && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#endif
                //10秒钟发送心跳
                if(CCWorker::JVGetTime() - pWorker->m_dwLastCommTime > 2000)
                {
                    pWorker->m_pHelpConn->SendToHelpActive();
                    pWorker->m_dwLastCommTime = CCWorker::JVGetTime();
                }
                
                if(!pWorker->m_bPass)
                {
                    CCWorker::jvc_sleep(10);
                    continue;
                }
                
                if (0 > (rs = pWorker->m_pHelpConn->RecvFromHelp((unsigned char *)&pWorker->m_puchRecvBuf[pWorker->m_nRecvPos], JVNC_DATABUFLEN - pWorker->m_nRecvPos)))
                {//出错
#ifndef WIN32
                    if(pWorker->m_bExit || pWorker->m_bEndR)
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        break;
                    }
#else
                    if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        if(pWorker->m_hEndEventR > 0)
                        {
                            CloseHandle(pWorker->m_hEndEventR);
                            pWorker->m_hEndEventR = 0;
                        }
                        
                        break;
                    }
#endif
                    
                    if(pWorker->m_pWorker != NULL && !pWorker->m_bDisConnectShow)
                    {
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                    }
                    else
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                    }
                    
                    break;
                }
                else if(rs == 0)
                {//未收到任何数据
                    if(CCWorker::JVGetTime() - pWorker->m_dwLastCommTime > 2000)
                    {
                        pWorker->m_pHelpConn->SendToHelpActive();
                        pWorker->m_dwLastCommTime = CCWorker::JVGetTime();
                    }
                    
                    CCWorker::jvc_sleep(2);
                    continue;
                }
                dwRecvTime = CCWorker::JVGetTime();
                
                pWorker->m_nRecvPos += rs;
                
                while(TRUE)
                {
                    if(!pWorker->ParseHelpMsg(chMAP))
                    {//解析不出完整数据 停止解析 继续接收
                        break;
                    }
                }
            }
            
            //结束线程
            if(pWorker->m_ServerSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ServerSocket);
            }
            pWorker->m_ServerSocket = 0;
            
            if(pWorker->m_SocketSTmp > 0)
            {
                closesocket(pWorker->m_SocketSTmp);
            }
            pWorker->m_SocketSTmp = 0;
            
            pWorker->m_nStatus = FAILD;
            
#ifdef WIN32
            if(pWorker->m_hEndEventR > 0)
            {
                CloseHandle(pWorker->m_hEndEventR);
                pWorker->m_hEndEventR = 0;
            }
#endif
            
            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收线程正常退出. ", __FILE__,__LINE__);
            }
            else
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread stop sucessed", __FILE__,__LINE__);
            }
            pWorker-> m_recvThreadExit=TRUE;
            return 0;
        }
        
#ifndef WIN32
        void* CCChannel::RecvProcTCP(void* pParam)
#else
        UINT WINAPI CCChannel::RecvProcTCP(LPVOID pParam)
#endif
        {
            CCChannel *pWorker = (CCChannel *)pParam;
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
            
            ::WaitForSingleObject(pWorker->m_hStartEventR, INFINITE);
            if(pWorker->m_hStartEventR > 0)
            {
                CloseHandle(pWorker->m_hStartEventR);
                pWorker->m_hStartEventR = 0;
            }
#endif
            
            BYTE chMAP[1000]={0};
            
            int rs = 0;
            int nLenRead=-1;
            BYTE uchTypeRead=0;
            int nBufferRate = -1;
            //	DWORD dwstartReq = CCWorker::JVGetTime();
            //	DWORD dwendtime = 0;
            //	int nReqCount = 0;
            //	STREQ *pSTPullReqs = new STREQ[5000];
            STREQS STPullReqs;
            STPullReqs.reserve(5000);
            
            pWorker->m_dBeginTime = CCWorker::JVGetTime();//视频数据计时
            pWorker->m_dTimeUsed = pWorker->m_dBeginTime;
            pWorker->m_bError = FALSE;
            
            while(TRUE)
            {
#ifndef WIN32
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || pWorker->m_bEndR)
                {
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_ServerSocket <= 0 && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#else
                if(pWorker->m_ServerSocket <= 0 || pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                {
                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                    CCWorker::jvc_sleep(1);
                    
                    if(pWorker->m_hEndEventR > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventR);
                        pWorker->m_hEndEventR = 0;
                    }
                    
                    if(pWorker->m_ServerSocket <= 0 && !pWorker->m_bDisConnectShow && pWorker->m_pWorker != NULL)
                    {//套接字已失效并且还未对外提示过
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    break;
                }
#endif
                
                if(!pWorker->m_bPass)
                {
                    CCWorker::jvc_sleep(10);
                    continue;
                }
                
                /*读缓冲区,显示*/
                nBufferRate = -1;
                uchTypeRead = 0;
                if(pWorker->m_pBuffer != NULL && pWorker->m_pBuffer->ReadPlayBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead, nBufferRate))
                {
                    if(pWorker->m_nLastRate != 100)
                    {
                        pWorker->m_nLastRate = 100;
                        pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, 100);
                    }
                    pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
                }
                else
                {//播放失败 检查是否需要提示缓冲进度
                    if(pWorker->m_bJVP2P && nBufferRate >= 0 && nBufferRate <= 100)
                    {
                        pWorker->m_nLastRate = nBufferRate;
                        //提示缓冲信息
                        pWorker->m_pWorker->BufRate(pWorker->m_nLocalChannel, 0, NULL, 0, nBufferRate);
                        
                        //if(pWorker->m_nLocalChannel == 2)
                        //{
                        //	OutputDebugString("buf........\n");
                        //}
                    }
                }
                
                if (0 > (rs = tcpreceive2(pWorker->m_ServerSocket, (char *)&pWorker->m_puchRecvBuf[pWorker->m_nRecvPos], JVNC_DATABUFLEN - pWorker->m_nRecvPos, 1)))
                {//出错
#ifndef WIN32
                    if(pWorker->m_bExit || pWorker->m_bEndR)
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        break;
                    }
#else
                    if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                    {
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        CCWorker::jvc_sleep(1);
                        
                        if(pWorker->m_hEndEventR > 0)
                        {
                            CloseHandle(pWorker->m_hEndEventR);
                            pWorker->m_hEndEventR = 0;
                        }
                        
                        break;
                    }
#endif
                    
                    if(pWorker->m_pWorker != NULL && !pWorker->m_bDisConnectShow)
                    {
                        pWorker->m_bDisConnectShow = TRUE;
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                    }
                    
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    
                    break;
                }
                else if(rs == 0)
                {//未收到任何数据
                    CCWorker::jvc_sleep(2);
                    continue;
                }
                
                pWorker->m_nRecvPos += rs;
                
                while(TRUE)
                {
                    if(!pWorker->ParseMsg(chMAP))
                    {//解析不出完整数据 停止解析 继续接收
                        break;
                    }
                }
            }
            
            if(pWorker->m_ListenSocket > 0)
            {
                pWorker->m_pWorker->pushtmpsock(pWorker->m_ListenSocket);
            }
            pWorker->m_ListenSocket = 0;
            
            if(pWorker->m_ListenSocketTCP > 0)
            {
                closesocket(pWorker->m_ListenSocketTCP);
            }
            pWorker->m_ListenSocketTCP = 0;
            
            //结束线程
            if(pWorker->m_ServerSocket > 0)
            {
                closesocket(pWorker->m_ServerSocket);
            }
            pWorker->m_ServerSocket = 0;
            
            if(pWorker->m_SocketSTmp > 0)
            {
                closesocket(pWorker->m_SocketSTmp);
            }
            pWorker->m_SocketSTmp = 0;
            
            //	pWorker->m_PartnerCtrl.ClearPartner();//
            
            pWorker->m_nStatus = FAILD;
            
#ifdef WIN32
            if(pWorker->m_hEndEventR > 0)
            {
                CloseHandle(pWorker->m_hEndEventR);
                pWorker->m_hEndEventR = 0;
            }
#endif
            
            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收线程正常退出. ", __FILE__,__LINE__);
            }
            else
            {
                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread stop sucessed", __FILE__,__LINE__);
            }
            
#ifdef WIN32
            return 0;
#else
            return NULL;
#endif
        }
        
        /****************************************************************************
         *名称  : reveivefrom
         *功能  : 非阻塞接收数据(定时)
         *参数  : [IN] s
         [IN] pchbuf
         [IN] nlen
         [IN] nflags
         [IN] from
         [IN] fromlen
         [IN] ntimeoverSec
         *返回值: >0    数据长度
         JVS_SERVER_R_OVERTIME     超时
         其他                      失败
         *其他  :
         *****************************************************************************/
#ifndef WIN32
        int CCChannel::receivefrom(SOCKET s, char *pchbuf, int nlen, int nflags, struct sockaddr * from, int * fromlen, int ntimeoverSec)
#else
        int CCChannel::receivefrom(SOCKET s, char *pchbuf, int nlen, int nflags, struct sockaddr FAR * from, int FAR * fromlen, int ntimeoverSec)
#endif
        {
#ifdef WIN32
            __try
#endif
            {
                int   status,nbytesreceived;
                if(s ==-1)
                {
                    return -1;
                }
                //ntimeoverSec = 1;
                struct   timeval   tv={ntimeoverSec,0};
                fd_set   fd;
                FD_ZERO(&fd);
                FD_SET(s,&fd);
                
//                fd_set   fds;
//                FD_ZERO(&fds);
//                FD_SET(s,&fds);
                if(ntimeoverSec==0)
                {
                    status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);
                }
                else
                {
                    status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);
                }
                switch(status)
                {
                    case   -1:
                        //"设置读取超时失败.");
                        return -1;
                    case   0:
                        //"读取注册返回信息超时(10s).";
//                        OutputDebug("recvfrom time: %d ,byte: %d, nLen: %d,fromLen: %d,line: %d, error: %d",ntimeoverSec,nbytesreceived,nlen,fromlen, __LINE__);
                        return 0;
                    default:
                        if(FD_ISSET(s,&fd))
                        {
#ifndef WIN32
                            if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,(socklen_t *)fromlen))==-1)
                            {
                                return -1;
                            }
                            else
                            {//WSAGetLastError();
                                return nbytesreceived;
                            }
#else
                            if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,fromlen))==-1)
                            {
                                //					int iii = GetLastError();
                                return -1;
                            }
                            else
				{
                                return nbytesreceived;
                            }
#endif
                        }
                }
                return -1;
            }
#ifdef WIN32
            __except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
            {
                return -1;
            }
#endif
        }
        
        /****************************************************************************
         *名称  : reveivefromm
         *功能  : 非阻塞接收数据(定时)
         *参数  : [IN] s
         [IN] pchbuf
         [IN] nlen
         [IN] nflags
         [IN] from
         [IN] fromlen
         [IN] ntimeoverSec 毫秒
         *返回值: >0    数据长度
         JVS_SERVER_R_OVERTIME     超时
         其他                      失败
         *其他  :
         *****************************************************************************/
#ifndef WIN32
        int CCChannel::receivefromm(SOCKET s, char *pchbuf, int nlen, int nflags, struct sockaddr * from, int * fromlen, int ntimeoverSec)
#else
        int CCChannel::receivefromm(SOCKET s, char *pchbuf, int nlen, int nflags, struct sockaddr FAR * from, int FAR * fromlen, int ntimeoverSec)
#endif
        {
#ifdef WIN32
            __try
#endif
            {
                int   status,nbytesreceived;
                if(s ==-1)
                {
                    return -1;
                }
                struct   timeval   tv={0,ntimeoverSec};
                fd_set   fd;
                FD_ZERO(&fd);
                FD_SET(s,&fd);
                if(ntimeoverSec==0)
                {
                    status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);
                }
                else
                {
                    status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);
                }
                switch(status)
                {
                    case   -1:
                        //"设置读取超时失败.");
                        return -1;
                    case   0:
                        //"读取注册返回信息超时(10s).";
                        return 0;
                    default:
                        if(FD_ISSET(s,&fd))
                        {
#ifndef WIN32
                            if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,(socklen_t *)fromlen))==-1)
                            {
                                return -1;
                            }
                            else
                            {
                                return nbytesreceived;
                            }
#else
                            if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,fromlen))==-1)
                            {
                                //					int iii = GetLastError();
                                return -1;
                            }
                            else
                            {
                                return nbytesreceived;
                            }
#endif
                        }
                }
                return -1;
            }
#ifdef WIN32
            __except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
            {
                return -1;
            }
#endif
        }
        /****************************************************************************
         *名称  : sendtoclient
         *功能  : 非阻塞发送数据(定时)
         *参数  :
         [IN] ntimeoverSec
         *返回值: >0    数据长度
         JVS_SERVER_R_OVERTIME     超时
         其他                      失败
         *其他  :
         *****************************************************************************/
#ifndef WIN32
        int CCChannel::sendtoclient(SOCKET s, char * pchbuf, int nlen, int nflags, const struct sockaddr * to, int ntolen, int ntimeoverSec)
#else
        int CCChannel::sendtoclient(SOCKET s, char * pchbuf, int nlen, int nflags, const struct sockaddr FAR * to, int ntolen, int ntimeoverSec)
#endif
        {
#ifdef WIN32
            __try
#endif
            {
                int   status,nbytesreceived;
                if(s ==-1)
                {
                    return -1;
                }
                struct   timeval   tv={ntimeoverSec,0};
                fd_set   fd;
                FD_ZERO(&fd);
                FD_SET(s,&fd);
                if(ntimeoverSec==0)
                {
                    status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);
                }
                else
                {
                    status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);
                }
                switch(status)
                {
                    case   -1:
                        //"设置读取超时失败.");
                        return -1;
                    case   0:
                        //"读取注册返回信息超时(10s).";
                        return 0;
                    default:
                        if(FD_ISSET(s,&fd))
                        {
                            if((nbytesreceived=sendto(s,pchbuf,nlen,nflags,to,ntolen))==-1)
                            {
                                return -1;
                            }
                            else
                            {
                                return nbytesreceived;
                            }
                        }
                }
                return -1;
            }
#ifdef WIN32
            __except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
            {
                return -1;
            }
#endif
        }
        
        /****************************************************************************
         *名称  : sendtoclient
         *功能  : 非阻塞发送数据(定时)
         *参数  :
         [IN] ntimeoverm
         *返回值: >0    数据长度
         JVS_SERVER_R_OVERTIME     超时
         其他                      失败
         *其他  :
         *****************************************************************************/
#ifndef WIN32
        int CCChannel::sendtoclientm(SOCKET s, char * pchbuf, int nlen, int nflags, const struct sockaddr * to, int ntolen, int ntimeoverm)
#else
        int CCChannel::sendtoclientm(SOCKET s, char * pchbuf, int nlen, int nflags, const struct sockaddr FAR * to, int ntolen, int ntimeoverm)
#endif
        {
#ifdef WIN32
            __try
#endif
            {
                int   status,nbytesreceived;
                if(s ==-1)
                {
                    return -1;
                }
                struct   timeval   tv={0,ntimeoverm};
                fd_set   fd;
                FD_ZERO(&fd);
                FD_SET(s,&fd);
                if(ntimeoverm==0)
                {
                    status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);
                }
                else
                {
                    status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);
                }
                switch(status)
                {
                    case   -1:
                        //"设置读取超时失败.");
                        return -1;
                    case   0:
                        //"读取注册返回信息超时(10s).";
                        return 0;
                    default:
                        if(FD_ISSET(s,&fd))
                        {
                            if((nbytesreceived=sendto(s,pchbuf,nlen,nflags,to,ntolen))==-1)
                            {
                                return -1;
                            }
                            else
                            {
                                return nbytesreceived;
                            }
                        }
                }
                return -1;
            }
#ifdef WIN32
            __except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
            {
                return -1;
            }
#endif
        }
        
        /****************************************************************************
         *名称  : tcpreceive
         *功能  : 非阻塞接收数据(定时)
         *参数  :
         [IN] ntimeoverSec
         *返回值: >0    数据长度
         *其他  :
         *****************************************************************************/
        int CCChannel::tcpreceive(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec)
        {
            int   status,nbytesreceived;
            if(s==-1)
            {
                return   0;
            }
            struct   timeval   tv={ntimeoverSec,0};
            fd_set   fd;
            FD_ZERO(&fd);
            FD_SET(s,&fd);
            if(ntimeoverSec==0)
            {
                status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);
            }
            else
            {
                status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);
            }
            switch(status)
            {
                case   -1:
                    //printf("read   select   error\n");
                    return   -1;
                case   0:
                    //printf("receive   time   out\n");
                    return   0;
                default:
                    if(FD_ISSET(s,&fd))
                    {
                        if((nbytesreceived=recv(s,pchbuf,nsize,0))==-1)
                        {
                            //	int kkk=0;
                            //	kkk=WSAGetLastError();
                            return   0;
                        }
                        else
                        {
                            return   nbytesreceived;
                        }
                    }
            }
            return 0;
        }
        
        int CCChannel::tcpreceive2(SOCKET s,char *pchbuf,int nsize,int ntimeoverMSec)
        {
            int   status,nbytesreceived;
            if(s==-1)
            {
                return   0;
            }
            struct   timeval   tv={0,ntimeoverMSec};
            fd_set   fd;
            FD_ZERO(&fd);
            FD_SET(s,&fd);
            if(ntimeoverMSec==0)
            {
                status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);
            }
            else
            {
                status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);
            }
            switch(status)
            {
                case   -1:
                    //printf("read   select   error\n");
                    return   -1;
                case   0:
                    //printf("receive   time   out\n");
                    return   0;
                default:
                    if(FD_ISSET(s,&fd))
                    {
                        if((nbytesreceived=recv(s,pchbuf,nsize,0))==-1)
                        {
                            //	int kkk=0;
                            //	kkk=WSAGetLastError();
                            return   0;
                        }
                        else
                        {
                            return   nbytesreceived;
                        }
                    }
            }
            return 0;
        }
        
        /****************************************************************************
         *名称  : tcpsend
         *功能  : 非阻塞发送数据(定时)
         *参数  :
         [IN] ntimeoverSec
         *返回值: >0    数据长度
         *其他  :
         *****************************************************************************/
        int CCChannel::tcpsend(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec)
        {
            int   status,nbytessended;
            if(s==-1)
            {
                return   0;
            }
            struct   timeval   tv={ntimeoverSec,0};
            fd_set   fd;
            FD_ZERO(&fd);
            FD_SET(s,&fd);
            if(ntimeoverSec==0)
            {
                status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);
            }
            else
            {
                status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);
            }
            switch(status)
            {
                case   -1:
                    //printf("read   select   error\n");
                    return   -1;
                case   0:
                    //printf("receive   time   out\n");
                    return   0;
                default:
                    if(FD_ISSET(s,&fd))
                    {
                        if((nbytessended=send(s,pchbuf,nsize,0))==-1)
                        {
                            return   0;
                        }
                        else
                        {
                            return   nbytessended;
                        }
                    }
            }
            return 0;
        }
        
        /*****************************************************************************
         *名称  : WaitThreadExit
         *功能  : 等待指定的线程关闭，若在300毫秒内没有主动退出，则强制关闭该线程
         *参数  : [IN] HANDEL hThread	 需要结束的线程
         *返回值:
         *其他  :s
         ****************************************************************************/
#ifdef WIN32
        void CCChannel::WaitThreadExit(HANDLE &hThread)
        {
            DWORD dwExitCode;
            unsigned int iWaitMilliSecond = 0;
            if (hThread > 0)
            {
                for ( ;; )
                {
                    if ( ::GetExitCodeThread(hThread, &dwExitCode) )
                    {
                        if (dwExitCode != STILL_ACTIVE)
                        {
                            break;
                        }
                        else
                        {
                            CCWorker::jvc_sleep(1);
                            iWaitMilliSecond += 1;
                            if (iWaitMilliSecond > 300) //等待300毫秒后强行退出线程
                            {
                                if ( TerminateThread(hThread, 1) )
                                {
                                    //OutputDebugString("thread force*********************************\r\n");
                                    //"线程强制结束.
                                    //							if(hThread> 0)
                                    //							{
                                    //								CloseHandle(hThread);
                                    //							}
                                    
                                    break;
                                }
                                else
                                {
                                    //"线程强制结束失败."
                                    iWaitMilliSecond = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                
                //		if(hThread> 0)
                //		{
                //			CloseHandle(hThread);
                //		}
                //		hThread = 0;
            }
        }
#endif
        
        int CCChannel::Send2A(BYTE *puchBuf, int nSize)
        {
            int nret = 0;
            if(puchBuf != NULL && nSize > 0 && m_ServerSocket > 0)
            {
#ifndef WIN32
                pthread_mutex_lock(&m_ct);
#else
                EnterCriticalSection(&m_ct);
#endif
                
                nret = UDT::send(m_ServerSocket, (char*)puchBuf, nSize, 0);
                
#ifndef WIN32
                pthread_mutex_unlock(&m_ct);
#else
                LeaveCriticalSection(&m_ct);
#endif
            }
            
            return nret;
        }
        
        //更新当前伙伴BM
        void CCChannel::SetBM(unsigned int unChunkID, int nCount, DWORD dwCTime[10])
        {
            if(unChunkID <= 0 || nCount <= 0)
            {
                return;
            }
            
            unsigned int unlastid = m_unBeginChunkID + m_nChunkCount;
            if(unlastid > 0)
            {
                unlastid -= 1;
            }
            else
            {
                unlastid = 0;
            }
            unsigned int unnewid = unChunkID;// + nCount - 1;
            m_unBeginChunkID = unChunkID - nCount + 1;
            m_nChunkCount = nCount;
            if(unnewid > unlastid)
            {//本次有新数据块
                m_PartnerCtrl.CheckIfNeedSetBuf(unChunkID, nCount, dwCTime, TRUE);
            }
        }
        
        //检查是否有该数据片，是否向该伙伴请求过当前数据片，是的话重置该请求，并更新伙伴性能
        BOOL CCChannel::CheckREQ(unsigned int unChunkID)
        {
            if(unChunkID <= 0)
            {
                return FALSE;
            }
            
            if(unChunkID >= m_unBeginChunkID && unChunkID < m_unBeginChunkID+m_nChunkCount)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        
        BOOL CCChannel::SendBMDREQ2A(unsigned int unChunkID)
        {
            if(m_ServerSocket <= 0 || unChunkID <= 0)
            {
                return FALSE;
            }
            
            //类型(1) + 长度(4) + ChunkID(4)
            int nSize = 4;
            BYTE data[10]={0};
            data[0] = JVN_REQ_BMD;
            memcpy(&data[1], &nSize, 4);
            nSize = 9;
            memcpy(&data[5], &unChunkID, 4);
            
#ifndef WIN32
            pthread_mutex_lock(&m_ct);
#else
            EnterCriticalSection(&m_ct);
#endif
            
            int ss=0;
            int ssize=0;
            while(ssize < nSize)
            {
                if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, jvs_min(nSize - ssize, 20000), 0)))
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
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送BMDREQ数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    else
                    {
                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Send BMDREQ Data failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                    }
                    
#ifndef WIN32
                    pthread_mutex_unlock(&m_ct);
#else
                    LeaveCriticalSection(&m_ct);
#endif
                    
                    return FALSE;
                }
            }
#ifndef WIN32
            pthread_mutex_unlock(&m_ct);
#else
            LeaveCriticalSection(&m_ct);
#endif
            
            m_PartnerCtrl.SetReqStartTime(TRUE, unChunkID, CCWorker::JVGetTime());
            
            return TRUE;
        }
        
        BOOL CCChannel::SendDataTCP(BYTE uchType, BYTE *pBuffer,int nSize)
        {
            if(m_ServerSocket > 0)
            {
                BYTE data[5 + 2*JVN_BAPACKDEFLEN]={0};
                memset(data, 0, sizeof(data));
                switch(uchType)
                {
                    case JVN_CMD_KEEPLIVE://回复心跳
                    {
                        //OutputDebugString("SendDataTCP JVN_CMD_KEEPLIVE\n");
                        //////////////////////////////////////////////////////////////////////////
                        int ss=0;
                        int ssize=0;
                        int nLen = nSize;
                        while(ssize < nLen)
                        {
#ifndef WIN32
                            if ((ss = send(m_ServerSocket, (char *)pBuffer + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                            {
                                if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                    if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)pBuffer + ssize, nLen - ssize, 0)))
                                    {
                                        int kkk = WSAGetLastError();
                                        if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                        {
                                            CCWorker::jvc_sleep(1);
                                            continue;
                                        }
                                        
                                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                        }
                                        
                                        //////////////////////////////////////////////////////////////////////////
                                        if(m_ServerSocket > 0)
                                        {
                                            closesocket(m_ServerSocket);
                                            m_ServerSocket = 0;
                                        }
                                        //////////////////////////////////////////////////////////////////////////
                                        return FALSE;
                                    }
                                ssize += ss;
                            }
                            //////////////////////////////////////////////////////////////////////////
                        }
                        break;
                    case JVN_REQ_CHECK://请求录像检索
                        {
                            if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
                            {
                                //BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
                                data[0] = uchType;
                                memcpy(&data[1], &nSize, 4);
                                memcpy(&data[5], pBuffer, nSize);
                                send(m_ServerSocket, (char *)data,5 + nSize, 0);
                            }
                        }
                        break;
                    case JVN_CMD_LADDR://本地地址
                    case JVN_REQ_DOWNLOAD://请求录像下载
                    case JVN_REQ_PLAY://请求远程回放
                        {
                            if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
                            {
                                //BYTE data[5 + MAX_PATH]={0};
                                data[0] = uchType;
                                memcpy(&data[1], &nSize, 4);
                                memcpy(&data[5], pBuffer, nSize);
                                
                                int nLen = nSize + 5;
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
#ifndef WIN32
                                    if ((ss = send(m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                    {
                                        if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                            if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                            {
                                                int kkk = WSAGetLastError();
                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                {
                                                    CCWorker::jvc_sleep(1);
                                                    continue;
                                                }
                                                
                                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                {
                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                }
                                                else
                                                {
                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                }
                                                
                                                //////////////////////////////////////////////////////////////////////////
                                                if(m_ServerSocket > 0)
                                                {
                                                    closesocket(m_ServerSocket);
                                                    m_ServerSocket = 0;
                                                }
                                                //////////////////////////////////////////////////////////////////////////
                                                return FALSE;
                                            }
                                        ssize += ss;
                                    }
                                    if(uchType == JVN_REQ_DOWNLOAD || uchType == JVN_REQ_PLAY)
                                    {
                                        m_bDAndP = TRUE;
                                    }
                                }
                            }
                            break;
                            
                        case JVN_CMD_PLAYSEEK://播放定位
                        case JVN_CMD_YTCTRL://云台控制
                            {
                                if(pBuffer != NULL && nSize == 4)
                                {
                                    //BYTE data[9]={0};
                                    data[0] = uchType;
                                    memcpy(&data[1], &nSize, 4);
                                    memcpy(&data[5], pBuffer, nSize);
                                    send(m_ServerSocket, (char *)data,9, 0);
                                }
                            }
                            break;
                        case JVN_RSP_CHATACCEPT://同意语音
                            m_bAcceptChat = TRUE;
                            goto DOSEND;
                        case JVN_RSP_TEXTACCEPT://同意文本
                            m_bAcceptText = TRUE;
                            goto DOSEND;
                        case JVN_REQ_CHAT://请求语音聊天
                        case JVN_REQ_TEXT://请求文本聊天
                        case JVN_CMD_DISCONN://断开连接
                        case JVN_CMD_PLAYSTOP://暂停播放
                        case JVN_CMD_DOWNLOADSTOP://停止下载数据
                        case JVN_CMD_CHATSTOP://停止语音聊天
                        case JVN_CMD_TEXTSTOP://停止文本聊天
                        case JVN_CMD_VIDEO://请求实时监控数据
                        case JVN_CMD_VIDEOPAUSE://请求实时监控数据
                        case JVN_CMD_PLAYUP://快进
                        case JVN_CMD_PLAYDOWN://慢放
                        case JVN_CMD_PLAYDEF://原速播放
                        case JVN_CMD_PLAYPAUSE://暂停播放
                        case JVN_CMD_PLAYGOON://继续播放
                        case JVN_DATA_DANDP://下载回放确认
                        case JVN_CMD_PLIST://请求伙伴列表
                            {
                            DOSEND:
                                int nLen = 5;
                                //BYTE data[5]={0};
                                data[0] = uchType;
                                
                                int ss=0;
                                int ssize=0;
                                while(ssize < nLen)
                                {
#ifndef WIN32
                                    if ((ss = send(m_ServerSocket, (char *)data + ssize, nLen-ssize, MSG_NOSIGNAL)) <= 0)
                                    {
                                        if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                            if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                            {
                                                int kkk = WSAGetLastError();
                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                {
                                                    CCWorker::jvc_sleep(1);
                                                    continue;
                                                }
                                                
                                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                {
                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                }
                                                else
                                                {
                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                }
                                                
                                                //////////////////////////////////////////////////////////////////////////
                                                if(m_ServerSocket > 0)
                                                {
                                                    closesocket(m_ServerSocket);
                                                    m_ServerSocket = 0;
                                                }
                                                //////////////////////////////////////////////////////////////////////////
                                                
                                                return FALSE;
                                            }
                                        ssize += ss;
                                    }
                                }
                                break;
                            case JVN_DATA_OK://视频帧确认
                                {
                                    int nLen = 9;
                                    //BYTE data[9]={0};
                                    data[0] = uchType;
                                    memcpy(&data[1], &nSize, 4);
                                    if(pBuffer != NULL)
                                    {
                                        memcpy(&data[5], pBuffer, nSize);
                                    }
                                    
                                    int ss=0;
                                    int ssize=0;
                                    while(ssize < nLen)
                                    {
#ifndef WIN32
                                        if ((ss = send(m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                        {
                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                                {
                                                    int kkk = WSAGetLastError();
                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                    {
                                                        CCWorker::jvc_sleep(1);
                                                        continue;
                                                    }
                                                    
                                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                    {
                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                    }
                                                    else
                                                    {
                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                    }
                                                    
                                                    //////////////////////////////////////////////////////////////////////////
                                                    if(m_ServerSocket > 0)
                                                    {
                                                        closesocket(m_ServerSocket);
                                                        m_ServerSocket = 0;
                                                    }
                                                    //////////////////////////////////////////////////////////////////////////
                                                    return FALSE;
                                                }
                                            ssize += ss;
                                        }
                                    }
                                    break;
                                case JVN_RSP_TEXTDATA://文本数据
                                    {
                                        if(m_bAcceptText)
                                        {
                                            if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                                            {
                                                int nLen = 5+nSize;
                                                //BYTE data[JVN_BAPACKDEFLEN]={0};
                                                data[0] = uchType;
                                                memcpy(&data[1], &nSize, 4);
                                                memcpy(&data[5], pBuffer, nSize);
                                                
                                                int ss=0;
                                                int ssize=0;
                                                while(ssize < nLen)
                                                {
#ifndef WIN32
                                                    if ((ss = send(m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                    {
                                                        if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                            if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                                            {
                                                                int kkk = WSAGetLastError();
                                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                {
                                                                    CCWorker::jvc_sleep(1);
                                                                    continue;
                                                                }
                                                                
                                                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                {
                                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                                }
                                                                else
                                                                {
                                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                                }
                                                                
                                                                
                                                                //////////////////////////////////////////////////////////////////////////
                                                                if(m_ServerSocket > 0)
                                                                {
                                                                    closesocket(m_ServerSocket);
                                                                    m_ServerSocket = 0;
                                                                }
                                                                //////////////////////////////////////////////////////////////////////////
                                                                return FALSE;
                                                            }
                                                        ssize += ss;
                                                    }
                                                }
                                            }
                                        }
                                        break;
                                    case JVN_RSP_CHATDATA://语音数据
                                        {
                                            if(m_bAcceptChat)
                                            {
                                                if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                                                {
                                                    int nLen = 5+nSize;
                                                    //BYTE data[JVN_BAPACKDEFLEN]={0};
                                                    data[0] = uchType;
                                                    memcpy(&data[1], &nSize, 4);
                                                    memcpy(&data[5], pBuffer, nSize);
                                                    
                                                    int ss=0;
                                                    int ssize=0;
                                                    while(ssize < nLen)
                                                    {
#ifndef WIN32
                                                        if ((ss = send(m_ServerSocket, (char *)data + ssize,jvs_min(nLen - ssize, 20000) , MSG_NOSIGNAL)) <= 0)
                                                        {
                                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, 20000), 0)))
                                                                {
                                                                    int kkk = WSAGetLastError();
                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                    {
                                                                        CCWorker::jvc_sleep(1);
                                                                        continue;
                                                                    }
                                                                    
                                                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                    {
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                                    }
                                                                    else
                                                                    {
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                                    }
                                                                    
                                                                    //////////////////////////////////////////////////////////////////////////
                                                                    if(m_ServerSocket > 0)
                                                                    {
                                                                        closesocket(m_ServerSocket);
                                                                        m_ServerSocket = 0;
                                                                    }
                                                                    //////////////////////////////////////////////////////////////////////////
                                                                    return FALSE;
                                                                }
                                                            ssize += ss;
                                                        }
                                                    }
                                                }
                                            }
                                            break;
                                        case JVN_CMD_ONLYI://只要关键帧
                                        case JVN_CMD_FULL://回复满帧发送
                                        case JVN_CMD_ALLAUDIO://请求音频全转发
                                            {
                                                int nLen = 5+nSize;
                                                //BYTE data[JVN_BAPACKDEFLEN]={0};
                                                data[0] = uchType;
                                                
                                                if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                                                {
                                                    nLen = 5+nSize;
                                                    memcpy(&data[1], &nSize, 4);
                                                    memcpy(&data[5], pBuffer, nSize);
                                                }
                                                else
                                                {
                                                    nLen = 5;
                                                }
                                                
                                                int ss=0;
                                                int ssize=0;
                                                while(ssize < nLen)
                                                {
#ifndef WIN32
                                                    if ((ss = send(m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                    {
                                                        if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                            if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                                            {
                                                                int kkk = WSAGetLastError();
                                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                {
                                                                    CCWorker::jvc_sleep(1);
                                                                    continue;
                                                                }
                                                                
                                                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                {
                                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                                }
                                                                else
                                                                {
                                                                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                                }
                                                                
                                                                //////////////////////////////////////////////////////////////////////////
                                                                if(m_ServerSocket > 0)
                                                                {
                                                                    closesocket(m_ServerSocket);
                                                                    m_ServerSocket = 0;
                                                                }
                                                                //////////////////////////////////////////////////////////////////////////
                                                                return FALSE;
                                                            }
                                                        ssize += ss;
                                                    }
                                                }
                                                break;
                                            case JVN_REQ_RATE:
                                                {
                                                    int nLen = 5+nSize;
                                                    //BYTE data[JVN_BAPACKDEFLEN]={0};
                                                    data[0] = uchType;
                                                    
                                                    if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                                                    {
                                                        nLen = 5+nSize;
                                                        memcpy(&data[1], &nSize, 4);
                                                        memcpy(&data[5], pBuffer, nSize);
                                                    }
                                                    else
                                                    {
                                                        break;
                                                    }
                                                    
                                                    int ss=0;
                                                    int ssize=0;
                                                    while(ssize < nLen)
                                                    {
#ifndef WIN32
                                                        if ((ss = send(m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                        {
                                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                                                {
                                                                    int kkk = WSAGetLastError();
                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                    {
                                                                        CCWorker::jvc_sleep(1);
                                                                        continue;
                                                                    }
                                                                    
                                                                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                    {
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                                    }
                                                                    else
                                                                    {
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                                    }
                                                                    
                                                                    //////////////////////////////////////////////////////////////////////////
                                                                    if(m_ServerSocket > 0)
                                                                    {
                                                                        closesocket(m_ServerSocket);
                                                                        m_ServerSocket = 0;
                                                                    }
                                                                    //////////////////////////////////////////////////////////////////////////
                                                                    return FALSE;
                                                                }
                                                            ssize += ss;
                                                        }
                                                    }
                                                    break;
                                                case JVN_DATA_O://自定义数据
                                                    {
                                                        if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
                                                        {
                                                            int nLen = 5+nSize;
                                                            //BYTE data[JVN_BAPACKDEFLEN]={0};
                                                            data[0] = uchType;
                                                            memcpy(&data[1], &nSize, 4);
                                                            memcpy(&data[5], pBuffer, nSize);
                                                            
                                                            int ss=0;
                                                            int ssize=0;
                                                            while(ssize < nLen)
                                                            {
#ifndef WIN32
                                                                if ((ss = send(m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                                {
                                                                    if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                        if (SOCKET_ERROR == (ss = send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                                                                        {
                                                                            int kkk = WSAGetLastError();
                                                                            if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                            {
                                                                                CCWorker::jvc_sleep(1);
                                                                                continue;
                                                                            }
                                                                            
                                                                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                            {
                                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败", __FILE__,__LINE__);
                                                                            }
                                                                            else
                                                                            {
                                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed", __FILE__,__LINE__);
                                                                            }
                                                                            
                                                                            //////////////////////////////////////////////////////////////////////////
                                                                            if(m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(m_ServerSocket);
                                                                                m_ServerSocket = 0;
                                                                            }
                                                                            //////////////////////////////////////////////////////////////////////////
                                                                            return FALSE;
                                                                        }
                                                                    ssize += ss;
                                                                }
                                                            }
                                                        }
                                                        break;
                                                    default:
                                                        break;
                                                    }
                                                }
                                                return TRUE;
                                            }
                                            
                                            void CCChannel::ProcessDisConnect()
                                            {
                                                BYTE buff[5]={0};
                                                buff[0] = JVN_CMD_DISCONN;
                                                int ss=0,ssize=0;
                                                if(m_nProtocolType==TYPE_PC_UDP || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
                                                {
                                                    while(ssize<5)
                                                    {
                                                        if((ss=UDT::send(m_ServerSocket,(char *)buff,5-ssize,0)) < 0)
                                                        {
                                                            break;
                                                        }
                                                        else if(ss == 0)
                                                        {
                                                            CCWorker::jvc_sleep(1);
                                                            continue;
                                                        }
                                                        ssize+=ss;
                                                    }
                                                }
                                                else if(m_nProtocolType==TYPE_PC_TCP || m_nProtocolType==TYPE_MO_TCP)
                                                {
                                                    while(ssize<5)
                                                    {
#ifndef WIN32
                                                        if ((ss = send(m_ServerSocket, (char *)buff,5-ssize, MSG_NOSIGNAL)) <= 0)
                                                        {
                                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                if((ss=send(m_ServerSocket,(char *)buff,5-ssize,0)) == SOCKET_ERROR)
                                                                {
                                                                    int kkk = WSAGetLastError();
                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                    {
                                                                        CCWorker::jvc_sleep(1);
                                                                        continue;
                                                                    }
                                                                    break;
                                                                }
                                                            ssize+=ss;
                                                        }
                                                    }
                                                }
                                                
                                                //预先打洞函数
                                                void CCChannel::PrePunch(int sock,SOCKADDR_IN addrA)
                                                {
                                                    if(m_pWorker != NULL)
                                                    {//测试提示
                                                        m_pWorker->NormalData(m_nLocalChannel,0x20,(BYTE*)&addrA, sizeof(SOCKADDR_IN), 0,0);
                                                    }
                                                    char ch[30]={0};
                                                    ch[0] = JVN_CMD_TRYTOUCH;
                                                    sendtoclient(sock,ch,20,0,(SOCKADDR *)&addrA, sizeof(SOCKADDR),1);//原发了1，为防止最小包限制，增加至20
                                                    CCWorker::jvc_sleep(2);//5
                                                    
                                                    /*向前后20个端口打洞(预测)*/
                                                    SOCKADDR_IN addrB;
                                                    memcpy(&addrB,&addrA,sizeof(SOCKADDR_IN));
                                                    addrB.sin_port = htons(ntohs(addrA.sin_port)+1);
                                                    sendtoclient(sock,ch,20,0,(SOCKADDR *)&addrB, sizeof(SOCKADDR),1);//原发了1，为防止最小包限制，增加至20
                                                    
                                                    SOCKADDR_IN A;
                                                    memset(&A, 0, sizeof(SOCKADDR_IN));
                                                    memcpy(&A,&addrA,sizeof(SOCKADDR_IN));
                                                    
                                                    int port = ntohs(A.sin_port) - 20;
                                                    for(int k = 0;k < 40;k ++)
                                                    {
                                                        A.sin_port = htons(port + k);
                                                        
                                                        sendtoclient(sock, ch, 20, 0, (SOCKADDR *)&(A), sizeof(SOCKADDR),1);
                                                    }
                                                }
                                                //p2p打洞函数
                                                BOOL CCChannel::Punch(int nYSTNO,int sock,SOCKADDR_IN *addrA)
                                                {
                                                    if(m_pWorker != NULL)
                                                    {//测试提示
                                                        m_pWorker->NormalData(m_nLocalChannel,0x20,(BYTE*)addrA, sizeof(SOCKADDR_IN), 0,0);
                                                    }
                                                    char ch[30]={0};
                                                    ch[0] = JVN_CMD_TRYTOUCH;
                                                    //进行打洞
                                                    SOCKADDR_IN addrAtmp,addrT;
                                                    
                                                    int ii = 0;
                                                    int ll=sizeof(SOCKADDR_IN);
                                                    int nMax = 0;
                                                    memcpy(&addrT,addrA,sizeof(SOCKADDR_IN));
                                                    addrT.sin_port=htons(ntohs(addrT.sin_port)+1);
                                                    while(TRUE)
                                                    {
                                                        ch[0] = JVN_CMD_TRYTOUCH;
                                                        sendtoclientm(sock,ch,20,0,(SOCKADDR *)addrA, sizeof(SOCKADDR),200);//原发了1，为防止最小包限制，增加至20
                                                        //		Sleep(5);
                                                        sendtoclientm(sock,ch,20,0,(SOCKADDR *)&addrT, sizeof(SOCKADDR),200);//原发了1，为防止最小包限制，增加至20
                                                        
                                                        SOCKADDR_IN A;
                                                        memset(&A, 0, sizeof(SOCKADDR_IN));
                                                        memcpy(&A,&addrA,sizeof(SOCKADDR_IN));
                                                        
                                                        int port = ntohs(A.sin_port) - 20;
                                                        for(int k = 0;k < 40;k ++)
                                                        {
                                                            A.sin_port = htons(port + k);
                                                            
                                                            sendtoclientm(sock, ch, 20, 0, (SOCKADDR *)&(A), sizeof(SOCKADDR),200);
                                                        }
                                                        ii = 0;
                                                        ii = receivefrom(sock, ch, 20, 0, (SOCKADDR *)&addrAtmp, &ll,1);//原最多发了5，为防止最小包限制，增加至20
                                                        if(ii > 0)
                                                        {
                                                            if(ii == 8)
                                                            {//完整的打洞包，严格判断
                                                                int ntmp = 0;
                                                                memcpy(&ntmp, &ch[0], 4);
                                                                if(ntmp == JVN_CMD_TRYTOUCH)
                                                                {
                                                                    int nyst=0;
                                                                    memcpy(&nyst, &ch[4], 4);
                                                                    if(nyst == nYSTNO)
                                                                    {//打洞包合法，可以进行连接
                                                                        memcpy(addrA, &addrAtmp, sizeof(SOCKADDR_IN));
                                                                        return TRUE;
                                                                    }
                                                                }
                                                                //打洞包无效，重新接收
                                                            }
                                                            else
                                                            {//不完整的打洞包，只判断地址是否相符
                                                                if(addrAtmp.sin_addr.s_addr == (*addrA).sin_addr.s_addr)
                                                                {//目标和实际相符，暂且认为是正确的打洞包(实际上局域网里多个主控时这样判断存在漏洞)
                                                                    memcpy(addrA, &addrAtmp, sizeof(SOCKADDR_IN));
                                                                    return TRUE;
                                                                }
                                                                //打洞包无效，重新接收
                                                            }
                                                        }
                                                        
                                                        nMax ++;
                                                        if(nMax > 1)
                                                        {
                                                            return FALSE;
                                                        }
                                                    }
                                                    return FALSE;
                                                }
                                                
                                                void CCChannel::GetPartnerInfo(char *pMsg, int &nSize)
                                                {//是否多播(1)+在线总个数(4)+伙伴总个数(4)+[IP(16) + port(4)+连接状态(1)+协议(1)+下载速度(4)+下载总量(4)+上传总量(4)] +[...]...
                                                    DWORD dwend = CCWorker::JVGetTime();
                                                    STPTINFO stinfo;
                                                    if(m_pOldChannel == NULL)
                                                    {
                                                        sprintf(stinfo.chIP,"%s",inet_ntoa(m_addressA.sin_addr));
                                                    }
                                                    else
                                                    {
                                                        sprintf(stinfo.chIP,"%s","0.0.0.0");
                                                    }
                                                    
                                                    stinfo.uchStatus = 1;
                                                    stinfo.uchProcType = 0;
                                                    if(!m_bTURN)
                                                    {
                                                        if(m_pOldChannel == NULL)
                                                        {
                                                            stinfo.nPort = ntohs(m_addressA.sin_port);
                                                        }
                                                        else
                                                        {
                                                            stinfo.nPort = 0;
                                                        }
                                                        
                                                        stinfo.uchType = 5;//(m_bTURN?6:5);//5;
                                                    }
                                                    else
                                                    {
                                                        stinfo.nPort = 0;//ntohs(m_addressA.sin_port);
                                                        stinfo.uchType = 6;//(m_bTURN?6:5);//5;
                                                    }
                                                    if(dwend > m_dwLastInfoTime)
                                                    {
                                                        stinfo.nDownSpeed = (int)((float)((m_nDownLoadTotalMB - m_nLastDownLoadTotalMB)*1024000 + (m_nDownLoadTotalKB - m_nLastDownLoadTotalKB)*1024 + (m_nDownLoadTotalB-m_nLastDownLoadTotalB))/(dwend-m_dwLastInfoTime));
                                                    }
                                                    else
                                                    {
                                                        stinfo.nDownSpeed = 0;
                                                    }
                                                    
                                                    
                                                    stinfo.fDownTotal = m_nDownLoadTotalMB + (float)((float)m_nDownLoadTotalKB/1000) + (float)((float)m_nDownLoadTotalB/1024000);//M
                                                    stinfo.fUpTotal = 0;
                                                    
                                                    m_dwLastInfoTime = dwend;
                                                    m_nLastDownLoadTotalMB = m_nDownLoadTotalMB;
                                                    m_nLastDownLoadTotalKB = m_nDownLoadTotalKB;
                                                    m_nLastDownLoadTotalB = m_nDownLoadTotalB;
                                                    if(sizeof(STPTINFO)+9 > nSize)
                                                    {
                                                        return;
                                                    }
                                                    memcpy(&pMsg[9], &stinfo, sizeof(STPTINFO));
                                                    
                                                    pMsg[0] = m_bJVP2P?1:0;
                                                    if(m_pHelpConn != NULL)
                                                    {
                                                        if(m_pchPartnerInfo != NULL)
                                                        {
                                                            memcpy(pMsg,m_pchPartnerInfo,m_nPartnerLen);
                                                            nSize = m_nPartnerLen;
                                                        }
                                                        else
                                                        {
                                                            int ntotal = 1;
                                                            memset(pMsg, 0, 9);
                                                            memcpy(&pMsg[1], &ntotal, 4);
                                                            memcpy(&pMsg[5], &ntotal, 4);
                                                            nSize = sizeof(STPTINFO)+9;
                                                        }
                                                        
                                                        return;
                                                        
                                                    }
                                                    if(!m_bJVP2P)
                                                    {
                                                        int ntotal = 1;
                                                        memset(pMsg, 0, 9);
                                                        memcpy(&pMsg[1], &ntotal, 4);
                                                        memcpy(&pMsg[5], &ntotal, 4);
                                                        nSize = sizeof(STPTINFO)+9;
                                                        return;
                                                    }
                                                    
                                                    m_PartnerCtrl.GetPartnerInfo(pMsg, nSize, dwend);
                                                }
                                                
                                                void CCChannel::ClearBuffer()
                                                {
                                                    if(m_pBuffer != NULL && (!m_bJVP2P || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP))
                                                    {
                                                        m_pBuffer->ClearBuffer();
                                                    }
                                                }
                                                
                                                /****************************************************************************
                                                 *名称  : connectnb
                                                 *功能  : 非阻塞连接(定时)
                                                 *参数  : [IN] s
                                                 [IN] to
                                                 [IN] tolen
                                                 [IN] ntimeoverSec
                                                 *返回值: >0    数据长度
                                                 JVS_SERVER_R_OVERTIME     超时
                                                 其他                      失败
                                                 *其他  :
                                                 *****************************************************************************/
#ifndef WIN32
                                                int CCChannel::connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec)
#else
                                                int CCChannel::connectnb(SOCKET s,struct sockaddr FAR * to,int namelen,int ntimeoverSec)
#endif
                                                {
#ifdef WIN32
                                                    __try
#endif
                                                    {
                                                        if(s ==-1)
                                                        {
                                                            return -1;
                                                        }
                                                        struct timeval tv={ntimeoverSec,0};
                                                        fd_set   fd;
                                                        FD_ZERO(&fd);
                                                        FD_SET(s,&fd);
                                                        
                                                        int nRet = connect( s, to, namelen);
#ifndef WIN32
                                                        if (nRet != 0)
#else
                                                            if ( SOCKET_ERROR == nRet )
#endif
                                                            {
                                                                switch (select(s + 1, NULL, &fd, NULL, &tv))
                                                                {
                                                                    case -1:
                                                                        return -1;//错误
                                                                        break;
                                                                    case 0:
                                                                        return -1;//超时
                                                                        break;
                                                                    default: //连接成功或者失败都会writeable
                                                                        char error = 0;
                                                                        
#ifndef WIN32
                                                                        socklen_t len = sizeof(error);//sizeof(SOCKET);
#else
                                                                        int len = sizeof(SOCKET);//int len = sizeof(error);//sizeof(SOCKET);
#endif
                                                                        //成功的话error应该为0
                                                                        if ((0 == getsockopt(s,SOL_SOCKET,SO_ERROR,&error,&len)))
                                                                        {
                                                                            if(0 == error)
                                                                            {
                                                                                return 0;
                                                                            }
                                                                        }
                                                                        break;
                                                                }
                                                            }
                                                            else
                                                            {
                                                                return 0;
                                                            }
                                                        return -1;
                                                    }
#ifdef WIN32
                                                    __except(EXCEPTION_EXECUTE_HANDLER)//catch (...)
                                                    {
                                                        return -1;
                                                    }
#endif
                                                }
                                                
                                                //发送udp数据(是否一次发送完整)
                                                int CCChannel::udpsenddata(UDTSOCKET sock, char *pchbuf, int nsize, BOOL bcomplete)
                                                {
                                                    if(sock <= 0 || pchbuf == NULL || nsize <= 0)
                                                    {//出错
                                                        return -1;
                                                    }
                                                    
                                                    int ssize = 0;
                                                    int ss = 0;
                                                    while (ssize < nsize)
                                                    {
                                                        if(0 < (ss = UDT::send(sock, pchbuf + ssize,jvs_min(nsize - ssize, 20000) , 0)))
                                                        {//部分(或全部)发送成功
                                                            ssize += ss;
                                                        }
                                                        else if(ss == 0)
                                                        {//发送失败但连接没问题
                                                            if(!bcomplete)
                                                            {//不要求一次就全部发送成功
                                                                break;
                                                            }
                                                            
                                                            CCWorker::jvc_sleep(1);
                                                            continue;//要求一次发送完整 继续
                                                        }
                                                        else
                                                        {//连接出问题
                                                            //异常关闭连接
                                                            return -1;
                                                        }
                                                    }
                                                    
                                                    return ssize;
                                                }
                                                //发送tcp数据(是否一次发送完整)
                                                int CCChannel::tcpsenddata(SOCKET sock, char *pchbuf, int nsize, BOOL bcomplete)
                                                {
                                                    if(sock <= 0 || pchbuf == NULL || nsize <= 0)
                                                    {//出错
                                                        return -1;
                                                    }
                                                    
                                                    int ssize = 0;
                                                    int ss = 0;
                                                    while (ssize < nsize)
                                                    {
#ifndef WIN32
                                                        if ((ss = send(sock, (char *)pchbuf + ssize,jvs_min(nsize - ssize, 1400) , MSG_NOSIGNAL)) <= 0)
                                                        {
                                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                if (SOCKET_ERROR == (ss = send(sock, pchbuf + ssize,jvs_min(nsize - ssize, 1400) , 0)))
                                                                {
                                                                    int kkk = WSAGetLastError();
                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
#endif
                                                                    {//本次发送失败 但连接没问题
                                                                        if(!bcomplete)
                                                                        {//不要求一次就全部发送成功
                                                                            break;
                                                                        }
                                                                        CCWorker::jvc_sleep(1);
                                                                        continue;//要求一次发送完整 继续
                                                                    }
                                                                    //异常关闭连接
                                                                    return -1;
                                                                }
                                                            ssize += ss;
                                                        }
                                                        return ssize;
                                                    }
                                                    
                                                    BOOL CCChannel::CheckInternalIP(const unsigned int ip_addr)
                                                    {
                                                        
                                                        //检查3类地址
                                                        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF ) ||
                                                            (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF ) ||
                                                            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF ) ||
                                                            (ip_addr == 0x7F000001))
                                                        {
                                                            return TRUE;//内网ip
                                                        }
                                                        
                                                        return FALSE;//其他
                                                        
                                                    }
                                                    
                                                    
                                                    BOOL CCChannel::ConnectStatus(SOCKET s,SOCKADDR_IN* addr,int nTimeOut,BOOL bFinish,UDTSOCKET uSocket)//连接成功 直接去连接
                                                    {
                                                        if(bFinish)//连接失败
                                                        {
                                                            if(CheckNewHelp() > 0)
                                                                return TRUE;
                                                            
                                                            //去转发
                                                            if(m_nStatus == WAIT_MAKE_HOLE)
                                                                m_nStatus = NEW_TURN;
                                                            
                                                            return TRUE;
                                                        }
                                                        //////////////////////////////////////////////////////////////////////////
                                                        if(m_ServerSocket > 0)
                                                        {
                                                            m_pWorker->pushtmpsock(m_ServerSocket);
                                                        }
                                                        m_ServerSocket = uSocket;
                                                        
                                                        /*	m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                                                         BOOL bReuse = TRUE;
                                                         UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                                                         //////////////////////////////////////////////////////////////////////////
                                                         int len1 = JVC_MSS;
                                                         UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                                                         //////////////////////////////////////////////////////////////////////////
                                                         if (UDT::ERROR == UDT::bind(m_ServerSocket, s))
                                                         {//绑定到指定端口失败，改为绑定到随机端口
                                                         if(m_ServerSocket > 0)
                                                         {
                                                         m_pWorker->pushtmpsock(m_ServerSocket);
                                                         }
                                                         m_ServerSocket = 0;
                                                         
                                                         if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                         {
                                                         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败.连接主控失败(可能是端口被占) 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                         }
                                                         else
                                                         {
                                                         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                         }
                                                         m_nStatus = NEW_TURN;
                                                         
                                                         return FALSE;
                                                         }
                                                         
                                                         //将套接字置为非阻塞模式
                                                         BOOL block = FALSE;
                                                         UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                                                         UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                                                         LINGER linger;
                                                         linger.l_onoff = 0;
                                                         linger.l_linger = 0;
                                                         UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
                                                         //////////////////////////////////////////////////////////////////////////
                                                         
                                                         //////////////////////////////////////////////////////////////////////////
                                                         //char ch[100]={0};
                                                         //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                                                         //////////////////////////////////////////////////////////////////////////
                                                         
                                                         STJUDTCONN stcon;
                                                         stcon.u = m_ServerSocket;
                                                         stcon.name = (SOCKADDR *)addr;
                                                         stcon.namelen = sizeof(SOCKADDR);
                                                         stcon.nChannelID = m_stConnInfo.nChannel;
                                                         stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
                                                         memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
                                                         stcon.nLVer_new = JVN_YSTVER;
                                                         stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
                                                         stcon.nMinTime = nTimeOut;
                                                         
                                                         SOCKADDR_IN srv = {0};
                                                         char strServer[100] = {0};
                                                         memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
                                                         
                                                         sprintf(strServer,"connecting a %s:%d  m_ServerSocket = %d line %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
                                                         //	OutputDebug(strServer);
                                                         if(UDT::ERROR == UDT::connect(stcon))//3000
                                                         {
                                                         m_nStatus = NEW_TURN;
                                                         return FALSE;
                                                         }
                                                         */
                                                        //打洞连接的不允许TCP连接
                                                        
                                                        if(SendReCheck(TRUE))
                                                        {//发送成功 等待验证结果
                                                            m_nStatus = WAIT_NRECHECK;
                                                            m_dwStartTime = CCWorker::JVGetTime();
                                                            
                                                            memcpy(&m_addrAN,addr,sizeof(SOCKADDR_IN));
                                                            memcpy(&m_addrAL,addr,sizeof(SOCKADDR_IN));
                                                            memcpy(&m_addressA,addr,sizeof(SOCKADDR_IN));
                                                            //		OutputDebug("connect ok. %d  %d\t%d",m_stConnInfo.nLocalChannel,__LINE__,m_ServerSocket);
                                                            return TRUE;
                                                        }
                                                        
                                                        m_nStatus = NEW_TURN;
                                                        
                                                        return FALSE;
                                                    }
                                                    
                                                    int CCChannel::CheckNewHelp()
                                                    {
                                                        if(JVN_ONLYTURN == m_stConnInfo.nTURNType && m_SList.size() > 0)
                                                        {
															AddCSelfServer();

                                                            m_nStatus = NEW_TURN;
                                                            return 1;
                                                        }
                                                        
                                                        if(JVN_ONLYTURN != m_stConnInfo.nTURNType)
                                                        {
                                                            char ip[20] = {0};
                                                            int port = 0;
                                                            int nRet = 0;
                                                            SOCKET s = 0;
                                                            nRet = m_pWorker->GetConnectInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,s,ip,port);
//                                                            if(nRet == 1)//直接TCP
//                                                            {
//                                                                sprintf(m_stConnInfo.chServerIP, "%s", ip);
//                                                                m_stConnInfo.nServerPort = port;
//                                                                
//                                                                m_nStatus = NEW_HAVE_LOCAL;
//                                                                if(m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
//                                                                    m_nStatus = NEW_HAVEIP;
//                                                                writeLog("CheckNewHelp %s, %d = %s: %d, line: %d",
//                                                                         m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_stConnInfo.chServerIP,m_stConnInfo.nServerPort,
//                                                                         __LINE__);
//                                                                return 1;
//                                                            }
                                                            if(nRet == 2)//直接udp
                                                            {
                                                                sprintf(m_stConnInfo.chServerIP, "%s", ip);
                                                                m_stConnInfo.nServerPort = port;
                                                                m_stConnInfo.sRandSocket = s;
                                                                
                                                                m_nStatus = NEW_VIRTUAL_IP;
                                                                writeLog("CheckNewHelp %s, %d = %s: %d, line: %d",
                                                                         m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_stConnInfo.chServerIP,m_stConnInfo.nServerPort,
                                                                         __LINE__);
                                                                return 1;
                                                            }
                                                            if(m_stConnInfo.nWhoAmI == JVN_WHO_M && m_pWorker->m_pHelpCtrl != NULL)
                                                            {//提速模块已在工作
                                                                //向小助手询问该号码的连接状态
                                                                STVLINK stvlink;
                                                                stvlink.nYSTNO = m_stConnInfo.nYSTNO;
                                                                memcpy(&stvlink.chGroup, &m_stConnInfo.chGroup, 4);
                                                                stvlink.nChannel = m_stConnInfo.nChannel;
                                                                memcpy(&stvlink.chUserName, &m_stConnInfo.chPassName, strlen(m_stConnInfo.chPassName));
                                                                memcpy(&stvlink.chPasswords, &m_stConnInfo.chPassWord, strlen(m_stConnInfo.chPassWord));
                                                                int nret = m_pWorker->m_pHelpCtrl->SearchYSTNO(&stvlink);
                                                                
                                                                if(JVN_HELPRET_LOCAL == nret)
                                                                {
                                                                    memcpy(&m_stConnInfo.quickAddr, &stvlink.addrVirtual, sizeof(SOCKADDR_IN));
                                                                    sprintf(m_stConnInfo.chServerIP, "%s", inet_ntoa(stvlink.addrVirtual.sin_addr));
                                                                    m_stConnInfo.nServerPort = ntohs(stvlink.addrVirtual.sin_port);
                                                                    m_stConnInfo.sRandSocket = stvlink.sRandSocket;
                                                                    
                                                                    			OutputDebug("Find help %d(%d) = %s : %d",m_stConnInfo.sRandSocket,m_pWorker->m_WorkerUDPSocket,m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                                                                    
                                                                    m_nStatus = NEW_VIRTUAL_IP;
                                                                    writeLog("CheckNewHelp %s, %d = %s: %d, line: %d",
                                                                             m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,m_stConnInfo.chServerIP,m_stConnInfo.nServerPort,
                                                                             __LINE__);
                                                                    return 1;
                                                                }
                                                                else
                                                                {
                                                                    if(m_SList.size() > 0)
                                                                    {
                                                                        m_nStatus = NEW_TURN;
                                                                        return 1;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        
                                                        
                                                        return 0;
                                                    }
                                                    
                                                    void CCChannel::UpdateYSTNOList()//更新连接地址
                                                    {
                                                        //转发连接 不更新
                                                        if(m_bIsTurn)
                                                            return;
                                                        
                                                        DWORD dwTime = CCWorker::JVGetTime();
                                                        
                                                        if(dwTime > m_dwLastUpdateTime + 15 * 1000)//不需要一直更新
                                                        {
                                                            m_pWorker->UpdateYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addrConnectOK, m_stConnInfo.sRandSocket);
                                                            m_dwLastUpdateTime = dwTime;
                                                        }
                                                    }
                                                    
                                                    int CCChannel::ReceiveIndexFromServer(STCONNPROCP *pstConn,BOOL bFirstCall)
                                                    {
                                                        memset(pstConn->chdata, 0, JVN_ASPACKDEFLEN);
                                                        int nRecvSize = receivefrom(m_sQueryIndexSocket, (char *)pstConn->chdata, JVN_ASPACKDEFLEN, 0, &pstConn->sockAddr, &pstConn->nSockAddrLen, 1);
                                                        OutputDebug("nRecvSize : %d，m_sQueryIndexSocket： %d",nRecvSize,m_sQueryIndexSocket);
                                                        if(nRecvSize > 0)
                                                        {
                                                            //验证数据包，格式：类型(1)+数据区长度(4)+n*服务器地址(sizeof(sockaddr_in))
                                                            int nServerNum = 0;
                                                            bool bPkgTrue = pstConn->nSockAddrLen >= 5 && pstConn->chdata[0] == JVN_REQ_QUERYYSTNUM;
                                                            if(bPkgTrue)
                                                            {
                                                                DWORD dwDataSize = ntohl(*(DWORD*)&(pstConn->chdata[1]));
                                                                if(dwDataSize % sizeof(sockaddr_in) != 0)
                                                                {
                                                                    bPkgTrue = false;
                                                                }
                                                                
                                                                nServerNum = dwDataSize / sizeof(sockaddr_in);
                                                            }
                                                            OutputDebug("nServerNum : %d, bPkgTrue: %d",nServerNum,bPkgTrue);
                                                            if(nServerNum > 0 && bPkgTrue)
                                                            {
                                                                OutputDebug("query index result...%d",nServerNum);
                                                                
                                                                sockaddr_in *pAddrs = (sockaddr_in*)&(pstConn->chdata[5]);
                                                                for(int i = 0; i < nServerNum; ++i)
                                                                {
                                                                    STSERVER server = {0};
                                                                    memcpy(&server.addr, pAddrs, sizeof(sockaddr_in));
                                                                    server.buseful = TRUE;
                                                                    server.nver = 0;
//                                                                    inet_ntoa(addrs.sin_addr),ntohs(addrs.sin_port)
                                                                    SOCKADDR_IN mmm;
                                                                    memcpy(&mmm, &pstConn->sockAddr, sizeof(SOCKADDR_IN));
                                                                    
                                                                    OutputDebug("ip: %s, port: %d",inet_ntoa(mmm.sin_addr),ntohs(mmm.sin_port));
                                                                    //testcode
//                                                                    server.addr.sin_addr.s_addr = inet_addr("123.129.242.85");
//                                                                    server.addr.sin_port = htons(9210);
                                                                    ///////
                                                                    if(bFirstCall)
                                                                    {
                                                                        pstConn->slisttmp.push_back(server);
                                                                        OutputDebug("Add server1  %s : %d \n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
                                                                    }
                                                                    else
                                                                    {
                                                                        
                                                                        BOOL bFind = FALSE;
                                                                        for(int k = 0;k < m_SList.size();k ++)
                                                                        {
                                                                            if(AddrIsTheSame((sockaddr *)&server.addr,(sockaddr *)&m_SList[k].addr))
                                                                            {
                                                                                bFind = TRUE;
                                                                                break;
                                                                            }
                                                                        }
                                                                        
                                                                        if(!bFind)
                                                                        {
                                                            
                                                                            
                                                                            m_SList.push_back(server);
                                                                            OutputDebug("Add new server  %s : %d                  %d\n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port),m_SList.size());
                                                                        }
                                                                        
                                                                        
//                                                                        m_SList.push_back(server);
//                                                                        OutputDebug("Add new server2  %s : %d \n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
                                                                    }

         OutputDebug(" index server  %s : %d \n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
pAddrs++;
}
if(bFirstCall)
{
//获取成功，进入下一步
m_SList.clear();
m_SList = pstConn->slisttmp;
AddCSelfServer();

m_SListTurn.clear();
m_SListTurn = m_SList;
m_nStatus = WAIT_CHECK_A_NAT;//查询主控NAT类型

if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
{
    m_pWorker->WriteYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addressA, 0);
}

if(JVN_ONLYTURN == m_stConnInfo.nTURNType || m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP || m_stConnInfo.nConnectType == TYPE_3GMOHOME_UDP)//只转发
{
    m_nStatus = NEW_TURN;//OutputDebug("This is line ==============  %d",__LINE__);
    return nServerNum;
}

}
return nServerNum;
}
}
	else
	{
		ReRequestIndexAddr();
	}
	return 0;
}

void CCChannel::ReRequestTurnAddr()
{
	DWORD currentTime  = CCWorker::JVGetTime();
	if(m_nRequestTurnTimes < 5 && (currentTime-m_dwSendTurnTime) >=300)
	{
		m_dwSendTurnTime = currentTime;
		m_nRequestTurnTimes ++;
		OutputDebug("重新请求地址…*****************..%d",m_nRequestTurnTimes);
		sendtoclient(m_sRequestTurn, (char *)m_strRequestTurnData, 10, 0, (SOCKADDR *)&(m_RequestTurnAddr), sizeof(SOCKADDR),2);
	}
	
}

void CCChannel::ReRequestIndexAddr()
{
	DWORD currentTime  = CCWorker::JVGetTime();
	if(m_nRequestIndexTimes < 5 && (currentTime-m_dwSendIndexTime) >=300)
	{
		int ncount = m_ISList.size();
		m_dwSendIndexTime = currentTime;
		m_nRequestIndexTimes ++;
		OutputDebug("重新请求地址…***********ReRequestIndexAddr*%d*****..%d",ncount,m_nRequestIndexTimes);

		for (int i = 0; i < ncount; i++) {
			try
			{
				sendtoclient(m_sRequestIndex, (char *) m_strRequestIndexData, 13, 0,
									(SOCKADDR *) &(m_ISList[i].addr), sizeof(SOCKADDR), 2);
			}catch(...)
			{
				writeLog("*****************************exception ReRequestIndexAddr******..%d,, i: %d, m_sRequestIndex: %d",
								ncount, i,m_sRequestIndex);
				OutputDebug("*****************************exception ReRequestIndexAddr******..%d,, i: %d, m_sRequestIndex: %d",
												ncount, i,m_sRequestIndex);
			}

		}
	}

}

void CCChannel::AddCSelfServer()
{
	for(int j = 0;j < m_pWorker->m_CSelfDefineServer.size();j ++)
	{
        char strG[4] = {0};
        strcpy(strG,m_pWorker->m_CSelfDefineServer[j].strGroup);
        if(strcmp(strG,m_stConnInfo.chGroup) != 0)
            continue;
        
		char ch2[50]={0};
		sprintf(ch2,"%s:%d",m_pWorker->m_CSelfDefineServer[j].strIP,m_pWorker->m_CSelfDefineServer[j].nPort);
		int nCount = m_SList.size();
		BOOL bFind = FALSE;
		for(int i=0; i<nCount; i++)
		{
			char ch1[50]={0};
			sprintf(ch1,"%s:%d",inet_ntoa(m_SList[i].addr.sin_addr),ntohs(m_SList[i].addr.sin_port));
			
			if(strcmp(ch1,ch2) == 0)
			{
				bFind = TRUE;
				break;
			}
		}
		if(!bFind)
		{
			STSERVER stserver;
#ifndef WIN32
			stserver.addr.sin_addr.s_addr = inet_addr(m_pWorker->m_CSelfDefineServer[j].strIP);
#else
			stserver.addr.sin_addr.S_un.S_addr = inet_addr(m_pWorker->m_CSelfDefineServer[j].strIP);
#endif
			stserver.addr.sin_family = AF_INET;
			stserver.addr.sin_port = htons(m_pWorker->m_CSelfDefineServer[j].nPort);
			
			stserver.buseful = TRUE;
			stserver.nver = 0;
			OutputDebug("Add Server============= %s %d",m_pWorker->m_CSelfDefineServer[j].strIP,m_pWorker->m_CSelfDefineServer[j].nPort);
			
			m_SList.insert(m_SList.begin(),stserver);
			//	m_SList.push_back(stserver);
		}
	}
}


int CCChannel::SelectAliveSvrList(ServerList OnlineSvrList)
{
	for(int i = 0; i < m_GroupSvrList.addrlist.size(); i++)
	{
		bool bFind = false;
		for(int j = 0; j < OnlineSvrList.size(); j++)
		{
#ifdef WIN32
				if(OnlineSvrList[j].addr.sin_addr.S_un.S_addr == m_GroupSvrList.addrlist[i].addr.sin_addr.S_un.S_addr)
#else 
				if(OnlineSvrList[j].addr.sin_addr.s_addr == m_GroupSvrList.addrlist[i].addr.sin_addr.s_addr)
#endif
				{
					bFind = true;
					break;
				}
		}
		if(!bFind)  //可能通畅的地址
		{
			m_GroupSvrList.addrlist[i].buseful = false;
			writeLog(".may alive addr [%s:%d]",inet_ntoa(m_GroupSvrList.addrlist[i].addr.sin_addr),ntohs(m_GroupSvrList.addrlist[i].addr.sin_port));

			m_AliveSvrList.addrlist.push_back(m_GroupSvrList.addrlist[i]);
		}
	}
	return m_AliveSvrList.addrlist.size();
}
void CCChannel::AddYstSvr(STSERVER svr)
{
	//for( int i = 0; i < m_GroupSvrList.addrlist.size(); i++)
	//{
	//#ifdef WIN32
	//				if(m_GroupSvrList.addrlist[i].addr.sin_addr.S_un.S_addr == svr.addr.sin_addr.S_un.S_addr)
	//#else 
	//				if(m_GroupSvrList.addrlist[j].addr.sin_addr.s_addr== svr.addr.sin_addr.s_addr)
	//#endif
	//				{

	//				}
	//}
}
int CCChannel::RcvTurnAddrList(SOCKET stmp)
{
	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = 0;
	memset(data, 0, JVN_ASPACKDEFLEN);
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	int nrecvlen=0;
	STSERVER turnAddr;
	if((nrecvlen = receivefromm(stmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
	{
		memcpy(&nType, &data[0], 4);
		writeLog(".......rcv turnaddrlist from svr [%s:%d],nType:%x",inet_ntoa(addrtemp.sin_addr),ntohs(addrtemp.sin_port),nType);
       
		//返回：类型4字节+号码4字节+权限4字节+端口2字节+ip个数1字节+【ip4字节+负载1字节】
		if(nType == JVN_INDIRRECT_CONN)
		{
			int nYstNO = 0;
			unsigned short port = 0;
			unsigned char num = 0;
			int authority = 0;

			memcpy(&nYstNO,&data[4],sizeof(nYstNO));
			if(nYstNO != m_stConnInfo.nYSTNO)
			{
				writeLog("ERROR, find wrong ystnum,get yst:%d,realyst:%d",nYstNO,m_stConnInfo.nYSTNO);
				return 0;
			}
			memcpy(&authority,&data[8],sizeof(authority));
			memcpy(&port,&data[12],sizeof(port));
			num = data[14];
			for(int i = 0; i < num; i++)
			{
				int ip = 0;
				memcpy(&ip,&data[15+5*i],sizeof(ip));
				turnAddr.addr.sin_port = htons(port);
#ifdef WIN32
				//turnAddr.addr.sin_addr.S_un.S_addr = htonl(ip);
				turnAddr.addr.sin_addr.S_un.S_addr = ip;
#else 
				//turnAddr.addr.sin_addr.s_addr = htonl(ip);
				turnAddr.addr.sin_addr.s_addr = ip;
#endif
				m_TurnSvrAddrList.push_back(turnAddr);
				writeLog(".....get turnaddr:[%s:%d],num:%d",inet_ntoa(turnAddr.addr.sin_addr),ntohs(turnAddr.addr.sin_port),num);
			}
			return num;
			
	/*		addrtemp.sin_port
			m_stConnInfo.nYSTNO*/
		}
	}
	return 0;
}
// 请求畅通的服务器向yst上线但分控不通的服务器转发请求连接
BOOL CCChannel::SendReqTurnAddr(SOCKADDR_IN addr,SOCKET stmp)
{
	//类型4字节+号码4字节+请求类型1字节+服务器地址4字节
	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = JVN_INDIRRECT_CONN;
	memcpy(&data[0], &nType, 4);
	memcpy(&data[4], &m_stConnInfo.nYSTNO, 4);
	data[8] = m_stConnInfo.nTURNType;
	BOOL bRet = FALSE;
	int objsvr  = 0;
	for (int j = 0; j < m_SList.size(); j++)
	{
			
#ifdef WIN32
			//objsvr = ntohl(m_SList[j].addr.sin_addr.S_un.S_addr);
			objsvr = m_SList[j].addr.sin_addr.S_un.S_addr;//c51d10ac
#else 
			//objsvr = ntohl(m_SList[j].addr.sin_addr.s_addr);
			objsvr = m_SList[j].addr.sin_addr.s_addr; // c51d10ac
#endif
			memcpy(&data[9],&objsvr,4);
			writeLog(".........obj svr [%s:%d],obj:%x",inet_ntoa(m_SList[j].addr.sin_addr),ntohs(m_SList[j].addr.sin_port),objsvr);
			
			if(sendtoclient(stmp, (char *)data, 13, 0, (SOCKADDR *)&(addr), sizeof(SOCKADDR),2) > 0)
			{
				writeLog("...........send succsee !");
				bRet = TRUE;
			}else
			{
				writeLog(".............sendtoclient error !");
				#ifdef WIN32
					if(WSAGetLastError() == 10038)
					{
						if(stmp > 0)
						{
							closesocket(stmp);
						}
						return FALSE;
					}
				#endif
					if(stmp > 0)
					{
						closesocket(stmp);
					}
					return FALSE;
			}
	}
	return bRet;
}
void CCChannel::ReqTurnAddrViaSvr(void)
{
	//创建临时UDP套接字
	SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
#ifndef WIN32
		int flags=0;
		flags = fcntl(stmp, F_GETFL, 0);
		fcntl(stmp, F_SETFL, flags | O_NONBLOCK);
#endif

	SOCKADDR_IN addrSrv;
#ifndef WIN32
	//addrSrv.sin_addr.s_addr = m_pWorker->m_addrLocal.sin_addr.s_addr;
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	//addrSrv.sin_addr.S_un.S_addr = m_pWorker->m_addrLocal.sin_addr.S_un.S_addr;
	addrSrv.sin_addr.S_un.S_addr = m_pWorker->GetCurEthAddr();
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(0);//htons(m_nLocalStartPort);

	BOOL bReuse = TRUE;
	setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    int len1 =0;
#ifdef MOBILE_CLIENT
	len1=1500*1024;
	UDT::setsockopt(stmp, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
		
	len1=1000*1024;
	UDT::setsockopt(stmp, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
	//绑定套接字
	bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));

//	SOCKADDR_IN addrs;

	int nsize = m_AliveSvrList.addrlist.size();
	for(int i = 0; i < nsize; i++)
	{
		if(m_bExit)
		{
			writeLog("..............****************..........FIND EXITFLAG !");
			m_nStatus = FAILD;
			return;
		}
		srand((int)time(0));
		int index = 0;
		int cursize = m_AliveSvrList.addrlist.size();
		if(cursize == 0)
		{
			break;
		}
		index = rand() % cursize;
		
		if(SendReqTurnAddr(m_AliveSvrList.addrlist[index].addr,stmp))
		{
			DWORD start = CCWorker::JVGetTime();
			while(1)
			{
				if(m_bExit)
				{
					writeLog("..............****************..........FIND EXITFLAG !");
					m_nStatus = FAILD;
					return;
				}
				if(CCWorker::JVGetTime() - start > 3000)
				{
					writeLog("...................RcvTurnAddrList timeout !");
					break;
				}
				int nsize = RcvTurnAddrList(stmp);
				//
				if(nsize > 0)
				{
					m_nStatus = RECV_TURNLIST;
					if(stmp > 0)
					{
						closesocket(stmp);
					}
					return;
				}
				CCWorker::jvc_sleep(100);
			}
			writeLog(".................. call RcvTurnAddrList return:%d",nsize);
		}else
		{
			writeLog("error to send reqturnadddr to [%s:%d]",inet_ntoa(m_AliveSvrList.addrlist[index].addr.sin_addr),ntohs(m_AliveSvrList.addrlist[index].addr.sin_port));
		}
		m_AliveSvrList.addrlist.erase( m_AliveSvrList.addrlist.begin()+index);
		CCWorker::jvc_sleep(100);
	}
	/*
	for(int i = 0; i < m_AliveSvrList.addrlist.size(); i++) // 分别向通畅的服务器列表发转发请求
	{
		outdbgs(0,"ready to send reqturnadddr to [%s:%d],size:%d,i:%d",inet_ntoa(m_AliveSvrList.addrlist[i].addr.sin_addr),ntohs(m_AliveSvrList.addrlist[i].addr.sin_port),m_AliveSvrList.addrlist.size(),i);
		if(SendReqTurnAddr(m_AliveSvrList.addrlist[i].addr,stmp))
		{
			int nsize = RcvTurnAddrList(stmp);
			if(nsize > 0)
			{
				m_nStatus = RECV_TURNLIST;
				return;
			}
		}
		CCWorker::jvc_sleep(1000);
	}
	*/



	if(stmp > 0)
	{
		closesocket(stmp);
	}
	m_nStatus = FAILD;
	if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
	{
			char chMsg[] = "请求转发地址失败 !";
//			strcpy(m_strConnectError,chMsg);
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败.请求转发地址失败", __FILE__,__LINE__);
	}
	else
	{
			char chMsg[] = "request turnadddr fail !";
//			strcpy(m_strConnectError,chMsg);
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. req turn addr fail", __FILE__,__LINE__);
	}
	return;
}

BOOL CCChannel::DealRecvTURNLIST(STCONNPROCP *pstConn)
{
	BOOL bfind=FALSE;
	int ncount = m_TurnSvrAddrList.size();
	
	writeLog("........................................turnsvraddrlist:%d",ncount);
	for(int i=0; i<ncount; i++)
	{
			bfind = TRUE;
			memcpy(&m_addrTS,&m_TurnSvrAddrList[i].addr,sizeof(SOCKADDR_IN));
			m_addrTS.sin_family = AF_INET;
			//m_addrTS.
			if(ConnectTURN(i, pstConn->chError))
			{//向对端发送有效性验证消息
				writeLog("..connet turn addr [%s:%d] sucess",inet_ntoa(m_addrTS.sin_addr),ntohs(m_addrTS.sin_port));
				if(SendReCheck(TRUE))
				{//发送成功 等待验证结果
					writeLog("............SendReCheck success !");
					m_nStatus = WAIT_TSRECHECK;
					m_dwStartTime = CCWorker::JVGetTime();
					return true;
				}else
				{
					writeLog("............SendReCheck error !");
				}
			}else
			{
				writeLog("..connet turn addr [%s:%d] error",inet_ntoa(m_TurnSvrAddrList[i].addr.sin_addr),ntohs(m_TurnSvrAddrList[i].addr.sin_port));
				//CCWorker::jvc_sleep(1000);
			}
			CCWorker::jvc_sleep(20);
	}
	m_nStatus = FAILD;
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			char chMsg[] = "参数错误失败!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 连接转发服务器地址失败", __FILE__,__LINE__);
		}
		else
		{
			char chMsg[] = "dataerr failed!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. connect turn svr fail", __FILE__,__LINE__);
		}
	return FALSE;
}

int CCChannel::WaitTurnAddrList(STCONNPROCP *pstConn)
{
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	int nrecvlen=0;
	ServerList Slist;
	SOCKADDR_IN addr;
	if((nrecvlen = receivefromm(pstConn->udpsocktmp, (char *)pstConn->chdata, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
	{
		//返回：类型4字节+号码4字节+权限4字节+端口2字节+ip个数1字节+【ip4字节+负载1字节】
		if(nrecvlen > 0)
		{
			int nType = 0;
			int nYst = 0;
			int autho = 0;
			unsigned short port = 0;
			char num = 1;
			int ip = 0;

			memcpy(&nType,&pstConn->chdata[0],4);
			memcpy(&nYst,&pstConn->chdata[4],4);
			memcpy(&autho,&pstConn->chdata[8],4);
			memcpy(&port,&pstConn->chdata[12],2);
			num = pstConn->chdata[14];
			if(nType == JVN_CONN_DEV && nYst == m_stConnInfo.nYSTNO)
			{
				for(int i = 0; i < num; i++)
				{
					memcpy(&ip,&pstConn->chdata[15+5*i],4);
#ifdef WIN32
					addr.sin_addr.S_un.S_addr = htonl(ip);
#else 
					addr.sin_addr.s_addr = htonl(ip);
#endif

				}
			}
		}
	}
	return 0;
}
//接受设备公网地址
int CCChannel::WaitDevPubAddr(SOCKET stmp,ServerList &list) 
{
	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = 0;
	memset(data, 0, JVN_ASPACKDEFLEN);
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	int nrecvlen=0;
	STSERVER turnAddr;
	STSERVER addr;
	if((nrecvlen = receivefromm(stmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
	{
		//返回：类型4字节+号码4字节+IP地址4字节+移动标志1字节
		if(nrecvlen == 13)
		{
			int nYst = 0;
			int ip = 0;
			char flag = 0;
			memcpy(&nType,&data[0],4);
			memcpy(&nYst,&data[4],4);
			memcpy(&ip,&data[8],4);
			flag = data[12];
			if(nType == JVN_CHECK_DEVADDR)
			{
				if(nYst != m_stConnInfo.nYSTNO)
				{
					writeLog("......................recv wrong,rcvystnum:%d,realnum:%d",nYst,m_stConnInfo.nYSTNO);
					return -1;
				}
				for(int i = 0; i < list.size(); i++)
				{
#ifdef WIN32
					if(list[i].addr.sin_addr.S_un.S_addr == addrtemp.sin_addr.S_un.S_addr)
#else 
					if(list[i].addr.sin_addr.s_addr == addrtemp.sin_addr.s_addr)
#endif
					{
						writeLog("..............find same addr[%s:%d",inet_ntoa(addrtemp.sin_addr),ntohs(addrtemp.sin_port));
						return -1;
					}
				}
				writeLog("........svr [%s:%d] return dev pubaddr",inet_ntoa(addrtemp.sin_addr),ntohs(addrtemp.sin_port));

				SOCKADDR_IN tt;
				tt.sin_addr.s_addr = ip;
				writeLog("...........dev pubaddr1:%s,ip:%x,yidongflag:%d",inet_ntoa(tt.sin_addr),ip,flag); // good

				memcpy(&addr.addr,&addrtemp,sizeof(SOCKADDR_IN));
				list.push_back(addr);
			}
		}
	}
	return 0;
}

// 一次接受全部转发地址
int CCChannel::DealRcvCompleteTurn(STCONNPROCP *pstConn)
{
	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = 0;
	memset(data, 0, JVN_ASPACKDEFLEN);
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	int nrecvlen=0;
	STSERVER turnAddr;
	int stmp = pstConn->udpsocktmp;
	if((nrecvlen = receivefromm(stmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
	{
		memcpy(&nType, &data[0], 4);
		writeLog(".......rcv turnaddrlist from svr [%s:%d],nType:%x,len:%d",inet_ntoa(addrtemp.sin_addr),ntohs(addrtemp.sin_port),nType,nrecvlen);
		//返回：类型4字节+号码4字节+权限4字节+端口2字节+ip个数1字节+【ip4字节+负载1字节】
		if(nType == JVN_CONN_DEV)
		{
			int nYstNO = 0;
			unsigned short port = 0;
			unsigned char num = 0;
			int authority = 0;
			memcpy(&nYstNO,&data[4],sizeof(nYstNO));
			if(nYstNO != m_stConnInfo.nYSTNO)
			{
				writeLog("ERROR, find wrong ystnum,get yst:%d,realyst:%d",nYstNO,m_stConnInfo.nYSTNO);
				return 0;
			}
			memcpy(&authority,&data[8],sizeof(authority));
			memcpy(&port,&data[12],sizeof(port));
			num = data[14];
			writeLog("..........................turnaddrlistsize:%d",num);
			for(int i = 0; i < num; i++)
			{
				int ip = 0;
				memcpy(&ip,&data[15+5*i],sizeof(ip));
				turnAddr.addr.sin_port = htons(port);

//#ifdef WIN32
//				turnAddr.addr.sin_addr.S_un.S_addr = htonl(ip);
//#else 
//				turnAddr.addr.sin_addr.s_addr = htonl(ip);
//#endif
#ifdef WIN32
				turnAddr.addr.sin_addr.S_un.S_addr = ip;
#else 
				turnAddr.addr.sin_addr.s_addr = ip;
#endif
				m_TurnSvrAddrList.push_back(turnAddr);
				writeLog(".....get turnaddr:[%s:%d],i:%d,ip:%x",inet_ntoa(turnAddr.addr.sin_addr),ntohs(turnAddr.addr.sin_port),i,ip);
			}
			return num;
		}
	}
	return 0;
}
void CCChannel::DealReqCompleteTurn(STCONNPROCP *pstConn) // 向已经上线并可联通的服务器发送请求全部转发地址指令
{
	//创建临时UDP套接字
	SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
#ifndef WIN32
	int flags=0;
	flags = fcntl(stmp, F_GETFL, 0);
	fcntl(stmp, F_SETFL, flags | O_NONBLOCK);
#endif

	SOCKADDR_IN addrSrv;
#ifndef WIN32
	//addrSrv.sin_addr.s_addr = m_pWorker->m_addrLocal.sin_addr.s_addr;
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	//addrSrv.sin_addr.S_un.S_addr = m_pWorker->m_addrLocal.sin_addr.S_un.S_addr;
	addrSrv.sin_addr.S_un.S_addr = m_pWorker->GetCurEthAddr();
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(0);//htons(m_nLocalStartPort);

	BOOL bReuse = TRUE;
	setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    int len1 =0;
#ifdef MOBILE_CLIENT
	len1=1500*1024;
	UDT::setsockopt(stmp, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
	len1=1000*1024;
	UDT::setsockopt(stmp, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
	//绑定套接字
	bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));

	pstConn->udpsocktmp = stmp;

	memset(pstConn->chdata, 0, 50);
	//发送：请求：类型4字节+号码4字节+请求类型1字节+VIP标志1字节（默认0即可）
	int nt = JVN_CONN_DEV;
	memcpy(&pstConn->chdata[0], &nt, 4);
	memcpy(&pstConn->chdata[4], &m_stConnInfo.nYSTNO, 4);
	pstConn->chdata[8] = m_stConnInfo.nTURNType;
	pstConn->chdata[9] = 0;
	int ncount = pstConn->slisttmp.size();

	for(int i=0; i<ncount; i++)
	{
		sendtoclient(stmp, (char*)pstConn->chdata,10, 0, (sockaddr *)&pstConn->slisttmp[i].addr, sizeof(sockaddr_in), 1);
		writeLog(".........req turnaddrlist from svr[%s:%d]",inet_ntoa(pstConn->slisttmp[i].addr.sin_addr),ntohs(pstConn->slisttmp[i].addr.sin_port));
		CCWorker::jvc_sleep(20);
	}

	pstConn->slisttmp.clear();
	DWORD start = CCWorker::JVGetTime();
	while(1)
	{
		if(CCWorker::JVGetTime() - start > 2000)
		{
			writeLog("..........WaitCompleturn for timeout !");
			break;
		}
		if(m_TurnSvrAddrList.size() == ncount)
		{
			writeLog("..........WaitCompleturn for reach max !");
			break;
		}
		DealRcvCompleteTurn(pstConn); // 接受转发服务器地址列表，地址存放在m_TurnSvrAddrList里面
		CCWorker::jvc_sleep(200);
	}

	writeLog("............after DealRcvCompleteTurn,size;%d",m_TurnSvrAddrList.size());
	if(m_TurnSvrAddrList.size() > 0)
	{
		if(!DealRecvTURNLIST(pstConn)) //链接转发服务器
		{
				m_nStatus = FAILD;
				if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
				{
					char chMsg[] = "参数错误失败!";
					m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
					m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 未获取到转发服务器地址", __FILE__,__LINE__);
				}
				else
				{
					char chMsg[] = "dataerr failed!";
					m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
					m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. not get turnsvr addr", __FILE__,__LINE__);
				}
		}
	}else
	{
		m_nStatus = FAILD;
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			char chMsg[] = "参数错误失败!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 未获取到转发服务器地址", __FILE__,__LINE__);
		}
		else
		{
			char chMsg[] = "dataerr failed!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. not get turnsvr addr", __FILE__,__LINE__);
		}
	}

	closesocket(stmp);
	pstConn->udpsocktmp = 0;
}
//向服务器发送查询设备公网地址
void CCChannel::DealWaitReqDevPubAddr(STCONNPROCP *pstConn)
{
		//创建临时UDP套接字
	SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
#ifndef WIN32
	int flags=0;
	flags = fcntl(stmp, F_GETFL, 0);
	fcntl(stmp, F_SETFL, flags | O_NONBLOCK);
#endif

	SOCKADDR_IN addrSrv;
#ifndef WIN32
	//addrSrv.sin_addr.s_addr = m_pWorker->m_addrLocal.sin_addr.s_addr;
	addrSrv.sin_addr.s_addr =  htonl(INADDR_ANY);
#else
	//addrSrv.sin_addr.S_un.S_addr = m_pWorker->m_addrLocal.sin_addr.S_un.S_addr;
	addrSrv.sin_addr.S_un.S_addr = m_pWorker->GetCurEthAddr();
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(0);//htons(m_nLocalStartPort);

	BOOL bReuse = TRUE;
	setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
    int len1 =0;
#ifdef MOBILE_CLIENT
	len1=1500*1024;
	UDT::setsockopt(stmp, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
		
	len1=1000*1024;
	UDT::setsockopt(stmp, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif

	//绑定套接字
	bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));

	int ncount = m_GroupSvrList.addrlist.size();
				
	memset(pstConn->chdata, 0, 50);
	//发送：请求：类型4字节+号码4字节
	int nt = JVN_CHECK_DEVADDR;
	memcpy(&pstConn->chdata[0], &nt, 4);
	memcpy(&pstConn->chdata[4], &m_stConnInfo.nYSTNO, 4);
	writeLog("..............grouplist size:%d",ncount);
	for(int i=0; i<ncount; i++)
	{
		sendtoclient(stmp, (char*)pstConn->chdata,8, 0, (sockaddr *)&m_GroupSvrList.addrlist[i].addr, sizeof(sockaddr_in), 1);
		writeLog("send request dev pub addr pakcet to svr [%s:%d]",inet_ntoa(m_GroupSvrList.addrlist[i].addr.sin_addr),ntohs(m_GroupSvrList.addrlist[i].addr.sin_port));
		CCWorker::jvc_sleep(20);
	}

	ServerList Slist;//online svr list
	pstConn->slisttmp.clear();
	DWORD start = CCWorker::JVGetTime();
	while(1)
	{
		if(CCWorker::JVGetTime() - start > 2000)
		{
			break;
		}
		if(pstConn->slisttmp.size() == ncount)
		{
			break;
		}
		WaitDevPubAddr(stmp,pstConn->slisttmp); //等待接受有返回的服务器地址，服务器地址存放在slistttmp
		CCWorker::jvc_sleep(100);
	}
	
	writeLog(".........%d svr return dev pub addr !",pstConn->slisttmp.size());
	if(pstConn->slisttmp.size() > 0) // 有服务器返回了,说明设备上线了
	{
		DealReqCompleteTurn(pstConn); //向上线的服务器发送请求全部转发地址并接受
	}else
	{
		m_nStatus = FAILD;
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			char chMsg[] = "参数错误失败!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "连接主控失败. 设备未上线", __FILE__,__LINE__);
		}
		else
		{
			char chMsg[] = "dataerr failed!";
			m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
			m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. dev not online", __FILE__,__LINE__);
		}
		
	}
	if(stmp > 0)
	{
		closesocket(stmp);
	}
	pstConn->slisttmp.clear();
}
