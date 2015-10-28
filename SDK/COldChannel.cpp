// COldChannel.cpp: implementation of the CCOldChannel class.
//
//////////////////////////////////////////////////////////////////////

#include "COldChannel.h"
#include "JVN_DBG.h"
#include "CWorker.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCOldChannel::CCOldChannel()
{
}

CCOldChannel::CCOldChannel(CCWorker *pWorker, CCChannel *pChannel, BOOL bIMMO)
{
    m_nProtocolType = pChannel->m_nProtocolType;//pChannel->m_stConnInfo.nConnectType;
    m_bExit = FALSE;
    pChannel->m_stConnInfo.nShow = 0;
    m_bClose = FALSE;
    
    pChannel->m_nFYSTVER = 0;
    m_bShouldSendMOType = FALSE;
    if((m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP) && bIMMO)
    {//手机连接新主控时需要告知对方这是手机连接
        m_bShouldSendMOType = TRUE;
    }
    
    m_nOCount = 0;
    
    m_bAcceptChat = FALSE;
    m_bAcceptText = FALSE;
    m_bDAndP = FALSE;
    
    m_nLocalChannel = pChannel->m_nLocalChannel;
    m_pWorker = pWorker;
    m_pChannel = pChannel;
    m_bPass = FALSE;
    
    m_hConnThread = 0;
    m_hRecvThread = 0;
    m_hPlayThread = 0;
    
    m_recvThreadExit =TRUE;//??????????
    m_connectThreadExit = FALSE;//????????
    m_playProExit = TRUE;
    
    //////////////////////////////////////////////////////////////////////////
#ifndef WIN32
    m_bEndP = FALSE;
    m_bEndR = FALSE;
    m_bEndC = FALSE;
#else
    m_hStartEventR = 0;
    m_hEndEventR = 0;
    
    m_hStartEventC = 0;
    m_hEndEventC = 0;
    
    m_hStartEventP = 0;
    m_hEndEventP = 0;
#endif
    //////////////////////////////////////////////////////////////////////////
    
    m_nChannel = pChannel->m_nChannel;//0;
    
    m_pBuffer = NULL;
    m_precBuf = pChannel->m_puchRecvBuf;
    m_preadBuf = pChannel->m_preadBuf;
    
    m_nPACKLEN = 20000;
    
    m_bCanDelS=TRUE;
    m_bByStreamServer=FALSE;
    
    if(pChannel->m_nStatus == WAIT_IP_CONNECTOLD)
    {//ip连接预验证超时，只尝试进行ip连接，失败后直接结束所有连接过程(连接能建立，验证不通过，没必要再进行其他尝试)
        m_nStatus = WAIT_IPRECHECK;
    }
    else if(pChannel->m_nStatus == WAIT_LW_CONNECTOLD)
    {//内网探测预验证超时，只尝试进行内网探测连接，失败后返回继续外网尝试
        m_nStatus = WAIT_LRECHECK;
    }
    else if(pChannel->m_nStatus == WAIT_NW_CONNECTOLD)
    {//外网预验证超时，只尝试进行外网连接，失败后直接结束所有连接过程(连接能建立，验证不通过，没必要再进行其他尝试)
        m_nStatus = WAIT_NRECHECK;
    }
    else if(pChannel->m_nStatus == WAIT_TSW_CONNECTOLD)
    {//转发预验证超时，只尝试进行转发连接，失败后直接结束所有连接过程(连接能建立，验证不通过，没必要再进行其他尝试)
        m_nStatus = WAIT_TSRECHECK;
    }
    else
    {//其他情况都是异常，直接结束连接过程
        pChannel->m_nStatus = FAILD;
        m_nStatus = FAILD;
        return;
    }
    
    m_pStream = NULL;
    
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
    
    if (0 != pthread_create(&m_hConnThread, pAttr, ConnProc, this))
    {
        m_hConnThread = 0;
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
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "再次连接主控失败. 创建连接线程失败", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "reconnect failed. create connect thread failed.", __FILE__,__LINE__);
                }
            }
            
            m_nStatus = FAILD;
            
            if(m_pBuffer != NULL)
            {
                delete m_pBuffer;
                m_pBuffer = NULL;
            }
            
            m_connectThreadExit = TRUE;
        }
        
    }
    
    CCOldChannel::~CCOldChannel()
    {
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
        if (0 != m_hPlayThread)
        {
            m_bEndP = TRUE;
            pthread_join(m_hPlayThread, NULL);
            m_hPlayThread = 0;
        }
        CCWorker::jvc_sleep(10);
#else
        if(m_hEndEventC>0)
        {
            SetEvent(m_hEndEventC);
        }
        
        if(m_hEndEventR>0)
        {
            SetEvent(m_hEndEventR);
        }
        //	if(m_hEndEventP>0)
        //	{
        //		SetEvent(m_hEndEventP);
        //	}
        
        CCWorker::jvc_sleep(10);
        CCChannel::WaitThreadExit(m_hConnThread);
        CCChannel::WaitThreadExit(m_hRecvThread);
        //	CCChannel::WaitThreadExit(m_hPlayThread);
        
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
        //	if(m_hEndEventP > 0)
        //	{
        //		CloseHandle(m_hEndEventP);
        //		m_hEndEventP = 0;
        //	}
        
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
        //	if(m_hPlayThread > 0)
        //	{
        //		CloseHandle(m_hPlayThread);
        //		m_hPlayThread = 0;
        //	}
#endif
        
        if(m_pStream!=NULL)
        {
            delete m_pStream;
            m_pStream=NULL;
        }
        
        if(m_pBuffer != NULL)
        {
            delete m_pBuffer;
            m_pBuffer = NULL;
        }
        m_bCanDelS=TRUE;
    }
    
#ifndef WIN32
    void* CCOldChannel::ConnProc(void* pParam)
#else
    UINT WINAPI CCOldChannel::ConnProc(LPVOID pParam)
#endif
    {
        //OutputDebugString("ccoldchannel_cannproc...........0\n");
        CCOldChannel *pWorker = (CCOldChannel *)pParam;
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
        
        DWORD dwendtime = 0;
        
        int nret = 0;
        //	int nver = 0;//0版本兼容 >0本地版本太低 <0对方版本太低
        //	char chAVersion[MAX_PATH]={0};
        char chError[20480]={0};
        
        //	BOOL bInfo = FALSE;//用于避免云视通连接时重复提示
        //OutputDebugString("ccoldchannel_cannproc...........1\n");
        int lastStatus = 0;
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
            if (lastStatus ==0) {
                writeLog("ccoldchannel: pWorker->m_nStatus %d ,line: %d",pWorker->m_nStatus,__LINE__);
            }else if(lastStatus != pWorker->m_nStatus){
                writeLog("ccoldchannel: pWorker->m_nStatus %d ,line: %d",pWorker->m_nStatus,__LINE__);
            }
            lastStatus = pWorker->m_nStatus;
            switch(pWorker->m_nStatus)
            {
                case WAIT_IPRECHECK://等待IP直连预验证消息超时
                {
                    //OutputDebugString("ccoldchannel_cannproc...........2\n");
                    pWorker->m_pChannel->m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
                    
                    if(pWorker->SendPWCheck())
                    {//发送身份验证消息成功 等待结果
                        pWorker->m_nStatus = WAIT_IPPWCHECK;
                        pWorker->m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送身份验证消息失败 直接结束连接
                        pWorker->m_pChannel->m_bShowInfo = TRUE;
                        
                        pWorker->m_nStatus = FAILD;
                        
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_bPass = FALSE;
                        
                        pWorker->m_pChannel->m_nStatus = WAIT_IP_CONNECTOLD_F;//FAILD;
                        
                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                        {
                            char chMsg[] = "身份验证失败!";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "旧版尝试，IP连接主控失败. 发送身份验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                        }
                        else
                        {
                            char chMsg[] = "PassWord is wrong!";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "OldTry IP connect failed. send pass info failed. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                        }
                        
                    }
                }
                    break;
                case WAIT_IPPWCHECK://等待身份验证
                {
                    dwendtime = CCWorker::JVGetTime();
                    if(dwendtime > pWorker->m_dwStartTime + 5000)//2000)
                    {//等待超时，连接失败
                        OutputDebug("%d old pwd time out.",pWorker->m_pChannel->m_stConnInfo.nLocalChannel);
                        writeLog("old pwd time out...%d",__LINE__);
                        pWorker->m_pChannel->m_bShowInfo = TRUE;
                        
                        pWorker->m_nStatus = FAILD;
                        pWorker->m_pChannel->m_nStatus = WAIT_IP_CONNECTOLD_F;//FAILD;
                        
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_bPass = FALSE;
                        
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "身份验证超时!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "check password timeout!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                        }
                        
                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                        {
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "旧版尝试，IP连接主控失败. 等待身份验证数据失败", __FILE__,__LINE__);
                        }
                        else
                        {
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "OldTry IP connect failed. wait pass info failed.", __FILE__,__LINE__);
                        }
                    }
                    else
                    {//接收数据
                        int nPWData=0;
                        nret = pWorker->RecvPWCheck(nPWData);
                        if(nret == 1)
                        {//验证通过 连接成功建立
                            OutputDebug("%d old pwd ok.",pWorker->m_pChannel->m_stConnInfo.nLocalChannel);
                            pWorker->m_nStatus = OK;
                            //	memcpy(&pWorker->m_addressA, &pWorker->m_addrAL, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addressA, &pWorker->m_pChannel->m_addrAL, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addrConnectOK, &pWorker->m_pChannel->m_addrAL, sizeof(SOCKADDR_IN));
                        }
                        else if(nret == -10)//无通道服务
                        {
                            pWorker->SetNoChannel();
                        }
                        else if(nret == 0)
                        {//身份验证未通过 直接结束连接
                            OutputDebug("old pwd err. LINE: %d",__LINE__);
                            pWorker->m_pChannel->m_bShowInfo = TRUE;
                            
                            pWorker->m_nStatus = FAILD;
                            pWorker->m_pChannel->m_nStatus = WAIT_IP_CONNECTOLD_F;//FAILD;
                            pWorker->m_pChannel->m_bPassWordErr = TRUE;
                            
                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                            
                            pWorker->m_bPass = FALSE;
                            
                            if(pWorker->m_pWorker != NULL)
                            {
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    char chMsg[] = "身份未通过验证!";
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                                }
                                else
                                {
                                    char chMsg[] = "password is wrong!";
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg, nPWData,__FILE__,__LINE__,__FUNCTION__);
                                }
                            }
                            
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "旧版尝试，IP连接主控失败. 身份验证失败", __FILE__,__LINE__);
                            }
                            else
                            {
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "OldTry IP connect failed. pass failed.", __FILE__,__LINE__);
                            }
                        }
                        else if(nret == -2)
                        {//以流媒体方式建立连接
                            if(pWorker->m_pBuffer != NULL)
                            {
                                delete pWorker->m_pBuffer;
                                pWorker->m_pBuffer = NULL;
                            }
                            
                            pWorker->m_bPass = TRUE;
                            pWorker->m_pChannel->m_nStatus = OK_OLD;
                            pWorker->m_bByStreamServer=TRUE;
                            pWorker->m_bCanDelS=FALSE;  //此时不能清除
                            
                            pWorker->m_pChannel->m_bTURN = FALSE;
                            char chMsg[] = "(P2P Stream)";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket=0;
                            pWorker->m_connectThreadExit = TRUE;
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
                    }
                }
                    break;
                    
                case OK://连接成功
                {//创建其他工作线程
                    if(pWorker->m_pBuffer != NULL)
                    {
                        delete pWorker->m_pBuffer;
                        pWorker->m_pBuffer = NULL;
                    }
                    
                    pWorker->m_pChannel->m_nFYSTVER = UDT::getystverF(pWorker->m_pChannel->m_ServerSocket);//获取远端协议版本
                    
                    //windows中数据不再缓存，直接向应用层扔
#ifndef WIN32
                    if(pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER1 || pWorker->m_nProtocolType == TYPE_PC_TCP)
                    {//新协议，支持更好的播放控制
                        pWorker->m_pBuffer = new CCSingleBufferCtrl(pWorker->m_nLocalChannel,(pWorker->m_pChannel->m_stConnInfo.nShow == JVN_CONNTYPE_TURN)?TRUE:FALSE);
                    }
                    else
                    {
                        pWorker->m_pBuffer = new CCOldBufferCtrl(pWorker->m_nLocalChannel,(pWorker->m_pChannel->m_stConnInfo.nShow == JVN_CONNTYPE_TURN)?TRUE:FALSE);
                    }
#endif
                    
                    if(!pWorker->StartWorkThread())
                    {
                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                        {
                            char chMsg[] = "开启工作线程失败!";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                        else
                        {
                            char chMsg[] = "Start work thread failed!";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        }
                        
                        pWorker->m_nStatus = FAILD;
                        pWorker->m_pChannel->m_nStatus = FAILD;
                    }
                    else
                    {
                        if(pWorker->m_pChannel->m_stConnInfo.nShow == JVN_CONNTYPE_TURN)
                        {
                            pWorker->m_pChannel->m_bTURN = TRUE;
                            char chMsg[] = "(TURN)";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            if(pWorker->m_pChannel->m_stConnInfo.bYST && pWorker->m_pChannel->m_stConnInfo.nYSTNO > 0)
                            {
                                OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",pWorker->m_nLocalChannel,pWorker->m_pChannel->m_stConnInfo.nYSTNO,__LINE__);
                                pWorker->m_pWorker->YSTNOCushion(pWorker->m_pChannel->m_stConnInfo.chGroup,pWorker->m_pChannel->m_stConnInfo.nYSTNO, -1);
                            }
                            //OutputDebugString("channel connectok(turn)\n");
                        }
                        else
                        {
                            pWorker->m_pChannel->m_bTURN = FALSE;
                            char chMsg[] = "(P2P)";
                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            //OutputDebugString("channel connectok(p2p)\n");
                            
                            //////////////////////////////////////////////////////////////////////////
                            if(pWorker->m_pChannel->m_stConnInfo.bYST && pWorker->m_pChannel->m_stConnInfo.nYSTNO > 0)
                            {
                                pWorker->m_pWorker->WriteYSTNOInfo(pWorker->m_pChannel->m_stConnInfo.chGroup,pWorker->m_pChannel->m_stConnInfo.nYSTNO, pWorker->m_pChannel->m_SList, pWorker->m_pChannel->m_addressA, 1,pWorker->m_pChannel->m_stConnInfo.sRandSocket);
                                OutputDebug("----------------------------channel localchannel: %d, yst: %d, line: %d\n",pWorker->m_nLocalChannel,pWorker->m_pChannel->m_stConnInfo.nYSTNO,__LINE__);
                                pWorker->m_pWorker->YSTNOCushion(pWorker->m_pChannel->m_stConnInfo.chGroup,pWorker->m_pChannel->m_stConnInfo.nYSTNO, -1);
                            }
                            //////////////////////////////////////////////////////////////////////////
                        }
                        
                        pWorker->m_bPass = TRUE;
                        pWorker->m_pChannel->m_nStatus = OK_OLD;
                        if(pWorker->m_pChannel->m_stConnInfo.isBeRequestVedio==TRUE)
                        {
                            if(!pWorker->SendData(JVN_CMD_VIDEO, NULL, 0))
                            {
                                if(!pWorker->SendData(JVN_CMD_VIDEO, NULL, 0))
                                {
                                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                    {
                                        char chMsg[] = "请求预览失败!";
                                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "首次请求录像失败.", __FILE__,__LINE__);
                                    }
                                    else
                                    {
                                        char chMsg[] = "Request video failed!";
                                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel,"first REQ video failed.",__FILE__,__LINE__);
                                    }
                                    pWorker->m_nStatus = FAILD;
                                    pWorker->m_pChannel->m_nStatus = FAILD;
                                    break;
                                }
                            }
                        }
                        
#ifdef WIN32
                        //连接线程结束
                        if(pWorker->m_hEndEventC > 0)
                        {
                            CloseHandle(pWorker->m_hEndEventC);
                        }
                        pWorker->m_hEndEventC = 0;
                        return 0;
#else
                        pWorker->m_connectThreadExit = TRUE;
                        return NULL;
#endif
                    }
                }
                    break;
                case FAILD://连接失败
                {
#ifdef WIN32
                    if(pWorker->m_hEndEventC > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventC);
                        pWorker->m_hEndEventC = 0;
                    }
                    return 0;
#else
                    pWorker->m_connectThreadExit = TRUE;
                    return NULL;
#endif
                }
                    break;
                case WAIT_LRECHECK://内网探测预验证超时
                {
                    pWorker->m_pChannel->m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
                    int ncount = pWorker->m_pChannel->m_SList.size();
                    int i=-1;
                    for(i=0; i<ncount; i++)
                    {
                        if(pWorker->m_pChannel->m_SList[i].buseful)
                        {
                            break;
                        }
                    }
                    
                    if(pWorker->SendPWCheck())
                    {//发送身份验证消息成功 等待结果
                        pWorker->m_nStatus = WAIT_LPWCHECK;
                        pWorker->m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送身份验证消息失败 开始外网连接
                        pWorker->m_nStatus = FAILD;
                        
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_bPass = FALSE;
                        
                        pWorker->m_pChannel->m_nStatus = WAIT_LW_CONNECTOLD_F;//NEW_P2P_N;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]旧版尝试，内网探测失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]OldTry, LocalTry failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                }
                    break;
                case WAIT_LPWCHECK://等待身份验证
                {//验证成功置为OK, 否则置为FAILD
                    dwendtime = CCWorker::JVGetTime();
                    if(dwendtime > pWorker->m_dwStartTime + 4000)//1000
                    {//等待超时，结束连接
                        int ncount = pWorker->m_pChannel->m_SList.size();
                        int nIndex=-1;
                        for(int i=0; i<ncount; i++)
                        {
                            if(pWorker->m_pChannel->m_SList[i].buseful)
                            {
                                nIndex = i;
                                break;
                            }
                        }
                        writeLog("old pwd time out...%d",__LINE__);
                        OutputDebug("old pwd time out...%d",__LINE__);
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_nStatus = FAILD;
                        pWorker->m_pChannel->m_nStatus = WAIT_LW_CONNECTOLD_F;//NEW_P2P;
                        
                        pWorker->m_bPass = FALSE;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]旧版尝试，内网探测失败. 等待身份验证数据超时",nIndex);
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]OldTry, LocalTry failed. wait pass info failed.",nIndex);
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                    else
                    {//接收数据
                        int nPWData = 0;
                        nret = pWorker->RecvPWCheck(nPWData);
                        if(nret == 1)
                        {//验证通过 连接成功建立
                            OutputDebug("old pwd ok. line: %d",__LINE__);
                            pWorker->m_nStatus = OK;
                            //						memcpy(&pWorker->m_addressA, &pWorker->m_addrAL, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addressA, &pWorker->m_pChannel->m_addrAL, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addrConnectOK, &pWorker->m_pChannel->m_addrAL, sizeof(SOCKADDR_IN));
                        }
                        else if(nret == -10)//无通道服务
                        {
                            pWorker->SetNoChannel();
                        }
                        else if(nret == 0)
                        {//身份验证未通过 直接结束连接
                            OutputDebug("old pwd err. LINE: %d",__LINE__);
                            int ncount = pWorker->m_pChannel->m_SList.size();
                            int nIndex=-1;
                            for(int i=0; i<ncount; i++)
                            {
                                if(pWorker->m_pChannel->m_SList[i].buseful)
                                {
                                    nIndex = i;
                                    break;
                                }
                            }
                            
                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                            
                            pWorker->m_nStatus = FAILD;
                            pWorker->m_pChannel->m_nStatus = WAIT_LW_CONNECTOLD_F;//NEW_P2P;
                            pWorker->m_pChannel->m_bPassWordErr = TRUE;
                            
                            pWorker->m_bPass = FALSE;
                            
                            if(pWorker->m_pWorker->m_bNeedLog)
                            {
                                char chMsg[MAX_PATH]={0};
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    sprintf(chMsg,"<[S%d]旧版尝试，内网探测失败. 身份验证失败",nIndex);
                                    strcat(chError, chMsg);
                                }
                                else
                                {
                                    sprintf(chMsg,"<[S%d]OldTry, LocalTry failed.pass failed.",nIndex);
                                    strcat(chError, chMsg);
                                }
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                            }
                        }
                    }
                }
                    break;
                case WAIT_NRECHECK://外网直连预验证失败，进行旧版尝试
                {
                    pWorker->m_pChannel->m_stConnInfo.nShow = JVN_CONNTYPE_P2P;
                    //BOOL bfind = FALSE;
                    int ncount = pWorker->m_pChannel->m_SList.size();
                    int i = -1;
                    for(i=0; i<ncount; i++)
                    {
                        if(pWorker->m_pChannel->m_SList[i].buseful)
                        {
                            break;
                        }
                    }
                    
                    if(pWorker->SendPWCheck())
                    {//发送身份验证消息成功 等待结果
                        pWorker->m_nStatus = WAIT_NPWCHECK;
                        pWorker->m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送身份验证消息失败 尝试其他服务器
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        pWorker->m_bPass = FALSE;
                        
                        pWorker->m_nStatus = FAILD;
                        pWorker->m_pChannel->m_nStatus = WAIT_NW_CONNECTOLD_F;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]旧版尝试，外网直连失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]OldTry, Net Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                }
                    break;
                case WAIT_NPWCHECK://等待身份验证
                {
                    dwendtime = CCWorker::JVGetTime();
                    if(dwendtime > pWorker->m_dwStartTime + 10000)
                    {//等待超时，结束连接
                        pWorker->m_nStatus = FAILD;//NEW_P2P;
                        int ncount = pWorker->m_pChannel->m_SList.size();
                        int nIndex=-1;
                        for(int i=0; i<ncount; i++)
                        {
                            if(pWorker->m_pChannel->m_SList[i].buseful)
                            {
                                nIndex = i;
                                break;
                            }
                        }
                        writeLog("old pwd time out...%d",__LINE__);
                        OutputDebug("old pwd time out...%d",__LINE__);
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_bPass = FALSE;
                        
                        pWorker->m_pChannel->m_nStatus = WAIT_NW_CONNECTOLD_F;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]旧版尝试，外网直连失败. 等待身份验证数据超时",nIndex);
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]OldTry, Net Connect failed. wait pass info failed.",nIndex);
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                    else
                    {//接收数据
                        int nPWData=0;
                        nret = pWorker->RecvPWCheck(nPWData);
                        if(nret == 1)
                        {//验证通过 连接成功建立
                            OutputDebug("old pwd ok. line: %d",__LINE__);
                            pWorker->m_nStatus = OK;
                            //						memcpy(&pWorker->m_addressA, &pWorker->m_addrAN, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addressA, &pWorker->m_pChannel->m_addrAN, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addrConnectOK, &pWorker->m_pChannel->m_addrAN, sizeof(SOCKADDR_IN));
                        }
                        else if(nret == -10)//无通道服务
                        {
                            pWorker->SetNoChannel();
                        }
                        else if(nret == 0)
                        {//身份验证未通过 直接结束连接
                            pWorker->m_nStatus = FAILD;//NEW_P2P;
                            OutputDebug("old pwd err. LINE: %d",__LINE__);
                            
                            int ncount = pWorker->m_pChannel->m_SList.size();
                            int nIndex=-1;
                            for(int i=0; i<ncount; i++)
                            {
                                if(pWorker->m_pChannel->m_SList[i].buseful)
                                {
                                    nIndex = i;
                                    break;
                                }
                            }
                            
                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                            
                            pWorker->m_bPass = FALSE;
                            
                            pWorker->m_pChannel->m_nStatus = WAIT_NW_CONNECTOLD_F;
                            pWorker->m_pChannel->m_bPassWordErr = TRUE;
                            
                            if(pWorker->m_pWorker->m_bNeedLog)
                            {
                                char chMsg[MAX_PATH]={0};
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    sprintf(chMsg,"<[S%d]旧版尝试，外网直连失败. 身份验证失败",nIndex);
                                    strcat(chError, chMsg);
                                }
                                else
                                {
                                    sprintf(chMsg,"<[S%d]OldTry, Net Connect failed.pass failed.",nIndex);
                                    strcat(chError, chMsg);
                                }
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                            }
                        }
                    }
                }
                    break;
                case WAIT_TSRECHECK://取得转发地址
                {
                    pWorker->m_pChannel->m_stConnInfo.nShow = JVN_CONNTYPE_TURN;
                    
                    //BOOL bfind=FALSE;
                    int i = -1;
                    int ncount = pWorker->m_pChannel->m_SList.size();
                    for(i=0; i<ncount; i++)
                    {
                        if(pWorker->m_pChannel->m_SList[i].buseful)
                        {
                            break;
                        }
                    }
                    
                    if(pWorker->SendPWCheck())
                    {//发送身份验证消息成功 等待结果
                        pWorker->m_nStatus = WAIT_TSPWCHECK;
                        pWorker->m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//发送身份验证消息失败 尝试其他服务器
                        writeLog("ccoldchannel: error SendPwdCheck, line: %d",__LINE__);
                        pWorker->m_nStatus = FAILD;//NEW_TURN;
                        
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        
                        pWorker->m_bPass = FALSE;
                        
                        pWorker->m_pChannel->m_nStatus = WAIT_TSW_CONNECTOLD_F;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]旧版尝试，TURN连接失败. 发送身份验证数据失败 详细:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]OldTry, TURN Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                    
                }
                    break;
                case WAIT_TSPWCHECK://等待身份验证
                {
                    dwendtime = CCWorker::JVGetTime();
                    if(dwendtime > pWorker->m_dwStartTime + 5000)
                    {//等待超时，结束连接
                        pWorker->m_nStatus = FAILD;//NEW_TURN;
                        
                        int ncount = pWorker->m_pChannel->m_SList.size();
                        int nIndex=-1;
                        for(int i=0; i<ncount; i++)
                        {
                            if(pWorker->m_pChannel->m_SList[i].buseful)
                            {
                                nIndex = i;
                                break;
                            }
                        }
                        
                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                        writeLog("old pwd time out...%d",__LINE__);
                        OutputDebug("old pwd time out...%d",__LINE__);
                        pWorker->m_bPass = FALSE;
                        pWorker->m_pChannel->m_nStatus = WAIT_TSW_CONNECTOLD_F;
                        
                        if(pWorker->m_pWorker->m_bNeedLog)
                        {
                            char chMsg[MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]TURN连接失败. 等待身份验证数据超时",nIndex);
                                strcat(chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]TURN Connect failed. wait pass info failed.",nIndex);
                                strcat(chError, chMsg);
                            }
                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                        }
                    }
                    else
                    {//接收数据
                        int nPWData = 0;
                        nret = pWorker->RecvPWCheck(nPWData);
//                        writeLog("ccoldchannel: RecvPWCheck nret: %d,line: %d",nret,__LINE__);
                        if(nret == 1)
                        {//验证通过 连接成功建立
                            OutputDebug("old pwd ok. line: %d",__LINE__);
                            pWorker->m_nStatus = OK;
                            //						memcpy(&pWorker->m_addressA, &pWorker->m_addrTS, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addressA, &pWorker->m_pChannel->m_addrTS, sizeof(SOCKADDR_IN));
                            memcpy(&pWorker->m_pChannel->m_addrConnectOK, &pWorker->m_pChannel->m_addrTS, sizeof(SOCKADDR_IN));
                        }
                        else if(nret == -10)//无通道服务
                        {
                            pWorker->SetNoChannel();
                        }
                        else if(nret == 0)
                        {//身份验证未通过 直接结束连接
                            OutputDebug("old pwd err. LINE: %d",__LINE__);
                            pWorker->m_nStatus = FAILD;//NEW_TURN;
                            int ncount = pWorker->m_pChannel->m_SList.size();
                            int nIndex=-1;
                            for(int i=0; i<ncount; i++)
                            {
                                if(pWorker->m_pChannel->m_SList[i].buseful)
                                {
                                    nIndex = i;
                                    break;
                                }
                            }
                            
                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                            
                            pWorker->m_bPass = FALSE;
                            pWorker->m_pChannel->m_nStatus = WAIT_TSW_CONNECTOLD_F;
                            pWorker->m_pChannel->m_bPassWordErr = TRUE;
                            
                            if(pWorker->m_pWorker->m_bNeedLog)
                            {
                                char chMsg[MAX_PATH]={0};
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    sprintf(chMsg,"<[S%d]TURN连接失败. 身份验证失败",nIndex);
                                    strcat(chError, chMsg);
                                }
                                else
                                {
                                    sprintf(chMsg,"<[S%d]TURN Connect failed.pass failed.",nIndex);
                                    strcat(chError, chMsg);
                                }
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "", __FILE__,__LINE__,chError);
                            }
                        }
                    }
                }
                    break;
                default:
                    break;
            }
            
            CCWorker::jvc_sleep(2);
        }
        
        pWorker->m_connectThreadExit = TRUE;
        // pWorker->m_pWorker->g_vm->DetachCurrentThread();
#ifdef WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    BOOL CCOldChannel::SendPWCheck()
    {//类型(1)+长度(4)+用户名长(4)+密码长(4)+用户名+密码
        int nNLen = strlen(m_pChannel->m_stConnInfo.chPassName);
        if(nNLen > MAX_PATH)
        {
            nNLen = 0;
        }
        int nWLen = strlen(m_pChannel->m_stConnInfo.chPassWord);
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
            memcpy(&data[9], m_pChannel->m_stConnInfo.chPassName, nNLen);
        }
        if(nWLen > 0 && nWLen < MAX_PATH)
        {
            memcpy(&data[9+nNLen], m_pChannel->m_stConnInfo.chPassWord, nWLen);
        }
        
        OutputDebug("-----------------name: %s, pwd: %s, line: %d, function: %s",m_pChannel->m_stConnInfo.chPassName,m_pChannel->m_stConnInfo.chPassWord,__LINE__,__FUNCTION__);
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            int n=0;
            if(0 >= (n=CCChannel::tcpsenddata(m_pChannel->m_ServerSocket, (char *)data, nNLen + nWLen + 9, TRUE)))
            {
                return FALSE;
            }
            else
            {
                //			OutputDebug("send old pwd.");
                return TRUE;
            }
        }
        else
        {//UDP连接
            if(m_bShouldSendMOType)
            {
                char chtmp[10]={0};
                chtmp[0] = JVN_CMD_MOTYPE;
                
                int ret = CCChannel::udpsenddata(m_pChannel->m_ServerSocket, chtmp, 5, TRUE);
                writeLog("**********ccoldchannel sebd JVN_CMD_MoTYPE ret: %d,line: %d",ret,__LINE__);
            }
            
            if(0 >= CCChannel::udpsenddata(m_pChannel->m_ServerSocket, (char *)data, nNLen + nWLen + 9, TRUE))
            {
                 writeLog("**********ccoldchannel sebd JVN_CMD_MoTYPE line: %d,error ",__LINE__);
                return FALSE;
            }
            else
            {
                //			OutputDebug("%d send old pwd.",m_pChannel->m_stConnInfo.nLocalChannel);
                writeLog("**********ccoldchannel sebd JVN_CMD_MoTYPE line: %d,success ",__LINE__);
                return TRUE;
            }
        }
    }
    
    int CCOldChannel::RecvPWCheck(int &nPWData)
    {
        int rs = 0;
        int rsize = 0;
        int nLen = 0;
        nPWData = 0;
        
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            if(0 < (rs = CCChannel::tcpreceive2(m_pChannel->m_ServerSocket, (char *)m_precBuf, 1, 1)))
            {//收到数据
                nLen=-1;
                BYTE uchtype = 0;
                uchtype = m_precBuf[0];
                if(uchtype == JVN_RSP_NOSERVER)
                {
                    return -10;//无通道服务
                }
                
                if(m_precBuf[0] == JVN_RSP_CHECKPASST || m_precBuf[0] == JVN_RSP_CHECKPASSF)
                {//身份验证 是否通过(1)+长度(4)+[附加值(4)]
                    rsize = 0;
                    rs = 0;
                    while (rsize < 4)
                    {
                        if(0 > (rs = CCChannel::tcpreceive(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], 4 - rsize, 1)))
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
                    
                    memcpy(&nLen, m_precBuf, 4);
                    if(nLen == 1)
                    {
                        if(m_bByStreamServer)
                        {
                            return -1;
                        }
                        if(uchtype == JVN_RSP_CHECKPASST)
                        {
                            rsize = 0;
                            rs = 0;
                            while (rsize < nLen)
                            {
                                if(0 > (rs = CCChannel::tcpreceive(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], nLen - rsize, 1)))
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
                            
                            if(m_precBuf[0] == 1)
                            {
                                return 1;
                            }
                            
                            return 0;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    else if(nLen == 4)
                    {//新协议 带附加值
                        rsize = 0;
                        rs = 0;
                        while (rsize < nLen)
                        {
                            if(0 > (rs = CCChannel::tcpreceive(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], nLen - rsize, 1)))
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
                        
                        memcpy(&nPWData, &m_precBuf[0], 4);
                        
                        if(m_bByStreamServer)
                        {
                            return -1;
                        }
                        
                        if(uchtype == JVN_RSP_CHECKPASST)
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
            //		else if(rs < 0)
            //		{
            //			int kkk=WSAGetLastError();
            //		}
        }
        else
        {//UDP连接
            m_pChannel->m_nFYSTVER = UDT::getystverF(m_pChannel->m_ServerSocket);//获取远端协议版本
//            writeLog("ccoldchannel: m_pChannel->m_nFYSTVER :%d,line: %d",m_pChannel->m_nFYSTVER,__LINE__);
            if(m_pChannel->m_nFYSTVER >= JVN_YSTVER4)
            {//支持msg
                rs = UDT::recvmsg(m_pChannel->m_ServerSocket, (char *)m_precBuf, JVNC_DATABUFLEN);
//                writeLog("ccoldchannel rs: %d, line: %d",rs,__LINE__);
                if(0 < rs)
                {//收到数据
                    nLen=-1;
                    BYTE uchtype = 0;
                    uchtype = m_precBuf[0];
                    writeLog("ccoldchannel: uchType: %d, line: %d",uchtype,__LINE__);
                    if(uchtype == JVN_RSP_NOSERVER)
                    {
                        return -10;//无通道服务
                    }
                    
                    if(m_precBuf[0] == JVN_RSP_CHECKPASST || m_precBuf[0] == JVN_RSP_CHECKPASSF)
                    {//身份验证 是否通过(1)+长度(4)+[附加值(4)]
                        rsize = 0;
                        rs = 0;
                        
                        memcpy(&nLen, &m_precBuf[1], 4);
                        if(nLen == 0)
                        {
                            if(m_bByStreamServer)
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            
                            if(uchtype == JVN_RSP_CHECKPASST)
                            {
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                        else if(nLen == 4)
                        {//新协议 带附加值
                            rsize = 0;
                            rs = 0;
                            
                            memcpy(&nPWData, &m_precBuf[5], 4);
                            
                            if(m_bByStreamServer)
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            
                            if(uchtype == JVN_RSP_CHECKPASST)
                            {
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                    }
                    else if(uchtype == JVN_CMD_CONNSSERVER)
                    {//对方是流媒体连接方式
                        rsize = 0;
                        rs = 0;
                        
                        memcpy(&nLen, &m_precBuf[1], 4);
                        if(nLen == 20)
                        {
                            rsize = 0;
                            rs = 0;
                            
                            unsigned int unsid=0;//通道ID
                            STJVID  stcid={0,0};  //自身ID
                            unsigned long ip=0;
                            int port=0;
                            memcpy((char *)&unsid,&m_precBuf[5],4);
                            memcpy((char *)&stcid,&m_precBuf[9],8);
                            memcpy((char *)&ip,&m_precBuf[17],4);
                            memcpy((char *)&port,&m_precBuf[21],4);
                            if((unsid>=0) && (!ISIDZERO(stcid)))
                            {
                                if(m_pStream == NULL)
                                {
                                    m_pStream=new CCStream(m_nChannel,m_nLocalChannel,m_nProtocolType,m_pWorker,this);
                                }
                                
                                m_pStream->m_idSServerID=unsid;
                                COPYID(m_pStream->m_idMyID,stcid);
                                m_pStream->m_nChannel=m_nChannel;
                                m_pStream->m_nSServerIP=ip;
                                m_pStream->m_nSServerPort=port;
                                SendData(JVN_RSP_ID,0,0);
                                if(m_pStream->ConnectStreamServer())
                                {
                                    return -2;
                                    
                                }
                                else
                                {
                                    m_bByStreamServer=FALSE;
                                    SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                    return -1;
                                }
                            }
                            else
                            {
                                m_bByStreamServer=FALSE;
                                SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                        }
                    }
                }
            }
            else
            {
                if(0 < (rs = UDT::recv(m_pChannel->m_ServerSocket, (char *)m_precBuf, 1, 0)))
                {//收到数据
                    nLen=-1;
                    BYTE uchtype = 0;
                    uchtype = m_precBuf[0];
                    if(uchtype == JVN_RSP_NOSERVER)
                    {
                        return -10;//无通道服务
                    }
                    if(m_precBuf[0] == JVN_RSP_CHECKPASST || m_precBuf[0] == JVN_RSP_CHECKPASSF)
                    {//身份验证 是否通过(1)+长度(4)+[附加值(4)]
                        rsize = 0;
                        rs = 0;
                        while (rsize < 4)
                        {
                            if(UDT::ERROR == (rs = UDT::recv(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], 4 - rsize, 0)))
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        memcpy(&nLen, m_precBuf, 4);
                        if(nLen == 0)
                        {
                            if(m_bByStreamServer)
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            
                            if(uchtype == JVN_RSP_CHECKPASST)
                            {
                                
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                        else if(nLen == 4)
                        {//新协议 带附加值
                            rsize = 0;
                            rs = 0;
                            while (rsize < nLen)
                            {
                                if(UDT::ERROR == (rs = UDT::recv(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], nLen - rsize, 0)))
                                {
                                    writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                    return -1;
                                }
                                else if(rs == 0)
                                {
                                    CCWorker::jvc_sleep(1);
                                    continue;
                                }
                                
                                rsize += rs;
                            }
                            
                            memcpy(&nPWData, &m_precBuf[0], 4);
                            
                            if(m_bByStreamServer)
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            
                            if(uchtype == JVN_RSP_CHECKPASST)
                            {
                                return 1;
                            }
                            else
                            {
                                return 0;
                            }
                        }
                    }
                    else if(uchtype == JVN_CMD_CONNSSERVER)
                    {//对方是流媒体连接方式
                        rsize = 0;
                        rs = 0;
                        while (rsize < 4)
                        {
                            if(UDT::ERROR == (rs = UDT::recv(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], 4 - rsize, 0)))
                            {
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        memcpy(&nLen, m_precBuf, 4);
                        if(nLen == 20)
                        {
                            rsize = 0;
                            rs = 0;
                            while (rsize < nLen)
                            {
                                if (UDT::ERROR == (rs = UDT::recv(m_pChannel->m_ServerSocket, (char *)&m_precBuf[rsize], nLen - rsize, 0)))
                                {
                                    writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                    return -1;
                                }
                                else if(rs == 0)
                                {
                                    CCWorker::jvc_sleep(1);
                                    continue;
                                }
                                rsize += rs;
                            }
                            
                            unsigned int unsid=0;//通道ID
                            STJVID  stcid={0,0};  //自身ID
                            unsigned long ip=0;
                            int port=0;
                            memcpy((char *)&unsid,&m_precBuf[0],4);
                            memcpy((char *)&stcid,&m_precBuf[4],8);
                            memcpy((char *)&ip,&m_precBuf[12],4);
                            memcpy((char *)&port,&m_precBuf[16],4);
                            if((unsid>=0) && (!ISIDZERO(stcid)))
                            {
                                if(m_pStream == NULL)
                                {
                                    m_pStream=new CCStream(m_nChannel,m_nLocalChannel,m_nProtocolType,m_pWorker,this);
                                }
                                
                                m_pStream->m_idSServerID=unsid;
                                COPYID(m_pStream->m_idMyID,stcid);
                                m_pStream->m_nChannel=m_nChannel;
                                m_pStream->m_nSServerIP=ip;
                                m_pStream->m_nSServerPort=port;
                                SendData(JVN_RSP_ID,0,0);
                                if(m_pStream->ConnectStreamServer())
                                {
                                    return -2;
                                    
                                }
                                else
                                {
                                    m_bByStreamServer=FALSE;
                                    SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                    return -1;
                                }
                            }
                            else
                            {
                                m_bByStreamServer=FALSE;
                                SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
                                return -1;
                            }
                        }
                    }
                }
            }
            
        }
//        writeLog("ccoldchannel RecvPWCheck return -1 line: %d",__LINE__);
        return -1;
    }
    
    BOOL CCOldChannel::StartWorkThread()
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
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP连接
            m_recvThreadExit = FALSE;
            if (0 != pthread_create(&m_hRecvThread, pAttr, RecvProcTCP, this))
            {m_recvThreadExit = TRUE;
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
        {m_recvThreadExit = FALSE;
            int nret = -1;
            if(m_pChannel->m_nFYSTVER >= JVN_YSTVER4)
            {//支持msg接收
                nret = pthread_create(&m_hRecvThread, pAttr, RecvMsgProc, this);
            }
            else
            {
                nret = pthread_create(&m_hRecvThread, pAttr, RecvProc, this);
            }
            if (0 != nret)
            {m_recvThreadExit = TRUE;
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
        
        m_playProExit = FALSE;
        /*	if (0 != pthread_create(&m_hPlayThread, pAttr, PlayProc, this))
         {
         m_hPlayThread = 0;
         if(m_pWorker != NULL)
         {
         if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
         {
         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "创建播放线程失败", __FILE__,__LINE__);
         }
         else
         {
         m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "connect failed. create play thread failed.", __FILE__,__LINE__);
         }
         }
         m_playProExit = TRUE;
         //return FALSE;
         }
         #else
         //启动接收线程
         UINT unTheadID;
         //创建本地监听线程
         m_hStartEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
         m_hEndEventR = CreateEvent(NULL, FALSE, FALSE, NULL);
         
         if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
         {//TCP
         m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvProcTCP, (void *)this, 0, &unTheadID);
         }
         else
         {
         if(m_pChannel->m_nFYSTVER >= JVN_YSTVER4)
         {//支持msg接收
         m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsgProc, (void *)this, 0, &unTheadID);
         }
         else
         {
         m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvProc, (void *)this, 0, &unTheadID);
         }
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
         */
        /*
         //创建本地监听线程
         m_hStartEventP = CreateEvent(NULL, FALSE, FALSE, NULL);
         m_hEndEventP = CreateEvent(NULL, FALSE, FALSE, NULL);
         
         m_hPlayThread = (HANDLE)_beginthreadex(NULL, 0, PlayProc, (void *)this, 0, &unTheadID);
         
         SetEvent(m_hStartEventP);
         if (m_hPlayThread == 0)//创建线程失败
         {
         //清理线程
         if(m_hStartEventP > 0)
         {
         CloseHandle(m_hStartEventP);
         m_hStartEventP = 0;
         }
         
         if(m_hEndEventP > 0)
         {
         CloseHandle(m_hEndEventP);
         m_hEndEventP = 0;
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
         
         //	return FALSE;
         }
         */
#endif
        
        return TRUE;
    }
    
    BOOL CCOldChannel::SendData(BYTE uchType, BYTE *pBuffer,int nSize)
    {
        if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
        {//TCP
            
            return SendDataTCP(uchType, pBuffer,nSize);
            
            
        }
        
        if(m_bByStreamServer && m_pStream != NULL)
        {
            return m_pStream->SendData(uchType,pBuffer,nSize);
        }
        
        if(m_pChannel->m_ServerSocket > 0)
        {
            BYTE data[5 + 2*JVN_BAPACKDEFLEN]={0};
            memset(data, 0, sizeof(data));
            switch(uchType)
            {
				
                case JVN_REQ_CHECK://请求录像检索
                {
                    if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
                    {
//                        //BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
                        data[0] = uchType;
                        memcpy(&data[1], &nSize, 4);
                        memcpy(&data[5], pBuffer, nSize);
                        UDT::send(m_pChannel->m_ServerSocket, (char *)data,5 + nSize, 0);
                    }
                }
                    break;
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
                            if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                
                                return FALSE;
                            }
                        }
                        m_bDAndP = TRUE;
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
                        UDT::send(m_pChannel->m_ServerSocket, (char *)data,9, 0);
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
                {
                DOSEND:
                    int nLen = 5;
                    //BYTE data[5]={0};
                    data[0] = uchType;
                    
                    int ss=0;
                    int ssize=0;
                    while(ssize < nLen)
                    {
                        if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                            
                            return FALSE;
                        }
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
                        if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                //							if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, min(nLen - ssize, 1400), 0)))
                                if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, m_nPACKLEN), 0)))
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
                        if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                        memcpy(&data[5], pBuffer, nSize);//0 停止 1 开启
                    }
                    else
                    {
                        break;
                    }
                    
                    int ss=0;
                    int ssize=0;
                    while(ssize < nLen)
                    {
                        if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                            if(0 < (ss = UDT::send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
        return TRUE;
    }
    
    BOOL CCOldChannel::DisConnect()
    {
        if(m_bByStreamServer && m_pStream!=NULL)
        {
            m_pStream->DisConnectFromStream();
        }
        
        SendData(JVN_CMD_DISCONN, NULL, 0);
        //	Sleep(1);
        
        m_bClose = TRUE;
        m_bExit = TRUE;
#ifdef MOBILE_CLIENT
        if (!m_recvThreadExit)
        {
            m_bExit = TRUE;
            m_bEndR = TRUE;
            while (TRUE)
            {
                if (m_recvThreadExit)
                {//
                    break;
                }
                //            printf("run here old channel discconect %d nlocalChannel: %d\n",m_nLocalChannel,m_nChannel);
                CCWorker::jvc_sleep(100);//usleep(10);
            }
        }
        else
        {
            //		printf("disconnect oldChannel m_recvThreadExit and m_playProExit\n");
        }
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
        if (0 != m_hPlayThread)
        {
            m_bEndP = TRUE;
            pthread_join(m_hPlayThread, NULL);
            m_hPlayThread = 0;
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
        //	if(m_hEndEventP>0)
        //	{
        //		SetEvent(m_hEndEventP);
        //	}
#endif
        
        CCWorker::jvc_sleep(10);
        //    WaitThreadExit(m_hRecvThread);
        
        if(m_pChannel->m_ServerSocket > 0)
        {
            if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
            {//TCP
                closesocket(m_pChannel->m_ServerSocket);
            }
            else
            {
                m_pWorker->pushtmpsock(m_pChannel->m_ServerSocket);
            }
        }
        m_pChannel->m_ServerSocket = 0;
        
        m_bCanDelS=TRUE;
        return TRUE;
    }
    
    BOOL CCOldChannel::StopConnect()
    {
        m_bClose = TRUE;
        
        return TRUE;
    }
    
    void CCOldChannel::ClearBuffer()
    {
        if(m_pBuffer != NULL)
        {
            m_pBuffer->ClearBuffer();
        }
    }
    
#ifndef WIN32
    void* CCOldChannel::PlayProc(void* pParam)
#else
    UINT WINAPI CCOldChannel::PlayProc(LPVOID pParam)
#endif
    {
        CCOldChannel *pWorker = (CCOldChannel *)pParam;
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
        
        ::WaitForSingleObject(pWorker->m_hStartEventP, INFINITE);
        if(pWorker->m_hStartEventP > 0)
        {
            CloseHandle(pWorker->m_hStartEventP);
            pWorker->m_hStartEventP = 0;
        }
#endif
        
        int nLenRead=-1;
        BYTE uchTypeRead=0;
        int timeout = 0;
        while(TRUE)
        {
#ifndef WIN32
            if(pWorker->m_bExit || pWorker->m_bEndP)
            {
                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                CCWorker::jvc_sleep(1);
                
                break;
            }
#else
            if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventP, 0))
            {
                //	pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                //	CCWorker::jvc_sleep(1);
                
                if(pWorker->m_hEndEventP > 0)
                {
                    CloseHandle(pWorker->m_hEndEventP);
                    pWorker->m_hEndEventP = 0;
                }
                
                break;
            }
#endif
            
            if(!pWorker->m_bPass)
            {
                CCWorker::jvc_sleep(2);
                continue;
            }
            
            /*读缓冲区,显示*/
            uchTypeRead = 0;
            
            if(pWorker->m_pBuffer !=NULL && pWorker->m_pBuffer->ReadBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead))
            {
                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
            }
            
#ifndef WIN32
            CCWorker::jvc_sleep(timeout);
#else
            CCWorker::jvc_sleep(2);
#endif
            
        }
        
        //结束线程
#ifdef WIN32
        if(pWorker->m_hEndEventP > 0)
        {
            CloseHandle(pWorker->m_hEndEventP);
            pWorker->m_hEndEventP = 0;
        }
        
        CloseHandle(pWorker->m_hPlayThread);
        pWorker->m_hPlayThread = 0;
        return 0;
#else
        pWorker->m_hPlayThread = 0;
        pWorker->m_playProExit = TRUE;
        return NULL;
#endif
    }
    
#ifndef WIN32
    void* CCOldChannel::RecvProc(void* pParam)
#else
    UINT WINAPI CCOldChannel::RecvProc(LPVOID pParam)
#endif
    {
        CCOldChannel *pWorker = (CCOldChannel *)pParam;
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
        
        int rsize = 0;
        int rs=0;
        
        //计时相关
        DWORD dBeginPassTime;//开始身份验证超时计时
        DWORD dBeginTime=0;//视频数据计时
        DWORD dBeginTimeDP=0;//下载回放计时
        DWORD dEnd=0;
        DWORD dTimePassUsed=0;
        DWORD dTimeUsed=0;
        DWORD dTimeUsedDP=0;
        
        unsigned int unIID = 0;
        short int snFID = 0;
        unsigned int unFTime = 0;
        
        //计时
        dBeginPassTime = CCWorker::JVGetTime();
        dBeginTime = CCWorker::JVGetTime();
        dBeginTimeDP = CCWorker::JVGetTime();
        
        int nIRetryCount = 0;
        int nDPRetryCount = 0;
        
        int nFrameIndex = 0;
        BOOL bError = FALSE;//数据出错，此时视频帧必须从I帧开始继续接收，防止出现马赛克
        
        //	BOOL bRet = TRUE;
        int nLen=-1;
        BYTE uchType=0;
        int nLenRead=-1;
        BYTE uchTypeRead=0;
        //	OutputDebug("thread start %d\n\n\n",pWorker->m_nLocalChannel);
        
        while(TRUE)
        {
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
            
            if(!pWorker->m_bPass)
            {
                CCWorker::jvc_sleep(2);
                continue;
            }
            
            /*读缓冲区,显示*/
            //		uchTypeRead = 0;
            //		if(pWorker->m_pBuffer->ReadBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead))
            //		{
            //			pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
            //		}
            
            uchType = 0;
            rs = 0;
            //		memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, 1, 0)))
            {
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
                
                if(pWorker->m_pWorker != NULL)
                {
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
            {
                
                ////////////////////////////////////////计时 超过40s未收到任何帧,发送响应帧,超过5分钟仍无数据,不再发送
                dEnd = CCWorker::JVGetTime();
                dTimeUsed = dEnd - dBeginTime;
                dTimeUsedDP = dEnd - dBeginTimeDP;
                dTimePassUsed = dEnd - dBeginPassTime;
                
                if(!pWorker->m_bPass && dTimeUsed > 30000)
                {
#ifdef WIN32
                    if(pWorker->m_hEndEventR > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventR);
                        pWorker->m_hEndEventR = 0;
                    }
#endif
                    
                    //结束线程
                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                    {
                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                    }
                    pWorker->m_pChannel->m_ServerSocket = 0;
                    
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "连接超时.请重试或重新运行程序.";
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收身份验证数据超时,接收线程退出.", __FILE__,__LINE__);
                    }
                    else
                    {
                        char chMsg[] = "connect time out!";
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive pass info time out.", __FILE__,__LINE__);
                    }
                    
#ifdef WIN32
                    CloseHandle(pWorker->m_hRecvThread);
#endif
                    pWorker->m_hRecvThread = 0;
                    pWorker->m_recvThreadExit = TRUE;
                    return 0;
                }
                
                if(dTimeUsed > 40000 && (dTimeUsed-40000 > 40000*nIRetryCount) && nIRetryCount < 10)
                {
                    //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到帧数据.", __FILE__,__LINE__);
                    pWorker->SendData(JVN_DATA_OK, NULL, 4);//0);//视频帧确认
                    nIRetryCount++;
                }
                if(pWorker->m_bDAndP && dTimeUsedDP > 40000 && (dTimeUsedDP-40000 > 40000*nDPRetryCount) && nDPRetryCount < 2)
                {
                    //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到下载或回放数据.", __FILE__,__LINE__);
                    pWorker->SendData(JVN_DATA_DANDP, NULL, 0);//下载回放确认
                    nDPRetryCount++;
                }
                
                CCWorker::jvc_sleep(5);
                continue;
            }
            
            nLen=-1;
            uchType = pWorker->m_precBuf[0];
            
            //char ch[100]={0};
            //sprintf(ch,"uchType=0x%X\n",uchType);
            //OutputDebugString(ch);
            
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                case JVN_DATA_A://音频
                case JVN_DATA_S://帧尺寸
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头
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
                case JVN_DATA_SPEED://码率
                {
                    rsize = 0;
                    rs = 0;
                    //				memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                    while (rsize < 4)
                    {
#ifndef WIN32
                        if(pWorker->m_bExit || pWorker->m_bEndR)
                        {
                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                            CCWorker::jvc_sleep(1);
                            
                            //结束线程
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket = 0;
                            
                            pWorker->m_hRecvThread = 0;
                            pWorker->m_recvThreadExit = TRUE;
                            return 0;
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
                            
                            //结束线程
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket = 0;
                            
                            CloseHandle(pWorker->m_hRecvThread);
                            pWorker->m_hRecvThread = 0;
                            return 0;
                        }
#endif
                        
                        if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, 0)))
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            //异常关闭
                            pWorker->m_bPass = FALSE;
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            //结束线程
                            
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket = 0;
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                            }
                            else
                            {
                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                            }
                            
#ifdef WIN32
                            CloseHandle(pWorker->m_hRecvThread);
#endif
                            pWorker->m_hRecvThread = 0;
                            pWorker->m_recvThreadExit = TRUE;
                            return 0;
                        }
                        else if(rs == 0)
                        {
                            CCWorker::jvc_sleep(1);
                            continue;
                        }
                        
                        rsize += rs;
                    }
                    
                    memcpy(&nLen, pWorker->m_precBuf, 4);
                    
                    if(nLen < 0 || nLen > JVNC_DATABUFLEN)
                    {
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错", __FILE__,__LINE__);
                        
                        continue;
                    }
                    //////////////////////////////////////////////////////////////////////////
                    //				char ch[100]={0};
                    //				sprintf(ch,".........................uchType=0x%X len:%d\n",uchType,nLen);
                    //				OutputDebugString(ch);
                    //////////////////////////////////////////////////////////////////////////
                    
                }
                    break;
                default:
                    UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, JVNC_DATABUFLEN, 0);
                    //			pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:收到无效数据.", __FILE__,__LINE__);
                    //			char ch[100]={0};
                    //			sprintf(ch,"default.........................uchType=0x%X\n",uchType);
                    //			OutputDebugString(ch);
                    
                    continue;
            }
            
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        if(pWorker->m_pChannel->m_nFYSTVER < JVN_YSTVER1)
                        {
                            nLen += 4;//多接收4个帧编号字节
                        }
                        
                        rsize = 0;
                        rs = 0;
                        //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                pWorker->m_hRecvThread = 0;
                                return NULL;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                pWorker->m_recvThreadExit = TRUE;
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                /*读缓冲区,显示*/
                                //							uchTypeRead = 0;
                                //							if(pWorker->m_pBuffer->ReadBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead))
                                //							{
                                //								pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
                                //							}
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                            
                            /*读缓冲区,显示*/
                            //						uchTypeRead = 0;
                            //						if(pWorker->m_pBuffer->ReadBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead))
                            //						{
                            //							pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
                            //						}
                        }
                        
                        
                        if(pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER1)
                        {//类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                            memcpy(&unIID, pWorker->m_precBuf, 4);
                            memcpy(&snFID, &pWorker->m_precBuf[4], 2);
                            memcpy(&unFTime, &pWorker->m_precBuf[6], 4);
                            //nFID = nFID>>16;
                            /*
                             char ch[100]={0};
                             if(uchType == JVN_DATA_I)
                             {
                             sprintf(ch,"%d.%d.%d....................................\n",unIID, snFID, unFTime);
                             }
                             else
                             {
                             sprintf(ch,"%d.%d.%d.................\n",unIID, snFID, unFTime);
                             }
                             OutputDebugString(ch);
                             */						if(uchType == JVN_DATA_I)
                             {//收到I视频帧，确认
                                 bError = FALSE;
                                 pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 6);
                                 pWorker->m_pChannel->UpdateYSTNOList();
                                 
                             }
                             else
                             {
                                 pWorker->m_nOCount++;
                                 if(pWorker->m_nOCount%JVNC_ABFRAMERET == 0)
                                 {
                                     pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 6);
                                 }
                             }
                        }
                        else
                        {//类型+长度+nframeindex(4) +数据区
                            /*判断收到的帧编号，I帧确认，一个序列内每10帧一个确认*/
                            memcpy(&nFrameIndex, pWorker->m_precBuf, 4);
                            if(uchType == JVN_DATA_I)
                            {//收到I视频帧，确认
                                bError = FALSE;
                                pWorker->m_nOCount = 1;
                                pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 4);
                            }
                            else
                            {
                                pWorker->m_nOCount++;
                                if(pWorker->m_nOCount%JVNC_ABFRAMERET == 0)
                                {
                                    pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 4);
                                }
                            }
                            
                            dTimeUsed = CCWorker::JVGetTime() - dBeginTime;
                        }
                        
                        //计时
                        dBeginTime = CCWorker::JVGetTime();
                        
                        nIRetryCount=0;
                        
                        if(pWorker->m_pWorker != NULL && !bError)
                        {
                            if(pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER1)
                            {//新协议，支持帧时间戳，播放控制更优
                                //写入缓冲
                                //pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[10], nLen-10, unIID, (int)snFID, unFTime);
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[10], nLen-10, unIID, (int)snFID, unFTime);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[10], nLen-10,0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[10], nLen-10,0,0);
#endif
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                            }
                            else
                            {
                                pWorker->m_precBuf[3] = nFrameIndex & 0xFF;
                                
                                //写入缓冲
                                
                                //pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[3], nLen-3, dTimeUsed);
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[3], nLen-3, dTimeUsed);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[4], nLen-4,0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[4], nLen-4,0,0);
#endif
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_DATA_A://音频
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        rsize = 0;
                        rs = 0;
                        //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                pWorker->m_hRecvThread = 0;
                                return NULL;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    pWorker->m_hRecvThread = 0;
                                    return NULL;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        if(uchType == JVN_DATA_A && pWorker->m_pChannel->m_bTURN && pWorker->m_pChannel->m_bOpenTurnAudio && pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER3)
                        {
                            if(pWorker->m_pWorker != NULL)
                            {
                                //新协议，支持帧时间戳，播放控制更优，写入缓冲区
                                //类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                                memcpy(&unIID, pWorker->m_precBuf, 4);
                                memcpy(&snFID, &pWorker->m_precBuf[4], 2);
                                memcpy(&unFTime, &pWorker->m_precBuf[6], 4);
                                //nFID = nFID>>16;
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[10], nLen - 10, unIID, (int)snFID, unFTime);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[10], nLen - 10, 0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[10], nLen - 10, 0,0);
#endif
                            }
                        }
                        else
                        {
                            if(pWorker->m_pWorker != NULL)
                            {
                                if(uchType == JVN_DATA_HEAD && pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->ClearBuffer();
                                }
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen, 0,0);
                                
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_FRAMETIME://帧间隔
                case JVN_DATA_S://帧尺寸
                {
                    int nWidth = 0, nHeight = 0;
                    
                    if(nLen == 8)
                    {
                        rsize = 0;
                        rs = 0;
                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        memcpy(&nWidth, &pWorker->m_precBuf[0], 4);
                        memcpy(&nHeight, &pWorker->m_precBuf[4], 4);
                        
                        if(uchType == JVN_DATA_S)
                        {
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
                            }
                        }
                        else
                        {
                            if(pWorker->m_pBuffer != NULL)
                            {
                                pWorker->m_pBuffer->m_nFrameTime = nWidth;
                                pWorker->m_pBuffer->m_nFrames = nHeight>0?nHeight:50;
                                
                                pWorker->m_pBuffer->m_nFrameTime = jvs_max(pWorker->m_pBuffer->m_nFrameTime, 0);
                                pWorker->m_pBuffer->m_nFrameTime = jvs_min(pWorker->m_pBuffer->m_nFrameTime, 10000);
                            }
                            
                            /*						char ch[100]={0};
                             char ch1[10]={0};
                             itoa(pWorker->m_pBuffer->m_nFrameTime, ch, 10);
                             strcat(ch, " frames=");
                             itoa(pWorker->m_pBuffer->m_nFrames, ch1, 10);
                             strcat(ch, ch1);
                             if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                             {
                             pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:收到帧设置数据, time=", __FILE__,__LINE__,ch);
                             }
                             else
                             {
                             pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "test:receive frameinfo,time=", __FILE__,__LINE__,ch);
                             }
                             */
                        }
                        
                        break;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_NOSERVER://无该通道服务
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "无该通道服务!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "channel is not open!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        
                        pWorker->m_hRecvThread = 0;
                        pWorker->m_recvThreadExit = TRUE;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_INVALIDTYPE://连接类型无效
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "连接类型无效!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "connect type invalid!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        
                        pWorker->m_hRecvThread = 0;
                        pWorker->m_recvThreadExit = TRUE;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_OVERLIMIT://超过最大连接数目
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "超过主控最大连接限制!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "client count limit!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        
                        pWorker->m_hRecvThread = 0;
                        pWorker->m_recvThreadExit = TRUE;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_REQ_CHAT://请求语音聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_REQ_TEXT://请求文本聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_CHATACCEPT://同意语音请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptChat = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_TEXTACCEPT://同意文本请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptText = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_CHATSTOP://停止语音
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptChat = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_TEXTSTOP://停止文本
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptText = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_CHECKDATA://检索结果
                case JVN_RSP_CHATDATA://语音数据
                case JVN_RSP_TEXTDATA://文本数据
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        /*接收数据*/
                        rsize = 0;
                        rs = 0;
                        //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        if(uchType == JVN_RSP_CHATDATA && pWorker->m_bAcceptChat)
                        {
                            /*回调函数语音*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_TEXTDATA && pWorker->m_bAcceptText)
                        {
                            /*回调函数文本*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKDATA)
                        {
                            /*回调函数检索结果*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,pWorker->m_precBuf, nLen);
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
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
                        pWorker->m_bDAndP = FALSE;
                        dBeginTimeDP = 0;
                        if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
                        {
                            /*回调函数下载*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, NULL, 0, 0);
                            }
                        }
                        else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
                        {
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,uchType, NULL, 0, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKOVER)
                        {
                            /*回调函数检索结果*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,NULL, 0);
                            }
                        }
                        else if(uchType == JVN_CMD_DISCONN)
                        {
                            pWorker->m_bPass = FALSE;
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                            //进行断开确认处理
                            pWorker->ProcessDisConnect();
                            
                            //结束线程
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket = 0;
                            
#ifdef WIN32
                            CloseHandle(pWorker->m_hRecvThread);
#endif
                            pWorker->m_hRecvThread = 0;
                            pWorker->m_recvThreadExit = TRUE;
                            return 0;
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    
                    break;
                case JVN_RSP_DOWNLOADDATA://下载数据
                {
                    int nFileLen = -1;
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        rsize = 0;
                        rs = 0;
                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < 4)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        memcpy(&nFileLen, pWorker->m_precBuf, 4);
                        
                        /*接收数据*/
                        nLen=nLen-4;
                        rsize = 0;
                        rs = 0;
                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            //						OutputDebug("recv  ========= %d / %d",rs,rsize);
                            rsize += rs;
                        }
                        
                        //计时
                        dBeginTimeDP = CCWorker::JVGetTime();
                        ////////////////////////////////////////////////
                        
                        //确认
                        pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                        
                        nDPRetryCount=0;
                        
                        /*回调函数下载*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen, nFileLen);
                            
                            //						OutputDebug("download %d",nLen);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_PLAYDATA://回放数据
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        /*接收数据*/
                        rsize = 0;
                        rs = 0;
                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        //计时
                        dBeginTimeDP = CCWorker::JVGetTime();
                        ////////////////////////////////////////////////
                        //确认
                        pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                        nDPRetryCount=0;
                        
                        if(pWorker->m_precBuf[0] == JVN_DATA_S)
                        {
                            int nWidth = 0, nHeight = 0, nTotalFrames = 0;
                            memcpy(&nWidth, &pWorker->m_precBuf[5], 4);
                            memcpy(&nHeight, &pWorker->m_precBuf[9], 4);
                            memcpy(&nTotalFrames, &pWorker->m_precBuf[13], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], NULL, 0, nWidth,nHeight,nTotalFrames, uchType,pWorker->m_precBuf, nLen);
                            }
                        }
                        else if(pWorker->m_precBuf[0] == JVN_DATA_O)
                        {
                            if(pWorker->m_pBuffer != NULL)
                            {
                                pWorker->m_pBuffer->ClearBuffer();
                            }
                            
                            int nHeadLen = 0;
                            memcpy(&nHeadLen, &pWorker->m_precBuf[1], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], &pWorker->m_precBuf[5], nHeadLen, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                            }
                        }
                        else
                        {
                            int nFrameLen;
                            memcpy(&nFrameLen, &pWorker->m_precBuf[1], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], &pWorker->m_precBuf[13], nFrameLen, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_DATA_SPEED:
                {
                    if(nLen == 4)
                    {
                        rsize = 0;
                        rs = 0;
                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        while (rsize < nLen)
                        {
#ifndef WIN32
                            if(pWorker->m_bExit || pWorker->m_bEndR)
                            {
                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                CCWorker::jvc_sleep(1);
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
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
                                
                                //结束线程
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                
                                CloseHandle(pWorker->m_hRecvThread);
                                pWorker->m_hRecvThread = 0;
                                return 0;
                            }
#endif
                            
                            if (UDT::ERROR == (rs = UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0)))
                            {
#ifndef WIN32
                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                {
                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                    CCWorker::jvc_sleep(1);
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    pWorker->m_hRecvThread = 0;
                                    pWorker->m_recvThreadExit = TRUE;
                                    return 0;
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
                                    
                                    //结束线程
                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                    {
                                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                    }
                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                    
                                    CloseHandle(pWorker->m_hRecvThread);
                                    pWorker->m_hRecvThread = 0;
                                    return 0;
                                }
#endif
                                
                                //异常关闭
                                pWorker->m_bPass = FALSE;
                                if(pWorker->m_pWorker != NULL)
                                {
                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                }
                                //结束线程
                                
                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                {
                                    pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                                }
                                pWorker->m_pChannel->m_ServerSocket = 0;
                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                else
                                {
                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                }
                                
#ifdef WIN32
                                CloseHandle(pWorker->m_hRecvThread);
#endif
                                
                                pWorker->m_hRecvThread = 0;
                                pWorker->m_recvThreadExit = TRUE;
                                return 0;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                    }
                    if(pWorker->m_pWorker != NULL)
                    {
                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, 4, 0, 0);
                        
                        if(uchType == JVN_DATA_I)
                        {
                            pWorker->m_pChannel->UpdateYSTNOList();
                        }
                        
                    }
                }
                    break;
                default:
                    break;
            }
        }
        //	OutputDebug("thread end %d\n\n\n",pWorker->m_nLocalChannel);
        
        //结束线程
        if(pWorker->m_pChannel->m_ServerSocket > 0)
        {
            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
        }
        pWorker->m_pChannel->m_ServerSocket = 0;
        
#ifdef WIN32
        if(pWorker->m_hEndEventR > 0)
        {
            CloseHandle(pWorker->m_hEndEventR);
            pWorker->m_hEndEventR = 0;
        }
        
        CloseHandle(pWorker->m_hRecvThread);
        pWorker->m_hRecvThread = 0;
        return 0;
#else
        pWorker->m_hRecvThread = 0;
        pWorker->m_recvThreadExit = TRUE;
        return NULL;
#endif
    }
    
#ifndef WIN32
    void* CCOldChannel::RecvMsgProc(void* pParam)
#else
    UINT WINAPI CCOldChannel::RecvMsgProc(LPVOID pParam)
#endif
    {
        CCOldChannel *pWorker = (CCOldChannel *)pParam;
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
        
        int rsize = 0;
        int rs=0;
        
        //计时相关
        DWORD dBeginPassTime;//开始身份验证超时计时
        DWORD dBeginTime=0;//视频数据计时
        DWORD dBeginTimeDP=0;//下载回放计时
        DWORD dEnd=0;
        DWORD dTimePassUsed=0;
        DWORD dTimeUsed=0;
        DWORD dTimeUsedDP=0;
        
        unsigned int unlastIID = 0;
        short int snlastFID = 0;
        
        unsigned int unIID = 0;
        short int snFID = 0;
        unsigned int unFTime = 0;
        
        //计时
        dBeginPassTime = CCWorker::JVGetTime();
        dBeginTime = CCWorker::JVGetTime();
        dBeginTimeDP = CCWorker::JVGetTime();
        
        int nIRetryCount = 0;
        int nDPRetryCount = 0;
        
        int nFrameIndex = 0;
        BOOL bError = FALSE;//数据出错，此时视频帧必须从I帧开始继续接收，防止出现马赛克
        
        //	BOOL bRet = TRUE;
        int nLen=-1;
        BYTE uchType=0;
        int nLenRead=-1;
        BYTE uchTypeRead=0;
        //	OutputDebug("thread start %d\n\n\n",pWorker->m_nLocalChannel);
        
        while(TRUE)
        {
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
            
            if(!pWorker->m_bPass)
            {
                CCWorker::jvc_sleep(2);
                continue;
            }
            
            /*读缓冲区,显示*/
            //		uchTypeRead = 0;
            //		if(pWorker->m_pBuffer->ReadBuffer(uchTypeRead, pWorker->m_preadBuf, nLenRead))
            //		{
            //			pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchTypeRead, pWorker->m_preadBuf, nLenRead, 0,0);
            //		}
            
            uchType = 0;
            rs = 0;
            //		memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
            if (UDT::ERROR == (rs =  UDT::recvmsg(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, JVNC_DATABUFLEN)))
            {
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
                
                if(pWorker->m_pWorker != NULL)
                {
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
            {
                
                ////////////////////////////////////////计时 超过40s未收到任何帧,发送响应帧,超过5分钟仍无数据,不再发送
                dEnd = CCWorker::JVGetTime();
                dTimeUsed = dEnd - dBeginTime;
                dTimeUsedDP = dEnd - dBeginTimeDP;
                dTimePassUsed = dEnd - dBeginPassTime;
                
                if(!pWorker->m_bPass && dTimeUsed > 30000)
                {
#ifdef WIN32
                    if(pWorker->m_hEndEventR > 0)
                    {
                        CloseHandle(pWorker->m_hEndEventR);
                        pWorker->m_hEndEventR = 0;
                    }
#endif
                    
                    //结束线程
                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                    {
                        pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                    }
                    pWorker->m_pChannel->m_ServerSocket = 0;
                    
                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "连接超时.请重试或重新运行程序.";
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收身份验证数据超时,接收线程退出.", __FILE__,__LINE__);
                    }
                    else
                    {
                        char chMsg[] = "connect time out!";
                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive pass info time out.", __FILE__,__LINE__);
                    }
                    pWorker->m_recvThreadExit = TRUE;
#ifdef WIN32
                    CloseHandle(pWorker->m_hRecvThread);
#endif
                    pWorker->m_hRecvThread = 0;
                    return 0;
                }
                
                if(dTimeUsed > 40000 && (dTimeUsed-40000 > 40000*nIRetryCount) && nIRetryCount < 10)
                {
                    //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到帧数据.", __FILE__,__LINE__);
                    pWorker->SendData(JVN_DATA_OK, NULL, 4);//0);//视频帧确认
                    nIRetryCount++;
                }
                if(pWorker->m_bDAndP && dTimeUsedDP > 40000 && (dTimeUsedDP-40000 > 40000*nDPRetryCount) && nDPRetryCount < 2)
                {
                    //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到下载或回放数据.", __FILE__,__LINE__);
                    pWorker->SendData(JVN_DATA_DANDP, NULL, 0);//下载回放确认
                    nDPRetryCount++;
                }
                
                CCWorker::jvc_sleep(5);
                continue;
            }
            
            nLen=-1;
            uchType = pWorker->m_precBuf[0];
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                case JVN_DATA_A://音频
                case JVN_DATA_S://帧尺寸
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头
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
                case JVN_DATA_SPEED://码率
                {
                    memcpy(&nLen, &pWorker->m_precBuf[1], 4);
                    
                    if(nLen < 0 || nLen > JVNC_DATABUFLEN || rs < nLen)
                    {
                        if(nLen < 0)
                        {
                            //						OutputDebugString("!!!!!!!!!!!!!!!!!! nLen<0\n");
                        }
                        if(nLen < JVNC_DATABUFLEN)
                        {
                            //						OutputDebugString("!!!!!!!!!!!!!!!!!! nLen<JVNC_DATABUFLEN\n");
                        }
                        if(rs < nLen)
                        {
                            //						OutputDebugString("!!!!!!!!!!!!!!!!!! rs < nLen\n");
                        }
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错", __FILE__,__LINE__);
                        
                        continue;
                    }
                }
                    break;
                default:
                    //			UDT::recv(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, JVNC_DATABUFLEN, 0);
                    //			pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:收到无效数据.", __FILE__,__LINE__);
                    continue;
            }
            
            switch(uchType)
            {
                case JVN_DATA_B://视频B帧
                case JVN_DATA_P://视频P帧
                case JVN_DATA_I://视频I帧
                case JVN_DATA_SKIP://视频S帧
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        if(pWorker->m_pChannel->m_nFYSTVER < JVN_YSTVER1)
                        {
                            nLen += 4;//多接收4个帧编号字节
                        }
                        
                        if(pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER1)
                        {//类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                            memcpy(&unIID, &pWorker->m_precBuf[5], 4);
                            memcpy(&snFID, &pWorker->m_precBuf[9], 2);
                            memcpy(&unFTime, &pWorker->m_precBuf[11], 4);
                            //nFID = nFID>>16;
                            /*
                             char ch[100]={0};
                             if(uchType == JVN_DATA_I)
                             {
                             sprintf(ch,"[%d]%d.%d len:%d....................................\n",pWorker->m_nLocalChannel,unIID, snFID, nLen);
                             }
                             else
                             {
                             sprintf(ch,"%d.%d.%d.................\n",unIID, snFID, unFTime);
                             }
                             OutputDebugString(ch);
                             */						if(uchType == JVN_DATA_I)
                             {//收到I视频帧，确认
                                 if(nLen + 5 != rs)
                                 {
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     char ch2[200]={0};
                                     sprintf(ch2,"[%d]nlen(%d)+5 != rs(%d)...................................................I\n\n\n",pWorker->m_nLocalChannel,nLen, rs);
                                     //								OutputDebugString(ch2);
                                     break;
                                 }
                                 
                                 unlastIID = unIID;
                                 snlastFID = snFID;
                                 
                                 bError = FALSE;
                                 pWorker->SendData(JVN_DATA_OK, &pWorker->m_precBuf[5], 6);
                             }
                             else
                             {
                                 if(nLen + 5 != rs)
                                 {
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     //								OutputDebugString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                                     char ch2[200]={0};
                                     sprintf(ch2,"[%d][%d.%d][last:%d.%d]nlen(%d)+5 != rs(%d)......................\n",pWorker->m_nLocalChannel,unIID, snFID, unlastIID, snlastFID,nLen, rs);
                                     //								OutputDebugString(ch2);
                                     break;
                                 }
                                 if(unlastIID != unIID)
                                 {
                                     //continue;
                                     break;
                                 }
                                 else if(snFID != 0 && snFID != (snlastFID+1) && (pWorker->m_nProtocolType != TYPE_3GMO_UDP))//3GMO的音频没编号
                                 {
                                     //continue;
                                     break;
                                 }
                                 
                                 snlastFID = snFID;
                                 
                                 pWorker->m_nOCount++;
                                 if(pWorker->m_nOCount%JVNC_ABFRAMERET == 0)
                                 {
                                     pWorker->SendData(JVN_DATA_OK, &pWorker->m_precBuf[5], 6);
                                 }
                             }
                        }
                        else
                        {//类型+长度+nframeindex(4) +数据区
                            /*判断收到的帧编号，I帧确认，一个序列内每10帧一个确认*/
                            memcpy(&nFrameIndex, &pWorker->m_precBuf[5], 4);
                            if(uchType == JVN_DATA_I)
                            {//收到I视频帧，确认
                                snlastFID = 0;
                                
                                bError = FALSE;
                                pWorker->m_nOCount = 1;
                                pWorker->SendData(JVN_DATA_OK, &pWorker->m_precBuf[5], 4);
                                
                                pWorker->m_pChannel->UpdateYSTNOList();
                                
                            }
                            else
                            {
                                
                                pWorker->m_nOCount++;
                                if(pWorker->m_nOCount%JVNC_ABFRAMERET == 0)
                                {
                                    pWorker->SendData(JVN_DATA_OK, &pWorker->m_precBuf[5], 4);
                                }
                            }
                            
                            dTimeUsed = CCWorker::JVGetTime() - dBeginTime;
                        }
                        
                        //计时
                        dBeginTime = CCWorker::JVGetTime();
                        
                        nIRetryCount=0;
                        
                        if(pWorker->m_pWorker != NULL)// && !bError)
                        {
                            if(pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER1)
                            {//新协议，支持帧时间戳，播放控制更优
                                /*
                                 char ch[100]={0};
                                 if(uchType == JVN_DATA_I)
                                 {
                                 sprintf(ch,"[%d]%d.%d.%d....................................\n",pWorker->m_nLocalChannel,unIID, snFID, unFTime);
                                 }
                                 else
                                 {
                                 sprintf(ch,"[%d]%d.%d.%d.................\n",pWorker->m_nLocalChannel,unIID, snFID, unFTime);
                                 }
                                 OutputDebugString(ch);
                                 */
                                //写入缓冲
                                //pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[10], nLen-10, unIID, (int)snFID, unFTime);
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[15], nLen-10, unIID, (int)snFID, unFTime);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[15], nLen-10,0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[15], nLen-10,0,0);
#endif
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                            }
                            else
                            {
                                pWorker->m_precBuf[8] = nFrameIndex & 0xFF;
                                
                                //写入缓冲
                                
                                //pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[3], nLen-3, dTimeUsed);
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[8], nLen-3, dTimeUsed);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[9], nLen-4,0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[9], nLen-4,0,0);
#endif
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_DATA_A://音频
                case JVN_DATA_O://自定义数据
                case JVN_DATA_HEAD://解码头
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        rsize = 0;
                        rs = 0;
                        //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        
                        if(uchType == JVN_DATA_A &&
                           ((pWorker->m_pChannel->m_bTURN && pWorker->m_pChannel->m_bOpenTurnAudio && pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER3)
                            || (pWorker->m_pChannel->m_nFYSTVER >= JVN_YSTVER4)) )
                        {//新版本的音频转发是完整结构;4版本以后的音频全部是新结构
                            if(pWorker->m_pWorker != NULL)
                            {
                                //新协议，支持帧时间戳，播放控制更优，写入缓冲区
                                //类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                                memcpy(&unIID, &pWorker->m_precBuf[5], 4);
                                memcpy(&snFID, &pWorker->m_precBuf[9], 2);
                                memcpy(&unFTime, &pWorker->m_precBuf[11], 4);
                                //nFID = nFID>>16;
                                
                                if(snFID < 0 || snFID > 1000)
                                {//帧编号异常
                                    if(pWorker->m_nProtocolType == TYPE_3GMO_UDP)
                                    {//3GMO类型的音频数据有些是不发帧编号的(独立码流的那些)
                                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[5], nLen, 0,0);
                                        break;
                                    }
                                }
                                
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                if(pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[15], nLen - 10, unIID, (int)snFID, unFTime);
                                }
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[15], nLen-10,0,0);
#endif
#else
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[15], nLen-10,0,0);
#endif
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                                if(snFID == 0 || snFID == (snlastFID+1))
                                {
                                    snlastFID = snFID;
                                }
                                /*
                                 if(pWorker->m_nLocalChannel == 1)
                                 {
                                 char ch[100]={0};
                                 if(uchType == JVN_DATA_A)
                                 {
                                 sprintf(ch,"[%d]%d.%d.%d....................................AA\n",pWorker->m_nLocalChannel,unIID, snFID, unFTime);
                                 }
                                 OutputDebugString(ch);
                                 }
                                 */
                            }
                        }
                        else
                        {
                            if(pWorker->m_pWorker != NULL)
                            {
                                if(uchType == JVN_DATA_HEAD && pWorker->m_pBuffer != NULL)
                                {
                                    pWorker->m_pBuffer->ClearBuffer();
                                }
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[5], nLen, 0,0);
                                if(uchType == JVN_DATA_I)
                                {
                                    pWorker->m_pChannel->UpdateYSTNOList();
                                }
                                
                            }
                            
                            //char ch[100]={0};
                            //if(uchType == JVN_DATA_A)
                            //{
                            //	sprintf(ch,"AAAAAAAA[%d]%d.%d.%d....................................\n",pWorker->m_nLocalChannel,unIID, snFID, unFTime);
                            //}
                            //OutputDebugString(ch);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_FRAMETIME://帧间隔
                case JVN_DATA_S://帧尺寸
                {
                    int nWidth = 0, nHeight = 0;
                    
                    if(nLen == 8)
                    {
                        rsize = 0;
                        rs = 0;
                        
                        memcpy(&nWidth, &pWorker->m_precBuf[5], 4);
                        memcpy(&nHeight, &pWorker->m_precBuf[9], 4);
                        
                        if(uchType == JVN_DATA_S)
                        {
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
                            }
                        }
                        else
                        {
                            if(pWorker->m_pBuffer != NULL)
                            {
                                pWorker->m_pBuffer->m_nFrameTime = nWidth;
                                pWorker->m_pBuffer->m_nFrames = nHeight>0?nHeight:50;
                                
                                pWorker->m_pBuffer->m_nFrameTime = jvs_max(pWorker->m_pBuffer->m_nFrameTime, 0);
                                pWorker->m_pBuffer->m_nFrameTime = jvs_min(pWorker->m_pBuffer->m_nFrameTime, 10000);
                            }
                        }
                        
                        break;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_NOSERVER://无该通道服务
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "无该通道服务!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "channel is not open!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        pWorker->m_recvThreadExit = TRUE;
                        pWorker->m_hRecvThread = 0;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_INVALIDTYPE://连接类型无效
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "连接类型无效!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "connect type invalid!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        pWorker->m_recvThreadExit = TRUE;
                        pWorker->m_hRecvThread = 0;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_OVERLIMIT://超过最大连接数目
                {
                    if(nLen == 0)
                    {
                        pWorker->m_bPass = FALSE;
                        if(pWorker->m_pWorker != NULL)
                        {
                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                            {
                                char chMsg[] = "超过主控最大连接限制!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            else
                            {
                                char chMsg[] = "client count limit!";
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                        }
                        //结束线程
                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                        {
                            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                        }
                        pWorker->m_pChannel->m_ServerSocket = 0;
                        
#ifdef WIN32
                        CloseHandle(pWorker->m_hRecvThread);
#endif
                        pWorker->m_recvThreadExit = TRUE;
                        pWorker->m_hRecvThread = 0;
                        return 0;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_REQ_CHAT://请求语音聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_REQ_TEXT://请求文本聊天
                {
                    if(nLen == 0)
                    {
                        /*调用回调函数，*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_CHATACCEPT://同意语音请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptChat = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_TEXTACCEPT://同意文本请求
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptText = TRUE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_CHATSTOP://停止语音
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptChat = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_CMD_TEXTSTOP://停止文本
                {
                    if(nLen == 0)
                    {
                        /*回调函数通知是否同意语音请求*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                        }
                        pWorker->m_bAcceptText = FALSE;
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_CHECKDATA://检索结果
                case JVN_RSP_CHATDATA://语音数据
                case JVN_RSP_TEXTDATA://文本数据
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        /*接收数据*/
                        rsize = 0;
                        rs = 0;
                        //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                        
                        if(uchType == JVN_RSP_CHATDATA && pWorker->m_bAcceptChat)
                        {
                            /*回调函数语音*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[5], nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_TEXTDATA && pWorker->m_bAcceptText)
                        {
                            /*回调函数文本*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[5], nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKDATA)
                        {
                            /*回调函数检索结果*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,&pWorker->m_precBuf[5], nLen);
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
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
                        pWorker->m_bDAndP = FALSE;
                        dBeginTimeDP = 0;
                        if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
                        {
                            /*回调函数下载*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, NULL, 0, 0);
                            }
                        }
                        else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
                        {
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,uchType, NULL, 0, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                            }
                        }
                        else if(uchType == JVN_RSP_CHECKOVER)
                        {
                            /*回调函数检索结果*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,NULL, 0);
                            }
                        }
                        else if(uchType == JVN_CMD_DISCONN)
                        {
                            pWorker->m_bPass = FALSE;
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                            }
                            
                            //进行断开确认处理
                            pWorker->ProcessDisConnect();
                            
                            //结束线程
                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                            {
                                pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
                            }
                            pWorker->m_pChannel->m_ServerSocket = 0;
                            
#ifdef WIN32
                            CloseHandle(pWorker->m_hRecvThread);
#endif
                            pWorker->m_recvThreadExit = TRUE;
                            pWorker->m_hRecvThread = 0;
                            return 0;
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    
                    break;
                case JVN_RSP_DOWNLOADDATA://下载数据
                {
                    int nFileLen = -1;
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        rsize = 0;
                        rs = 0;
                        
                        memcpy(&nFileLen, &pWorker->m_precBuf[5], 4);
                        
                        /*接收数据*/
                        nLen=nLen-4;
                        rsize = 0;
                        rs = 0;
                        
                        //计时
                        dBeginTimeDP = CCWorker::JVGetTime();
                        ////////////////////////////////////////////////
                        
                        //确认
                        pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                        
                        nDPRetryCount=0;
                        
                        /*回调函数下载*/
                        if(pWorker->m_pWorker != NULL)
                        {
                            pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[9], nLen, nFileLen);
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_RSP_PLAYDATA://回放数据
                {
                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                    {
                        /*接收数据*/
                        rsize = 0;
                        rs = 0;
                        
                        //计时
                        dBeginTimeDP = CCWorker::JVGetTime();
                        ////////////////////////////////////////////////
                        //确认
                        pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                        nDPRetryCount=0;
                        
                        if(pWorker->m_precBuf[5] == JVN_DATA_S)
                        {
                            int nWidth = 0, nHeight = 0, nTotalFrames = 0;
                            memcpy(&nWidth, &pWorker->m_precBuf[10], 4);
                            memcpy(&nHeight, &pWorker->m_precBuf[14], 4);
                            memcpy(&nTotalFrames, &pWorker->m_precBuf[18], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[5], NULL, 0, nWidth,nHeight,nTotalFrames, uchType,&pWorker->m_precBuf[5], nLen);
                            }
                        }
                        else if(pWorker->m_precBuf[5] == JVN_DATA_O)
                        {
                            if(pWorker->m_pBuffer != NULL)
                            {
                                pWorker->m_pBuffer->ClearBuffer();
                            }
                            
                            int nHeadLen = 0;
                            memcpy(&nHeadLen, &pWorker->m_precBuf[6], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[5], &pWorker->m_precBuf[10], nHeadLen, 0,0,0, uchType,&pWorker->m_precBuf[5], nLen);
                            }
                        }
                        else
                        {
                            int nFrameLen;
                            memcpy(&nFrameLen, &pWorker->m_precBuf[6], 4);
                            /*回调函数回放*/
                            if(pWorker->m_pWorker != NULL)
                            {
                                pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[5], &pWorker->m_precBuf[18], nFrameLen, 0,0,0, uchType,&pWorker->m_precBuf[5], nLen);
                            }
                        }
                    }
                    else
                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                        bError = TRUE;
                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                    }
                }
                    break;
                case JVN_DATA_SPEED:
                {
                    if(nLen == 4)
                    {
                        rsize = 0;
                        rs = 0;
                    }
                    if(pWorker->m_pWorker != NULL)
                    {
                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, &pWorker->m_precBuf[5], 4, 0, 0);
                    }
                }
                    break;
                default:
                    break;
            }
        }
        //	OutputDebug("thread end %d\n\n\n",pWorker->m_nLocalChannel);
        
        //结束线程
        if(pWorker->m_pChannel->m_ServerSocket > 0)
        {
            pWorker->m_pWorker->pushtmpsock(pWorker->m_pChannel->m_ServerSocket);
        }
        pWorker->m_pChannel->m_ServerSocket = 0;
        
#ifdef WIN32
        if(pWorker->m_hEndEventR > 0)
        {
            CloseHandle(pWorker->m_hEndEventR);
            pWorker->m_hEndEventR = 0;
        }
        
        CloseHandle(pWorker->m_hRecvThread);
        pWorker->m_hRecvThread = 0;
        return 0;
#else
        pWorker->m_recvThreadExit = TRUE;
        pWorker->m_hRecvThread = 0;
        return NULL;
#endif
    }
    
    BOOL CCOldChannel::SendDataTCP(BYTE uchType, BYTE *pBuffer, int nSize)
    {
#ifdef MOBILE_CLIENT
        signal(SIGPIPE, SIG_IGN);
#endif
        
        if(m_pChannel->m_ServerSocket > 0)
        {
            BYTE data[5 + 2*JVN_BAPACKDEFLEN]={0};
            memset(data, 0, sizeof(data));
            switch(uchType)
            {
                case JVN_REQ_CHECK://请求录像检索
                {
                    if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
                    {
                        //BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
                        data[0] = uchType;
                        memcpy(&data[1], &nSize, 4);
                        memcpy(&data[5], pBuffer, nSize);
                        send(m_pChannel->m_ServerSocket, (char *)data,5 + nSize, 0);
                    }
                }
                    break;
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
                            if ((ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                            {
                                if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                    if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        return FALSE;
                                    }
                                ssize += ss;
                            }
                            m_bDAndP = TRUE;
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
                            send(m_pChannel->m_ServerSocket, (char *)data,9, 0);
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
                case JVN_CMD_KEEPLIVE://心跳数据
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
                            if ((ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen-ssize, MSG_NOSIGNAL)) <= 0)
                            {
                                if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                    if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        else
                                        {
                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                        }
                                        
                                        return FALSE;
                                    }
                                ssize += ss;
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
                            BYTE data[15]={0};
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
                                if ((ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                {
                                    if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                        if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                            }
                                            else
                                            {
                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                            }
                                            
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
                                            if ((ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize,nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                            {
                                                if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                    if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                        }
                                                        else
                                                        {
                                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                        }
                                                        
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
                                                if ((ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize,jvs_min(nLen - ssize, m_nPACKLEN) , MSG_NOSIGNAL)) <= 0)
                                                {
                                                    if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                        if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, m_nPACKLEN), 0)))
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
                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                            }
                                                            else
                                                            {
                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                            }
                                                            
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
                                            if ((ss = send(m_pChannel->m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                            {
                                                if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                    if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                        }
                                                        else
                                                        {
                                                            m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                        }
                                                        
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
                                                if ((ss = send(m_pChannel->m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                {
                                                    if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                        if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                            }
                                                            else
                                                            {
                                                                m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                            }
                                                            
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
                                                        if ((ss = send(m_pChannel->m_ServerSocket, (char *)data+ ssize, nLen-ssize , MSG_NOSIGNAL)) <= 0)
                                                        {
                                                            if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
#else
                                                                if (SOCKET_ERROR == (ss = send(m_pChannel->m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
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
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                                    }
                                                                    else
                                                                    {
                                                                        m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                                                                    }
                                                                    
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
                                    
                                    
#ifndef WIN32
                                    void* CCOldChannel::RecvProcTCP(void* pParam)
#else
                                    UINT WINAPI CCOldChannel::RecvProcTCP(LPVOID pParam)
#endif
                                    {
                                        CCOldChannel *pWorker = (CCOldChannel *)pParam;
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
                                        
                                        int rsize = 0;
                                        int rs=0;
                                        
                                        //计时相关
                                        DWORD dBeginPassTime;//开始身份验证超时计时
                                        DWORD dBeginTime=0;//视频数据计时
                                        DWORD dBeginTimeDP=0;//下载回放计时
                                        DWORD dBeginHeartTime=0;//心跳计时
                                        DWORD dEnd=0;
                                        DWORD dTimePassUsed=0;
                                        DWORD dTimeUsed=0;
                                        DWORD dTimeUsedDP=0;
                                        
                                        BOOL bHaveHeart = FALSE;//是否支持心跳，一般只考虑连接建立后必会收到至少一次心跳包
                                        
                                        unsigned int unIID = 0;
                                        short int snFID = 0;
                                        unsigned int unFTime = 0;
                                        
                                        //计时
                                        dBeginPassTime = CCWorker::JVGetTime();
                                        dBeginTime = dBeginPassTime;
                                        dBeginTimeDP = dBeginPassTime;
                                        dBeginHeartTime = dBeginPassTime;
                                        int nDeadCount = 0;
                                        
                                        int nIRetryCount = 0;
                                        int nDPRetryCount = 0;
                                        
                                        int nFrameIndex = 0;
                                        BOOL bError = FALSE;//数据出错，此时视频帧必须从I帧开始继续接收，防止出现马赛克
                                        
                                        //	BOOL bRet = TRUE;
                                        int nLen=-1;
                                        BYTE uchType=0;
                                        int nLenRead=-1;
                                        BYTE uchTypeRead=0;
                                        while(TRUE)
                                        {
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
                                            
                                            if(!pWorker->m_bPass)
                                            {
                                                CCWorker::jvc_sleep(2);
                                                continue;
                                            }
                                            
                                            uchType = 0;
                                            rs = 0;
                                            //		memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
#ifndef WIN32
                                            if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, 1, MSG_NOSIGNAL)) < 0)
                                            {
                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                {
                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                    CCWorker::jvc_sleep(1);
                                                    
                                                    break;
                                                }
                                                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
#else
                                                    if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)pWorker->m_precBuf, 1, 0)))
                                                    {
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
                                                        int kkk=WSAGetLastError();
                                                        if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
#endif
                                                            
                                                        {
                                                            ////////////////////////////////////////计时 超过40s未收到任何帧,发送响应帧,超过5分钟仍无数据,不再发送
                                                            dEnd = CCWorker::JVGetTime();
                                                            dTimeUsed = dEnd - dBeginTime;
                                                            dTimeUsedDP = dEnd - dBeginTimeDP;
                                                            dTimePassUsed = dEnd - dBeginPassTime;
                                                            
                                                            if(!pWorker->m_bPass && dTimePassUsed > 30000)
                                                            {
#ifdef WIN32
                                                                if(pWorker->m_hEndEventR > 0)
                                                                {
                                                                    CloseHandle(pWorker->m_hEndEventR);
                                                                    pWorker->m_hEndEventR = 0;
                                                                }
#endif
                                                                
                                                                //结束线程
                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                {
                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                }
                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                
                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                {
                                                                    char chMsg[] = "连接超时.请重试或重新运行程序.";
                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收身份验证数据超时,接收线程退出.", __FILE__,__LINE__);
                                                                }
                                                                else
                                                                {
                                                                    char chMsg[] = "connect time out!";
                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive pass info time out.", __FILE__,__LINE__);
                                                                }
                                                                pWorker->m_recvThreadExit = TRUE;
                                                                return 0;
                                                            }
                                                            
                                                            //			#ifndef WIN32
                                                            if(dTimeUsed > 10000 && !pWorker->m_bDAndP)
                                                            {//非回放下载状态，超过10秒没有数据
                                                                if(nDeadCount == 0)
                                                                {
                                                                    dBeginHeartTime = CCWorker::JVGetTime();
                                                                    nDeadCount++;
                                                                }
                                                                if(dEnd > dBeginHeartTime + 1000)
                                                                {//超过10秒没有数据帧，并且超过6秒没有心跳回复 暂认为设备已断开
                                                                    if(nDeadCount > 4)
                                                                    {
                                                                        //结束线程
                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                        {
                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                        }
                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                        
                                                                        if(pWorker->m_pWorker != NULL)
                                                                        {
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
                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                        return 0;
                                                                    }
                                                                    nDeadCount++;
                                                                    pWorker->SendData(JVN_CMD_KEEPLIVE, NULL, 0);
                                                                    dBeginHeartTime = CCWorker::JVGetTime();
                                                                }
                                                            }
                                                            //			#endif
                                                            if(dTimeUsed > 40000 && (dTimeUsed-40000 > 40000*nIRetryCount) && nIRetryCount < 2)
                                                            {
                                                                //pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到帧数据.", __FILE__,__LINE__);
                                                                pWorker->SendData(JVN_DATA_OK, NULL, 0);//视频帧确认
                                                                nIRetryCount++;
                                                            }
                                                            if(pWorker->m_bDAndP && dTimeUsedDP > 40000 && (dTimeUsedDP-40000 > 40000*nDPRetryCount) && nDPRetryCount < 2)
                                                            {
                                                                //pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到下载或回放数据.", __FILE__,__LINE__);
                                                                pWorker->SendData(JVN_DATA_DANDP, NULL, 0);//下载回放确认
                                                                nDPRetryCount++;
                                                            }
                                                            
                                                            CCWorker::jvc_sleep(5);
                                                            continue;
                                                        }
                                                        
                                                        if(pWorker->m_pWorker != NULL)
                                                        {
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
                                                    {
                                                        ////////////////////////////////////////计时 超过40s未收到任何帧,发送响应帧,超过5分钟仍无数据,不再发送
                                                        dEnd = CCWorker::JVGetTime();
                                                        dTimeUsed = dEnd - dBeginTime;
                                                        dTimeUsedDP = dEnd - dBeginTimeDP;
                                                        dTimePassUsed = dEnd - dBeginPassTime;
                                                        
                                                        if(!pWorker->m_bPass && dTimeUsed > 30000)
                                                        {
#ifdef WIN32
                                                            if(pWorker->m_hEndEventR > 0)
                                                            {
                                                                CloseHandle(pWorker->m_hEndEventR);
                                                                pWorker->m_hEndEventR = 0;
                                                            }
#endif
                                                            
                                                            //结束线程
                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                            {
                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                            }
                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                            
                                                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                            {
                                                                char chMsg[] = "连接超时.请重试或重新运行程序.";
                                                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收身份验证数据超时,接收线程退出.", __FILE__,__LINE__);
                                                            }
                                                            else
                                                            {
                                                                char chMsg[] = "connect time out!";
                                                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive pass info time out.", __FILE__,__LINE__);
                                                            }
                                                            pWorker->m_recvThreadExit = TRUE;
                                                            return 0;
                                                        }
                                                        
                                                        //		#ifndef WIN32
                                                        if(dTimeUsed > 10000 && !pWorker->m_bDAndP)
                                                        {//非回放下载状态，超过10秒没有数据
                                                            if(nDeadCount == 0)
                                                            {
                                                                dBeginHeartTime = CCWorker::JVGetTime();
                                                                nDeadCount++;
                                                            }
                                                            
                                                            if(dEnd > dBeginHeartTime + 1000)
                                                            {//超过10秒没有数据帧，并且超过3秒没有心跳回复 暂认为设备已断开
                                                                if(nDeadCount > 4)
                                                                {
                                                                    //结束线程
                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                    {
                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                    }
                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                    
                                                                    if(pWorker->m_pWorker != NULL)
                                                                    {
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
                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                    return 0;
                                                                }
                                                                nDeadCount++;
                                                                pWorker->SendData(JVN_CMD_KEEPLIVE, NULL, 0);
                                                                dBeginHeartTime = CCWorker::JVGetTime();
                                                            }
                                                        }
                                                        //		#endif
                                                        
                                                        if(dTimeUsed > 40000 && (dTimeUsed-40000 > 40000*nIRetryCount) && nIRetryCount < 2)
                                                        {
                                                            //pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到帧数据.", __FILE__,__LINE__);
                                                            pWorker->SendData(JVN_DATA_OK, NULL, 0);//视频帧确认
                                                            nIRetryCount++;
                                                        }
                                                        if(pWorker->m_bDAndP && dTimeUsedDP > 40000 && (dTimeUsedDP-40000 > 40000*nDPRetryCount) && nDPRetryCount < 2)
                                                        {
                                                            //pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:超过40s未收到下载或回放数据.", __FILE__,__LINE__);
                                                            pWorker->SendData(JVN_DATA_DANDP, NULL, 0);//下载回放确认
                                                            nDPRetryCount++;
                                                        }
                                                        
                                                        CCWorker::jvc_sleep(5);
                                                        continue;
                                                    }
                                                
                                                nDeadCount = 0;
                                                dBeginHeartTime = CCWorker::JVGetTime();
                                                
                                                nLen=-1;
                                                uchType = pWorker->m_precBuf[0];
                                                switch(uchType)
                                                {
                                                    case JVN_DATA_B://视频B帧
                                                    case JVN_DATA_P://视频P帧
                                                    case JVN_DATA_I://视频I帧
                                                    case JVN_DATA_SKIP://视频S帧
                                                    case JVN_DATA_A://音频
                                                    case JVN_DATA_S://帧尺寸
                                                    case JVN_DATA_O://自定义数据
                                                    case JVN_DATA_HEAD://解码头
                                                    case JVN_RSP_CHECKPASST://身份验证成功
                                                    case JVN_RSP_CHECKPASSF://身份验证失败
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
                                                    case JVN_CMD_KEEPLIVE://心跳回复
                                                    case JVN_DATA_SPEED://码率
                                                    {
                                                        rsize = 0;
                                                        rs = 0;
                                                        //				memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                        DWORD dSTime = CCWorker::JVGetTime();
                                                        DWORD dETime = 0;
                                                        while (rsize < 4)
                                                        {
#ifndef WIN32
                                                            if(pWorker->m_bExit || pWorker->m_bEndR)
                                                            {
                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                CCWorker::jvc_sleep(1);
                                                                
                                                                //结束线程
                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                {
                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                }
                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                pWorker->m_recvThreadExit = TRUE;
                                                                return 0;
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
                                                                
                                                                //结束线程
                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                {
                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                }
                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                pWorker->m_recvThreadExit = TRUE;
                                                                return 0;
                                                            }
#endif
                                                            
#ifndef WIN32
                                                            if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, MSG_NOSIGNAL)) <= 0)
                                                            {
                                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                {
                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                    CCWorker::jvc_sleep(1);
                                                                    
                                                                    //结束线程
                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                    {
                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                    }
                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                    return 0;
                                                                }
                                                                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || rs == 0)
#else
                                                                    if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, 0))
                                                                        || (0 == rs))
                                                                    {
                                                                        if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                        {
                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                            CCWorker::jvc_sleep(1);
                                                                            
                                                                            if(pWorker->m_hEndEventR > 0)
                                                                            {
                                                                                CloseHandle(pWorker->m_hEndEventR);
                                                                                pWorker->m_hEndEventR = 0;
                                                                            }
                                                                            
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            return 0;
                                                                        }
                                                                        
                                                                        int kkk=WSAGetLastError();
                                                                        if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                        {
                                                                            dETime = CCWorker::JVGetTime();
                                                                            if (dETime < dSTime + 10000)
                                                                            {
                                                                                CCWorker::jvc_sleep(1);
                                                                                continue;
                                                                            }
                                                                        }
                                                                        //异常关闭
                                                                        pWorker->m_bPass = FALSE;
                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                        {
                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                        }
                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                        if(pWorker->m_pWorker != NULL)
                                                                        {
                                                                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                        }
                                                                        //结束线程
                                                                        
                                                                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                        {
                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                        }
                                                                        else
                                                                        {
                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                        }
                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                        return 0;
                                                                    }
                                                                
                                                                rsize += rs;
                                                                dSTime = CCWorker::JVGetTime();
                                                            }
                                                            
                                                            memcpy(&nLen, pWorker->m_precBuf, 4);
                                                            
                                                            if(nLen < 0 || nLen > JVNC_DATABUFLEN)
                                                            {
                                                                bError = TRUE;
                                                                //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错", __FILE__,__LINE__);
                                                                
                                                                continue;
                                                            }
                                                        }
                                                        break;
                                                    default:
                                                        //			pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:收到无效数据.", __FILE__,__LINE__);
                                                        continue;
                                                    }
                                                        
                                                        switch(uchType)
                                                    {
                                                        case JVN_DATA_B://视频B帧
                                                        case JVN_DATA_P://视频P帧
                                                        case JVN_DATA_I://视频I帧
                                                        case JVN_DATA_SKIP://视频S帧
                                                        {
                                                            if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                                                            {
                                                                //	nLen += 4;//多接收4个帧编号字节
                                                                rsize = 0;
                                                                rs = 0;
                                                                //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                DWORD dSTime = CCWorker::JVGetTime();
                                                                DWORD dETime = 0;
                                                                while (rsize < nLen)
                                                                {
#ifndef WIN32
                                                                    if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                    {
                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                        CCWorker::jvc_sleep(1);
                                                                        
                                                                        //结束线程
                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                        {
                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                        }
                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                        return 0;
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
                                                                        
                                                                        //结束线程
                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                        {
                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                        }
                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                        return 0;
                                                                    }
#endif
                                                                    
#ifndef WIN32
                                                                    if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                    {
                                                                        if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                        {
                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                            CCWorker::jvc_sleep(1);
                                                                            
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                            return 0;
                                                                        }
                                                                        if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                            if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                || (0 == rs))
                                                                            {
                                                                                if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                {
                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                    CCWorker::jvc_sleep(1);
                                                                                    
                                                                                    if(pWorker->m_hEndEventR > 0)
                                                                                    {
                                                                                        CloseHandle(pWorker->m_hEndEventR);
                                                                                        pWorker->m_hEndEventR = 0;
                                                                                    }
                                                                                    
                                                                                    //结束线程
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    return 0;
                                                                                }
                                                                                
                                                                                int kkk=WSAGetLastError();
                                                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                {
                                                                                    dETime = CCWorker::JVGetTime();
                                                                                    if (dETime < dSTime + 10000)
                                                                                    {
                                                                                        CCWorker::jvc_sleep(1);
                                                                                        continue;
                                                                                    }
                                                                                }
                                                                                
                                                                                //异常关闭
                                                                                pWorker->m_bPass = FALSE;
                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                {
                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                }
                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                
                                                                                if(pWorker->m_pWorker != NULL)
                                                                                {
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                //结束线程
                                                                                
                                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                {
                                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                }
                                                                                else
                                                                                {
                                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                }
                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                return 0;
                                                                            }
                                                                        
                                                                        rsize += rs;
                                                                        dSTime = CCWorker::JVGetTime();
                                                                    }
                                                                    
                                                                    /*判断收到的帧编号，I帧确认，一个序列内每10帧一个确认*/
                                                                    //类型+长度+IID(4) + FID(2) + FTIME(4)+数据区
                                                                    memcpy(&unIID, pWorker->m_precBuf, 4);
                                                                    memcpy(&snFID, &pWorker->m_precBuf[4], 2);
                                                                    memcpy(&unFTime, &pWorker->m_precBuf[6], 4);
                                                                    if(uchType == JVN_DATA_I)
                                                                    {//收到I视频帧，确认
                                                                        bError = FALSE;
                                                                        pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 6);
                                                                    }
                                                                    else
                                                                    {
                                                                        pWorker->m_nOCount++;
                                                                        if(pWorker->m_nOCount%JVNC_ABFRAMERET == 0)
                                                                        {
                                                                            pWorker->SendData(JVN_DATA_OK, pWorker->m_precBuf, 6);
                                                                        }
                                                                    }
                                                                    
                                                                    //计时
                                                                    dBeginTime = CCWorker::JVGetTime();
                                                                    nIRetryCount=0;
                                                                    
                                                                    if(pWorker->m_pWorker != NULL && !bError)
                                                                    {
                                                                        //新协议，支持帧时间戳，播放控制更优
                                                                        //写入缓冲
#ifndef WIN32
#ifndef MOBILE_CLIENT
                                                                        if(pWorker->m_pBuffer != NULL)
                                                                        {
                                                                            pWorker->m_pBuffer->WriteBuffer(uchType, &pWorker->m_precBuf[10], nLen-10, unIID, (int)snFID, unFTime);
                                                                        }
#else
                                                                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[10], nLen-10,0,0);
#endif
#else
                                                                        pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType,&pWorker->m_precBuf[10], nLen-10,0,0);
#endif
                                                                    }
                                                                }
                                                                else
                                                                {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                    bError = TRUE;
                                                                    //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                }
                                                            }
                                                            break;
                                                        case JVN_DATA_A://音频
                                                        case JVN_DATA_O://自定义数据
                                                        case JVN_DATA_HEAD://解码头
                                                            {
                                                                if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                                                                {
                                                                    rsize = 0;
                                                                    rs = 0;
                                                                    //					memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                    DWORD dSTime = CCWorker::JVGetTime();
                                                                    DWORD dETime = 0;
                                                                    while (rsize < nLen)
                                                                    {
#ifndef WIN32
                                                                        if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                        {
                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                            CCWorker::jvc_sleep(1);
                                                                            
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                            return 0;
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
                                                                            
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            return 0;
                                                                        }
#endif
                                                                        
#ifndef WIN32
                                                                        if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                        {
                                                                            if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                            {
                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                CCWorker::jvc_sleep(1);
                                                                                
                                                                                //结束线程
                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                {
                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                }
                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                return 0;
                                                                            }
                                                                            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                    || (0 == rs))
                                                                                {
                                                                                    if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                    {
                                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                        CCWorker::jvc_sleep(1);
                                                                                        
                                                                                        if(pWorker->m_hEndEventR > 0)
                                                                                        {
                                                                                            CloseHandle(pWorker->m_hEndEventR);
                                                                                            pWorker->m_hEndEventR = 0;
                                                                                        }
                                                                                        
                                                                                        //结束线程
                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                        {
                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                        }
                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                        return 0;
                                                                                    }
                                                                                    
                                                                                    int kkk=WSAGetLastError();
                                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                    {
                                                                                        dETime = CCWorker::JVGetTime();
                                                                                        if (dETime < dSTime + 10000)
                                                                                        {
                                                                                            CCWorker::jvc_sleep(1);
                                                                                            continue;
                                                                                        }
                                                                                    }
                                                                                    
                                                                                    //异常关闭
                                                                                    pWorker->m_bPass = FALSE;
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                    }
                                                                                    //结束线程
                                                                                    
                                                                                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                    {
                                                                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                    }
                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                    return 0;
                                                                                }
                                                                            
                                                                            rsize += rs;
                                                                            dSTime = CCWorker::JVGetTime();
                                                                        }
                                                                        
                                                                        if(pWorker->m_pWorker != NULL)
                                                                        {
                                                                            if(uchType == JVN_DATA_HEAD && pWorker->m_pBuffer != NULL)
                                                                            {
                                                                                pWorker->m_pBuffer->ClearBuffer();
                                                                            }
                                                                            pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen, 0,0);
                                                                        }
                                                                    }
                                                                    else
                                                                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                        bError = TRUE;
                                                                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                    }
                                                                }
                                                                break;
                                                            case JVN_CMD_FRAMETIME://帧间隔
                                                            case JVN_DATA_S://帧尺寸
                                                                {
                                                                    int nWidth = 0, nHeight = 0;
                                                                    
                                                                    if(nLen == 8)
                                                                    {
                                                                        rsize = 0;
                                                                        rs = 0;
                                                                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                        DWORD dSTime = CCWorker::JVGetTime();
                                                                        DWORD dETime = 0;
                                                                        while (rsize < nLen)
                                                                        {
#ifndef WIN32
                                                                            if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                            {
                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                CCWorker::jvc_sleep(1);
                                                                                
                                                                                //结束线程
                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                {
                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                }
                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                return 0;
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
                                                                                
                                                                                //结束线程
                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                {
                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                }
                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                return 0;
                                                                            }
#endif
                                                                            
#ifndef WIN32
                                                                            if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                            {
                                                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                {
                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                    CCWorker::jvc_sleep(1);
                                                                                    
                                                                                    //结束线程
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                    return 0;
                                                                                }
                                                                                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                    if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                        || (0 == rs))
                                                                                    {
                                                                                        if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                        {
                                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                            CCWorker::jvc_sleep(1);
                                                                                            
                                                                                            if(pWorker->m_hEndEventR > 0)
                                                                                            {
                                                                                                CloseHandle(pWorker->m_hEndEventR);
                                                                                                pWorker->m_hEndEventR = 0;
                                                                                            }
                                                                                            
                                                                                            //结束线程
                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                            {
                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                            }
                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                            return 0;
                                                                                        }
                                                                                        
                                                                                        int kkk=WSAGetLastError();
                                                                                        if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                        {
                                                                                            dETime = CCWorker::JVGetTime();
                                                                                            if (dETime < dSTime + 10000)
                                                                                            {
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                continue;
                                                                                            }
                                                                                        }
                                                                                        
                                                                                        //异常关闭
                                                                                        pWorker->m_bPass = FALSE;
                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                        {
                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                        }
                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                        
                                                                                        if(pWorker->m_pWorker != NULL)
                                                                                        {
                                                                                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                        }
                                                                                        //结束线程
                                                                                        
                                                                                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                        {
                                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                        }
                                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                                        return 0;
                                                                                    }
                                                                                
                                                                                rsize += rs;
                                                                                dSTime = CCWorker::JVGetTime();
                                                                            }
                                                                            memcpy(&nWidth, &pWorker->m_precBuf[0], 4);
                                                                            memcpy(&nHeight, &pWorker->m_precBuf[4], 4);
                                                                            
                                                                            if(uchType == JVN_DATA_S)
                                                                            {
                                                                                if(pWorker->m_pWorker != NULL)
                                                                                {
                                                                                    pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
                                                                                }
                                                                            }
                                                                            else
                                                                            {
                                                                                if(pWorker->m_pBuffer != NULL)
                                                                                {
                                                                                    pWorker->m_pBuffer->m_nFrameTime = nWidth;
                                                                                    pWorker->m_pBuffer->m_nFrames = nHeight>0?nHeight:50;
                                                                                    
                                                                                    pWorker->m_pBuffer->m_nFrameTime = jvs_max(pWorker->m_pBuffer->m_nFrameTime, 0);
                                                                                    pWorker->m_pBuffer->m_nFrameTime = jvs_min(pWorker->m_pBuffer->m_nFrameTime, 10000);
                                                                                }
                                                                            }
                                                                            
                                                                            break;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_NOSERVER://无该通道服务
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            pWorker->m_bPass = FALSE;
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                {
                                                                                    char chMsg[] = "无该通道服务!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                else
                                                                                {
                                                                                    char chMsg[] = "channel is not open!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                
                                                                            }
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                            return 0;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_INVALIDTYPE://连接类型无效
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            pWorker->m_bPass = FALSE;
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                {
                                                                                    char chMsg[] = "连接类型无效!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                else
                                                                                {
                                                                                    char chMsg[] = "connect type invalid!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                
                                                                            }
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                            return 0;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_OVERLIMIT://超过最大连接数目
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            pWorker->m_bPass = FALSE;
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                {
                                                                                    char chMsg[] = "超过主控最大连接数目限制!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                else
                                                                                {
                                                                                    char chMsg[] = "client count limit!";
                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                }
                                                                                
                                                                            }
                                                                            //结束线程
                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                            {
                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                            }
                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                            return 0;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_CMD_KEEPLIVE://心跳回复
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            bHaveHeart = TRUE;
                                                                            dBeginHeartTime = CCWorker::JVGetTime();
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_REQ_CHAT://请求语音聊天
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*调用回调函数，*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                                                                            }
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_REQ_TEXT://请求文本聊天
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*调用回调函数，*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel, uchType, NULL, 0);
                                                                            }
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_CHATACCEPT://同意语音请求
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*回调函数通知是否同意语音请求*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                                                                            }
                                                                            pWorker->m_bAcceptChat = TRUE;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_TEXTACCEPT://同意文本请求
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*回调函数通知是否同意语音请求*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                                                                            }
                                                                            pWorker->m_bAcceptText = TRUE;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_CMD_CHATSTOP://停止语音
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*回调函数通知是否同意语音请求*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                                                                            }
                                                                            pWorker->m_bAcceptChat = FALSE;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_CMD_TEXTSTOP://停止文本
                                                                    {
                                                                        if(nLen == 0)
                                                                        {
                                                                            /*回调函数通知是否同意语音请求*/
                                                                            if(pWorker->m_pWorker != NULL)
                                                                            {
                                                                                pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, NULL, 0);
                                                                            }
                                                                            pWorker->m_bAcceptText = FALSE;
                                                                        }
                                                                        else
                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                            bError = TRUE;
                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                        }
                                                                    }
                                                                    break;
                                                                case JVN_RSP_CHECKDATA://检索结果
                                                                case JVN_RSP_CHATDATA://语音数据
                                                                case JVN_RSP_TEXTDATA://文本数据
                                                                    {
                                                                        if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                                                                        {
                                                                            /*接收数据*/
                                                                            rsize = 0;
                                                                            rs = 0;
                                                                            memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                            DWORD dSTime = CCWorker::JVGetTime();
                                                                            DWORD dETime = 0;
                                                                            while (rsize < nLen)
                                                                            {
#ifndef WIN32
                                                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                {
                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                    CCWorker::jvc_sleep(1);
                                                                                    
                                                                                    //结束线程
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                    return 0;
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
                                                                                    
                                                                                    //结束线程
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    return 0;
                                                                                }
#endif
                                                                                
#ifndef WIN32
                                                                                if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                                {
                                                                                    if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                    {
                                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                        CCWorker::jvc_sleep(1);
                                                                                        
                                                                                        //结束线程
                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                        {
                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                        }
                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                                        return 0;
                                                                                    }
                                                                                    if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                        if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                            || (0 == rs))
                                                                                        {
                                                                                            if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                            {
                                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                
                                                                                                if(pWorker->m_hEndEventR > 0)
                                                                                                {
                                                                                                    CloseHandle(pWorker->m_hEndEventR);
                                                                                                    pWorker->m_hEndEventR = 0;
                                                                                                }
                                                                                                
                                                                                                //结束线程
                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                {
                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                }
                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                return 0;
                                                                                            }
                                                                                            
                                                                                            int kkk=WSAGetLastError();
                                                                                            if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                            {
                                                                                                dETime = CCWorker::JVGetTime();
                                                                                                if (dETime < dSTime + 10000)
                                                                                                {
                                                                                                    CCWorker::jvc_sleep(1);
                                                                                                    continue;
                                                                                                }
                                                                                            }
                                                                                            
                                                                                            //异常关闭
                                                                                            pWorker->m_bPass = FALSE;
                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                            {
                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                            }
                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                            
                                                                                            if(pWorker->m_pWorker != NULL)
                                                                                            {
                                                                                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                            }
                                                                                            //结束线程
                                                                                            
                                                                                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                            {
                                                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                            }
                                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                                            return 0;
                                                                                        }
                                                                                    
                                                                                    rsize += rs;
                                                                                    dSTime = CCWorker::JVGetTime();
                                                                                }
                                                                                
                                                                                if(uchType == JVN_RSP_CHATDATA && pWorker->m_bAcceptChat)
                                                                                {
                                                                                    /*回调函数语音*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->ChatData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen);
                                                                                    }
                                                                                }
                                                                                else if(uchType == JVN_RSP_TEXTDATA && pWorker->m_bAcceptText)
                                                                                {
                                                                                    /*回调函数文本*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->TextData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen);
                                                                                    }
                                                                                }
                                                                                else if(uchType == JVN_RSP_CHECKDATA)
                                                                                {
                                                                                    /*回调函数检索结果*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,pWorker->m_precBuf, nLen);
                                                                                    }
                                                                                }
                                                                            }
                                                                            else
                                                                            {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                                bError = TRUE;
                                                                                //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
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
                                                                                pWorker->m_bDAndP = FALSE;
                                                                                dBeginTimeDP = 0;
                                                                                if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
                                                                                {
                                                                                    /*回调函数下载*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, NULL, 0, 0);
                                                                                    }
                                                                                }
                                                                                else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
                                                                                {
                                                                                    /*回调函数回放*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,uchType, NULL, 0, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                                                                                    }
                                                                                }
                                                                                else if(uchType == JVN_RSP_CHECKOVER)
                                                                                {
                                                                                    /*回调函数检索结果*/
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->CheckResult(pWorker->m_nLocalChannel,NULL, 0);
                                                                                    }
                                                                                }
                                                                                else if(uchType == JVN_CMD_DISCONN)
                                                                                {
                                                                                    pWorker->m_bPass = FALSE;
                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                    {
                                                                                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                    }
                                                                                    
                                                                                    //进行断开确认处理
                                                                                    pWorker->ProcessDisConnect();
                                                                                    
                                                                                    //结束线程
                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                    {
                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                    }
                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                    return 0;
                                                                                }
                                                                            }
                                                                            else
                                                                            {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                                bError = TRUE;
                                                                                //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                            }
                                                                        }
                                                                        
                                                                        break;
                                                                    case JVN_RSP_DOWNLOADDATA://下载数据
                                                                        {
                                                                            int nFileLen = -1;
                                                                            if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                                                                            {
                                                                                rsize = 0;
                                                                                rs = 0;
                                                                                memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                                DWORD dSTime = CCWorker::JVGetTime();
                                                                                DWORD dETime = 0;
                                                                                while (rsize < 4)
                                                                                {
#ifndef WIN32
                                                                                    if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                    {
                                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                        CCWorker::jvc_sleep(1);
                                                                                        
                                                                                        //结束线程
                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                        {
                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                        }
                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                                        return 0;
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
                                                                                        
                                                                                        //结束线程
                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                        {
                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                        }
                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                        return 0;
                                                                                    }
#endif
                                                                                    
#ifndef WIN32
                                                                                    if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, MSG_NOSIGNAL)) <= 0)
                                                                                    {
                                                                                        if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                        {
                                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                            CCWorker::jvc_sleep(1);
                                                                                            
                                                                                            //结束线程
                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                            {
                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                            }
                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                                            return 0;
                                                                                        }
                                                                                        if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                            if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], 4 - rsize, 0))
                                                                                                || (0 == rs))
                                                                                            {
                                                                                                if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                                {
                                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                    CCWorker::jvc_sleep(1);
                                                                                                    
                                                                                                    if(pWorker->m_hEndEventR > 0)
                                                                                                    {
                                                                                                        CloseHandle(pWorker->m_hEndEventR);
                                                                                                        pWorker->m_hEndEventR = 0;
                                                                                                    }
                                                                                                    
                                                                                                    //结束线程
                                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                    {
                                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                    }
                                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                    return 0;
                                                                                                }
                                                                                                
                                                                                                int kkk=WSAGetLastError();
                                                                                                if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                                {
                                                                                                    dETime = CCWorker::JVGetTime();
                                                                                                    if (dETime < dSTime + 10000)
                                                                                                    {
                                                                                                        CCWorker::jvc_sleep(1);
                                                                                                        continue;
                                                                                                    }
                                                                                                }
                                                                                                
                                                                                                //异常关闭
                                                                                                pWorker->m_bPass = FALSE;
                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                {
                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                }
                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                
                                                                                                if(pWorker->m_pWorker != NULL)
                                                                                                {
                                                                                                    pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                                }
                                                                                                //结束线程
                                                                                                
                                                                                                if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                                {
                                                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                                }
                                                                                                else
                                                                                                {
                                                                                                    pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                                }
                                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                                return 0;
                                                                                            }
                                                                                        
                                                                                        rsize += rs;
                                                                                        dSTime = CCWorker::JVGetTime();
                                                                                    }
                                                                                    memcpy(&nFileLen, pWorker->m_precBuf, 4);
                                                                                    
                                                                                    /*接收数据*/
                                                                                    nLen=nLen-4;
                                                                                    rsize = 0;
                                                                                    rs = 0;
                                                                                    memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                                    dSTime = CCWorker::JVGetTime();
                                                                                    dETime = 0;
                                                                                    while (rsize < nLen)
                                                                                    {
#ifndef WIN32
                                                                                        if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                        {
                                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                            CCWorker::jvc_sleep(1);
                                                                                            
                                                                                            //结束线程
                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                            {
                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                            }
                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                                            return 0;
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
                                                                                            
                                                                                            //结束线程
                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                            {
                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                            }
                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                            return 0;
                                                                                        }
#endif
                                                                                        
#ifndef WIN32
                                                                                        if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                                        {
                                                                                            if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                            {
                                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                
                                                                                                //结束线程
                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                {
                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                }
                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                                return 0;
                                                                                            }
                                                                                            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                                if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                                    || 0 == rs)
                                                                                                {
                                                                                                    if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                                    {
                                                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                        CCWorker::jvc_sleep(1);
                                                                                                        
                                                                                                        if(pWorker->m_hEndEventR > 0)
                                                                                                        {
                                                                                                            CloseHandle(pWorker->m_hEndEventR);
                                                                                                            pWorker->m_hEndEventR = 0;
                                                                                                        }
                                                                                                        
                                                                                                        //结束线程
                                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                        {
                                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                        }
                                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                        return 0;
                                                                                                    }
                                                                                                    
                                                                                                    int kkk=WSAGetLastError();
                                                                                                    if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                                    {
                                                                                                        dETime = CCWorker::JVGetTime();
                                                                                                        if (dETime < dSTime + 10000)
                                                                                                        {
                                                                                                            CCWorker::jvc_sleep(1);
                                                                                                            continue;
                                                                                                        }
                                                                                                    }
                                                                                                    
                                                                                                    //异常关闭
                                                                                                    pWorker->m_bPass = FALSE;
                                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                    {
                                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                    }
                                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                    
                                                                                                    if(pWorker->m_pWorker != NULL)
                                                                                                    {
                                                                                                        pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                                    }
                                                                                                    //结束线程
                                                                                                    
                                                                                                    if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                                    {
                                                                                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                                    }
                                                                                                    else
                                                                                                    {
                                                                                                        pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                                    }
                                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                                    return 0;
                                                                                                }
                                                                                            
                                                                                            rsize += rs;
                                                                                            dSTime = CCWorker::JVGetTime();
                                                                                        }
                                                                                        
                                                                                        //计时
                                                                                        dBeginTimeDP = CCWorker::JVGetTime();
                                                                                        ////////////////////////////////////////////////
                                                                                        
                                                                                        //确认
                                                                                        pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                                                                                        
                                                                                        nDPRetryCount=0;
                                                                                        
                                                                                        /*回调函数下载*/
                                                                                        if(pWorker->m_pWorker != NULL)
                                                                                        {
                                                                                            pWorker->m_pWorker->DownLoad(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, nLen, nFileLen);
                                                                                        }
                                                                                    }
                                                                                    else
                                                                                    {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                                        bError = TRUE;
                                                                                        //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                                    }
                                                                                }
                                                                                break;
                                                                            case JVN_RSP_PLAYDATA://回放数据
                                                                                {
                                                                                    if(nLen > 0 && nLen < JVNC_DATABUFLEN)
                                                                                    {
                                                                                        /*接收数据*/
                                                                                        rsize = 0;
                                                                                        rs = 0;
                                                                                        memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                                        DWORD dSTime = CCWorker::JVGetTime();
                                                                                        DWORD dETime = 0;
                                                                                        while (rsize < nLen)
                                                                                        {
#ifndef WIN32
                                                                                            if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                            {
                                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                
                                                                                                //结束线程
                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                {
                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                }
                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                                return 0;
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
                                                                                                
                                                                                                //结束线程
                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                {
                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                }
                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                return 0;
                                                                                            }
#endif
                                                                                            
#ifndef WIN32
                                                                                            if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                                            {
                                                                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                                {
                                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                    CCWorker::jvc_sleep(1);
                                                                                                    
                                                                                                    //结束线程
                                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                    {
                                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                    }
                                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                                    return 0;
                                                                                                }
                                                                                                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                                    if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                                        || (0 == rs))
                                                                                                    {
                                                                                                        if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                                        {
                                                                                                            pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                            CCWorker::jvc_sleep(1);
                                                                                                            
                                                                                                            if(pWorker->m_hEndEventR > 0)
                                                                                                            {
                                                                                                                CloseHandle(pWorker->m_hEndEventR);
                                                                                                                pWorker->m_hEndEventR = 0;
                                                                                                            }
                                                                                                            
                                                                                                            //结束线程
                                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                            {
                                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                            }
                                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                            return 0;
                                                                                                        }
                                                                                                        
                                                                                                        int kkk=WSAGetLastError();
                                                                                                        if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                                        {
                                                                                                            dETime = CCWorker::JVGetTime();
                                                                                                            if (dETime < dSTime + 10000)
                                                                                                            {
                                                                                                                CCWorker::jvc_sleep(1);
                                                                                                                continue;
                                                                                                            }
                                                                                                        }
                                                                                                        
                                                                                                        //异常关闭
                                                                                                        pWorker->m_bPass = FALSE;
                                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                        {
                                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                        }
                                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                        
                                                                                                        if(pWorker->m_pWorker != NULL)
                                                                                                        {
                                                                                                            pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                                        }
                                                                                                        //结束线程
                                                                                                        
                                                                                                        if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                                        {
                                                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                                        }
                                                                                                        else
                                                                                                        {
                                                                                                            pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__);
                                                                                                        }
                                                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                                                        return 0;
                                                                                                    }
                                                                                                
                                                                                                rsize += rs;
                                                                                                dSTime = CCWorker::JVGetTime();
                                                                                            }
                                                                                            
                                                                                            //计时
                                                                                            dBeginTimeDP = CCWorker::JVGetTime();
                                                                                            ////////////////////////////////////////////////
                                                                                            //确认
                                                                                            pWorker->SendData(JVN_DATA_DANDP, NULL, 0);
                                                                                            nDPRetryCount=0;
                                                                                            
                                                                                            if(pWorker->m_precBuf[0] == JVN_DATA_S)
                                                                                            {
                                                                                                int nWidth = 0, nHeight = 0, nTotalFrames = 0;
                                                                                                memcpy(&nWidth, &pWorker->m_precBuf[5], 4);
                                                                                                memcpy(&nHeight, &pWorker->m_precBuf[9], 4);
                                                                                                memcpy(&nTotalFrames, &pWorker->m_precBuf[13], 4);
                                                                                                /*回调函数回放*/
                                                                                                if(pWorker->m_pWorker != NULL)
                                                                                                {
                                                                                                    pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], NULL, 0, nWidth,nHeight,nTotalFrames, uchType,pWorker->m_precBuf, nLen);
                                                                                                }
                                                                                            }
                                                                                            else if(pWorker->m_precBuf[0] == JVN_DATA_O)
                                                                                            {
                                                                                                if(pWorker->m_pBuffer != NULL)
                                                                                                {
                                                                                                    pWorker->m_pBuffer->ClearBuffer();
                                                                                                }
                                                                                                
                                                                                                int nHeadLen = 0;
                                                                                                memcpy(&nHeadLen, &pWorker->m_precBuf[1], 4);
                                                                                                /*回调函数回放*/
                                                                                                if(pWorker->m_pWorker != NULL)
                                                                                                {
                                                                                                    pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], &pWorker->m_precBuf[5], nHeadLen, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                                                                                                }
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                int nFrameLen;
                                                                                                memcpy(&nFrameLen, &pWorker->m_precBuf[1], 4);
                                                                                                /*回调函数回放*/
                                                                                                if(pWorker->m_pWorker != NULL)
                                                                                                {
                                                                                                    pWorker->m_pWorker->PlayData(pWorker->m_nLocalChannel,pWorker->m_precBuf[0], &pWorker->m_precBuf[13], nFrameLen, 0,0,0, uchType,pWorker->m_precBuf, nLen);
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                        else
                                                                                        {//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
                                                                                            bError = TRUE;
                                                                                            //					pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "测试:数据出错.", __FILE__,__LINE__);
                                                                                        }
                                                                                    }
                                                                                    break;
                                                                                case JVN_DATA_SPEED:
                                                                                    {
                                                                                        if(nLen == 4)
                                                                                        {
                                                                                            rsize = 0;
                                                                                            rs = 0;
                                                                                            memset(pWorker->m_precBuf, 0, JVNC_DATABUFLEN);
                                                                                            DWORD dSTime = CCWorker::JVGetTime();
                                                                                            DWORD dETime = 0;
                                                                                            while (rsize < nLen)
                                                                                            {
#ifndef WIN32
                                                                                                if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                                {
                                                                                                    pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                    CCWorker::jvc_sleep(1);
                                                                                                    
                                                                                                    //结束线程
                                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                    {
                                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                    }
                                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                    pWorker->m_recvThreadExit = TRUE;
                                                                                                    return 0;
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
                                                                                                    
                                                                                                    //结束线程
                                                                                                    if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                    {
                                                                                                        closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                    }
                                                                                                    pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                    return 0;
                                                                                                }
#endif
                                                                                                
#ifndef WIN32
                                                                                                if ((rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, MSG_NOSIGNAL)) <= 0)
                                                                                                {
                                                                                                    if(pWorker->m_bExit || pWorker->m_bEndR)
                                                                                                    {
                                                                                                        pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                        CCWorker::jvc_sleep(1);
                                                                                                        
                                                                                                        //结束线程
                                                                                                        if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                        {
                                                                                                            closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                        }
                                                                                                        pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                        pWorker->m_recvThreadExit = TRUE;
                                                                                                        return 0;
                                                                                                    }
                                                                                                    if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || 0 == rs)
#else
                                                                                                        if (SOCKET_ERROR == (rs = recv(pWorker->m_pChannel->m_ServerSocket, (char *)&pWorker->m_precBuf[rsize], nLen - rsize, 0))
                                                                                                            || (0 == rs))
                                                                                                        {
                                                                                                            if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventR, 0))
                                                                                                            {
                                                                                                                pWorker->SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                                                CCWorker::jvc_sleep(1);
                                                                                                                
                                                                                                                if(pWorker->m_hEndEventR > 0)
                                                                                                                {
                                                                                                                    CloseHandle(pWorker->m_hEndEventR);
                                                                                                                    pWorker->m_hEndEventR = 0;
                                                                                                                }
                                                                                                                
                                                                                                                //结束线程
                                                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                                {
                                                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                                }
                                                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                                return 0;
                                                                                                            }
                                                                                                            
                                                                                                            int kkk=WSAGetLastError();
                                                                                                            if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || 0 == rs)
#endif
                                                                                                            {
                                                                                                                dETime = CCWorker::JVGetTime();
                                                                                                                if (dETime < dSTime + 10000)
                                                                                                                {
                                                                                                                    CCWorker::jvc_sleep(1);
                                                                                                                    continue;
                                                                                                                }
                                                                                                            }
                                                                                                            
                                                                                                            //异常关闭
                                                                                                            pWorker->m_bPass = FALSE;
                                                                                                            if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                                            {
                                                                                                                closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                                            }
                                                                                                            pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                                            
                                                                                                            if(pWorker->m_pWorker != NULL)
                                                                                                            {
                                                                                                                pWorker->m_pWorker->ConnectChange(pWorker->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
                                                                                                            }
                                                                                                            //结束线程
                                                                                                            
                                                                                                            if(JVN_LANGUAGE_CHINESE == pWorker->m_pWorker->m_nLanguage)
                                                                                                            {
                                                                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "接收数据失败,接收线程退出.", __FILE__,__LINE__);
                                                                                                            }
                                                                                                            else
                                                                                                            {
                                                                                                                pWorker->m_pWorker->m_Log.SetRunInfo(pWorker->m_nLocalChannel, "receive thread over,receive failed.", __FILE__,__LINE__);
                                                                                                            }
                                                                                                            pWorker->m_recvThreadExit = TRUE;
                                                                                                            return 0;
                                                                                                        }
                                                                                                    
                                                                                                    rsize += rs;
                                                                                                    dSTime = CCWorker::JVGetTime();
                                                                                                }
                                                                                            }
                                                                                            
                                                                                            if(pWorker->m_pWorker != NULL)
                                                                                            {
                                                                                                pWorker->m_pWorker->NormalData(pWorker->m_nLocalChannel,uchType, pWorker->m_precBuf, 4, 0,0);
                                                                                            }
                                                                                        }
                                                                                        break;
                                                                                    default:
                                                                                        break;
                                                                                    }
                                                                                }
                                                                                
                                                                                //结束线程
                                                                                if(pWorker->m_pChannel->m_ServerSocket > 0)
                                                                                {
                                                                                    closesocket(pWorker->m_pChannel->m_ServerSocket);
                                                                                }
                                                                                pWorker->m_pChannel->m_ServerSocket = 0;
                                                                                
#ifdef WIN32
                                                                                if(pWorker->m_hEndEventR > 0)
                                                                                {
                                                                                    CloseHandle(pWorker->m_hEndEventR);
                                                                                    pWorker->m_hEndEventR = 0;
                                                                                }
                                                                                return 0;
#else
                                                                                pWorker->m_recvThreadExit = TRUE;
                                                                                return NULL;
#endif
                                                                            }
                                                                            
                                                                            void CCOldChannel::ProcessDisConnect()
                                                                            {
                                                                                BYTE buff[5]={0};
                                                                                buff[0] = JVN_CMD_DISCONN;
                                                                                int ss=0,ssize=0;
                                                                                DWORD d_last = CCWorker::JVGetTime();
                                                                                DWORD d_cur  = d_last;
                                                                                if(m_nProtocolType==TYPE_PC_UDP || m_nProtocolType == TYPE_MO_UDP || m_nProtocolType == TYPE_3GMO_UDP || m_nProtocolType == TYPE_3GMOHOME_UDP)
                                                                                {
                                                                                    while(ssize<5)
                                                                                    {
                                                                                        if((ss=UDT::send(m_pChannel->m_ServerSocket,(char *)buff,5-ssize,0)) < 0)
                                                                                        {
                                                                                            break;
                                                                                        }
                                                                                        else if(ss == 0)
                                                                                        {
                                                                                            CCWorker::jvc_sleep(1);
                                                                                            d_cur  = CCWorker::JVGetTime();
                                                                                            if (d_cur > d_last + 10000)
                                                                                            {
                                                                                                break;
                                                                                            }
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
                                                                                        if ((ss = send(m_pChannel->m_ServerSocket, (char *)buff, 5-ssize, MSG_NOSIGNAL)) <= 0)
                                                                                        {
                                                                                            if(ss <= 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                                                                                            {
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                d_cur  = CCWorker::JVGetTime();
                                                                                                if (d_cur > d_last + 10000)
                                                                                                {
                                                                                                    break;
                                                                                                }
                                                                                                continue;
                                                                                            }
                                                                                            break;
                                                                                        }
#else
                                                                                        if((ss=send(m_pChannel->m_ServerSocket,(char *)buff,5-ssize,0)) == SOCKET_ERROR)
                                                                                        {
                                                                                            int kkk = WSAGetLastError();
                                                                                            if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
                                                                                            {
                                                                                                CCWorker::jvc_sleep(1);
                                                                                                d_cur  = CCWorker::JVGetTime();
                                                                                                if (d_cur > d_last + 10000)
                                                                                                {
                                                                                                    break;
                                                                                                }
                                                                                                continue;
                                                                                            }
                                                                                            break;
                                                                                        }
#endif
                                                                                        
                                                                                        ssize+=ss;
                                                                                    }
                                                                                }
                                                                            }
                                                                            
                                                                            void CCOldChannel::SetNoChannel()
                                                                            {
                                                                                
                                                                                m_pChannel->m_bShowInfo = TRUE;
                                                                                
                                                                                m_nStatus = FAILD;
                                                                                m_pChannel->m_nStatus = WAIT_IP_CONNECTOLD_F;//FAILD;
                                                                                m_pChannel->m_bPassWordErr = FALSE;
                                                                                
                                                                                SendData(JVN_CMD_DISCONN, NULL, 0);
                                                                                
                                                                                m_bPass = FALSE;
                                                                                
                                                                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                                                                {
                                                                                    char chMsg[] = "无该通道服务!";
                                                                                    {
                                                                                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                                                                                    }
                                                                                }
                                                                                else
                                                                                {
                                                                                    char chMsg[] = "channel is not open!";
                                                                                    {
                                                                                        m_pWorker->ConnectChange(m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0 ,__FILE__,__LINE__,__FUNCTION__);
                                                                                    }
                                                                                }
                                                                                
                                                                            }
                                                                            
