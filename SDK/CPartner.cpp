// CPartner.cpp: implementation of the CCPartner class.
//
//////////////////////////////////////////////////////////////////////

#include "CPartner.h"
#include "CChannel.h"
#include "CWorker.h"
#include "CPartnerCtrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int JVC_MSS;

CCPartner::CCPartner()
{
}

CCPartner::CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel)
{
	m_bTCP = FALSE;
	
	m_bTURNC = ptli.bIsTC;
	m_bCache = ptli.bIsCache;
	m_bProxy2 = FALSE;
	m_bAccept = FALSE;
	m_bSuperProxy = ptli.bIsSuper;
	m_bLan2A = ptli.bIsLan2A;
	m_bLan2B = ptli.bIsLan2B;
	m_ndesLinkID = ptli.nLinkID;
	m_unADDR = ntohl(ptli.sAddr.sin_addr.s_addr);
	memcpy(&m_addr, &ptli.sAddr, sizeof(SOCKADDR_IN));

	m_bTryNewLink = FALSE;

	m_nConnFCount = 0;

	m_socketTCtmp = 0;
	m_socket = 0;
	m_socketTCP = 0;
	m_dwstart = 0;	
	m_nstatus = PNODE_STATUS_NEW;
		
	m_pWorker = pWorker;
	m_pChannel = pChannel;

	m_PushQueue.clear();
	m_PushQueue.reserve(500);

	m_puchSendBuf = new BYTE[JVN_PTBUFSIZE];
	m_puchRecvBuf = new BYTE[JVN_PTBUFSIZE];
	ResetPack();

	//------伙伴网络性能-----------
	m_nTimeOutCount = 0;
	m_dwLastInfoTime = 0;
	m_nLastDownLoadTotalB = 0;
	m_nLastDownLoadTotalKB = 0;
	m_nLastDownLoadTotalMB = 0;
	m_dwRUseTTime = 0;
	m_nLPSAvage = 0;
	m_nDownLoadTotalB = 0;
	m_nDownLoadTotalKB = 0;
	m_nDownLoadTotalMB = 0;
	m_nUpTotalB = 0;
	m_nUpTotalKB = 0;
	m_nUpTotalMB = 0;

	m_dwLastConnTime = 0;
	m_fLastConnTotal = 0;

	m_dwLastDataTime = 0;
	//------伙伴最新BM-------------
	m_stMAP.ChunkBTMap.clear();
	m_stMAP.unBeginChunkID = 0;
	m_stMAP.nChunkCount = 0;
}

CCPartner::CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel,SOCKADDR_IN addr, UDTSOCKET socket, BOOL bTCP)
{
//OutputDebugString("new accept....\n");
	m_bTCP = bTCP;

	m_bTURNC = ptli.bIsTC;
	m_bCache = ptli.bIsCache;
	m_bProxy2 = FALSE;
	m_bAccept = TRUE;
	m_bSuperProxy = ptli.bIsSuper;
	m_bLan2A = ptli.bIsLan2A;
	m_bLan2B = ptli.bIsLan2B;
	m_ndesLinkID = ptli.nLinkID;
	m_unADDR = ntohl(ptli.sAddr.sin_addr.s_addr);
	memcpy(&m_addr, &ptli.sAddr, sizeof(SOCKADDR_IN));

	m_nConnFCount = 0;

	memcpy(&m_addr, &addr, sizeof(SOCKADDR_IN));
	m_dwstart = CCWorker::JVGetTime();

	m_socket = 0;
	m_socketTCP = 0;
	if(bTCP)
	{
		m_socketTCP = socket;
	}
	else
	{
		m_socket = socket;
	}

	m_socketTCtmp = 0;
	
	m_nstatus = PNODE_STATUS_ACCEPT;
	
	m_pWorker = pWorker;
	m_pChannel = pChannel;

	m_PushQueue.clear();
	m_PushQueue.reserve(500);

	m_puchSendBuf = new BYTE[JVN_PTBUFSIZE];
	m_puchRecvBuf = new BYTE[JVN_PTBUFSIZE];

	ResetPack(TRUE);
	
	//------伙伴网络性能-----------
	m_nTimeOutCount = 0;
	m_dwLastInfoTime = 0;
	m_nLastDownLoadTotalB = 0;
	m_nLastDownLoadTotalKB = 0;
	m_nLastDownLoadTotalMB = 0;
	m_dwRUseTTime = 0;
	m_nLPSAvage = 0;
	m_nDownLoadTotalB = 0;
	m_nDownLoadTotalKB = 0;
	m_nDownLoadTotalMB = 0;
	m_nUpTotalB = 0;
	m_nUpTotalKB = 0;
	m_nUpTotalMB = 0;

	m_dwLastConnTime = 0;
	m_fLastConnTotal = 0;

	m_dwLastDataTime = 0;
	//------伙伴最新BM-------------
	m_stMAP.ChunkBTMap.clear();
	m_stMAP.unBeginChunkID = 0;
	m_stMAP.nChunkCount = 0;
}

CCPartner::~CCPartner()
{
	if(m_socketTCP > 0)
	{
		closesocket(m_socketTCP);
	}
	m_socketTCP = 0;

	if(m_socket > 0)
	{
		UDT::close(m_socket);
	}
	m_socket = 0;

	if(m_puchSendBuf != NULL)
	{
		delete[] m_puchSendBuf;
		m_puchSendBuf = NULL;
	}

	if(m_puchRecvBuf != NULL)
	{
		delete[] m_puchRecvBuf;
		m_puchRecvBuf = NULL;
	}
}

//进行伙伴连接
int CCPartner::PartnerLink(CCPartnerCtrl *pPCtrl)
{
	int nSize = 0;
//	DWORD dwendtime = 0;

	switch(m_nstatus)
	{
	case PNODE_STATUS_FAILD:
	case PNODE_STATUS_NEW:
		{
			if(!m_bTURNC)
			{
				m_nstatus = PNODE_STATUS_CONNECTING;
				goto CONNECT;
			}
			else
			{
				m_nstatus = PNODE_STATUS_TCCONNECTING;
				goto TCCONNECT;
			}
				
			return 0;//正在连接
		}
		break;
	case PNODE_STATUS_CONNECTING:
		{//已取得伙伴地址 判断连接类型
CONNECT:
			if(m_socketTCP > 0)
			{
				closesocket(m_socketTCP);
			}
			m_socketTCP = 0;
//OutputDebugString("close 3\n");			
			if(m_socket > 0)
			{
				UDT::close(m_socket);
			}
			m_socket = 0;
			
//OutputDebugString("rspaddr...................try TCP\n");

            //首先尝试TCP连接
//////////////////////////////////////////////////////////////////////////
//char ch[1000];
//sprintf(ch,"connected B->...........................connect TCP %s:%d [localchannel:%d]\n",inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port), m_pChannel->m_nLocalChannel);
//OutputDebugString(ch);		
            m_bTCP = TRUE;
			m_socketTCP = socket(AF_INET, SOCK_STREAM,0);
			
			int iSockStatus = 0;
			//将套接字置为非阻塞模式
		#ifndef WIN32
			int flags=0;
			if ((flags = fcntl(m_socketTCP, F_GETFL, 0)) < 0) 
			{ 
				//;
			} 
			
			if (fcntl(m_socketTCP, F_SETFL, flags | O_NONBLOCK) >= 0)
		#else
			unsigned long ulBlock = 1; 
			iSockStatus = ioctlsocket(m_socketTCP, FIONBIO, (unsigned long*)&ulBlock);   
			if (SOCKET_ERROR != iSockStatus)
		#endif 
			{   
				int nSetSize = 128*1024;
				setsockopt(m_socketTCP, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
				nSetSize = 128*1024;
				setsockopt(m_socketTCP, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));

				BOOL bReuse = TRUE;
				setsockopt(m_socketTCP, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));

				// 不使用Nagle算法
				//BOOL bNoDelay = TRUE;
				//setsockopt(m_socketTCP, SOL_SOCKET, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));

				//将套接字置为不等待未处理完的数据
				LINGER linger;
				linger.l_onoff = 1;//0;
				linger.l_linger = 0;
				iSockStatus = setsockopt(m_socketTCP, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));   

				if(0 == CCPartnerCtrl::connectnb(m_socketTCP,(SOCKADDR *)&m_addr, sizeof(SOCKADDR), 1))
				{	
//OutputDebugString("connect tcp ok(connectnb)..............\n");
					//初步连接成功 发送 类型 + 长度(4)+ 当前连接ID(4) 信息  + PTLinkID+号码+ip
					BYTE data[MAX_PATH]={0};
					int ntmp = 16;
					data[0] = JVN_CMD_TCP;
					memcpy(&data[1], &ntmp, 4);
					memcpy(&data[5], &m_pChannel->m_nLinkID, 4);
					memcpy(&data[9], &m_ndesLinkID, 4);
					memcpy(&data[13], &m_pChannel->m_stConnInfo.nYSTNO, 4);
					memcpy(&data[17], &m_pChannel->m_addressA.sin_addr.s_addr, 4);
					
					ntmp = 21;
					int ntimeout=0;
					if(CCPartnerCtrl::tcpsend(m_socketTCP, (char *)data, ntmp, 1, ntimeout) == ntmp)
					{//TCP连接成功，设置超时，发送预验证消息  类型(1) + 长度(4) + 本地linkid(4)
						m_nstatus = PNODE_STATUS_WAITLCHECK;
						m_chdata[0] = JVN_REQ_RECHECK;
						nSize = 4;
						memcpy(&m_chdata[1], &nSize, 4);
						memcpy(&m_chdata[5], &m_ndesLinkID, 4);
						Send2Partner(m_chdata, 9, NULL, 200);
						
						m_dwstart = CCWorker::JVGetTime();
						
						//"向伙伴 内网探测成功 等待验证";
//char ch[1000];
//sprintf(ch,"connected B->...........................connect TCP ok(send recheck ok jj:%d) %s:%d [localchannel:%d]\n",jj,inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port), m_pChannel->m_nLocalChannel);
//OutputDebugString(ch);		
                        return 0;//正在连接
					}
				}
			}   
			
			if(m_socketTCP > 0)
			{
				closesocket(m_socketTCP);
			}
			m_socketTCP = 0;
//OutputDebugString("close 4\n");
//////////////////////////////////////////////////////////////////////////

            m_bTCP = FALSE;
			
            //TCP连接未成功，进行UDP连接
			m_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
			//////////////////////////////////////////////////////////////////////////
			int len1 = JVC_MSS;
			UDT::setsockopt(m_socket, 0, UDT_MSS, &len1, sizeof(int));
			//////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
            len1=1500*1024;
            UDT::setsockopt(m_socket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
            
            len1=1000*1024;
            UDT::setsockopt(m_socket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
			if (UDT::ERROR == UDT::bind(m_socket, m_pWorker->m_WorkerUDPSocket))
			{//绑定到指定端口失败，改为绑定到随机端口
				
				//"连接失败.指定端口可能被占");
				if(m_socket > 0)
				{
					UDT::close(m_socket);
					m_socket = 0;
				}
//OutputDebugString("check link case rspaddr->faild\n");				
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				
				return -1;//未连接
			}
			
			//将套接字置为非阻塞模式
			BOOL block = FALSE;
			UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
			UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
			LINGER linger;
			linger.l_onoff = 0;
			linger.l_linger = 0;
			UDT::setsockopt(m_socket, 0, UDT_LINGER, &linger, sizeof(LINGER));
			
			if(m_bLan2B)
			{//内网伙伴 内网连接
//char ch[1000];
//sprintf(ch,"connected B->...........................connect L %s:%d [localchannel:%d  id:%d]\n",inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port), m_pChannel->m_nLocalChannel,m_pChannel->m_nLinkID);
//OutputDebugString(ch);

//m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel,"",__FILE__,__LINE__,ch);

//OutputDebugString("connected B...........................connect L\n");
				STJUDTCONN stcon;
				stcon.u = m_socket;
				stcon.name = (SOCKADDR *)&m_addr;
				stcon.namelen = sizeof(SOCKADDR);
				stcon.nChannelID = m_pChannel->m_nLinkID;
				stcon.nPTLinkID = m_ndesLinkID;
				stcon.nPTYSTNO = m_pChannel->m_stConnInfo.nYSTNO;
				stcon.nPTYSTADDR = (int)m_pChannel->m_addressA.sin_addr.s_addr;
				stcon.nLVer_new = JVN_YSTVER;
				stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
				stcon.nMinTime = 3000;
				if(UDT::ERROR == UDT::connect(stcon))
				{//内网连接失败，下次进行外网尝试	
//OutputDebugString("connected B->...........................connect L f 0\n");
//char ch[1000];
//sprintf(ch,"errmsg: %s \n",UDT::getlasterror().getErrorMessage());
//OutputDebugString(ch);
					if(m_socket > 0)
					{
						UDT::close(m_socket);
					}
					m_socket = 0;
					
					m_nstatus = PNODE_STATUS_FAILD;
					m_nConnFCount++;
					ResetPack();
//OutputDebugString("err............\n");					
//OutputDebugString("connected B...........................connect L f 1\n");
					return -1;//未连接
				} 
				else
				{//内网连接成功，设置超时，发送预验证消息  类型(1) + 长度(4) + 本地linkid(4)
					m_nstatus = PNODE_STATUS_WAITLCHECK;
//OutputDebugString("connected B...........................connect L OK 0\n");
					m_chdata[0] = JVN_REQ_RECHECK;
					nSize = 4;
					memcpy(&m_chdata[1], &nSize, 4);
					memcpy(&m_chdata[5], &m_ndesLinkID, 4);
					int jj = Send2Partner(m_chdata, 9, NULL, 200);
					
					m_dwstart = CCWorker::JVGetTime();
					
					//"向伙伴 内网探测成功 等待验证";
//sprintf(ch,"connected B->...........................connect L ok(jj=%d) %s:%d [localchannel:%d]\n",jj,inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port), m_pChannel->m_nLocalChannel);
//OutputDebugString(ch);							
					return 0;//正在连接
				}
			}
			else
			{//外网伙伴 先打洞 再连接
//char ch[1000];
//sprintf(ch,"connected B->...........................connect N %s:%d\n",inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
//OutputDebugString(ch);

                
				if(CCPartnerCtrl::CheckInternalIP(ntohl(m_addr.sin_addr.s_addr)))
				{//是外网伙伴 但得到的却是内网地址 不进行连接
					if(m_socket > 0)
					{
						UDT::close(m_socket);
					}
					m_socket = 0;
					
					m_nstatus = PNODE_STATUS_FAILD;
					m_nConnFCount++;
					ResetPack();
//OutputDebugString("connected B...........................connect N f address err\n");					
					return -1;//未连接
				}

				//类型(1) + 长度(4) + 对方linkID(4) + type(1)
				memset(m_chdata, 0, 10);
				m_chdata[0] = JVN_CMD_ADDR;
				nSize = 5;
				memcpy(&m_chdata[1], &nSize, 4);
				memcpy(&m_chdata[5], &m_ndesLinkID, 4);
				m_chdata[9] = PARTNER_NATTRY;
				m_pChannel->Send2A(m_chdata, 10);
//////////////////////////////////////////////////////////////////////////
				//打洞
				//////////////////////////////////////////////////////////////////////////
				UDTSOCKET udts = UDT::socket(AF_INET, SOCK_STREAM, 0);
				BOOL bReuse = TRUE;
				UDT::setsockopt(udts, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
				//////////////////////////////////////////////////////////////////////////
				int len1 = JVC_MSS;
				UDT::setsockopt(udts, 0, UDT_MSS, &len1, sizeof(int));
				//////////////////////////////////////////////////////////////////////////
				UDT::bind(udts, m_pWorker->m_WorkerUDPSocket);
				
				//将套接字置为非阻塞模式
				BOOL block = FALSE;
				UDT::setsockopt(udts, 0, UDT_SNDSYN, &block, sizeof(BOOL));
				UDT::setsockopt(udts, 0, UDT_RCVSYN, &block, sizeof(BOOL));
				LINGER linger;
				linger.l_onoff = 0;
				linger.l_linger = 0;
				UDT::setsockopt(udts, 0, UDT_LINGER, &linger, sizeof(LINGER));
				
				SOCKADDR_IN addrtmp;
				memcpy(&addrtmp, &m_addr, sizeof(SOCKADDR_IN));
//				UDT::ystpunch(udts, &addrtmp, m_pChannel->m_stConnInfo.nYSTNO, 500);
				m_pWorker->pushtmpsock(udts);
				//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


				STJUDTCONN stcon;
				stcon.u = m_socket;
				stcon.name = (SOCKADDR *)&addrtmp;//m_addr
				stcon.namelen = sizeof(SOCKADDR);
				stcon.nChannelID = m_pChannel->m_nLinkID;
				stcon.nPTLinkID = m_ndesLinkID;
				stcon.nPTYSTNO = m_pChannel->m_stConnInfo.nYSTNO;
				stcon.nPTYSTADDR = (int)m_pChannel->m_addressA.sin_addr.s_addr;
				stcon.nLVer_new = JVN_YSTVER;
				stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
				stcon.nMinTime = 3000;
				if(UDT::ERROR == UDT::connect(stcon))
				{//外网连接失败，下次进行对方尝试	
					if(m_socket > 0)
					{
						UDT::close(m_socket);
					}
					m_socket = 0;
					
					m_nstatus = PNODE_STATUS_FAILD;
					m_nConnFCount++;
					ResetPack();
//OutputDebugString("connected B...........................connect N f\n");					
					return -1;//未连接
				} 
				else
				{//外网连接成功，设置超时，等待外网验证 发送预验证消息  类型(1) + 长度(4) + 本地linkid(4)
					m_chdata[0] = JVN_REQ_RECHECK;
					nSize = 4;
					memcpy(&m_chdata[1], &nSize, 4);
					memcpy(&m_chdata[5], &m_ndesLinkID, 4);
					int jj = Send2Partner(m_chdata, 9, NULL,200);
					m_nstatus = PNODE_STATUS_WAITNCHECK;
					m_dwstart = CCWorker::JVGetTime();
//sprintf(ch,"connected B...........................connect(udt::connect) N ok jj=%d\n",jj);					
//OutputDebugString(ch);	
					return 0;//正在连接
				}
			}
		}
		break;
	case PNODE_STATUS_TCCONNECTING:
		{
TCCONNECT:
			BOOL bfind = FALSE;
			int ncount = m_pChannel->m_SList.size();
			for(int i=0; i<ncount; i++)
			{
				if(m_pChannel->m_SList[i].buseful)
				{//开始尝试请求主控地址
					if(SendSTURN(m_pChannel->m_SList[i].addr))
					{//向服务器发送请求成功
						bfind = TRUE;
						m_nstatus = PNODE_STATUS_TCWAITTS;
						m_dwstart = CCWorker::JVGetTime();
						break;
					}
					else
					{//向服务器发送请求失败 尝试下一个服务器
						m_pChannel->m_SList[i].buseful = FALSE;
					}
				}
			}
			if(!bfind)
			{//服务器都常试过，失败
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				
				return -1;
			}
			
			return 0;
		}
		break;
	default:
		break;
	}

	return -1;
}

//<0未连接 0连接中 1连接成功
int CCPartner::CheckLink(CCPartnerCtrl *pPCtrl, BOOL bNewConnect, DWORD &dwlasttime)
{
//	int nSize = 0;
	DWORD dwendtime = 0;
	dwlasttime = m_dwLastDataTime;
	switch(m_nstatus)
	{
	case PNODE_STATUS_CONNECTED:
		{
			if(m_nTimeOutCount >= JVN_TIMEOUTCOUNT)
			{//连续超时次数过多 断开连接
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 1\n");					
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
//OutputDebugString("timeout disconnectp.....1\n");				
				return -1;//未连接
			}
			return 1;
		}
		break;
	case PNODE_STATUS_FAILD:
		{
			if(!bNewConnect)
			{
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 2\n");				
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				
				return -1;//未连接
			}
			else
			{
				if(!m_bTURNC)
				{
					m_nstatus = PNODE_STATUS_CONNECTING;
				}
				else
				{
					m_nstatus = PNODE_STATUS_TCCONNECTING;
				}
				
				return 0;//正在连接
			}
		}
		break;
	case PNODE_STATUS_NEW:
		{//新节点：向主控请求对方节点地址，节点状态改为 地址请求中，开始计时；
			if(!bNewConnect)
			{
				return -1;//未连接
			}
			
			if(!m_bTURNC)
			{
				m_nstatus = PNODE_STATUS_CONNECTING;
			}
			else
			{
				m_nstatus = PNODE_STATUS_TCCONNECTING;
			}
			
			return 0;//正在连接
		}
		break;
	case PNODE_STATUS_CONNECTING:
	case PNODE_STATUS_TCCONNECTING:
		{
			return 0;
		}
		break;
	case PNODE_STATUS_WAITLCHECK:
		{//内网待验证：检查是否超时，超时时更新节点状态为内网连接失败；	
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > JVN_TIME_WAITLRECHECK + m_dwstart)
			{//超过10s 未收验证信息
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 5\n");
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
//OutputDebugString("waitcheck outtime\n");				
				return -1;//未连接
			}

			return 0;//正在连接
		}
		break;
	case PNODE_STATUS_WAITNCHECK:
		{//外网待验证：检查是否超时，超时时更新节点状态为连接失败；
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > JVN_TIME_WAITNRECHECK + m_dwstart)
			{//超过15s 未收验证信息
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 6\n");
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
//OutputDebugString("check link case waitncheck->faild\n");				
				return -1;//未连接
			}
			return 0;//正在连接
		}
		break;
	case PNODE_STATUS_ACCEPT:
		{
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > JVN_TIME_WAITNRECHECK + m_dwstart)//10000
			{//超过15s 未收验证信息
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 7\n");
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
//OutputDebugString("check link case accept->faild\n");
				m_socket = 0;
                return -1;//未连接
			}
			return 0;//正在连接
		}
		break;
	case PNODE_STATUS_TCWAITTS:
		{
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > JVN_TIME_WAIT + m_dwstart)
			{//超过10s 
				if(m_socketTCtmp > 0)
				{
					closesocket(m_socketTCtmp);
				}
				m_socketTCtmp = 0;

				int ncount = m_pChannel->m_SList.size();
				for(int i=0; i<ncount; i++)
				{
					if(m_pChannel->m_SList[i].buseful)
					{//开始尝试请求服务器地址
						//向服务器发送请求失败 尝试下一个服务器
						m_pChannel->m_SList[i].buseful = FALSE;
						break;
					}
				}
				m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器
			}
			else
			{//接收数据
				int ncount = m_pChannel->m_SList.size();
				for(int i=0; i<ncount; i++)
				{
					if(m_pChannel->m_SList[i].buseful)
					{
						int nret = RecvSTURN();
						switch(nret)
						{//收到服务器回复
						case JVN_CMD_CONNS2://取得地址 开始连接
							{
								if(ConnectTURN())
								{//向对端发送有效性验证消息
									if(SendReCheck(TRUE))
									{//发送成功 等待验证结果
										m_nstatus = PNODE_STATUS_TCWAITRECHECK;
										m_dwstart = CCWorker::JVGetTime();
									}
									else
									{//发送失败 尝试其他服务器
										m_nstatus = PNODE_STATUS_TCCONNECTING;
										m_pChannel->m_SList[i].buseful = FALSE;
									}
								}
								else
								{//连接失败，尝试其他服务器
									m_nstatus = PNODE_STATUS_TCCONNECTING;
									m_pChannel->m_SList[i].buseful = FALSE;
								}

								if(m_socketTCtmp > 0)
								{
									closesocket(m_socketTCtmp);
								}
								m_socketTCtmp = 0;
							}
							
							break;
						case JVN_RSP_CONNAF://取地址失败 尝试其他服务器
							m_pChannel->m_SList[i].buseful = FALSE;
							m_nstatus = PNODE_STATUS_TCCONNECTING;
							if(m_socketTCtmp > 0)
							{
								closesocket(m_socketTCtmp);
							}
							m_socketTCtmp = 0;
							break;
						default://未收到信息或收到其他错误信息 继续接收
							break;
						}
						break;
					}
				}
			}

			return 0;
		}
		break;
	case PNODE_STATUS_TCWAITRECHECK:
		{
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > 2*JVN_TIME_WAIT + m_dwstart)
			{//超过20s
				int ncount = m_pChannel->m_SList.size();
				for(int i=0; i<ncount; i++)
				{
					if(m_pChannel->m_SList[i].buseful)
					{//开始尝试请求服务器地址
						//向服务器发送请求失败 尝试下一个服务器
						m_pChannel->m_SList[i].buseful = FALSE;
						break;
					}
				}
				m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器
			}

			return 0;
		}
		break;
	case PNODE_STATUS_TCWAITPWCHECK:
		{
			dwendtime = CCWorker::JVGetTime();
			if(dwendtime > 2*JVN_TIME_WAIT + m_dwstart)
			{//超过20s
				int ncount = m_pChannel->m_SList.size();
				for(int i=0; i<ncount; i++)
				{
					if(m_pChannel->m_SList[i].buseful)
					{//开始尝试请求服务器地址
						//向服务器发送请求失败 尝试下一个服务器
						m_pChannel->m_SList[i].buseful = FALSE;
						break;
					}
				}
				m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器
			}
		}
		break;
	default:
		break;
	}

	return -1;
}

BOOL CCPartner::BaseRecv(CCPartnerCtrl *pPCtrl)
{
	if(m_bTCP)
	{
		return BaseRecvTCP(pPCtrl);
	}

	int rs = 0;
	if((!m_bTURNC && m_ndesLinkID > 0 && m_socket > 0 
		&& (m_nstatus == PNODE_STATUS_CONNECTED || m_nstatus == PNODE_STATUS_WAITLCHECK 
		    || m_nstatus == PNODE_STATUS_WAITNCHECK || m_nstatus == PNODE_STATUS_ACCEPT))
	   || (m_bTURNC && m_socket > 0))// && m_nstatus == PNODE_STATUS_CONNECTED))
	{//如果ID有效，sock有效，则可以接收数据
		if(JVN_PTBUFSIZE-m_nRecvPos < 30*1024)
		{//剩余空间不多，避免接收不完整(这里暂假定发送的数据都小于30K)
			//OutputDebugString("pt recvfromp.....2\n");
			return TRUE;
		}

		if (UDT::ERROR == (rs = UDT::recvmsg(m_socket, (char *)&m_puchRecvBuf[m_nRecvPos], JVN_PTBUFSIZE-m_nRecvPos)))
		{
			if(m_socket > 0)
			{
				UDT::close(m_socket);
			}
			m_socket = 0;
			m_nstatus = PNODE_STATUS_FAILD;
			m_nConnFCount++;
			ResetPack();
			
			//把本等待的数据复位，便于切换到其他数据源获取
			if(pPCtrl != NULL)
			{
				//遍历本伙伴正在请求的数据块
				::std::map<unsigned int, DWORD>::iterator iterbt;
				for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
				{
					if(iterbt->second > 0)
					{//将正在请求中的记录从总记录中清除，便于切换
						pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
					}
				}
			}
			
			//char chMsg[] = "接收伙伴数据失败 断掉该伙伴连接";
//OutputDebugString("pt recvfromp.....1\n");			
			return TRUE;
		}
		else if(rs == 0)
		{
			//OutputDebugString("pt recvfromp.....2\n");
			return TRUE;
		}

		m_nRecvPos += rs;
		
		while(TRUE)
		{
			if(!ParseMsg(pPCtrl))
			{
				return TRUE;
			}
		}
	}

	return TRUE;
}

BOOL CCPartner::BaseRecvTCP(CCPartnerCtrl *pPCtrl)
{
	int rs = 0;
	if(m_ndesLinkID > 0 && m_socketTCP > 0
		&& (m_nstatus == PNODE_STATUS_CONNECTED
		|| m_nstatus == PNODE_STATUS_WAITLCHECK
		|| m_nstatus == PNODE_STATUS_WAITNCHECK
		|| m_nstatus == PNODE_STATUS_ACCEPT))
	{//如果ID有效，sock有效，则可以接收数据
	#ifndef WIN32
		if ((rs = recv(m_socketTCP, (char *)&m_puchRecvBuf[m_nRecvPos], JVN_PTBUFSIZE-m_nRecvPos, MSG_NOSIGNAL)) < 0)
		{
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
			{
				return TRUE;
			}
	#else
		if(SOCKET_ERROR == (rs = recv(m_socketTCP, (char *)&m_puchRecvBuf[m_nRecvPos], JVN_PTBUFSIZE-m_nRecvPos, 0)))
		{
			int kkk=WSAGetLastError();
//char ch[100];
//sprintf(ch,"baserecvtcp rs== err  recvpos:%d  kkk:%d\n",m_nRecvPos,kkk);
//OutputDebugString(ch);
			if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
			{
				return TRUE;
			}
	#endif	
			if(m_socketTCP > 0)
			{
				closesocket(m_socketTCP);
			}
			m_socketTCP = 0;
//OutputDebugString("close 8\n");
			m_nstatus = PNODE_STATUS_FAILD;
			m_nConnFCount++;
			ResetPack();
			
			//把本等待的数据复位，便于切换到其他数据源获取
			if(pPCtrl != NULL)
			{
				//遍历本伙伴正在请求的数据块
				::std::map<unsigned int, DWORD>::iterator iterbt;
				for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
				{
					if(iterbt->second > 0)
					{//将正在请求中的记录从总记录中清除，便于切换
						pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
					}
				}
			}
			
			//char chMsg[] = "接收伙伴数据失败 断掉该伙伴连接";
//char ch[100];
//sprintf(ch,"pt recvfromp.....1 err:%d\n",kkk);
//OutputDebugString(ch);			
			return TRUE;
		}
		else if(rs == 0)
		{
			//OutputDebugString("pt recvfromp.....2\n");
			return TRUE;
		}
//char ch[100];
//sprintf(ch,"baserecvtcp ss=%d  recvpos:%d\n",rs,m_nRecvPos);
//OutputDebugString(ch);

        m_nRecvPos += rs;

		while(TRUE)
		{
			if(!ParseMsg(pPCtrl))
			{
				return TRUE;
			}
		}
	}

	return TRUE;
}

BOOL CCPartner::ParseMsg(CCPartnerCtrl *pPCtrl)
{
	if(m_nRecvPos < 5)
	{
		return FALSE;//过小 判断不出是什么包
	}
	
	BYTE uchType=m_puchRecvBuf[0];
	int nLen=0;
	STCHUNKHEAD stHead;
	unsigned int unChunkID = 0;
	int nChunkCount=0;
	memcpy(&nLen, &m_puchRecvBuf[1], 4);

	if(uchType != JVN_CMD_BM//BM消息
		&& uchType != JVN_REQ_BMD//请求某数据片
		&& uchType != JVN_RSP_BMD//某数据片数据
		&& uchType != JVN_RSP_BMDNULL//请求某数据片失败
		&& uchType != JVN_RSP_BMDBUSY//请求某数据片失败对方忙碌
		&& uchType != JVN_RSP_CHECKPASST//身份验证成功
		&& uchType != JVN_RSP_CHECKPASSF//身份验证失败
		&& uchType != JVN_REQ_RECHECK//预验证信息
		&& uchType != JVN_CMD_DISCONN
		&& uchType != JVN_RSP_RECHECK)//伙伴连接断开)
	{//错误类型，丢弃该部分数据
		memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVN_PTBUFSIZE-1);
		m_nRecvPos -= 1;
//char ch[100];
//sprintf(ch,"[%d]parsemsg type err3 recvpos:%d  type[0]:%d type[1]:%d\n",m_pChannel->m_nLocalChannel,m_nRecvPos, uchType,m_puchRecvBuf[1]);
//OutputDebugString(ch);
		return TRUE;
	}

	if(nLen < 0 || nLen >= JVN_PTBUFSIZE)
	{//错误长度，丢弃该部分数据
		memmove(m_puchRecvBuf, m_puchRecvBuf + 5, JVN_PTBUFSIZE-5);
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
	case JVN_CMD_BM://BM消息
		{
//OutputDebugString("parsemsg ok........BM\n");
			if(!m_bTURNC)
			{
				//类型(1)+总长度(4)+CHUNKID(4)+总块数(4)+BM(?)
				unChunkID = 0;
				nChunkCount = 0;
				memcpy(&unChunkID, &m_puchRecvBuf[5], 4);
				memcpy(&nChunkCount, &m_puchRecvBuf[9], 4);

//char ch[100]={0};
//sprintf(ch, "[%d]B %d %d [%d, %d]**************************BM [rp:%d len:%d]\r\n",m_pChannel->m_nLocalChannel, unChunkID, nChunkCount, unChunkID, unChunkID+nChunkCount-1, m_nRecvPos,nLen);
//OutputDebugString(ch);
/*
char chMsg[1000]={0};
//////////////////////////////////////////////////////////////////////////
sprintf(chMsg, "[%d]B %d %d [%d, %d]**********************BM ",m_pChannel->m_nLocalChannel,unChunkID,nChunkCount,unChunkID, unChunkID+nChunkCount-1);
int nByte = nChunkCount/8 + ((nChunkCount%8)?1:0);
for(int i=0; i<nByte; i++)
{
	char ch[10];
	sprintf(ch,"%X",m_puchRecvBuf[13+i]);
	strcat(chMsg, ch);
}
strcat(chMsg,"\n");
OutputDebugString(chMsg);
//////////////////////////////////////////////////////////////////////////
*/
				//更新BM
				SetBM(unChunkID, nChunkCount, &m_puchRecvBuf[13], nLen-8, pPCtrl);
			}
			else
			{//类型(1)+总长度(4)+CHUNKID(4)+总帧数(4)
				unChunkID = 0;
				nChunkCount = 0;
				memcpy(&unChunkID, &m_puchRecvBuf[5], 4);
				memcpy(&nChunkCount, &m_puchRecvBuf[9], 4);
				
				
//char chMsg[100]={0};
//sprintf(chMsg, "B6 %d %d**************************BM\r\n", unChunkID, nChunkCount);
//OutputDebugString(chMsg);
			
				SetBM(unChunkID, nChunkCount, NULL, 0, pPCtrl);
			}

			m_nDownLoadTotalB += nLen+5;
			m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
			m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
			m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
			m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;

			RefreshPartner(0, 0);//借用该函数更新"最后数据到达时间"
		}
		break;
	case JVN_REQ_BMD://请求某数据片
		{//类型(1)+总长度(4)+CHUNKID(4)
			if(nLen == 4)
			{
//OutputDebugString("parsemsg ok........REQBMD\n");
				unChunkID = 0;
					
				memcpy(&unChunkID, &m_puchRecvBuf[5], 4);

				STREQ stpushreq;
				stpushreq.unChunkID = unChunkID;
				stpushreq.bAnswer = FALSE;
				stpushreq.dwStartTime = CCWorker::JVGetTime();
					
				BOOL bfind = FALSE;
				int ncount = m_PushQueue.size();
				for(int i=0; i<ncount; i++)
				{
					if(m_PushQueue[i].unChunkID == stpushreq.unChunkID)
					{
						bfind = TRUE;
						m_PushQueue[i].dwStartTime = stpushreq.dwStartTime;
						break;
					}
				}
				if(!bfind)
				{
					m_PushQueue.push_back(stpushreq);
				}
					
				m_nDownLoadTotalB += nLen+5;
				m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
				m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
				m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
				m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
//char ch[100]={0};
//sprintf(ch,"[%d]REQ_BMD--------------%d  [rp:%d  len:%d] \r\n",m_pChannel->m_nLocalChannel, unChunkID, m_nRecvPos, nLen);
//OutputDebugString(ch);
			}
//			else
//			{
//OutputDebugString("err2.................\n");
//			}
		}
		break;
	case JVN_RSP_BMD://某数据片数据
		{//类型(1)+总长度(4)+数据块头(?)+数据(?)
//			DWORD dws=CCWorker::JVGetTime();
//			DWORD dwe=0;
		
			memcpy(&stHead, &m_puchRecvBuf[5], sizeof(STCHUNKHEAD));
//if(m_pChannel->m_nLocalChannel == 2)
//{				
//char ch[100]={0};
//sprintf(ch,"[%d]RSP_BMD+++++++++++++++%d[len:%d]  [rp:%d len:%d] haveI:%d\r\n",m_pChannel->m_nLocalChannel, stHead.unChunkID, nLen, m_nRecvPos, nLen,stHead.bHaveI);
//OutputDebugString(ch);
//}
			
			m_nDownLoadTotalB += nLen+5;
			m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
			m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
			m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
			m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;	
			
			//更新当前伙伴参数
			RefreshPartner(stHead.unChunkID, nLen);
			pPCtrl->SetReqStartTime(FALSE, unChunkID, 0);
			pPCtrl->m_dwLastPTData = CCWorker::JVGetTime();	
			//更新本地缓存
			m_pChannel->m_pBuffer->WriteBuffer(stHead, &m_puchRecvBuf[5+sizeof(STCHUNKHEAD)]);			
		}
		break;
	case JVN_RSP_BMDNULL://请求某数据片失败
		{//类型(1)+总长度(4)+CHUNKID(4)
			if(nLen == 4)
			{
				unChunkID = 0;
				
				memcpy(&unChunkID, &m_puchRecvBuf[5], 4);				
//char ch[100]={0};
//sprintf(ch,"[%d]RSP_BMDNULL+++++++++++++++%d [recvpos:%d nlen:%d][%d-%d]\n",m_pChannel->m_nLocalChannel,unChunkID,m_nRecvPos,nLen,m_stMAP.unBeginChunkID,m_stMAP.unBeginChunkID+m_stMAP.nChunkCount-1);
//OutputDebugString(ch);
//m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nChannel, "", __FILE__, __LINE__, ch);
			
				m_nDownLoadTotalB += nLen+5;
				m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
				m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
				m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
				m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
				
				//更新当前伙伴参数
				RefreshPartner(unChunkID, nLen, TRUE);
				pPCtrl->SetReqStartTime(FALSE, unChunkID, 0);
			}
//			else
//			{
//OutputDebugString("err3.................\n");
//			}
		}
		break;
	case JVN_RSP_BMDBUSY://对方忙碌，从其他数据源索取或重新索取
		{//类型(1)+总长度(4)+CHUNKID(4)
			if(nLen == 4)
			{
				unChunkID = 0;
				
				memcpy(&unChunkID, &m_puchRecvBuf[5], 4);				
//char ch[100]={0};
//sprintf(ch,"[%d]RSP_BMDBUSY+++++++++++++++%d [recvpos:%d nlen:%d][%d-%d]\n",m_pChannel->m_nLocalChannel,unChunkID,m_nRecvPos,nLen,m_stMAP.unBeginChunkID,m_stMAP.unBeginChunkID+m_stMAP.nChunkCount-1);
//OutputDebugString(ch);
//m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nChannel, "", __FILE__, __LINE__, ch);
				
				m_nDownLoadTotalB += nLen+5;
				m_nDownLoadTotalKB += m_nDownLoadTotalB/1024;//满KB
				m_nDownLoadTotalB = m_nDownLoadTotalB%1024;
				m_nDownLoadTotalMB += m_nDownLoadTotalKB/1000;//满MB
				m_nDownLoadTotalKB = m_nDownLoadTotalKB%1000;
				
				//更新当前伙伴参数
				RefreshPartner(unChunkID, nLen);
				pPCtrl->SetReqStartTime(FALSE, unChunkID, 0);
			}
//			else
//			{
//				OutputDebugString("err3.................\n");
//			}
		}
		break;
	case JVN_RSP_CHECKPASST://身份验证成功
		{
			if(!m_bTURNC)
			{
				if(nLen == 0)
				{
//char ch[100];
//sprintf(ch,"[%d]parsemsg ok........rsp checkpass t [recvpos:%d nlen:%d]\n",m_pChannel->m_nLocalChannel,m_nRecvPos,nLen);
//OutputDebugString(ch);
					ResetPack();
					m_nstatus = PNODE_STATUS_CONNECTED;
					m_nConnFCount = 0;
					m_nTimeOutCount = 0;
					m_dwLastConnTime = CCWorker::JVGetTime();
					m_fLastConnTotal = m_nDownLoadTotalMB*1024 + (float)((float)m_nDownLoadTotalKB) + (float)((float)m_nDownLoadTotalB/1024)//M
						+ m_nUpTotalMB*1024 + (float)((float)m_nUpTotalKB) + (float)((float)m_nUpTotalB/1024);//KB
					//来自伙伴 验证通过
				}
//				else
//				{
//OutputDebugString("err4.................\n");
//				}
			}
			else
			{
				if(nLen == 1)
				{
					if(m_puchRecvBuf[5] == 1)
					{//验证通过 连接成功建立
						m_nstatus = PNODE_STATUS_CONNECTED;
					}
					else
					{//身份验证未通过 直接结束连接
						if(m_socketTCP > 0)
						{
							closesocket(m_socketTCP);
						}
						m_socketTCP = 0;
						if(m_socket > 0)
						{
							UDT::close(m_socket);
						}
						m_socket = 0;
						m_nstatus = PNODE_STATUS_FAILD;
						m_nConnFCount++;
						ResetPack();
					}
				}
			}
		}
		break;
	case JVN_RSP_CHECKPASSF://身份验证失败
		{
//OutputDebugString("pt recvfromp.....JVN_RSP_CHECKPASSF\n");
			if(nLen == 0)
			{
//OutputDebugString("parsemsg ok........rsp checkpass f\n");
				BYTE data[10]={0};
				data[0] = JVN_CMD_DISCONN;
				Send2Partner(data, 5, NULL,0, FALSE);
				
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				//结束线程
				
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;

				if(m_socketTCP > 0)
				{
					closesocket(m_socketTCP);
				}
				m_socketTCP = 0;
//OutputDebugString("close 9\n");
				//来自伙伴 验证未通过
				break;
			}
//			else
//			{
//OutputDebugString("err5.................\n");
//			}
		}
		break;
	case JVN_REQ_RECHECK://预验证信息
		{
//OutputDebugString("pt recvfromp.....JVN_REQ_RECHECK\n");
			if(nLen == 4)
			{
//OutputDebugString("parsemsg ok........req recheck\n");
				//数据类型(1)+数据总长度(4)+本地ID(4)
				memcpy(&nChunkCount, &m_puchRecvBuf[5], 4);
				BYTE data[10]={0};
				if(nChunkCount == m_pChannel->m_nLinkID)
				{//验证通过 是发给本地的连接
//char ch[100];
//sprintf(ch,"[%d]parsemsg ok........req recheck nid=nlinkid OK [rp:%d len:%d]\n",m_pChannel->m_nLocalChannel, m_nRecvPos, nLen);
//OutputDebugString(ch);
					//验证通过 是发给本地的连接	
					data[0] = JVN_RSP_CHECKPASST;
					Send2Partner(data, 5, NULL,200);
					
					ResetPack(TRUE);
					m_nstatus = PNODE_STATUS_CONNECTED;
					m_nConnFCount = 0;
					m_dwstart = 0;
					
					m_nTimeOutCount = 0;
					m_dwLastConnTime = CCWorker::JVGetTime();
					m_fLastConnTotal = m_nDownLoadTotalMB*1024 + (float)((float)m_nDownLoadTotalKB) + (float)((float)m_nDownLoadTotalB/1024)//KB
					                   + m_nUpTotalMB*1024 + (float)((float)m_nUpTotalKB) + (float)((float)m_nUpTotalB/1024);//KB
				}		
				else
				{
//OutputDebugString("parsemsg ok........req recheck nid != nlinkid f\n");
					//验证未通过 不是发给本地的连接
					data[0] = JVN_RSP_CHECKPASSF;
					Send2Partner(data, 5, NULL,100);
					
					CCWorker::jvc_sleep(5);
					if(m_socket > 0)
					{
						UDT::close(m_socket);
					}
					m_socket = 0;
					
					if(m_socketTCP > 0)
					{
						closesocket(m_socketTCP);
					}
					m_socketTCP = 0;
//OutputDebugString("close 10\n");					
					m_nstatus = PNODE_STATUS_FAILD;
					m_nConnFCount++;
					ResetPack();
				}
			}
//			else
//			{
//OutputDebugString("err6.................\n");
//			}
		}
		break;
	case JVN_CMD_DISCONN://伙伴连接断开
		{
//OutputDebugString("pt recvfromp.....JVN_CMD_DISCONN\n");
//OutputDebugString("parsemsg ok........disconn\n");
			m_nstatus = PNODE_STATUS_FAILD;
			ResetPack();
			//结束线程
			if(m_socket > 0)
			{
				UDT::close(m_socket);
			}
			m_socket = 0;

			if(m_socketTCP > 0)
			{
				closesocket(m_socketTCP);
			}
			m_socketTCP = 0;
//OutputDebugString("close 11\n");
			//把本等待的数据复位，便于切换到其他数据源获取
			if(pPCtrl != NULL)
			{
				//遍历本伙伴正在请求的数据块
				::std::map<unsigned int, DWORD>::iterator iterbt;
				for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
				{
					if(iterbt->second > 0)
					{//将正在请求中的记录从总记录中清除，便于切换
						pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
					}
				}
			}
		}
		break;
	case JVN_RSP_RECHECK:
		{//预验证 类型(1) + 长度(4) + 是否通过(1) + 版本号1(2) + 版本号2(2) + 版本号3(2) + 版本号4(2) + LinkID(4) + BMCHUNK(4)
			short sVer1=0;
			short sVer2=0;
			short sVer3=0;
			short sVer4=0;

			if(nLen == 15)
			{
				memcpy(&sVer1, &m_puchRecvBuf[6], 2);
				memcpy(&sVer2, &m_puchRecvBuf[8], 2);
				memcpy(&sVer3, &m_puchRecvBuf[10], 2);
				memcpy(&sVer4, &m_puchRecvBuf[12], 2);
				memcpy(&m_ndesLinkID, &m_puchRecvBuf[14], 4);
				
//				m_bJVP2P = (m_puchRecvBuf[18]==1)?TRUE:FALSE;
//				m_bLan2A = (m_puchRecvBuf[19] == 1)?TRUE:FALSE;
				if(m_puchRecvBuf[18] != 0 && m_puchRecvBuf[18] != 1)
				{
					return 0;
				}
				
				int nver = m_pChannel->CheckVersion(sVer1, sVer2, sVer3, sVer4);
				if(nver != 0)
				{
					//预验证未通过 尝试其他服务器
					int ncount = m_pChannel->m_SList.size();
					for(int i=0; i<ncount; i++)
					{
						if(m_pChannel->m_SList[i].buseful)
						{//开始尝试请求服务器地址
							//向服务器发送请求失败 尝试下一个服务器
							m_pChannel->m_SList[i].buseful = FALSE;
							break;
						}
					}
					m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器	
				}
				
				if(m_puchRecvBuf[5] == 1)
				{
					//验证通过
					if(SendPWCheck())
					{//发送身份验证消息成功 等待结果
						m_nstatus = PNODE_STATUS_TCWAITPWCHECK;
						m_dwstart = CCWorker::JVGetTime();
					}
					else
					{//发送身份验证消息失败 尝试其他服务器
						int ncount = m_pChannel->m_SList.size();
						for(int i=0; i<ncount; i++)
						{
							if(m_pChannel->m_SList[i].buseful)
							{//开始尝试请求服务器地址
								//向服务器发送请求失败 尝试下一个服务器
								m_pChannel->m_SList[i].buseful = FALSE;
								break;
							}
						}
						m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器
					}
				}
				else
				{
					//预验证未通过 尝试其他服务器
					int ncount = m_pChannel->m_SList.size();
					for(int i=0; i<ncount; i++)
					{
						if(m_pChannel->m_SList[i].buseful)
						{//开始尝试请求服务器地址
							//向服务器发送请求失败 尝试下一个服务器
							m_pChannel->m_SList[i].buseful = FALSE;
							break;
						}
					}
					m_nstatus = PNODE_STATUS_TCCONNECTING;//继续尝试下一个服务器
				}			
			}
		}
		break;
	default:
		{//错误类型，丢弃该部分数据
//OutputDebugString("parsemsg ok........default\n");
			memmove(m_puchRecvBuf, m_puchRecvBuf + 1, JVN_PTBUFSIZE-1);
			m_nRecvPos -= 1;
			
			return TRUE;
		}
		break;
	}

	if(m_nRecvPos >= nLen+5)
	{//删除解析完的数据
		memmove(m_puchRecvBuf, m_puchRecvBuf + nLen+5, m_nRecvPos-nLen-5);
		m_nRecvPos -= (nLen+5);
	}
	else
	{
//char ch[100];
//sprintf(ch,"[%d]err7.................[nrecvpos:%d  nlen:%d]\n",m_pChannel->m_nLocalChannel,m_nRecvPos, nLen);
//OutputDebugString(ch);
	}
//char ch[100];
//sprintf(ch,"new recvpos:%d\n",m_nRecvPos);
//OutputDebugString(ch);
	return TRUE;
}


int CCPartner::Send2Partner(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete)
{
	if(m_bTCP)
	{
		return Send2PartnerTCP(puchBuf, nSize, pPCtrl, ntrylimit, bComplete);
	}

	if(puchBuf == NULL || nSize <= 0 || m_socket <= 0)
	{
		return -1;
	}

	int nfc=0;
//	DWORD dws=CCWorker::JVGetTime();
//	DWORD dwe=0;
	int ss=0;
	int ssize=0;
	while(ssize < nSize)
	{
		if(0 < (ss = UDT::send(m_socket, (char *)puchBuf + ssize, jvs_min(nSize - ssize, 20000), 0)))
		{
			nfc=0;
			ssize += ss;
		}
		else if(ss == 0)
		{
			if(!bComplete)
			{//发多少是多少
				return ssize;
			}

			nfc++;
			if(ntrylimit>0 && nfc>ntrylimit)
			{
				if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
				{
					m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "发送P数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
				}
				else
				{
					m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "SendPData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
				}
				
				UDT::close(m_socket);
				m_socket = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				
				//把本等待的数据复位，便于切换到其他数据源获取
				if(pPCtrl != NULL)
				{
					//遍历本伙伴正在请求的数据块
					::std::map<unsigned int, DWORD>::iterator iterbt;
					for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
					{
						if(iterbt->second > 0)
						{//将正在请求中的记录从总记录中清除，便于切换
							pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
						}
					}
				}
				return -1;
			}
		}
		else
		{
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "发送P数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "SendPData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
			
			UDT::close(m_socket);
			m_socket = 0;
			m_nstatus = PNODE_STATUS_FAILD;
			m_nConnFCount++;
			ResetPack();
			
			//把本等待的数据复位，便于切换到其他数据源获取
			if(pPCtrl != NULL)
			{
				//遍历本伙伴正在请求的数据块
				::std::map<unsigned int, DWORD>::iterator iterbt;
				for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
				{
					if(iterbt->second > 0)
					{//将正在请求中的记录从总记录中清除，便于切换
						pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
					}
				}
			}
			return -1;
		}
	}

	return ssize;
}

int CCPartner::Send2PartnerTCP(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete)
{
	if(puchBuf == NULL || nSize <= 0 || m_socketTCP <= 0)
	{
		return -1;
	}
	
	int nfc=0;
//	DWORD dws=CCWorker::JVGetTime();
//	DWORD dwe=0;
	int ntimeout=0;
	int ss=0;
	int ssize=0;
	while(ssize < nSize)
	{
		ntimeout=0;
		if(0 < (ss = CCPartnerCtrl::tcpsend(m_socketTCP, (char *)puchBuf + ssize,jvs_min(nSize - ssize, 20000) , 1, ntimeout)))
		{//一定成功发送了一些数据
			nfc=0;
			ssize += ss;
		}
		else if(ss == 0)
		{//有可能是正常发送超时，也有可能是产生了错误(如果是非阻塞错误，则应该继续发送，否则应该退出)
			if(ntimeout == 0)
			{//不是发送超时，需要判断错误类型
			#ifndef WIN32
				if(errno != EWOULDBLOCK)
				{
			#else
				int kkk = WSAGetLastError();
//char ch[100];
//sprintf(ch,"sendtcp ss=0  kkk:%d*************************************\n",kkk);
//OutputDebugString(ch);
				if(WSAEWOULDBLOCK != kkk)
				{//异常错误，直接结束
			#endif
					closesocket(m_socketTCP);
//OutputDebugString("close 12\n");
					m_socketTCP = 0;
					m_nstatus = PNODE_STATUS_FAILD;
					m_nConnFCount++;
					ResetPack();
					
					//把本等待的数据复位，便于切换到其他数据源获取
					if(pPCtrl != NULL)
					{
						//遍历本伙伴正在请求的数据块
						::std::map<unsigned int, DWORD>::iterator iterbt;
						for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
						{
							if(iterbt->second > 0)
							{//将正在请求中的记录从总记录中清除，便于切换
								pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
							}
						}
					}
					
//OutputDebugString("send2ptcp  err->faild\n");
					return -1;
				}
			}

			if(!bComplete)
			{//发多少是多少
				return ssize;
			}

			nfc++;
			if(ntrylimit > 0 && nfc>ntrylimit)
			{//完整发送也有尝试次数限制，超过限则也需要结束
				closesocket(m_socketTCP);
				m_socketTCP = 0;
				m_nstatus = PNODE_STATUS_FAILD;
				m_nConnFCount++;
				ResetPack();
				
				//把本等待的数据复位，便于切换到其他数据源获取
				if(pPCtrl != NULL)
				{
					//遍历本伙伴正在请求的数据块
					::std::map<unsigned int, DWORD>::iterator iterbt;
					for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
					{
						if(iterbt->second > 0)
						{//将正在请求中的记录从总记录中清除，便于切换
							pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
						}
					}
				}
				return -1;
			}
		}
		else
		{//异常，直接结束
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "发送P数据失败", __FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel, "SendPData failed", __FILE__,__LINE__);
			}
			
			closesocket(m_socketTCP);
//OutputDebugString("close 13\n");
			m_socketTCP = 0;
			m_nstatus = PNODE_STATUS_FAILD;
			m_nConnFCount++;
			ResetPack();

			//把本等待的数据复位，便于切换到其他数据源获取
			if(pPCtrl != NULL)
			{
				//遍历本伙伴正在请求的数据块
				::std::map<unsigned int, DWORD>::iterator iterbt;
				for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
				{
					if(iterbt->second > 0)
					{//将正在请求中的记录从总记录中清除，便于切换
						pPCtrl->SetReqStartTime(FALSE, iterbt->first, 0);
					}
				}
			}

//OutputDebugString("send2p  err->faild\n");			
			return -1;
		}
	}
	
	return ssize;
}

BOOL CCPartner::SendBM(BYTE *puchBuf, int nSize)
{
	if(puchBuf == NULL || nSize <= 0 || (m_socket <= 0 && m_socketTCP <= 0) || m_puchSendBuf == NULL)
	{
		return FALSE;
	}

	if(JVN_PTBUFSIZE - m_nSendPackLen > nSize)
	{
//char ch[100];
//sprintf(ch,"sendbm...........%d  packlen:%d\n",nSize, m_nSendPackLen+nSize);
//OutputDebugString(ch);
		memcpy(&m_puchSendBuf[m_nSendPackLen], puchBuf, nSize);
		m_nSendPackLen += nSize;

		return TRUE;
	}
	
	return FALSE;
}

BOOL CCPartner::SendBMD(CCPartnerCtrl *pPCtrl)
{
	if(m_puchSendBuf == NULL || (m_socket <= 0 && m_socketTCP <= 0) || m_puchSendBuf == NULL)
	{
		return FALSE;
	}

SENDPACK:	
	if(m_nSendPackLen > 0 && m_nSendPackLen < 102400 && m_nSendPos >= 0 && m_nSendPos < 102400)
	{//正常数据
		int ss = Send2Partner(&m_puchSendBuf[m_nSendPos], m_nSendPackLen-m_nSendPos, pPCtrl, 0, FALSE);
		if(ss <= 0)
		{//数据发送失败 说明当前节点的传输受到阻碍 结束此次发送
			return TRUE;
		}

		//数据发送成功 说明当前节点传输无阻 继续发送充分利用
		m_nSendPos += ss;
		if(m_nSendPos >= m_nSendPackLen)
		{//已发送完毕
			//更新本地上传统计
			m_nUpTotalB += m_nSendPackLen;
			m_nUpTotalKB += m_nUpTotalB/1024;//满KB
			m_nUpTotalB = m_nUpTotalB%1024;
			m_nUpTotalMB += m_nUpTotalKB/1000;//满MB
			m_nUpTotalKB = m_nUpTotalKB%1000;

			m_nSendPackLen = 0;
			m_nSendPos = 0;
			
			//继续读取新数据发送
			int ncount = m_PushQueue.size();
			for(int i=0; i<ncount; i++)
			{
				m_nSendPackLen = 0;
				m_nSendPos = 0;

				if(m_pChannel->m_bDAndP || m_pChannel->m_pBuffer == NULL)
				{//数据发送已暂停 
					//数据类型(1) + 数据总长度(4) + ChunkID(4)
					m_puchSendBuf[0] = JVN_RSP_BMDNULL;
					m_nSendPackLen = 4;
					memcpy(&m_puchSendBuf[1], &m_nSendPackLen, 4);
					memcpy(&m_puchSendBuf[5], &(m_PushQueue[i].unChunkID), 4);
					m_nSendPackLen = 9;

					m_PushQueue.erase(m_PushQueue.begin()+i);
					ncount--;
					i--;

					goto SENDPACK;
				}
				
				m_pChannel->m_pBuffer->ReadREQData(m_PushQueue[i].unChunkID, m_puchSendBuf, m_nSendPackLen);
				//读取新请求成功 删除该请求记录 并开始发送
				m_PushQueue.erase(m_PushQueue.begin()+i);
				ncount--;
				i--;
					
				goto SENDPACK;
			}

			return TRUE;//没有新请求 结束
		}
		
		goto SENDPACK;//数据发送成功 说明当前节点的传输无阻碍 继续发送
	}
	else
	{//没有数据 或是当前数据无效 重新读取数据发送
		m_nSendPackLen = 0;
		m_nSendPos = 0;
		//继续读取新数据发送
		int ncount = m_PushQueue.size();
		for(int i=0; i<ncount; i++)
		{
			m_nSendPackLen = 0;
			m_nSendPos = 0;
			
			if(m_pChannel->m_bDAndP || m_pChannel->m_pBuffer == NULL)
			{//数据发送已暂停 
				//数据类型(1) + 数据总长度(4) + nChunkID(4)
				m_puchSendBuf[0] = JVN_RSP_BMDNULL;
				m_nSendPackLen = 4;
				memcpy(&m_puchSendBuf[1], &m_nSendPackLen, 4);
				memcpy(&m_puchSendBuf[5], &(m_PushQueue[i].unChunkID), 4);
				m_nSendPackLen = 9;
				
				m_PushQueue.erase(m_PushQueue.begin()+i);
				ncount--;
				i--;

				goto SENDPACK;
			}
			
			m_pChannel->m_pBuffer->ReadREQData(m_PushQueue[i].unChunkID, m_puchSendBuf, m_nSendPackLen);
			//读取新请求成功 删除该请求记录 并开始发送
			m_PushQueue.erase(m_PushQueue.begin()+i);
			ncount--;
			i--;
			
			goto SENDPACK;
		}
		
		return TRUE;//没有新请求 结束
	}
}

BOOL CCPartner::SendBMDREQ(unsigned int unChunkID, CCPartnerCtrl *pPCtrl)
{
	if((m_socket <= 0 && m_socketTCP <= 0) || unChunkID <= 0)
	{
		pPCtrl->SetReqStartTime(FALSE, unChunkID, 0);
		return FALSE;
	}
	
	//类型(1) + 长度(4) + ChunkID(4)
	int nSize = 4;
	BYTE data[10]={0};
	data[0] = JVN_REQ_BMD;
	memcpy(&data[1], &nSize, 4);
	nSize = 9;
	memcpy(&data[5], &unChunkID, 4);

	if(JVN_PTBUFSIZE - m_nSendPackLen > nSize)
	{
		memcpy(&m_puchSendBuf[m_nSendPackLen], data, nSize);
		m_nSendPackLen += nSize;
		
		//找出当前chunk 记录开始请求时间
		DWORD dwtime = CCWorker::JVGetTime();
		m_stMAP.ChunkBTMap.insert(::std::map<unsigned int, DWORD>::value_type(unChunkID, dwtime));
		pPCtrl->SetReqStartTime(FALSE, unChunkID, dwtime);
		return TRUE;
	}

	pPCtrl->SetReqStartTime(FALSE, unChunkID, 0);
	return TRUE;
}

//更新当前伙伴BM
void CCPartner::SetBM(unsigned int unChunkID, int nCount, BYTE *pBuffer, int nLen, CCPartnerCtrl *pPCtrl)
{
	if(unChunkID <= 0 || nCount <= 0 || ((pBuffer == NULL || nLen <= 0) && !m_bTURNC))
	{
		return;
	}

	unsigned int unlastid = m_stMAP.unBeginChunkID + m_stMAP.nChunkCount;
	if(unlastid > 0)
	{
		unlastid -= 1;
	}
	else
	{
		unlastid = 0;
	}
	unsigned int unnewid = unChunkID + nCount - 1;
	m_stMAP.unBeginChunkID = unChunkID;
	m_stMAP.nChunkCount = nCount;

	if(!m_bTURNC)
	{
		memcpy(m_stMAP.uchMAP, pBuffer, jvs_min(nLen, 5000));
	}
	else
	{
		memset(m_stMAP.uchMAP, 0xFF, jvs_min(nCount/8+1,5000));
	}

//	if(unnewid > unlastid)
//	{//本次有新数据块
//		pPCtrl->CheckIfNeedSetBuf(unChunkID, nCount);
//	}

	::std::map<unsigned int, DWORD>::iterator iterbt;
	for(iterbt = m_stMAP.ChunkBTMap.begin(); iterbt != m_stMAP.ChunkBTMap.end(); iterbt++)
	{//找到记录
		if(iterbt->first >= unChunkID-100)
		{
			break;
		}
	}
	//清理过期数据记录
	m_stMAP.ChunkBTMap.erase(m_stMAP.ChunkBTMap.begin(), iterbt);
}

//刷新当前伙伴性能参数
void CCPartner::RefreshPartner(unsigned int unChunkID, int nLen, BOOL bNULL)
{
	DWORD dwend = CCWorker::JVGetTime();
	m_dwLastDataTime = dwend;
	if(unChunkID <= 0)
	{
		return;
	}
	
	DWORD dwlast = 0;
//	int nlast = 0;

	m_nTimeOutCount = 0;//收到确认，重置超时次数

	::std::map<unsigned int, DWORD>::iterator iterbt;
	iterbt = m_stMAP.ChunkBTMap.find(unChunkID);
	if(iterbt != m_stMAP.ChunkBTMap.end())
	{//找到记录
		if(bNULL)
		{//当前帧已无效
			int noffset = unChunkID - m_stMAP.unBeginChunkID;
			if(noffset >= 0)
			{
				BYTE uchtemp = 0x01;
				BYTE uchtempFF = 0xFF;
				m_stMAP.uchMAP[(noffset/8)] = (m_stMAP.uchMAP[(noffset/8)] & (uchtempFF^(uchtemp<<(7-noffset%8))));
			}
		}
		else
		{
			if(iterbt->second > 0 && dwend > iterbt->second)
			{
				dwlast = dwend-iterbt->second;
				m_dwRUseTTime += dwlast;
				if(m_dwRUseTTime > 0)
				{//本地下行带宽/该伙伴的上行带宽(KB/S)
					m_nLPSAvage = (int)((float)((m_nDownLoadTotalMB - m_nLastDownLoadTotalMB)*1024000 + (m_nDownLoadTotalKB - m_nLastDownLoadTotalKB)*1024 + (m_nDownLoadTotalB-m_nLastDownLoadTotalB))/m_dwRUseTTime);
//char ch[100];
//sprintf(ch,"refresh.....%d %d / %d   lps:%d\n",dwlast, 
//			(m_nDownLoadTotalMB - m_nLastDownLoadTotalMB)*1024000 + (m_nDownLoadTotalKB - m_nLastDownLoadTotalKB)*1024 + (m_nDownLoadTotalB-m_nLastDownLoadTotalB),
//			m_dwRUseTTime,m_nLPSAvage);
//OutputDebugString(ch);
				}
			}
			m_stMAP.ChunkBTMap.erase(unChunkID);
		}
	}
}

//检查是否有该数据片，是否向该伙伴请求过当前数据片，是的话重置该请求，并更新伙伴性能
BOOL CCPartner::CheckREQ(unsigned int unChunkID, BOOL bTimeOut)
{
	if(unChunkID <= 0)
	{
		return FALSE;
	}
	
//	DWORD dwend=CCWorker::JVGetTime();
	int noffset = unChunkID - m_stMAP.unBeginChunkID;
	if(noffset >= 0 && noffset < m_stMAP.nChunkCount)
	{//在区间内
		BYTE uchtemp = 0x01;
//		BYTE uchtempFF = 0xFF;
		if((m_stMAP.uchMAP[noffset/8] & (uchtemp<<(7 - noffset%8))) > 0)
		{//对方有数据
			::std::map<unsigned int, DWORD>::iterator iterbt;
			iterbt = m_stMAP.ChunkBTMap.find(unChunkID);
			if(iterbt != m_stMAP.ChunkBTMap.end())
			{//找到记录，也就是上次向该节点发过请求
				if(bTimeOut && iterbt->second > 0)
				{
					m_nTimeOutCount++;//该伙伴请求超时了一次
				}
				m_stMAP.ChunkBTMap.erase(unChunkID);

				return FALSE;
			}
			return TRUE;
		}
	}

	return FALSE;
}

void CCPartner::ResetPack(BOOL bAccept)
{
	m_nSendPackLen = 0;
	m_nSendPos = 0;
	m_dwSendPackLast = 0;

	m_nRecvPackLen = 0;
	m_nRecvPos = 0;
	m_dwRecvPackLast = 0;

	m_bAccept = bAccept;

	m_dwLastDataTime = 0;//最后一次接收数据时间复位
}

void CCPartner::GetPInfo(char *pMsg, int &nSize, DWORD dwend)
{
	dwend = CCWorker::JVGetTime();
	STPTINFO stinfo;

	sprintf(stinfo.chIP,"%s",inet_ntoa(m_addr.sin_addr));
	
	stinfo.uchStatus = ((m_nstatus == PNODE_STATUS_CONNECTED)?1:0);
	stinfo.uchType = (m_bAccept?PTYPE_A:(m_bLan2B?PTYPE_L:PTYPE_N));
	stinfo.uchProcType = (m_bTCP?1:0);

	if(!m_bTURNC)
	{
		stinfo.nPort = ntohs(m_addr.sin_port);
		stinfo.uchType = stinfo.uchType;//(m_bTURNC?6:stinfo.uchType);
	}
	else
	{
		stinfo.nPort = 0;//ntohs(m_addr.sin_port);
		stinfo.uchType = 6;//(m_bTURNC?6:stinfo.uchType);
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
	stinfo.fUpTotal = m_nUpTotalMB + (float)((float)m_nUpTotalKB/1000) + (float)((float)m_nUpTotalB/1024000);//M
	
	memcpy(&pMsg[nSize], &stinfo, sizeof(STPTINFO));
	nSize += sizeof(STPTINFO);

	m_dwLastInfoTime = dwend;
	m_nLastDownLoadTotalMB = m_nDownLoadTotalMB;
	m_nLastDownLoadTotalKB = m_nDownLoadTotalKB;
	m_nLastDownLoadTotalB = m_nDownLoadTotalB;
}

float CCPartner::GetPower()
{
	float fDownTotal = m_nDownLoadTotalMB*1000 + (float)((float)m_nDownLoadTotalKB/1000) + (float)((float)m_nDownLoadTotalB/1000);//M
	float fUpTotal = m_nUpTotalMB*1000 + (float)((float)m_nUpTotalKB/1000) + (float)((float)m_nUpTotalB/1000);//M
	float ftotal = fDownTotal + fUpTotal;

	float fpower = 0;
	DWORD dwend=CCWorker::JVGetTime();
	if(dwend > m_dwLastDataTime + PT_TIME_NODATA)
	{
		fpower = 0;
	}
	else if(dwend > m_dwLastConnTime)
	{
		fpower = (int)((float)(ftotal-m_fLastConnTotal)*1000/(dwend-m_dwLastConnTime));// KB/S
	}

	return fpower;
}

void CCPartner::DisConnectPartner()
{
	if(m_nSendPackLen <= 0)
	{
		BYTE data[5]={0};
		data[0] = JVN_CMD_DISCONN;
		Send2Partner(data, 5, NULL,100);
	}

	if(m_socket > 0)
	{
		UDT::close(m_socket);
	}
	m_socket = 0;

	if(m_socketTCP > 0)
	{
		closesocket(m_socketTCP);
	}
	m_socketTCP = 0;

//OutputDebugString("close 16\n");		
	m_nstatus = PNODE_STATUS_FAILD;
	m_nConnFCount++;
	ResetPack();
}


BOOL CCPartner::SendSTURN(SOCKADDR_IN addrs)
{
	if(m_pWorker->m_WorkerUDPSocket <= 0 || m_pChannel->m_stConnInfo.nYSTNO <= 0)
	{
		return FALSE;
	}
	
	//创建临时UDP套接字
	SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
	
	SOCKADDR_IN addrSrv;
#ifndef WIN32
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(0);
	
	//绑定套接字
	bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));

	//向服务器请求TS地址
	//向S发送请求
	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = JVN_REQ_S2;
	memcpy(&data[0], &nType, 4);
	memcpy(&data[4], &m_pChannel->m_stConnInfo.nYSTNO, 4);
	
	if(CCChannel::sendtoclient(stmp, (char *)data, 8, 0, (SOCKADDR *)&(addrs), sizeof(SOCKADDR),3) > 0)
	{
		if(m_socketTCtmp > 0)
		{
			closesocket(m_socketTCtmp);
		}
		m_socketTCtmp = stmp;
		m_nstatus = PNODE_STATUS_TCWAITTS;
		return TRUE;
	}
	else
	{	
		if(m_socketTCtmp > 0)
		{
			closesocket(m_socketTCtmp);
		}
		m_socketTCtmp = stmp;

		closesocket(stmp);
		return FALSE;
	}
}

int CCPartner::RecvSTURN()
{
	if(m_socketTCtmp <= 0 || m_pChannel->m_stConnInfo.nYSTNO <= 0)
	{
		return -1;
	}

	BYTE data[JVN_ASPACKDEFLEN]={0};
	int nType = JVN_CMD_CONNS2;
	memset(data, 0, JVN_ASPACKDEFLEN);
	SOCKADDR_IN addrtemp;
	int naddrlen = sizeof(SOCKADDR);
	if(CCChannel::receivefrom(m_socketTCtmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 1) > 0)
	{
		memcpy(&nType, &data[0], 4);
		if(nType == JVN_CMD_CONNS2)
		{//收到服务器成功数据
			memcpy(&m_pChannel->m_addrTS, &data[8], sizeof(SOCKADDR_IN));

			if(m_socket > 0)
			{
				UDT::close(m_socket);
				m_socket = 0;
			}

			m_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
			//////////////////////////////////////////////////////////////////////////
			int len1 = JVC_MSS;
			UDT::setsockopt(m_socket, 0, UDT_MSS, &len1, sizeof(int));
			//////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
            len1=1500*1024;
            UDT::setsockopt(m_socket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
            
            len1=1000*1024;
            UDT::setsockopt(m_socket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
			if (UDT::ERROR == UDT::bind(m_socket, m_pWorker->m_WorkerUDPSocket))
			{
				if(m_socket > 0)
				{
					UDT::close(m_socket);
				}
				m_socket = 0;
				
				return JVN_RSP_CONNAF;
			}

			//将套接字置为非阻塞模式
			BOOL block = FALSE;
			UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
			UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
			LINGER linger;
			linger.l_onoff = 0;
			linger.l_linger = 0;
			UDT::setsockopt(m_socket, 0, UDT_LINGER, &linger, sizeof(LINGER));
			
			return JVN_CMD_CONNS2;
		}
		else if(nType == JVN_RSP_CONNAF)
		{//收到服务器失败数据
			if(m_socket > 0)
			{
				UDT::close(m_socket);
			}
			m_socket = 0;

			return JVN_RSP_CONNAF;
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

BOOL CCPartner::ConnectTURN()
{
	if(m_pWorker->m_WorkerUDPSocket <= 0 || m_pChannel->m_stConnInfo.nYSTNO <= 0)
	{
		return FALSE;
	}
	
//	char ch[1000]={0};
//	sprintf(ch,"%s:%d\n",inet_ntoa(m_pChannel->m_addrTS.sin_addr), ntohs(m_pChannel->m_addrTS.sin_port));
//OutputDebugString(ch);
	memcpy(&m_addr, &m_pChannel->m_addrTS, sizeof(SOCKADDR_IN));

	STJUDTCONN stcon;
	stcon.u = m_socket;
	stcon.name = (SOCKADDR *)&m_pChannel->m_addrTS;
	stcon.namelen = sizeof(SOCKADDR);
	stcon.nChannelID = m_pChannel->m_stConnInfo.nChannel;
	memcpy(stcon.chGroup, m_pChannel->m_stConnInfo.chGroup, 4);
	stcon.nYSTNO = m_pChannel->m_stConnInfo.nYSTNO;
	stcon.nPTLinkID = m_ndesLinkID;
	stcon.nPTYSTNO = m_pChannel->m_stConnInfo.nYSTNO;
	stcon.nPTYSTADDR = (int)m_pChannel->m_addressA.sin_addr.s_addr;
	stcon.nLVer_new = JVN_YSTVER;
	stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
	stcon.nMinTime = 3000;
	if(UDT::ERROR == UDT::connect(stcon))
	{
		if(m_socket > 0)
		{
			UDT::close(m_socket);
		}
		m_socket = 0;
		
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL CCPartner::SendReCheck(BOOL bYST)
{//类型(1)+长度(4)+连接类型(1)+云视通号码(4)+是否作为高速缓存(1)+是否提速连接(1)
	if(m_pWorker->m_WorkerUDPSocket <= 0 || m_pChannel->m_stConnInfo.nYSTNO <= 0)
	{
		return -1;
	}
	int nLen = 7;
	BYTE data[20]={0};
	data[0] = JVN_REQ_RECHECK;
	memcpy(&data[1], &nLen, 4);
	if(bYST)
	{
		data[5] = 1;
		memcpy(&data[6], &m_pChannel->m_stConnInfo.nYSTNO, 4);
	}
//	if(m_pChannel->m_stConnInfo.bCache)
//	{
//		data[10] = 1;
//		m_bCache = TRUE;
//	}
//	else
//	{
		m_bCache = FALSE;
//	}

	data[11] = 1;
	
	if(0 >= UDT::send(m_socket, (char *)data, 12, 0))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


BOOL CCPartner::SendPWCheck()
{//类型(1)+长度(4)+用户名长(4)+密码长(4)+用户名+密码
	if(m_pWorker->m_WorkerUDPSocket <= 0 || m_pChannel->m_stConnInfo.nYSTNO <= 0)
	{
		return FALSE;
	}
	
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
	
	if(0 >= UDT::send(m_socket, (char *)data, nNLen + nWLen + 9, 0))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}








