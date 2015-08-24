// CPartnerCtrl.cpp: implementation of the CCPartnerCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "CPartnerCtrl.h"
#include "CChannel.h"
#include "CWorker.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCPartnerCtrl::CCPartnerCtrl()
{
	m_PList.reserve(30);
	m_PList.clear();

	m_PLINKList.reserve(30);
	m_PLINKList.clear();

	m_bClearing = FALSE;

	m_dwLastPTData = CCWorker::JVGetTime();

	m_pChannel = NULL;

	m_unChunkIDNew = 0;
	m_unChunkIDOld = 0;
	m_ChunkBTMap.clear();
	
	m_dwLastREQListTime = 0;
	m_dwLastTryNewLTime = 0;
	m_dwLastTryNewNTime = 0;
	m_dwLastDisOldLTime = 0;
	m_dwLastDisOldNTime = 0;
#ifndef WIN32
	pthread_mutex_init(&m_ct, NULL); //初始化临界区
	pthread_mutex_init(&m_ctCONN, NULL); //初始化临界区
	pthread_mutex_init(&m_ctPTINFO, NULL); //初始化临界区
#else
	InitializeCriticalSection(&m_ct); //初始化临界区
	InitializeCriticalSection(&m_ctCONN); //初始化临界区
	InitializeCriticalSection(&m_ctPTINFO); //初始化临界区
#endif
}

CCPartnerCtrl::~CCPartnerCtrl()
{
	ClearPartner();

#ifndef WIN32
	pthread_mutex_destroy(&m_ctPTINFO); //释放临界区
	pthread_mutex_destroy(&m_ctCONN); //释放临界区
	pthread_mutex_destroy(&m_ct);//释放临界区
#else
	DeleteCriticalSection(&m_ctPTINFO); //释放临界区
	DeleteCriticalSection(&m_ctCONN); //释放临界区
	DeleteCriticalSection(&m_ct); //释放临界区
#endif
}

BOOL CCPartnerCtrl::SetPartnerList(PartnerIDList partneridlist)
{
//OutputDebugString("ptcrl setplist.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl setplist.....1\n");
	//检查新列表和就列表的差异，多余的断开连接，已存在的保留，没有的添加
	int ncountnew = partneridlist.size();
	int ncountl = m_PList.size();
	BOOL bfind=FALSE;
	
	//清除多余节点
	int i=0;
	int j=0;
	for(j=0; j<ncountl; j++)
	{
		bfind = FALSE;
		if(m_PList[j] == NULL || (m_PList[j]->m_ndesLinkID <= 0 && !m_PList[j]->m_bTURNC) || m_PList[j]->m_bTURNC)
		{
			continue;
		}

		//检查本地某个节点是否在最新列表中
		for(i=0; i<ncountnew; i++)
		{
			if(m_PList[j]->m_ndesLinkID == partneridlist[i].nLinkID)
			{
				bfind = TRUE;
				break;
			}
		}
		
		if(!bfind)
		{//多余伙伴清除
			m_PList[j]->m_nstatus = PNODE_STATUS_FAILD;
			m_PList[j]->DisConnectPartner();
			m_PList[j]->m_ndesLinkID = 0;
			continue;
		}

		//清除重复的节点
		for(i=j+1; i<ncountl; i++)
		{
			if(m_PList[j]->m_ndesLinkID == m_PList[i]->m_ndesLinkID)
			{//重复节点
				if(m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD)
				{//删除空闲节点
					m_PList[i]->m_ndesLinkID = 0;
					continue;
				}
				else if(m_PList[j]->m_nstatus == PNODE_STATUS_NEW || m_PList[j]->m_nstatus == PNODE_STATUS_FAILD)
				{
					m_PList[j]->m_ndesLinkID = 0;
					break;
				}
				else
				{
					m_PList[i]->m_nstatus = PNODE_STATUS_FAILD;
					m_PList[i]->DisConnectPartner();
					m_PList[i]->m_ndesLinkID = 0;
					continue;
				}
			}
		}
	}
	
	//有效节点更新
	ncountl = m_PList.size();
	for(i=0; i<ncountnew; i++)
	{
		bfind = FALSE;
		for(j=0; j<ncountl; j++)
		{
			if(partneridlist[i].nLinkID == m_PList[j]->m_ndesLinkID)
			{//旧伙伴保留
				m_PList[j]->m_bSuperProxy = partneridlist[i].bIsSuper;
				m_PList[j]->m_bLan2A = partneridlist[i].bIsLan2A;
				m_PList[j]->m_bLan2B = partneridlist[i].bIsLan2B;
				bfind = TRUE;
				break;
			}
		}

		if(!bfind)
		{//新伙伴加入
			CCPartner *p = new CCPartner(partneridlist[i], m_pWorker, m_pChannel);
			m_PList.push_back(p);
			ncountl = m_PList.size();
		}
	}
	std::random_shuffle(m_PList.begin(), m_PList.end());//打乱顺序，防止因为先后关系总是选中某一个
/*
char ch[100];
sprintf(ch,"client.............lan2A:%d\n",m_pChannel->m_bLan2A);
OutputDebugString(ch);
for(i=0; i<partneridlist.size(); i++)
{
	sprintf(ch,"cache:%d lan2A:%d  lan2B:%d  super:%d IP:%s\n",
		partneridlist[i].bIsCache,
		partneridlist[i].bIsLan2A,
		partneridlist[i].bIsLan2B,
		partneridlist[i].bIsSuper,
		inet_ntoa(partneridlist[i].sAddr.sin_addr));
	OutputDebugString(ch);
}
*/
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}

//进行伙伴连接
BOOL CCPartnerCtrl::PartnerLink(BOOL bExit)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctCONN);
#else
	EnterCriticalSection(&m_ctCONN);
#endif

	int ncount = m_PLINKList.size();
	for(int i=0; i<ncount; i++)
	{
		if(bExit || m_bClearing)
		{
			break;
		}

		if(m_PLINKList[i] != NULL && m_PLINKList[i]->m_bTryNewLink)
		{//可进行连接尝试
			m_PLINKList[i]->PartnerLink(this);
			m_PLINKList[i]->m_bTryNewLink = FALSE;
		}

		//从列表中清理掉已尝试过的节点
	#ifndef WIN32
		pthread_mutex_lock(&m_ct);
	#else
		EnterCriticalSection(&m_ct);
	#endif
		
		m_PLINKList.erase(m_PLINKList.begin()+i);

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif

		i--;
		ncount--;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctCONN);
#endif

	return TRUE;
}

//检查伙伴连接情况 进行伙伴连接
BOOL CCPartnerCtrl::CheckPartnerLinks(BOOL bExit)
{
	if(bExit || m_bClearing)
	{
		return TRUE;
	}
//OutputDebugString("ptcrl checklink.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl checklink.....1\n");

	BOOL bhaveTURNpt = FALSE;//含有提速连接节点
	int nactiveL = 0;//内网主动连接数
	int nactiveN = 0;//外网主动连接数
	DWORD dwendtime = 0;
	DWORD dwlastdatatime=0;
	DWORD dwtmp=0;
	::std::map<unsigned int, unsigned int> PTaddrMap;
	PTaddrMap.clear();
	//检查memlist，逐个检查节点状态：
	int ncount = m_PList.size();
	if(ncount > 0)
	{//有伙伴
		//完成连接过程，并计算出当前活动连接数
		for(int i=0; i<ncount; i++)
		{
			if(bExit || m_bClearing)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return TRUE;
			}
			if(m_PList[i] == NULL || (m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC))
			{
				continue;
			}

			int nret = m_PList[i]->CheckLink(this, FALSE, dwlastdatatime);//<0未连接 0连接中 1连接成功
			if(m_PList[i]->m_bTURNC)
			{
				bhaveTURNpt = TRUE;
				continue;
			}
 
			if(nret >= 0)
			{//连接成功或是正在连接
				if(!m_PList[i]->m_bAccept && m_PList[i]->m_bLan2B)
				{//内网伙伴
					nactiveL++;
				}
				else if(!m_PList[i]->m_bAccept && !m_PList[i]->m_bLan2B)
				{//外网伙伴
					nactiveN++;
				}

				//记录该地址
				::std::map<unsigned int, unsigned int>::iterator iter;
				iter = PTaddrMap.find(m_PList[i]->m_unADDR);
				if(iter == PTaddrMap.end())
				{//未找到
					PTaddrMap.insert(::std::map<unsigned int, unsigned int>::value_type(m_PList[i]->m_unADDR, m_PList[i]->m_unADDR));
				}
			}

			//找出最短数据时间
			if(dwlastdatatime > 0 && dwlastdatatime < dwtmp)
			{
				dwtmp = dwlastdatatime;
			}
		}
//char ch[100];
//sprintf(ch,"ptcrl ncount:%d active:%d\n",ncount,nactive);
//OutputDebugString(ch);
		dwendtime = CCWorker::JVGetTime();
		if(dwendtime > dwtmp + PT_TIME_NODATA)
		{//如果最短数据时间都超过了1min，说明节点极差，立即更换 不再按常规周期
			m_dwLastDisOldLTime = 0;
			m_dwLastDisOldNTime = 0;
		}

		//内网主动连接检查
		if(nactiveL <= PT_LCONNECTMAX || m_pChannel->m_bCache)
		{//内网活动连接数未达到上限 继续尝试新连接
			if(dwendtime > PT_TIME_CNNEWPT + m_dwLastTryNewLTime)
			{
				m_dwLastTryNewLTime = dwendtime;
	
				//只寻找内网节点进行连接
				int nnewindex = -1;
				ncount = m_PList.size();
				int nFCcount = 0;
				for(int i=0; i<ncount; i++)
				{
					if(m_PList[i]->m_ndesLinkID > 0 
					   && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
					   && m_PList[i]->m_bLan2B
					   && !m_PList[i]->m_bTURNC)
					{//找到未连接的同网节点
						if(nnewindex >= 0)
						{//已至少找到过一个可选节点，有参考比较对象
							if(m_PList[i]->m_nConnFCount <= nFCcount)
							{//连接失败次数少 意味着连接成功机会也就更高
								nnewindex = i;
								nFCcount = m_PList[i]->m_nConnFCount;
							}
						}
						else
						{//还没有可选节点也就没有参考值，直接将当前这个作为参考值
							nnewindex = i;
							nFCcount = m_PList[i]->m_nConnFCount;
						}
					}
				}
				if(nnewindex >= 0)
				{//找到连接失败次数最少的内网节点进行连接
					//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
					m_PList[nnewindex]->m_bTryNewLink = TRUE;
					m_PLINKList.push_back(m_PList[nnewindex]);//将该伙伴加入连接队列
				}
			}
		}
		else
		{//内网活动连接数达到上限 断开最差伙伴
			if(dwendtime > PT_TIME_DISBADPT + m_dwLastDisOldLTime)
			{
				m_dwLastDisOldLTime = dwendtime;

				ncount = m_PList.size();
				float flittle = -1;
				float ftmp = -1;
				int nlittleindex = -1;
				for(int i=0; i<ncount; i++)
				{
					if(m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && dwendtime > m_PList[i]->m_dwLastConnTime + PT_TIME_DISBADPT && m_PList[i]->m_bLan2B)
					{//找到更差节点 只针对连接超过2分钟节点可保护新连接
						ftmp = m_PList[i]->GetPower();
						if(flittle < 0 || ftmp <= flittle)
						{//找到更弱的
							flittle = ftmp;
							nlittleindex = i;
						}
					}
				}
				if(nlittleindex >= 0)
				{//最差节点断开
					m_PList[nlittleindex]->m_nstatus = PNODE_STATUS_FAILD;
					m_PList[nlittleindex]->DisConnectPartner();
//OutputDebugString("checklink nlittleindex->faild\n");
				}
			}
		}

		if(!m_pChannel->m_bLan2A)
		{//如果本机与主控同内网 则不会有外网伙伴 不需要检查
			//外网主动连接检查
			if(nactiveN <= PT_NCONNECTMAX || m_pChannel->m_bCache || bhaveTURNpt)
			{//活动连接数未达到上限 本地是全速上传节点 或是 本地需要提速连接 继续尝试新连接
				if(dwendtime > PT_TIME_CNNEWPT + m_dwLastTryNewNTime)
				{
					m_dwLastTryNewNTime = dwendtime;
					
					//找出一个超级节点优先进行连接
					BOOL bHaveNew = FALSE;
					int nFCcount = 0;
					int nnewindex = -1;
					ncount = m_PList.size();
					for(int i=0; i<ncount; i++)
					{
						if(m_PList[i]->m_ndesLinkID > 0 
							&& (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
							&& m_PList[i]->m_bSuperProxy 
							&& !m_PList[i]->m_bLan2A
							&& !m_PList[i]->m_bTURNC)
						{//找到未连接的外网超级节点
							
							::std::map<unsigned int, unsigned int>::iterator iter;
							iter = PTaddrMap.find(m_PList[i]->m_unADDR);
							if(iter != PTaddrMap.end())
							{//找到该IP已连接或正在连接,跳过 防止对同一内网进行多次连接
								continue;
							}

							if(nnewindex >= 0)
							{//已至少找到过一个可选节点，有参考比较对象
								if(m_PList[i]->m_nConnFCount <= nFCcount)
								{//连接失败次数少 意味着连接成功机会也就更高
									nnewindex = i;
									nFCcount = m_PList[i]->m_nConnFCount;
								}
							}
							else
							{//还没有可选节点也就没有参考值，直接将当前这个作为参考值
								nnewindex = i;
								nFCcount = m_PList[i]->m_nConnFCount;
							}

							if(m_PList[i]->m_nConnFCount <= 0)
							{//该节点还没有失败过 确定不需要尝试普通节点
								bHaveNew = TRUE;
							}
						}
					}
					
					if(nnewindex >= 0)
					{//找到代理节点
						//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
						m_PList[nnewindex]->m_bTryNewLink = TRUE;
						m_PLINKList.push_back(m_PList[nnewindex]);//将该伙伴加入连接队列
					}

					if(nnewindex < 0 || (nnewindex >= 0 && !bHaveNew))
					{//没有代理节点 必须尝试普通节点
					 //有代理节点，但如果代理节点都增经失败过，就有可能代理节点都连不上，必须尝试普通节点
						nFCcount = 0;
						nnewindex = -1;
						for(int i=0; i<ncount; i++)
						{
							if(m_PList[i]->m_ndesLinkID > 0 
							   && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
							   && !m_PList[i]->m_bLan2A
							   && !m_PList[i]->m_bTURNC)
							{//找到未连接的普通节点
								::std::map<unsigned int, unsigned int>::iterator iter;
								iter = PTaddrMap.find(m_PList[i]->m_unADDR);
								if(iter != PTaddrMap.end())
								{//找到该IP已连接或正在连接,跳过 防止对同一内网进行多次连接
									continue;
								}

								if(nnewindex >= 0)
								{//已至少找到过一个可选节点，有参考比较对象
									if(m_PList[i]->m_nConnFCount <= nFCcount)
									{//连接失败次数少 意味着连接成功机会也就更高
										nnewindex = i;
										nFCcount = m_PList[i]->m_nConnFCount;
									}
								}
								else
								{//还没有可选节点也就没有参考值，直接将当前这个作为参考值
									nnewindex = i;
									nFCcount = m_PList[i]->m_nConnFCount;
								}
							}
						}

						if(nnewindex >= 0)
						{//找到普通节点
							//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
							m_PList[nnewindex]->m_bTryNewLink = TRUE;
							m_PLINKList.push_back(m_PList[nnewindex]);//将该伙伴加入连接队列
						}
					}

					//强制连接提速节点
					if(bhaveTURNpt)
					{
						for(int i=0; i<ncount; i++)
						{
							if(m_PList[i]->m_bTURNC && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD))
							{//找到未连接的提速节点
								if(m_PList[i]->m_nConnFCount <= 2)
								{//连接失败次数较多，没必要继续连接了
									//m_PList[i]->CheckLink(this, TRUE, dwlastdatatime);
									m_PList[i]->m_bTryNewLink = TRUE;
									m_PLINKList.push_back(m_PList[i]);//将该伙伴加入连接队列
									break;
								}
							}
						}
					}
				}
			}
			else
			{//活动连接数达到上限 断开最差伙伴
				if(dwendtime > PT_TIME_DISBADPT + m_dwLastDisOldNTime)
				{
					m_dwLastDisOldNTime = dwendtime;
					
					ncount = m_PList.size();
					float flittle = -1;
					float ftmp = -1;
					int nlittleindex = -1;
					for(int i=0; i<ncount; i++)
					{
						if(m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && dwendtime > m_PList[i]->m_dwLastConnTime + PT_TIME_DISBADPT && !m_PList[i]->m_bLan2B)
						{//找到更差外网节点 只针对连接超过2分钟节点可保护新连接
							ftmp = m_PList[i]->GetPower();
							if(flittle < 0 || ftmp <= flittle)
							{//找到更弱的
								flittle = ftmp;
								nlittleindex = i;
							}
						}
					}
					if(nlittleindex >= 0)
					{//最差节点断开
						m_PList[nlittleindex]->m_nstatus = PNODE_STATUS_FAILD;
						m_PList[nlittleindex]->DisConnectPartner();
//OutputDebugString("checklink nlittleindex->faild\n");
					}
				}
			}
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl ncount<=0 \n");
	dwendtime = CCWorker::JVGetTime();
	if(dwendtime > PT_TIME_REQPTLIST + m_dwLastREQListTime)
	{//判断上次请求伙伴列表已过去多久，每隔2分钟尝试一次
		if(m_dwLastREQListTime > 0)
		{
			m_pChannel->SendData(JVN_CMD_PLIST, NULL, 0);
		}
		m_dwLastREQListTime = dwendtime;
	}

	return TRUE;
}

//接收到一个伙伴连接 后续判断处理
void CCPartnerCtrl::AcceptPartner(UDTSOCKET socket, SOCKADDR_IN clientaddr, int nDesID, BOOL bTCP)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	BOOL bFind=FALSE;
	int ncount = m_PList.size();
	int nacceptL = 0;
	int nacceptN = 0;
	unsigned int tmp = ntohl(clientaddr.sin_addr.s_addr);
	int i=0;
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
		{
			continue;
		}

		if((m_PList[i]->m_nstatus != PNODE_STATUS_NEW && m_PList[i]->m_nstatus != PNODE_STATUS_FAILD) && m_PList[i]->m_bAccept)
		{//统计被动连接总数
			if(m_PList[i]->m_bLan2B)
			{
				nacceptL++;
			}
			else
			{
				nacceptN++;
			}
		}
	}

	if(CheckInternalIP(ntohl(clientaddr.sin_addr.s_addr)))
	{//内网连接
		if(nacceptL >= PT_LACCEPTMAX)
		{//内网被动连接达到上限 不接受新连接
			if(bTCP)
			{
				closesocket(socket);
			}
			else
			{
				UDT::close(socket);
			}
			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif

//OutputDebugString("acceptpartner nacceptL > LMAX\n");
			return;
		}
	}
	else
	{//外网连接
		if(nacceptN >= PT_NACCEPTMAX && !m_pChannel->m_bCache)
		{//被动连接达到上限 不接受新连接
			if(bTCP)
			{
				closesocket(socket);
			}
			else
			{
				UDT::close(socket);
			}
			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif

//OutputDebugString("acceptpartner nacceptN > NMAX\n");
			return;
		}

		//检查当前地址是否已在连接中或是已连接成功 若是的话不接受该连接 防止同内网多个连接
		for(i=0; i<ncount; i++)
		{
			if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
			{
				continue;
			}
			
			if((m_PList[i]->m_nstatus != PNODE_STATUS_NEW && m_PList[i]->m_nstatus != PNODE_STATUS_FAILD) && tmp == m_PList[i]->m_unADDR)
			{//相同的ip已连接过或是正在连接 不接受此次连接
				if(bTCP)
				{
					closesocket(socket);
				}
				else
				{
					UDT::close(socket);
				}
				
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
//OutputDebugString("acceptpartner IP exist\n");
				return;
			}
		}
	}

	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
		{
			continue;
		}

		if(nDesID == m_PList[i]->m_ndesLinkID)
		{//找到该伙伴记录
			bFind = TRUE;
			
			if(m_PList[i]->m_nstatus == PNODE_STATUS_NEW 
				|| m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTING
				|| m_PList[i]->m_nstatus == PNODE_STATUS_FAILD)
			{
				//其他情况
				if(m_PList[i]->m_socketTCP > 0)
				{
					closesocket(m_PList[i]->m_socketTCP);
				}
				m_PList[i]->m_socketTCP = 0;

				if(m_PList[i]->m_socket > 0)
				{
					UDT::close(m_PList[i]->m_socket);
				}
				m_PList[i]->m_socket = 0;

				m_PList[i]->m_bTCP = bTCP;
				if(bTCP)
				{
					m_PList[i]->m_socketTCP = socket;
				}
				else
				{
					m_PList[i]->m_socket = socket;
				}
				
				m_PList[i]->m_nstatus = PNODE_STATUS_ACCEPT;
				m_PList[i]->m_dwstart = CCWorker::JVGetTime();

				m_PList[i]->m_bAccept = TRUE;
				
				memcpy(&m_PList[i]->m_addr, &clientaddr, sizeof(SOCKADDR_IN));

				break;
			}
			else
			{//本地已连接成功或正在连接，放弃当前连接
				if(bTCP)
				{
					closesocket(socket);
				}
				else
				{
					UDT::close(socket);
				}
				
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif

//OutputDebugString("acceptpartner connecting or connected\n");
				return;
			}
		}
	}
	
	if(!bFind)
	{//现有伙伴列表中没有该记录，直接添加
		STPTLI pttmp;
		pttmp.nLinkID = nDesID;
		pttmp.bIsSuper = FALSE;
		pttmp.bIsLan2A = FALSE;
		pttmp.bIsCache = FALSE;
		pttmp.bIsTC = FALSE;
		memcpy(&pttmp.sAddr, &clientaddr, sizeof(SOCKADDR_IN));
		//设置内外网标志
		if(CheckInternalIP(ntohl(clientaddr.sin_addr.s_addr)))
		{
			pttmp.bIsLan2B = TRUE;
		}
		else
		{
			pttmp.bIsLan2B = FALSE;
		}

		CCPartner *p = new CCPartner(pttmp, m_pWorker, m_pChannel, clientaddr, socket, bTCP);
			
		m_PList.push_back(p);
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
}

//需要提速转发连接
void CCPartnerCtrl::AddTurnCachPartner()
{
	int nc = m_pChannel->m_SList.size();
	int i=0;
	for(i=0; i<nc; i++)
	{
		m_pChannel->m_SList[i].buseful = TRUE;
	}

	//先检查是否已经有了提速连接，不需要重复建立
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			continue;
		}

		if(m_PList[i]->m_bTURNC)
		{//存在提速连接
			m_PList[i]->m_nConnFCount = 0;

		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return;
		}
	}

	//当前还没有提速节点，创建一个
	STPTLI pttmp;
	pttmp.nLinkID = 0;
	pttmp.bIsSuper = FALSE;
	pttmp.bIsLan2A = FALSE;
	pttmp.bIsCache = FALSE;
	pttmp.bIsTC = TRUE;
	pttmp.bIsLan2B = FALSE;
	
	CCPartner *p = new CCPartner(pttmp, m_pWorker, m_pChannel);
	
	m_PList.push_back(p);

	//创建提速连接节点，节点的具体连接在连接函数中控制
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

BOOL CCPartnerCtrl::RecvFromPartners(BOOL bExit, HANDLE hEnd)
{
//OutputDebugString("ptcrl recvfromp.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl recvfromp.....1\n");
//	int nLen = 0;
	int ncount = m_PList.size();
	int i=0;
	for(i=0; i<ncount; i++)
	{
	#ifndef WIN32
		if(bExit)
	#else
		if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
	#endif
		{//线程需要退出 向所有伙伴通知断开连接
			ncount = m_PList.size();
			for(i=0; i<ncount; i++)
			{
				if(m_PList[i] != NULL && ((m_PList[i]->m_ndesLinkID > 0 && !m_PList[i]->m_bTURNC) || m_PList[i]->m_bTURNC))
				{
					m_PList[i]->DisConnectPartner();

					m_PList[i]->m_ndesLinkID = 0;

					m_PList[i]->m_bTURNC = FALSE;//置成普通节点，便于垃圾清理
				}
			}
				
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return FALSE;
		}

		if(m_bClearing)
		{
			break;
		}

		if(m_PList[i] == NULL || (m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC))
		{
			continue;
		}

		m_PList[i]->BaseRecv(this);
	}

	if(bExit)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

//读取BM并向全部或部分伙伴发送BM
BOOL CCPartnerCtrl::SendBM2Partners(BYTE *puchBuf, int nSize,BOOL bExit, HANDLE hEnd)
{
	if(puchBuf != NULL && nSize > 0)
	{
		if(!m_pChannel->m_bCache)
		{//普通伙伴，可向所有非高速缓存连接广播
		#ifndef WIN32
			pthread_mutex_lock(&m_ct);
		#else
			EnterCriticalSection(&m_ct);
		#endif
			
			int ncount = m_PList.size();
			for(int i=0; i<ncount; i++)
			{
			#ifndef WIN32
				if(bExit)
			#else
				if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
			#endif
				{
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return FALSE;
				}
				if(m_bClearing)
				{
					break;
				}
				if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && !m_PList[i]->m_bTURNC)
				{//普通伙伴节点都是广播目标，提速连接是与服务器相连，因此不需要向其广播
					m_PList[i]->SendBM(puchBuf, nSize);
				}
			}
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
		}
		else
		{//作为高速缓存，向所有广播将造成本地上传量巨大，因此只向速度较快节点广播
		#ifndef WIN32
			pthread_mutex_lock(&m_ct);
		#else
			EnterCriticalSection(&m_ct);
		#endif
			
			int ncount = m_PList.size();
			for(int i=0; i<ncount; i++)
			{
			#ifndef WIN32
				if(bExit)
				{
					pthread_mutex_unlock(&m_ct);
					return FALSE;
				}
			#else
				if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
				{
					LeaveCriticalSection(&m_ct);
					return FALSE;
				}
			#endif	
				
				if(m_bClearing)
				{
					break;
				}
				if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && m_PList[i]->m_bProxy2)
				{
					m_PList[i]->SendBM(puchBuf, nSize);
				}
			}
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
		}
	}

	return TRUE;
}

BOOL CCPartnerCtrl::SendBMDREQ2Partner(STREQS stpullreqs, int ncount, BOOL bExit)
{//遍历所有伙伴 找出最优伙伴发送数据请求
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	BOOL btimeout=FALSE;
	ncount = jvs_min(stpullreqs.size(), ncount);
	for(int i=0; i<ncount; i++)
	{
		if(bExit || m_bClearing)
		{
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
	
		btimeout = FALSE;
		//先从请求历史记录初步排除正在请求的数据，能提高执行效率
		::std::map<unsigned int, DWORD>::iterator iterbt;
		iterbt = m_ChunkBTMap.find(stpullreqs[i].unChunkID);
		if(iterbt != m_ChunkBTMap.end())
		{//找到记录
//			DWORD dw = iterbt->second;
			if(iterbt->second > 0)
			{
				DWORD dwend = CCWorker::JVGetTime();
				if(stpullreqs[i].bNEED)
				{
					if(dwend < iterbt->second + BM_CHUNK_TIMEOUT60)//120)//20)
					{//正在请求 不必重复
						continue;
					}
					btimeout = TRUE;
				}
				else
				{
					if(dwend < iterbt->second + BM_CHUNK_TIMEOUT240)//100)
					{//正在请求 不必重复
						continue;
					}
					btimeout = TRUE;
				}						
			}

//char ch[100]={0};
//sprintf(ch,"pull(%d)==================find[time:%d]\n",pstreqs[i].unChunkID,dw);
//OutputDebugString(ch);
			//检查是否有提供者 有则发送
			CheckAndSendChunk(stpullreqs[i].unChunkID, btimeout);
		}
		else
		{//没找到记录 是个全新数据块请求
//char ch[100]={0};
//sprintf(ch,"pull(%d)==================new\n",pstreqs[i].unChunkID);
//OutputDebugString(ch);
			m_ChunkBTMap.insert(::std::map<unsigned int, DWORD>::value_type(stpullreqs[i].unChunkID, 0));
			
			//检查是否有提供者 有则发送 请求一个chunk成功立即返回 可防止大量数据瞬间堆积
			CheckAndSendChunk(stpullreqs[i].unChunkID, btimeout);
		}		
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}


//判断是否有有效的提供者
BOOL CCPartnerCtrl::CheckAndSendChunk(unsigned int unChunkID, BOOL bTimeOut)
{
	int nTCAVG = 0;
	int nTCIndex = -1;
	int nBest = 0;//最优性能值(带宽)
	int nPartnerIndex = -1;//最优提供者编号
	
	BOOL bfindWANLink = FALSE;//找到外网有效伙伴
	BOOL bfindLink = FALSE;//找到连接成功的伙伴
	//遍历所有伙伴，检查是否还有潜在提供者，是否伙伴向某个伙伴请求过该数据
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_bClearing)
		{
			return FALSE;
		}

		if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
		{
			bfindLink = TRUE;
			if(!m_PList[i]->m_bLan2B && !m_PList[i]->m_bLan2A)
			{//伙伴不与主控同局域网，也不与本地同局域网，即应该是外网伙伴
				bfindWANLink = TRUE;
			}
			//检查是否有该数据片，是否向该伙伴请求过当前数据片，是的话重置该请求，并更新伙伴性能，并返回性能值
			if(m_PList[i]->CheckREQ(unChunkID, bTimeOut))
			{//该伙伴中该数据片有效
				if((m_PList[i]->m_nLPSAvage >= nBest && !m_PList[i]->m_bTURNC) || m_PList[i]->m_bLan2B)
				{//该伙伴性能更优 或是该伙伴是当前节点的内网伙伴 优先
					nPartnerIndex = i;
					nBest = m_PList[i]->m_nLPSAvage;
				}
				else if(m_PList[i]->m_bTURNC)
				{
					nTCIndex = i;
					nTCAVG = m_PList[i]->m_nLPSAvage;
				}
			}
		}
	}

	if(nTCIndex >= 0)
	{//有提速节点，判断下提速节点的性能是否很强，并且普通节点的性能是否很弱
		if(nPartnerIndex < 0)
		{//没有普通节点，只能从提速节点取数据
			nPartnerIndex = nTCIndex;
		}
		else
		{//有普通节点，看下普通节点是否非常弱，如果不是很弱，则不需要从提速节点取数据，减轻提速节点压力
			if(nBest < 10 && !m_PList[nPartnerIndex]->m_bLan2B)
			{
				nPartnerIndex = nTCIndex;
			}
		}
	}
	
	if(nPartnerIndex >= 0)
	{//伙伴中有提供者，将伙伴提供者和主控性能比较 向最优提供者发送请求
	 //(这里考虑主控压力，只要伙伴有就不从主控取, 除非主控和当前节点同内网)		
//if(m_pChannel->m_nLocalChannel == 2)
//{		
//char ch[100]={0};
//sprintf(ch,"B1A?_B(%d)==================BM:%d [B:%d A:%d]\r\n",nPartnerIndex, unChunkID, nBest, m_pChannel->m_nLPSAvage);
//OutputDebugString(ch);
//}
//		if(m_PList[nPartnerIndex]->m_bLan2B)
//		{//最优伙伴是个内网节点 可以直接从他取
//			m_PList[nPartnerIndex]->SendBMDREQ(unChunkID, this);
//			return TRUE;
//		}
		
//		if(m_pChannel->CheckREQ(unChunkID) && m_pChannel->m_bLan2A)
//		{//主控数据有效 并且主控与本机是同内网  从主控取数据
//			m_pChannel->SendBMDREQ2A(unChunkID);
//			return TRUE;
//		}
		
		//主控没数据或是主控是外网节点 伙伴也是外网节点 则从伙伴取 减轻主控压力
		m_PList[nPartnerIndex]->SendBMDREQ(unChunkID, this);
		return TRUE;
	}
	else
	{//伙伴没有提供者 检查主控是否是提供者
		if(m_pChannel->CheckREQ(unChunkID))
		{//主控chunk有效 是提供者
			DWORD dwcur = m_pChannel->m_pBuffer->JVGetTime();
			if((dwcur < m_pChannel->m_pBuffer->m_dwBeginBFTime + 3000) || ((!bfindLink || bfindWANLink) && dwcur > m_dwLastPTData + 10000))
			{//新连接的前3秒、没有伙伴或是有外网伙伴连续超过10秒没有伙伴传来数据，需要立即从主控索取数据，防止图像中断
				m_pChannel->SendBMDREQ2A(unChunkID);

//if(m_pChannel->m_nLocalChannel == 2)
//{			
//	char ch[100]={0};
//	sprintf(ch,"B0A1_A==================%d [%d:%d][%d:%d]\r\n",unChunkID, nBest, m_pChannel->m_nLPSAvage,dwcur,m_dwLastPTData);
//	OutputDebugString(ch);
//}

				return TRUE;
			}
		}
//if(m_pChannel->m_nLocalChannel == 2)
//{
//	char ch[100]={0};
//	sprintf(ch,"B0A0_0==================%d [%d:%d]\r\n",unChunkID, nBest, m_pChannel->m_nLPSAvage);
//	OutputDebugString(ch);
//}		
		return FALSE;
	}
}

BOOL CCPartnerCtrl::SendBMD()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_bClearing)
		{
			break;
		}
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_nstatus != PNODE_STATUS_CONNECTED)// || m_PList[i]->m_bTURNC)
		{//无效节点和提速节点不需要发送数据块
			continue;
		}

		m_PList[i]->SendBMD(this);
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

void CCPartnerCtrl::SetReqStartTime(BOOL bA, unsigned int unChunkID, DWORD dwtime)
{
	if(unChunkID <= 0)
	{
		return;
	}
	
	m_ChunkBTMap.erase(unChunkID);
	if(dwtime == 0)
	{//清除记录
//		m_ChunkBTMap.erase(unChunkID);
		return;
	}

	m_ChunkBTMap.insert(::std::map<unsigned int, DWORD>::value_type(unChunkID, dwtime));	
}

void CCPartnerCtrl::ClearPartner()
{
	m_bClearing = TRUE;
//OutputDebugString("ptcrl clearp.....0\n");
#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctCONN);
	EnterCriticalSection(&m_ct);
	EnterCriticalSection(&m_ctPTINFO);
#endif

//OutputDebugString("ptcrl clearp.....1\n");
	//必须先清理掉连接队列中的无效项才能释放，否则出现不同步
	m_PLINKList.clear();

	int ncount = m_PList.size();
	for(int i=0; i< ncount; i++)
	{
		if(m_PList[i] != NULL)
		{
			delete m_PList[i];
			m_PList[i] = NULL;
		}
	}
	m_PList.clear();

#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctPTINFO);
	LeaveCriticalSection(&m_ct);
	LeaveCriticalSection(&m_ctCONN);
#endif
	
	m_bClearing = FALSE;
}

void CCPartnerCtrl::DisConnectPartners()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_nstatus != PNODE_STATUS_CONNECTED)
		{
			continue;
		}
		
		m_PList[i]->DisConnectPartner();
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

//检查是否需要将新BM更新到本地缓冲 内部调用 不加保护
void CCPartnerCtrl::CheckIfNeedSetBuf(unsigned int unChunkID, int ncount, DWORD dwCTime[10], BOOL bA)
{
//	if(unChunkID+ncount-1 > m_unChunkIDNew && m_pChannel->m_pBuffer != NULL)
	if(unChunkID > m_unChunkIDNew && m_pChannel->m_pBuffer != NULL)
	{//是全新BM 需要更新到本地缓存 保证本地永远和最新视频保持同步
//char ch[1000]={0};
//sprintf(ch,"BM(pctrl needsetbuf)=============================================[%d,%d]>[%d,%d]\r\n",m_unChunkIDOld,m_unChunkIDNew,unChunkID,unChunkID+ncount-1);
//OutputDebugString(ch);
//m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel,"",__FILE__,__LINE__,ch);
		if(!bA)
		{
			m_unChunkIDNew = unChunkID;
			m_unChunkIDOld = unChunkID - ncount + 1;

			return;
		}

		unsigned int unnewid = 0;
		unsigned int unoldid = 0;
		//m_pChannel->m_pBuffer->AddNewBM(unChunkID, ncount, dwCTime, unnewid, unoldid, bA);
		m_pChannel->m_pBuffer->AddNewBM(unChunkID-ncount+1, ncount, dwCTime, unnewid, unoldid, bA);
		if(unnewid > 0)
		{
			m_unChunkIDNew = unnewid;
		}

		if(unoldid > 0)
		{
			m_unChunkIDOld = unoldid;
		}
	}
}

void CCPartnerCtrl::CheckGarbage()
{
//OutputDebugString("ptcrl gt.....0\n");
#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctCONN);
	EnterCriticalSection(&m_ct);
	EnterCriticalSection(&m_ctPTINFO);
#endif
	
//OutputDebugString("ptcrl gt.....1\n");
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
		if(m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC)
		{
			//必须先清理掉连接队列中的无效项才能释放，否则出现不同步
			int nlinkc = m_PLINKList.size();
			for(int j=0; j<nlinkc; j++)
			{
				if(m_PLINKList[j] == NULL)
				{
					m_PLINKList.erase(m_PLINKList.begin() + j);
					j--;
					nlinkc--;
					continue;
				}
				if(m_PLINKList[j]->m_ndesLinkID <= 0 && !m_PLINKList[j]->m_bTURNC)
				{
					m_PLINKList.erase(m_PLINKList.begin() + j);
					j--;
					nlinkc--;
					continue;
				}
			}

			delete m_PList[i];
			m_PList[i] = NULL;
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctPTINFO);
	LeaveCriticalSection(&m_ct);
	LeaveCriticalSection(&m_ctCONN);
#endif
}

//重置超级节点 周期性调用 主控只向超级节点发送数据
void CCPartnerCtrl::ResetProxy2()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	
	int i=0;
	::std::map<unsigned int, unsigned int> PTaddrMap;
	PartnerList partnertmp;
	partnertmp = m_PList;
	for(i=0; i<ncount; i++)
	{//查找性能最好的几个节点
		if(m_bClearing)
		{
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return;
		}

		if(partnertmp[i] == NULL || partnertmp[i]->m_ndesLinkID <= 0 || partnertmp[i]->m_bLan2B || partnertmp[i]->m_bLan2A || partnertmp[i]->m_bTURNC)
		{//去掉无效节点,主控同内网节点不能作代理节点,提速节点也不能做代理节点
			partnertmp.erase(partnertmp.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		for(int j=i+1; j<ncount; j++)
		{
			if(m_bClearing)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return;
			}
			
			if(partnertmp[j] == NULL || partnertmp[j]->m_ndesLinkID <= 0 || partnertmp[j]->m_bLan2B || partnertmp[j]->m_bLan2A || partnertmp[i]->m_bTURNC)
			{//去掉无效节点,主控同内网节点不能作代理节点,提速节点也不能做代理节点
				partnertmp.erase(partnertmp.begin() + j);
				j--;
				ncount--;
				continue;
			}
			
			if(partnertmp[i]->GetPower() >= partnertmp[j]->GetPower())
			{//j 性能更好
				CCPartner *p = partnertmp[i];
				partnertmp[i] = partnertmp[j];
				partnertmp[j] = p;
			}
		}
		
		//记录IP 相同的IP只能做一次代理
		::std::map<unsigned int, unsigned int>::iterator iter;
		iter = PTaddrMap.find(ntohl(partnertmp[i]->m_addr.sin_addr.s_addr));
		if(iter == PTaddrMap.end())
		{//未找到
			PTaddrMap.insert(::std::map<unsigned int, unsigned int>::value_type(ntohl(partnertmp[i]->m_addr.sin_addr.s_addr), 1));
		}
		else
		{//相同IP已入选 不必做重复代理
			partnertmp.erase(partnertmp.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		if(i >= JVC_CLIENTS_BM3-1)
		{//性能最好的节点够数了 不用继续排序了
			break;
		}
	}
	
	//将性能最好的节点化为超级节点 其他同时更新为非超级节点
	ncount = m_PList.size();
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] != NULL)
		{
			if(m_PList[i]->m_bLan2B)
			{
				m_PList[i]->m_bProxy2 = TRUE;
			}
			else
			{
				m_PList[i]->m_bProxy2 = FALSE;
			}
		}
	}
	ncount = partnertmp.size();

	for(i=0; i<jvs_min(ncount, JVC_CLIENTS_BM3); i++)
	{
		if(partnertmp[i] != NULL)
		{//内网节点禁止作为超级节点
			partnertmp[i]->m_bProxy2 = TRUE;
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif

}

//获取伙伴状态信息
void CCPartnerCtrl::GetPartnerInfo(char *pMsg, int &nSize, DWORD dwend)
{
	//是否多播(1)+在线总个数(4)+伙伴总个数(4)+[IP(16) + port(4)+连接状态(1)+下载速度(4)+下载总量(4)+上传总量(4)] +[...]...
#ifndef WIN32
	pthread_mutex_lock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctPTINFO);
#endif
	int ncount = m_PList.size();

	int ntotal = 1;
	if((ncount+1)*sizeof(STPTINFO)+9 > nSize)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ctPTINFO);
	#else
		LeaveCriticalSection(&m_ctPTINFO);
	#endif
		
		memset(pMsg, 0, 9);
		memcpy(pMsg, &ntotal, 4);
		return;
	}

	int nconn=0;
	nSize = sizeof(STPTINFO)+9;
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		if(m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
		{
			m_PList[i]->GetPInfo(pMsg, nSize, dwend);
			nconn++;
		}

		if(!m_PList[i]->m_bTURNC)
		{//普通节点，直接计数
			ntotal++;
		}
//		else
//		{//提速转发节点，只在连接成功的情况下才计数
//			if(m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
//			{
//				ntotal++;
//			}
//		}
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	LeaveCriticalSection(&m_ctPTINFO);
#endif

	nconn++;

	memcpy(&pMsg[1], &ntotal, 4);
	memcpy(&pMsg[5], &nconn, 4);
}

/****************************************************************************
*名称  : tcpreceive
*功能  : 非阻塞接收数据(定时)
*参数  : 
		 [IN] ntimeoverSec
*返回值: >0    数据长度
*其他  :
*****************************************************************************/
int CCPartnerCtrl::tcpreceive(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec)
{
	int   status,nbytesreceived;   
	if(s <= 0)//if(s==-1)     
	{
		return   -1;//0;   
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
int CCPartnerCtrl::tcpsend(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec, int &ntimeout)
{
	int   status=0, nbytessended=-1;   
	if(s <= 0)//if(s==-1)     
	{
		return   -1;//0;   
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
		ntimeout = 1;
		return   0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
			if((nbytessended=send(s,pchbuf,nsize,0))==-1) 
			{
				return   0;//产生了错误，但有些错误可能是因为非阻塞，不是真正的错误，需要判断   
			}
			else   
			{
				return   nbytessended;   
			}
		}   
	}   
	return -1;//0;   
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
int CCPartnerCtrl::connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec)
{
	if(s <= 0)//if(s ==-1)     
	{
		return -1;   
	}
	struct timeval tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);
	
	int nRet = connect( s, (SOCKADDR *)to, namelen);       
//int kkk = WSAGetLastError();
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
			//char error = 0;
			int error = 0;

			#ifndef WIN32
				socklen_t len = sizeof(error);//sizeof(SOCKET);
			#else
				int len = sizeof(error);//int len = sizeof(error);//sizeof(SOCKET);
			#endif
			//成功的话error应该为0
//int kkk = WSAGetLastError();
			if ((0 == getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&error,&len)))
			{   
				if(0 == error)
				{ 
					return 0;
				}
			}
//kkk = WSAGetLastError();
//char ch[100];
//sprintf(ch,"kkk:%d\n", kkk);
			break;
		}
	}
	else
	{
		return 0;
	}
	return -1; 
}

/****************************************************************************
*名称  : CheckInternalIP
*功能  : 检查是否是内部的IP地址 
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
BOOL CCPartnerCtrl::CheckInternalIP(const unsigned int ip_addr)
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






























