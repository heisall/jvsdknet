// CLanTool.cpp: implementation of the CCLanSerch class.
//
//////////////////////////////////////////////////////////////////////

#include "CLanTool.h"
#include "CWorker.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCLanTool::CCLanTool()
{
}

CCLanTool::CCLanTool(int nLPort, int nDesPort, CCWorker *pWorker)
{
	m_bOK = FALSE;
	m_pWorker = pWorker;
	m_nLANTPort = 0;
	m_unLANTID = 1;
	m_nTimeOut = 0;
	m_dwBeginTool = 0;
	m_bTimeOut = 0;
	m_bLANTooling = FALSE;

	m_bNewTool = FALSE;
	m_bStopToolImd = TRUE;

	m_hLANToolRcvThread = 0;
	m_hLANToolSndThread = 0;


#ifndef WIN32
	pthread_mutex_init(&m_ct, NULL);

	m_bRcvEnd = FALSE;
	m_bSndEnd = FALSE;
#else
	InitializeCriticalSection(&m_ct); //初始化临界区

	m_hLANToolRcvStartEvent = 0;
	m_hLANToolRcvEndEvent = 0;

	m_hLANToolSndStartEvent = 0;
	m_hLANToolSndEndEvent = 0;
#endif

	int err;
	//套接字
	m_SocketLANT = socket(AF_INET, SOCK_DGRAM,0);
	SOCKADDR_IN addrSrv;
#ifndef WIN32
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(nLPort);
	//绑定套接字
	err = bind(m_SocketLANT, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
	if(err != 0)
	{
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			m_pWorker->m_Log.SetRunInfo(0,"初始化LANToolSOCK失败.原因:绑定端口失败", __FILE__,__LINE__);
		}
		else
		{
			m_pWorker->m_Log.SetRunInfo(0,"init LANTool sock faild.Info:bind port faild.", __FILE__,__LINE__);
		}
		closesocket(m_SocketLANT);

		return;
	}

	// 有效SO_BROADCAST选项
    int nBroadcast = 1;
    ::setsockopt(m_SocketLANT, SOL_SOCKET, SO_BROADCAST, (char*)&nBroadcast, sizeof(int));

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
	if (0 != pthread_create(&m_hLANToolRcvThread, pAttr, LANTRcvProc, this))
	{
		m_hLANToolRcvThread = 0;
		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			m_pWorker->m_Log.SetRunInfo(0,"开启LANTool服务失败.原因:创建线程失败",__FILE__,__LINE__);
		}
		else
		{
			m_pWorker->m_Log.SetRunInfo(0,"start LANTool server failed.Info:create thread faild.",__FILE__,__LINE__);
		}
		return;
	}
#else
	//创建本地命令监听线程
	UINT unTheadID;
	if(m_hLANToolRcvStartEvent > 0)
	{
		CloseHandle(m_hLANToolRcvStartEvent);
		m_hLANToolRcvStartEvent = 0;
	}
	if(m_hLANToolRcvEndEvent > 0)
	{
		CloseHandle(m_hLANToolRcvEndEvent);
		m_hLANToolRcvEndEvent = 0;
	}
	//创建本地监听线程
	m_hLANToolRcvStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hLANToolRcvEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hLANToolRcvThread = (HANDLE)_beginthreadex(NULL, 0, LANTRcvProc, (void *)this, 0, &unTheadID);
	if(m_hLANToolRcvStartEvent > 0)
	{
		SetEvent(m_hLANToolRcvStartEvent);
	}
	if (m_hLANToolRcvThread == 0)//创建线程失败
	{
		if(m_hLANToolRcvStartEvent > 0)
		{
			CloseHandle(m_hLANToolRcvStartEvent);
			m_hLANToolRcvStartEvent=0;
		}
		if(m_hLANToolRcvEndEvent > 0)
		{
			CloseHandle(m_hLANToolRcvEndEvent);
			m_hLANToolRcvEndEvent=0;
		}

		if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
		{
			m_pWorker->m_Log.SetRunInfo(0,"开启LANTool服务失败.原因:创建线程失败",__FILE__,__LINE__);
		}
		else
		{
			m_pWorker->m_Log.SetRunInfo(0,"start LANTool server failed.Info:create thread faild.",__FILE__,__LINE__);
		}
		return;
	}
#endif

	m_bOK = TRUE;
	m_nLANTPort = nLPort;
	m_nDesPort = nDesPort;
	GetAdapterInfo();
	return;
}

CCLanTool::~CCLanTool()
{
#ifndef WIN32
	if (0 != m_hLANToolRcvThread)
	{
		m_bRcvEnd = TRUE;
		pthread_join(m_hLANToolRcvThread, NULL);
		m_hLANToolRcvThread = 0;
		CCWorker::jvc_sleep(5);
	}
	if (0 != m_hLANToolSndThread)
	{
		m_bSndEnd = TRUE;
		pthread_join(m_hLANToolSndThread, NULL);
		m_hLANToolSndThread = 0;
		CCWorker::jvc_sleep(5);
	}
#else
	//结束本地线程
	if(m_hLANToolRcvEndEvent > 0)
	{
		SetEvent(m_hLANToolRcvEndEvent);
		CCWorker::jvc_sleep(5);
	}
	if(m_hLANToolSndEndEvent > 0)
	{
		SetEvent(m_hLANToolSndEndEvent);
		CCWorker::jvc_sleep(5);
	}
	CCChannel::WaitThreadExit(m_hLANToolRcvThread);
	CCChannel::WaitThreadExit(m_hLANToolSndThread);
	if(m_hLANToolRcvThread > 0)
	{
		CloseHandle(m_hLANToolRcvThread);
		m_hLANToolRcvThread = 0;
	}
	if(m_hLANToolSndThread > 0)
	{
		CloseHandle(m_hLANToolSndThread);
		m_hLANToolSndThread = 0;
	}
#endif

	m_hLANToolRcvThread = 0;
	closesocket(m_SocketLANT);
	m_SocketLANT = 0;
	m_bOK = FALSE;

#ifndef WIN32
	pthread_mutex_destroy(&m_ct);
#else
	DeleteCriticalSection(&m_ct); //释放临界区
#endif
}

void CCLanTool::GetAdapterInfo()
{
	m_IpList.clear();

	_Adapter info;
	memset(&info,0,sizeof(_Adapter));
	//

	NATList LocalIPList;
	m_pWorker->GetLocalIP(&LocalIPList);

	if(LocalIPList.size() > 0)
	{
		sprintf(info.IP,"%d.%d.%d.%d",LocalIPList[0].ip[0],LocalIPList[0].ip[1],LocalIPList[0].ip[2],LocalIPList[0].ip[3]);
		unsigned int n[4] = {0};
		sscanf(info.IP,"%d.%d.%d.%d",&n[0],&n[1],&n[2],&n[3]);
		sprintf(info.IpHead,"%d.%d.",n[0],n[1]);
		info.nIP3 = n[2];
		m_IpList.push_back(info);

	}

	return;
}

BOOL CCLanTool::LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut)
{
	if(m_hLANToolRcvThread <= 0 || m_SocketLANT <= 0)
	{
		return FALSE;
	}

	if(m_hLANToolSndThread <= 0)
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
		//创建本地线程
		if (0 != pthread_create(&m_hLANToolSndThread, pAttr, LANTSndProc, this))
		{
			m_hLANToolSndThread = 0;
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(0,"开启LANToolSnd失败.原因:创建线程失败",__FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(0,"start LANToolSnd failed.Info:create thread faild.",__FILE__,__LINE__);
			}
			return FALSE;
		}
	#else
		//创建本地线程
		UINT unTheadID;
		if(m_hLANToolSndStartEvent > 0)
		{
			CloseHandle(m_hLANToolSndStartEvent);
			m_hLANToolSndStartEvent = 0;
		}
		if(m_hLANToolSndEndEvent > 0)
		{
			CloseHandle(m_hLANToolSndEndEvent);
			m_hLANToolSndEndEvent = 0;
		}
		//创建本地线程
		m_hLANToolSndStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hLANToolSndEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hLANToolSndThread = (HANDLE)_beginthreadex(NULL, 0, LANTSndProc, (void *)this, 0, &unTheadID);
		if(m_hLANToolSndStartEvent > 0)
		{
			SetEvent(m_hLANToolSndStartEvent);
		}
		if (m_hLANToolSndThread == 0)//创建线程失败
		{
			if(m_hLANToolSndStartEvent > 0)
			{
				CloseHandle(m_hLANToolSndStartEvent);
				m_hLANToolSndStartEvent=0;
			}
			if(m_hLANToolSndEndEvent > 0)
			{
				CloseHandle(m_hLANToolSndEndEvent);
				m_hLANToolSndEndEvent=0;
			}

			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(0,"开启LANToolSnd失败.原因:创建线程失败",__FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(0,"start LANToolSnd failed.Info:create thread faild.",__FILE__,__LINE__);
			}
			return FALSE;
		}
	#endif
	}

	m_bNewTool = FALSE;//置为没有新搜索
	m_bStopToolImd = TRUE;//停止当前正在进行的搜索操作

	if(nTimeOut >= 0)
	{
		m_nTimeOut = nTimeOut;
	}

	m_unLANTID++;

	BYTE data[700]={0};
	STTOOLPACK sthead;
	sthead.nPNLen = strlen(chPName);
	sthead.nPWLen = strlen(chPWord);
	sthead.uchCType = 1;

#ifndef WIN32
	time_t now = time(0);
	tm *tnow = localtime(&now);
	sprintf(sthead.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec);
#else
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sprintf(sthead.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d ",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
#endif

	//类型(1)+长度(4)+STTOOLPACK+用户名(?)+密码(?)+配置内容(?)
	data[0] = JVN_REQ_TOOL;
	int nLen = sizeof(STTOOLPACK) + sthead.nPNLen + sthead.nPWLen;
	memcpy(&data[1], &nLen, 4);
	memcpy(&data[5], &sthead, sizeof(STTOOLPACK));
	memcpy(&data[5+sizeof(STTOOLPACK)], chPName, sthead.nPNLen);
	memcpy(&data[5+sizeof(STTOOLPACK)+sthead.nPNLen], chPWord, sthead.nPWLen);

#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif

	m_nNeedSend = nLen + 5;
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

	int ntmp = CCChannel::sendtoclient(m_SocketLANT,(char *)data, m_nNeedSend, 0,(SOCKADDR *)&addrBcast, sizeof(SOCKADDR),1);
	m_bTimeOut = FALSE;
	m_bLANTooling = TRUE;
	m_dwBeginTool = CCWorker::JVGetTime();
	if(ntmp != m_nNeedSend)
	{
		return FALSE;
	}

	m_bNewTool = TRUE;

	return TRUE;
}

#ifndef WIN32
	void* CCLanTool::LANTSndProc(void* pParam)
#else
	UINT CCLanTool::LANTSndProc(LPVOID pParam)
#endif
{
	CCLanTool *pWorker = (CCLanTool *)pParam;
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

	WaitForSingleObject(pWorker->m_hLANToolSndStartEvent, INFINITE);
	if(pWorker->m_hLANToolSndStartEvent > 0)
	{
		CloseHandle(pWorker->m_hLANToolSndStartEvent);
		pWorker->m_hLANToolSndStartEvent = 0;
	}
#endif

	DWORD dwend = 0;

	while (TRUE)
	{
	#ifndef WIN32
		if(pWorker->m_bSndEnd)
		{
			break;
		}
	#else
		if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hLANToolSndEndEvent, 0))
		{
			break;
		}
	#endif

		if(pWorker->m_bNewTool)
		{//有新的搜索
			pWorker->m_bStopToolImd = FALSE;
			pWorker->m_bNewTool = FALSE;

			char ip[30] = {0};
			SOCKADDR_IN addRemote;
			addRemote.sin_family = AF_INET;
			addRemote.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
			addRemote.sin_port = htons(pWorker->m_nDesPort);

			BOOL bstop=FALSE;
			int ncount = pWorker->m_IpList.size();
		//	ncount = (ncount>1?1:ncount);
			for(int k = 0; k < ncount;k ++)
			{
				//只检测第一个网卡，网关为空的情况暂时不考虑了
				char ip_head[30] = {0};
				strcpy(ip_head,pWorker->m_IpList[k].IpHead);

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
							if(pWorker->m_bStopToolImd)
							{
								i=256;
								j=256;
								bstop = TRUE;
								break;
							}

							sprintf(ip,"%s%d.%d",ip_head,j,i);
							addRemote.sin_addr.s_addr = ::inet_addr(ip);

							CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

							if(pWorker->m_nTimeOut > 0)
							{
								dwend = CCWorker::JVGetTime();
								if(dwend > pWorker->m_dwBeginTool + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginTool)
								{//超时
									i=256;
									j=256;
									bstop = TRUE;
									break;
								}
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
					for(j=pWorker->m_IpList[k].nIP3+1; j<=n; j++)
					{
						for(int i = 1;i < 254;i ++)
						{
							if(pWorker->m_bStopToolImd)
							{
								i=256;
								j=256;
								bstop = TRUE;
								break;
							}

							sprintf(ip,"%s%d.%d",ip_head,j,i);
							addRemote.sin_addr.s_addr = ::inet_addr(ip);

							CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

							if(pWorker->m_nTimeOut > 0)
							{
								dwend = CCWorker::JVGetTime();
								if(dwend > pWorker->m_dwBeginTool + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginTool)
								{//超时
									i=256;
									j=256;
									bstop = TRUE;
									break;
								}
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
							if(pWorker->m_bStopToolImd)
							{
								i=256;
								j=256;
								bstop = TRUE;
								break;
							}

							sprintf(ip,"%s%d.%d",ip_head,j,i);
							addRemote.sin_addr.s_addr = ::inet_addr(ip);

							CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

							if(pWorker->m_nTimeOut > 0)
							{
								dwend = CCWorker::JVGetTime();
								if(dwend > pWorker->m_dwBeginTool + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginTool)
								{//超时
									i=256;
									j=256;
									bstop = TRUE;
									break;
								}
							}
						}
					}
					if(bstop)
					{
						break;
					}
					//向后面剩余段发送
					n = pWorker->m_IpList[k].nIP3 + 11;
					n = ((n<=255)?n:255);
					for(j=n; j<256; j++)
					{
						for(int i = 1;i < 254;i ++)
						{
							if(pWorker->m_bStopToolImd)
							{
								i=256;
								j=256;
								bstop = TRUE;
								break;
							}

							sprintf(ip,"%s%d.%d",ip_head,j,i);
							addRemote.sin_addr.s_addr = ::inet_addr(ip);

							CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)pWorker->m_uchData,pWorker->m_nNeedSend,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

							if(pWorker->m_nTimeOut > 0)
							{
								dwend = CCWorker::JVGetTime();
								if(dwend > pWorker->m_dwBeginTool + pWorker->m_nTimeOut || dwend < pWorker->m_dwBeginTool)
								{//超时
									i=256;
									j=256;
									bstop = TRUE;
									break;
								}
							}
						}
					}
					if(bstop)
					{
						break;
					}
				}
			}
		}
		else
		{
			CCWorker::jvc_sleep(10);
		}
	}

	return 0;
}

#ifndef WIN32
	void* CCLanTool::LANTRcvProc(void* pParam)
#else
	UINT CCLanTool::LANTRcvProc(LPVOID pParam)
#endif
{
	CCLanTool *pWorker = (CCLanTool *)pParam;
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

	WaitForSingleObject(pWorker->m_hLANToolRcvStartEvent, INFINITE);
	if(pWorker->m_hLANToolRcvStartEvent > 0)
	{
		CloseHandle(pWorker->m_hLANToolRcvStartEvent);
		pWorker->m_hLANToolRcvStartEvent = 0;
	}
#endif

	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(SOCKADDR_IN);

	BYTE recBuf[RC_DATA_SIZE]={0};
	BYTE *puchSendBuf = new BYTE[JVN_BAPACKDEFLEN];

	int nRecvLen = 0;
	int nType = 0;
	int nRLen = 0;
	DWORD dwEnd = 0;

	while (TRUE)
	{
	#ifndef WIN32
		if(pWorker->m_bRcvEnd)
		{
			break;
		}
	#else
		if(WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hLANToolRcvEndEvent, 0))
		{
			break;
		}
	#endif

		nRecvLen = 0;
		nRLen = 0;
		nType = 0;
		memset(recBuf, 0, RC_DATA_SIZE);
		nRecvLen = CCChannel::receivefrom(pWorker->m_SocketLANT,(char *)&recBuf, RC_DATA_SIZE, 0, (SOCKADDR*)&clientaddr,&addrlen,1);

		//接收：类型(1)+长度(4)+数据类型(1)+产品类型(4)+编组号(4)+云视通号码(4)+序列号(4)+生产日期(4)+GUID(?)
		if( nRecvLen > 0)
		{
			if(recBuf[0] == JVN_RSP_TOOL)
			{
				memcpy(&nRLen, &recBuf[1], 4);//长度(4)

				STLANTOOLINFO stinfo;
				stinfo.uchType = recBuf[5];
				memcpy(&stinfo.nCardType, &recBuf[6], 4);//搜索ID(4)
				memcpy(stinfo.chGroup, &recBuf[10], 4);
				memcpy(&stinfo.nYSTNUM, &recBuf[14], 4);
				memcpy(&stinfo.nSerial, &recBuf[18], 4);
				memcpy(&stinfo.nDate, &recBuf[22], 4);
				memcpy(&stinfo.guid, &recBuf[26], sizeof(GUID));
				sprintf(stinfo.chIP,"%s",inet_ntoa(clientaddr.sin_addr));
				stinfo.nPort = ntohs(clientaddr.sin_port);

				if(pWorker->m_nTimeOut <= 0)
				{//不需判断超时
					int nret = pWorker->m_pWorker->m_pfLANTData(&stinfo);
					if(nret == 1)
					{//进行配置 需向设备发送
					#ifndef WIN32
						time_t now = time(0);
						tm *tnow = localtime(&now);
						sprintf(stinfo.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec);
					#else
						SYSTEMTIME sys;
						GetLocalTime(&sys);
						sprintf(stinfo.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
					#endif

						//类型(1)+长度(4)+STTOOLPACK+用户名(?)+密码(?)+配置内容(?)
						puchSendBuf[0] = JVN_REQ_TOOL;
						if(stinfo.nYSTNUM > 0 && stinfo.pchData !=NULL && stinfo.nDLen >= 0 && stinfo.nDLen < JVN_BAPACKDEFLEN - sizeof(STLANTOOLINFO) - 1000)
						{//带配置
							STTOOLPACK sthead;
							sthead.uchCType = 2;
							sthead.nDLen = stinfo.nDLen;
							sthead.nPNLen = strlen(stinfo.chPName);
							sthead.nPWLen = strlen(stinfo.chPWord);
							memcpy(sthead.chGroup, stinfo.chGroup, 4);
							sthead.nYSTNUM = stinfo.nYSTNUM;
							memcpy(sthead.chCurTime, stinfo.chCurTime, sizeof(sthead.chCurTime));

							puchSendBuf[0] = JVN_REQ_TOOL;
							int nlen = sizeof(STTOOLPACK) + sthead.nPNLen + sthead.nPWLen + sthead.nDLen;
							memcpy(&puchSendBuf[1], &nlen, 4);
							memcpy(&puchSendBuf[5], &sthead, sizeof(STTOOLPACK));
							memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)], stinfo.chPName, sthead.nPNLen);
							memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen], stinfo.chPWord, sthead.nPWLen);
							memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen+sthead.nPWLen], stinfo.pchData, stinfo.nDLen);

							int nl = CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)puchSendBuf,nlen+5,0,(SOCKADDR *)&clientaddr, sizeof(SOCKADDR),100);
							int ll = nl;
						}
						else if(stinfo.nYSTNUM > 0 && stinfo.nDLen == 0)
						{//无配置
							STTOOLPACK sthead;
							sthead.uchCType = 2;
							sthead.nDLen = stinfo.nDLen;
							sthead.nPNLen = strlen(stinfo.chPName);
							sthead.nPWLen = strlen(stinfo.chPWord);
							memcpy(sthead.chGroup, stinfo.chGroup, 4);
							sthead.nYSTNUM = stinfo.nYSTNUM;
							memcpy(sthead.chCurTime, stinfo.chCurTime, sizeof(sthead.chCurTime));

							puchSendBuf[0] = JVN_REQ_TOOL;
							int nlen = sizeof(STTOOLPACK) + sthead.nPNLen + sthead.nPWLen;
							memcpy(&puchSendBuf[1], &nlen, 4);
							memcpy(&puchSendBuf[5], &sthead, sizeof(STTOOLPACK));
							memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)], stinfo.chPName, sthead.nPNLen);
							memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen], stinfo.chPWord, sthead.nPWLen);

							int nl = CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)puchSendBuf,nlen+5,0,(SOCKADDR *)&clientaddr, sizeof(SOCKADDR),100);
							int ll = nl;
						}
					}
				}
				else
				{
					dwEnd = CCWorker::JVGetTime();
					if(dwEnd <= pWorker->m_dwBeginTool + pWorker->m_nTimeOut && dwEnd >= pWorker->m_dwBeginTool)
					{//正常结果
						int nret = pWorker->m_pWorker->m_pfLANTData(&stinfo);
						if(nret == 1)
						{//进行配置 需向设备发送
						#ifndef WIN32
							time_t now = time(0);
							tm *tnow = localtime(&now);
							sprintf(stinfo.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec);
						#else
							SYSTEMTIME sys;
							GetLocalTime(&sys);
							sprintf(stinfo.chCurTime,"%4d-%02d-%02d %02d:%02d:%02d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
						#endif

							//类型(1)+长度(4)+STTOOLPACK+用户名(?)+密码(?)+配置内容(?)
							puchSendBuf[0] = JVN_REQ_TOOL;
							if(stinfo.nYSTNUM > 0 && stinfo.pchData !=NULL && stinfo.nDLen >= 0 && stinfo.nDLen < JVN_BAPACKDEFLEN - sizeof(STLANTOOLINFO) - 1000)
							{//带配置
								STTOOLPACK sthead;
								sthead.uchCType = 2;
								sthead.nDLen = stinfo.nDLen;
								sthead.nPNLen = strlen(stinfo.chPName);
								sthead.nPWLen = strlen(stinfo.chPWord);
								memcpy(sthead.chGroup, stinfo.chGroup, 4);
								sthead.nYSTNUM = stinfo.nYSTNUM;
								memcpy(sthead.chCurTime, stinfo.chCurTime, sizeof(sthead.chCurTime));

								puchSendBuf[0] = JVN_REQ_TOOL;
								int nlen = sizeof(STTOOLPACK) + sthead.nPNLen + sthead.nPWLen + sthead.nDLen;
								memcpy(&puchSendBuf[1], &nlen, 4);
								memcpy(&puchSendBuf[5], &sthead, sizeof(STTOOLPACK));
								memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)], stinfo.chPName, sthead.nPNLen);
								memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen], stinfo.chPWord, sthead.nPWLen);
								memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen+sthead.nPWLen], stinfo.pchData, stinfo.nDLen);

								int nl = CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)puchSendBuf,nlen+5,0,(SOCKADDR *)&clientaddr, sizeof(SOCKADDR),100);
								int ll = nl;
							}
							else if(stinfo.nYSTNUM > 0 && stinfo.nDLen == 0)
							{//无配置
								STTOOLPACK sthead;
								sthead.uchCType = 2;
								sthead.nDLen = stinfo.nDLen;
								sthead.nPNLen = strlen(stinfo.chPName);
								sthead.nPWLen = strlen(stinfo.chPWord);
								memcpy(sthead.chGroup, stinfo.chGroup, 4);
								sthead.nYSTNUM = stinfo.nYSTNUM;
								memcpy(sthead.chCurTime, stinfo.chCurTime, sizeof(sthead.chCurTime));

								puchSendBuf[0] = JVN_REQ_TOOL;
								int nlen = sizeof(STTOOLPACK) + sthead.nPNLen + sthead.nPWLen;
								memcpy(&puchSendBuf[1], &nlen, 4);
								memcpy(&puchSendBuf[5], &sthead, sizeof(STTOOLPACK));
								memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)], stinfo.chPName, sthead.nPNLen);
								memcpy(&puchSendBuf[5+sizeof(STTOOLPACK)+sthead.nPNLen], stinfo.chPWord, sthead.nPWLen);

								int nl = CCChannel::sendtoclientm(pWorker->m_SocketLANT,(char *)puchSendBuf,nlen+5,0,(SOCKADDR *)&clientaddr, sizeof(SOCKADDR),100);
								int ll = nl;
							}
						}
					}
					else
					{//超时结果直接舍弃,判断是否提示超时
						if(!pWorker->m_bTimeOut)
						{
							//pWorker->m_pWorker->m_pfBCData(unSerchID, &recBuf[9], nRLen-4, stLSResult.chClientIP, FALSE);
							//pWorker->m_pWorker->m_pfBCData(unSerchID, NULL, 0, "", TRUE);
						}
						pWorker->m_bTimeOut = TRUE;
						pWorker->m_bLANTooling = FALSE;
					}
				}
			}
		}
		else
		{
			if(pWorker->m_bLANTooling && pWorker->m_nTimeOut > 0 && !pWorker->m_bTimeOut)
			{//正在执行搜索,有计时要求，还没提示过
				dwEnd = CCWorker::JVGetTime();
				if(dwEnd >= pWorker->m_dwBeginTool + pWorker->m_nTimeOut || dwEnd < pWorker->m_dwBeginTool)
				{//超时结果直接舍弃,判断是否提示超时
				//	pWorker->m_pWorker->m_pfBCData(pWorker->m_unLANTID, NULL, 0, "", TRUE);
					pWorker->m_bTimeOut = TRUE;
					pWorker->m_bLANTooling = FALSE;
				}
			}

			CCWorker::jvc_sleep(10);
		}

		continue;//是自定义广播只执行到此，不执行设备搜索代码
	}

	if(puchSendBuf != NULL)
	{
		delete[] puchSendBuf;
	}
	return 0;
}








