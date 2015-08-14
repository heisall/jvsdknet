// CBufferCtrl.cpp: implementation of the CCBufferCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "CBufferCtrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//缓存基类
//////////////////////////////////////////////////////////////////////////
CCBaseBufferCtrl::CCBaseBufferCtrl(){}
CCBaseBufferCtrl::CCBaseBufferCtrl(BOOL bTURN)
{
	m_bTURN = bTURN;
	m_nFrameTime = 40;
	m_nFrames = 50;

	m_bLan2A = FALSE;
	m_nClientCount = 1;

	m_lTLENGTH = 0;//缓冲区总长度
	
	m_dwLastPlayTime = 0;
	m_dwLastFTime = 0;
	m_dwLastCTime = 0;
	m_unMaxWaitTime = JVN_TIME_FIRSTWAIT;//置为初次缓冲允许最大时间
	m_unChunkTimeSpace = 0;
	m_nLastWaitCC = JVN_PLAYNOW;
	m_dwLastJump = 0;

	m_bFirstBMDREQ = TRUE;
	m_bNoData = TRUE;

	m_nRate = 0;

	m_pLog = NULL;

	m_bFirstWait = TRUE;
	m_dwBeginBFTime = JVGetTime();

#ifndef WIN32
	time_t now = time(0); 
	tm *tnow = localtime(&now);
	sprintf(m_stSTAT.chBegainTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec); 
#else
	SYSTEMTIME sys; 
	GetLocalTime(&sys); 
	sprintf(m_stSTAT.chBegainTime,"%4d-%02d-%02d %02d:%02d:%02d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
#endif 

	m_stSTAT.dwBeginTime = JVGetTime();
	m_stSTAT.dwbegin = 0;
	m_stSTAT.unbeginid = 0;
	m_stSTAT.dwend = 0;

	m_nReqBegin = JVN_PLAYIMD;//保留数目
	m_nPLAYIMD = JVN_PLAYIMD;//立即播放阀值

#ifndef WIN32
	pthread_mutex_init(&m_ct, NULL);
#else
	InitializeCriticalSection(&m_ct); //初始化临界区
#endif
}
CCBaseBufferCtrl::~CCBaseBufferCtrl()
{
#ifndef WIN32
	time_t now = time(0); 
	tm *tnow = localtime(&now);
	sprintf(m_stSTAT.chEndTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec); 
#else
	SYSTEMTIME sys; 
	GetLocalTime(&sys); 
	sprintf(m_stSTAT.chEndTime,"%4d-%02d-%02d %02d:%02d:%02d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
#endif 

	m_stSTAT.finish();

	m_stSTAT.unWaitCount = jvs_max(1,m_stSTAT.unWaitCount);
	m_stSTAT.unWaitTimeAvg = m_stSTAT.unWaitTimeTotal/m_stSTAT.unWaitCount;
	m_stSTAT.dwEndTime = JVGetTime();
	m_stSTAT.dwTimeTotal = (m_stSTAT.dwEndTime - m_stSTAT.dwBeginTime)/1000;

	if(m_stSTAT.unWaitCount > 0)
	{
		m_stSTAT.unWaitFrequence = m_stSTAT.dwTimeTotal/m_stSTAT.unWaitCount;
	}

	char ch[1500]={0};
	sprintf(ch,"\n***********************************\nBegin    : %s\nEnd      : %s\nTotalTime: %d\nWaitCount: %d\nWaitTime : %d\nTimePWait: %d\nWaitFreqs: %d\nJumpCount: %d\nErrCount : %d\nNoBCount : %d\nRPCount  : %d\nNoIDCount: %d\nNoDCount : %d\nNoICount : %d\nDelayCount: %d\nFREQID   : %d\nFRCVID   : %d\nFirstPlay: [%d,%d]\nPLAYIMD  : %d\n***********************************",
		        m_stSTAT.chBegainTime,
				m_stSTAT.chEndTime,
				m_stSTAT.dwTimeTotal,
				m_stSTAT.unWaitCount,
				m_stSTAT.unWaitTimeTotal,
				m_stSTAT.unWaitTimeAvg,
				m_stSTAT.unWaitFrequence,
				m_stSTAT.unBufJumpCount,
				m_stSTAT.unBufErrCount,
				m_stSTAT.unNobufCount,
				m_stSTAT.unRepeatCount,
				m_stSTAT.unNotfindCount,
				m_stSTAT.unNoDataCount,
				m_stSTAT.unNoICount,
				m_stSTAT.unDelayCount,
				m_stSTAT.unBeginReqID,
				m_stSTAT.unFirstRcvID,
				m_stSTAT.unBeginPlayID0,
				m_stSTAT.unBeginPlayID1,
				m_nPLAYIMD);
	if(m_pLog != NULL)
	{
		m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,ch);
	}
/**/
#ifndef WIN32
	pthread_mutex_destroy(&m_ct);
#else
	DeleteCriticalSection(&m_ct); //释放临界区
#endif
}

/****************************************************************************
*名称  : GetTime
*功能  : 获取当前时间 ms
*参数  : 无
*返回值: 无
*其他  : 无
*****************************************************************************/
DWORD CCBaseBufferCtrl::JVGetTime()
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

//////////////////////////////////////////////////////////////////////////
//多播缓存类
//////////////////////////////////////////////////////////////////////////
CCMultiBufferCtrl::CCMultiBufferCtrl():CCBaseBufferCtrl(){}
CCMultiBufferCtrl::CCMultiBufferCtrl(BOOL bTURN, BOOL bJVP2P):CCBaseBufferCtrl(bTURN)
{
	m_bJVP2P = bJVP2P;
	
	m_lTLENGTH = JVN_BUF_MINM;
	
	//计算可以缓存多少数据块
	m_nChunkNUM = m_lTLENGTH/JVN_CHUNK_LEN;
	
	m_stBM.pnChunkIndex = new int[m_nChunkNUM];
	//初始化缓存编号，缓存顺序号和基址建立关系
	for(int i=0; i<m_nChunkNUM; i++)
	{
		m_stBM.pnChunkIndex[i] = i;
	}
	
	m_stBM.unNeedPlayID = 1;
	m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[m_nChunkNUM-1];

	//创建总缓冲区
	m_stBM.pBuffer = new BYTE[m_lTLENGTH];
	
	//初始化每个Chunk
	for(int j=0; j<m_nChunkNUM; j++)
	{
		STCHUNK stchunk;
		stchunk.ResetChunk();
		m_stBM.ChunksMap.insert(::std::map<int, STCHUNK>::value_type(j, stchunk));
		m_stBM.ChunksMap[j].lBeginPos = j*JVN_CHUNK_LEN;//为每个数据片分配内存 该值不随循环覆盖改变
		m_stBM.ChunksMap[j].ResetChunk();
	}
}

CCMultiBufferCtrl::~CCMultiBufferCtrl()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	m_stBM.ChunksMap.clear();
	
	if(m_stBM.pnChunkIndex != NULL)
	{
		delete[] m_stBM.pnChunkIndex;
		m_stBM.pnChunkIndex = NULL;
	}
	
	if(m_stBM.pBuffer != NULL)
	{
		delete[] m_stBM.pBuffer;
		m_stBM.pBuffer = NULL;
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
}

BOOL CCMultiBufferCtrl::WaitHighFrequency()
{
	m_stSTAT.unWaitCount = jvs_max(1,m_stSTAT.unWaitCount);
	DWORD dwend = JVGetTime();
	DWORD dwTimeTotal = (dwend - m_stSTAT.dwBeginTime)/1000;
//	unsigned int untmp = 0;
	if(m_stSTAT.unWaitCount > 0)
	{
		m_stSTAT.unWaitFrequence = dwTimeTotal/m_stSTAT.unWaitCount;
		m_stSTAT.unWaitTimeAvg = m_stSTAT.unWaitTimeTotal/m_stSTAT.unWaitCount;
	}

	//缓冲频率小于10s一次并且平均缓冲时间>3s
	//平均缓冲时间大于缓冲频率的1/3，也就是缓冲时间是播放时间的1/3以上
	//缓冲总时间大于播放总时间的1/3,这里要求播放5分钟以上才有效
	//平均缓冲时间大于30s 太长 
	if(dwTimeTotal > 180 
	   && ((m_stSTAT.unWaitFrequence < 10 && m_stSTAT.unWaitTimeAvg > 4 && m_stSTAT.unWaitFrequence > 0) 
	       || (m_stSTAT.unWaitFrequence < 3*m_stSTAT.unWaitTimeAvg && m_stSTAT.unWaitFrequence > 0)
		   || (dwTimeTotal < 3*m_stSTAT.unWaitTimeTotal && dwTimeTotal > 300)
	       || (m_stSTAT.unWaitTimeAvg > 30 && m_stSTAT.unWaitTimeAvg > m_stSTAT.unWaitFrequence && m_stSTAT.unWaitFrequence > 0)
		  )
	  )
	{
//char ch[200];
//sprintf(ch,"[WA:%d,WF:%d].",m_stSTAT.unWaitTimeAvg,m_stSTAT.unWaitFrequence);
//OutputDebugString("waithighfrequency......\n");
//m_pLog->SetRunInfo(m_nChannel,"need speedup,try CTRUN.",__FILE__,__LINE__,ch);
		return TRUE;
	}

	return FALSE;
}

void CCMultiBufferCtrl::ClearBuffer()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif

	m_dwLastPlayTime = 0;
	
	m_stBM.nOldestChunk = -1;
	m_stBM.unNeedPlayID = 1;
	m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[m_nChunkNUM-1];
	m_stBM.bNeedI = TRUE;
	m_stBM.bNeedA = FALSE;
	
	//初始化缓存编号
	for(int i=0; i<m_nChunkNUM; i++)
	{
		m_stBM.pnChunkIndex[i] = i;
	}
	
	//初始化每个Chunk
	for(int j=0; j<m_nChunkNUM; j++)
	{
		m_stBM.ChunksMap[j].ResetChunk();
	}
	
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

//检查是否BM有变化，即是否收到chunk数据，是的话可发送BM
BOOL CCMultiBufferCtrl::GetBM(BYTE *pBuffer, int &nSize)
{
	if(pBuffer == NULL || m_stBM.pnChunkIndex == NULL || !m_stBM.bBMChanged)
	{
		return FALSE;
	}
	
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif

	BYTE uchtemp=0x01;
	BYTE uchtempFF=0xFF;
	int ncount = 0;
	int ntotal = ((m_stBM.nOldestChunk>(m_nChunkNUM-1-JVN_CHUNK_PRIVATE))?(m_nChunkNUM-1-JVN_CHUNK_PRIVATE):m_stBM.nOldestChunk);
	int nlastindex = m_stBM.pnChunkIndex[ntotal];
	unsigned int unChunkID = m_stBM.ChunksMap[nlastindex].stHead.unChunkID;
	if(unChunkID > 0)
	{
		int nhave=0;
		//类型(1)+总长度(4)+CHUNKID(4)+chunk总数(4)+[BM(?)]
		pBuffer[0] = JVN_CMD_BM;
		memcpy(&pBuffer[5], &unChunkID, 4);
		memcpy(&pBuffer[9], &ntotal, 4);
		for(int i=ntotal; i>0; i--)
		{
			if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData && (m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset > 0))
			{//有数据 对应位置1
				pBuffer[13+(ncount/8)] = (pBuffer[13+(ncount/8)] | (uchtemp<<(7-ncount%8)));
				nhave++;
			}
			else
			{//无数据 对应位置0
				pBuffer[13+(ncount/8)] = (pBuffer[13+(ncount/8)] & (uchtempFF^(uchtemp<<(7-ncount%8))));
			}
			ncount ++;
		}
		
		int nByte = ncount/8 + ((ncount%8)?1:0);
		nSize = 8+nByte;
		memcpy(&pBuffer[1], &nSize, 4);
		
		nSize = 13+nByte;
//////////////////////////////////////////////////////////////////////////
/*		char chMsg[2000]={0};
		sprintf(chMsg, "[%d]B %d %d [%d, %d]**********************BM(have:%d) ",m_nChannel,unChunkID,ntotal,unChunkID, unChunkID+ntotal-1, nhave);
		for(i=0; i<nByte; i++)
		{
			char ch[10];
			sprintf(ch,"%X",pBuffer[13+i]);
			strcat(chMsg, ch);
		}
		strcat(chMsg,"\n");

		OutputDebugString(chMsg);
*/
//////////////////////////////////////////////////////////////////////////

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return TRUE;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}

BOOL CCMultiBufferCtrl::WriteBuffer(STCHUNKHEAD stHead, BYTE *pBuffer)
{
//	m_stBM.bBMChanged = TRUE;
//return FALSE;
	if(stHead.unChunkID <= 0 || pBuffer == NULL)
	{
		return FALSE;
	}
//////////////////////////////////////////////////////////////////////////
//if(m_nChannel == 2)
//{
//	char chMsg[100]={0};
//	sprintf(chMsg, "[%d]wirte bmdid %d\n",m_nChannel, stHead.unChunkID);
//	OutputDebugString(chMsg);
//}
	
//m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,chMsg);
//////////////////////////////////////////////////////////////////////////
	
	//数据类型(1)+数据长度(4)+BMID(4) + 数据片块头(?) +数据片内容(?)
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
BOOL bfind = FALSE;	
	for(int i=0; i<m_nChunkNUM; i++)
	{//从最新数据块向后遍历，看是否能找到指定数据块
		if(stHead.unChunkID == m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID)
		{//找到该数据块
			bfind = TRUE;
			if(!m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData || m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset <= 0)
			{//该数据片空缺，更新数据源
				if(stHead.lWriteOffset > 0 && stHead.lWriteOffset <= JVN_CHUNK_LEN)
				{//缓存够用
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset = stHead.lWriteOffset;
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.bHaveI = stHead.bHaveI;
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.nIOffset = stHead.nIOffset;
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.nNOffset = stHead.nNOffset;

//////////////////////////////////////////////////////////////////////////
//if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime != stHead.unCTime)
//{
//	char ch[100]={0};
//	sprintf(ch,"[write err ID:%d][ct:%u][newt:%u]\n",stHead.unChunkID,m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime,stHead.unCTime);
//	OutputDebugString(ch);
//}	
					//////////////////////////////////////////////////////////////////////////				
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime = stHead.unCTime;

					int nchunkoffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].lBeginPos;
					memcpy(&m_stBM.pBuffer[nchunkoffset], pBuffer, stHead.lWriteOffset);
					
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData = TRUE;

					if(stHead.nNOffset >= 0)
					{//数据块中含有某个帧头，初始播放位置就应该是帧头位置
						m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stPHead.lNextOffset = stHead.nNOffset;
					}
					else
					{//数据块中没有帧头，全是局部数据，初始播放位置应该在最开始
						m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stPHead.lNextOffset = 0;
					}

					m_stBM.bBMChanged = TRUE;

					if(m_bFirstBMDREQ)
					{
						m_stSTAT.unFirstRcvID = stHead.unChunkID;
						m_stSTAT.unBeginPlayID0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[m_nChunkNUM-1]].stHead.unChunkID;
						m_stSTAT.unBeginPlayID1 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
							
						m_bFirstBMDREQ = FALSE;
					}

					if(m_bNoData)
					{					
						m_bNoData = FALSE;
						
						m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[i];
						m_stBM.unNeedPlayID = stHead.unChunkID;
						
						if(stHead.unChunkID > 0)
						{
							m_stBM.unPlayedID = stHead.unChunkID-1;
						}
						else
						{
							m_stBM.unPlayedID = 0;
						}
						m_dwLastPlayTime = JVGetTime();//第一次收到数据，播放计时起点
//////////////////////////////////////////////////////////////////////////
//char chMsg[100]={0};
//sprintf(chMsg, "[%d]wirte bnodata id %d\r\n",m_nChannel, stHead.unChunkID);
//OutputDebugString(chMsg);
//m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,chMsg);
//////////////////////////////////////////////////////////////////////////
					}
					
					if(stHead.unChunkID < m_stBM.unPlayedID)
					{
						m_stSTAT.unDelayCount++;
					}

					//////////////////////////////////////////////////////////////////////////
//					if(m_nChannel == 1)
//					{
//						char chMsg[100]={0};
//						sprintf(chMsg, "[%d]wirte ok bmdid %d [%d]\n",m_nChannel, stHead.unChunkID,m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData);
//						OutputDebugString(chMsg);
//					}
					//////////////////////////////////////////////////////////////////////////
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return TRUE;
				}
				else
				{
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData = FALSE;

					m_stSTAT.unNobufCount++;
//if(m_nChannel == 2)
//{
//	char chMsg[1000]={0};
//	sprintf(chMsg, "write A.%d len:%d**************************Data  nobuf!!!\n", stHead.unChunkID, stHead.lWriteOffset);
//	OutputDebugString(chMsg);
//}
//m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,chMsg);
				}
			}
			m_stSTAT.unRepeatCount++;
//			if(m_nChannel == 2)
//			{
//				char chMsg[1000]={0};
//				sprintf(chMsg, "write A.%d len:%d**************************Data  repeat!!![have:%d][%d]\n", stHead.unChunkID, stHead.lWriteOffset,m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData,m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset);
//				OutputDebugString(chMsg);
//			}		
//m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,chMsg);
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return FALSE;
		}
	}

	if(m_stBM.unPlayedID >= stHead.unChunkID)
	{
		if(m_stSTAT.unNotfindCount > 10)
		{
			m_nPLAYIMD = jvs_max(m_nPLAYIMD-1, 1);//5);
			m_nReqBegin = jvs_min(m_nReqBegin+1, JVN_REQBEGINMAX);
		}
	}
//if(m_nChannel == 2)
//{
//char chMsg[1000]={0};
//sprintf(chMsg, "write A.%d len:%d**************************Data  notfindid!!![%d,%d]\r\n", 
//		stHead.unChunkID, stHead.lWriteOffset,
//		m_stBM.ChunksMap[m_stBM.pnChunkIndex[m_nChunkNUM-1]].stHead.unChunkID,
//		m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID);
//OutputDebugString(chMsg);
//}
//m_pLog->SetRunInfo(m_nChannel,"",__FILE__,__LINE__,chMsg);
	
	//若是新的BM 则创建
	if((m_bTURN || !m_bJVP2P) && stHead.unChunkID > m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID)
	{
		//old舍弃 wait new 降级
		int ntmp = m_stBM.pnChunkIndex[m_nChunkNUM-1];
		for(int i=m_nChunkNUM-1; i>0; i--)
		{
			m_stBM.pnChunkIndex[i] = m_stBM.pnChunkIndex[i-1];
		}
		m_stBM.pnChunkIndex[0] = ntmp;
		
		//播放指针若被覆盖则上移
		if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[0])
		{
//char ch[100];
//sprintf(ch,"m_nPlay=%d (%d) [%d->%d]...........0\n",m_nPlay, m_nBMIndex[JVN_BMMAX-1], 0, JVN_BMMAX-1);
//OutputDebugString(ch);
			m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[m_nChunkNUM-1];
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;
		}

		//最后更新new为新的BM
		m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].ResetChunk();
		m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID = stHead.unChunkID;
		
		//将新chunk写入
		if(stHead.lWriteOffset > 0 && stHead.lWriteOffset <= JVN_CHUNK_LEN)
		{//缓存够用
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.lWriteOffset = stHead.lWriteOffset;
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.bHaveI = stHead.bHaveI;
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.nIOffset = stHead.nIOffset;
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.nNOffset = stHead.nNOffset;
//////////////////////////////////////////////////////////////////////////
//if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime != stHead.unCTime)
//{
//	char ch[100]={0};
//	sprintf(ch,"[write err ID:%d][ct:%d][newt:%d]\n",stHead.unChunkID,m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime,stHead.unCTime);
//	OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime = stHead.unCTime;
			
			int nchunkoffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].lBeginPos;
			memcpy(&m_stBM.pBuffer[nchunkoffset], pBuffer, stHead.lWriteOffset);
			
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].bHaveData = TRUE;
			
			if(stHead.nNOffset >= 0)
			{//数据块中含有某个帧头，初始播放位置就应该是帧头位置
				m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stPHead.lNextOffset = stHead.nNOffset;
			}
			else
			{//数据块中没有帧头，全是局部数据，初始播放位置应该在最开始
				m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stPHead.lNextOffset = 0;
			}
			
			m_stBM.bBMChanged = TRUE;
			
			if(m_bFirstBMDREQ)
			{
				m_stSTAT.unFirstRcvID = stHead.unChunkID;
				m_stSTAT.unBeginPlayID0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[m_nChunkNUM-1]].stHead.unChunkID;
				m_stSTAT.unBeginPlayID1 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
				
				m_bFirstBMDREQ = FALSE;
			}
			
			if(m_bNoData)
			{
				m_bNoData = FALSE;
				
				m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
				m_stBM.unNeedPlayID = stHead.unChunkID;
				
				if(stHead.unChunkID > 0)
				{
					m_stBM.unPlayedID = stHead.unChunkID-1;
				}
				else
				{
					m_stBM.unPlayedID = 0;
				}
				m_dwLastPlayTime = JVGetTime();//第一次收到数据，播放计时起点
			}
			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
		else
		{
			m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].bHaveData = FALSE;
			
			m_stSTAT.unNobufCount++;
		}
	}
	else
	{
		m_stSTAT.unNotfindCount++;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}

//逐个检查chunk，未超时的跳过，超时的复位重新请求
BOOL CCMultiBufferCtrl::ReadChunkLocalNeed(STREQS &stpullreqs, int &ncount)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	ncount = 0;
	int nlast = m_stBM.nOldestChunk;//最旧数据块的顺序地址

	BOOL bfindindex = FALSE;
	stpullreqs.clear();
	int nbufstart = 0;//缓冲起始点
	if(!m_bFirstBMDREQ)
	{//不是第一次请求数据，正常获取
		for(int i=nlast; i>=0; i--)
		{
			int nindex = m_stBM.pnChunkIndex[i];//转为基址
			
			//寻找播放指针
			if(nindex == m_stBM.nNeedPlayIndex)
			{//找到播放指针
				bfindindex = TRUE;
				int nstartpos = m_nChunkNUM - m_nReqBegin;
				if(GetBufferPercent() < 20)
				{
					if(m_stSTAT.unNotfindCount > m_nChunkNUM && m_nChunkNUM > JVN_PLAYNOW)
					{//过期数据超过了缓冲区长度，属重度数据延迟
						nstartpos = m_nChunkNUM/3;
					}
					else if(m_stSTAT.unNotfindCount > (m_nChunkNUM/2))
					{//过期数据超过了一半缓冲区长度，属较重度数据延迟
						nstartpos = m_nChunkNUM/2;
					}
					else if(m_stSTAT.unNotfindCount > JVN_PLAYNOW)
					{
						nstartpos = jvs_max(m_nChunkNUM - 2*JVN_PLAYNOW,0);
						nstartpos = jvs_min(nstartpos, m_nChunkNUM - JVN_PLAYNOW);
					}
				}
				
				//从播放指针向后寻找需要请求的数据块
				nbufstart = i;
				for(int j=i; j>=0; j--)
				{	
					nindex = m_stBM.pnChunkIndex[j];//转为基址
					unsigned int unChunkID = m_stBM.ChunksMap[nindex].stHead.unChunkID;
					//if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData && unChunkID >= m_stBM.unPlayedID && (j < (m_nChunkNUM - JVN_PLAYIMD)))
					//if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData && unChunkID >= m_stBM.unPlayedID && (j < (m_nChunkNUM - m_nReqBegin)))
					if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData && unChunkID >= m_stBM.unPlayedID && (j < nstartpos))
					{//危险区的数据不再请求，防止产生大量过期数据					
						STREQ streq;
						streq.bAnswer = FALSE;
						streq.dwStartTime = 0;
						streq.unChunkID = unChunkID;
						
						if((j + JVN_PLAYNOW > nbufstart))//if(j < 100)//((j + JVN_PLAYNOW > nbufstart))//JVN_PLAYNOW
						{//处在缓冲中 急需(缓冲中 并且不在危险区)
							if(GetBufferPercent() > 90)
							{//当缓冲进度接近完成时，剩余的应该尽快索取
								streq.bNEED = TRUE;
							}
							else
							{
								streq.bNEED = FALSE;
							}
						}
						else
						{//不在缓冲过程中的，按正常流程索取
							streq.bNEED = FALSE;
						}
						stpullreqs.push_back(streq);
						ncount++;
					}
				}

				//播放指针的数据已经是当前播放时间之前的数据，需要以播放ID为标准来寻找
				break;
			}
		}
		if(!bfindindex)
		{
			//////////////////////////////////////////////////////////////////////////
			int nend = 0;
			BOOL btmp=TRUE;
			DWORD dwendtime = 0;
			DWORD dwbegintime = 0;
			int i=0;
			for(i=0; i<m_nChunkNUM; i++)
			{
				if(i == 0)
				{
					dwbegintime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime;
				}
				
				int nindex = m_stBM.pnChunkIndex[i];//转为基址
				dwendtime = m_stBM.ChunksMap[nindex].stHead.unCTime;
				if(dwbegintime > 0 && dwendtime > 0)
				{
					unsigned int unChunkID = m_stBM.ChunksMap[nindex].stHead.unChunkID;
					if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData)
					{//数据块合法并且现在没数据，应该请求
						nend = i;			
					}
				}
				
				if(dwbegintime > dwendtime + 8000)//5000
				{
					break;
				}
			}
			
			for(i=nend; i>=0; i--)
			{
				int nindex = m_stBM.pnChunkIndex[i];//转为基址
				dwendtime = m_stBM.ChunksMap[nindex].stHead.unCTime;
				if(dwendtime > 0)
				{
					unsigned int unChunkID = m_stBM.ChunksMap[nindex].stHead.unChunkID;
					if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData)
					{//数据块合法并且现在没数据，应该请求
						if(m_bNoData && btmp)
						{//将播放指针放到第一个请求的位置
							btmp = FALSE;
							m_stBM.nNeedPlayIndex = nindex;
							m_stBM.unNeedPlayID = unChunkID;
							m_stBM.bNeedI = TRUE;
							m_stBM.bNeedA = FALSE;
						}
						
						STREQ streq;
						streq.bAnswer = FALSE;
						streq.dwStartTime = 0;
						streq.bNEED = TRUE;//FALSE;
						streq.unChunkID = unChunkID;
						stpullreqs.push_back(streq);
						
						ncount++;
//////////////////////////////////////////////////////////////////////////
//if(m_nChannel == 2)
//{
//	char ch[100]={0};
//	sprintf(ch,"first req ID:%d\n",streq.unChunkID);
//	OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
						
					}
				}
			}
			//////////////////////////////////////////////////////////////////////////
		}
	}
	else
	{//如果是第一次请求数据块，只索取最新的部分数据，可以避免请求过多早期数据	
		//////////////////////////////////////////////////////////////////////////
		int nend = 0;
		BOOL btmp=TRUE;
		DWORD dwendtime = 0;
		DWORD dwbegintime = 0;
		int i=0;
		for(i=0; i<m_nChunkNUM; i++)
		{
			if(i == 0)
			{
				dwbegintime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime;
			}
	
			int nindex = m_stBM.pnChunkIndex[i];//转为基址
			dwendtime = m_stBM.ChunksMap[nindex].stHead.unCTime;
			if(dwbegintime > 0 && dwendtime > 0)
			{
				unsigned int unChunkID = m_stBM.ChunksMap[nindex].stHead.unChunkID;
				if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData)
				{//数据块合法并且现在没数据，应该请求
					nend = i;			
				}
			}
					
			if(dwbegintime > dwendtime + 5000)
			{
				break;
			}
		}

		for(i=nend; i>=0; i--)
		{
			int nindex = m_stBM.pnChunkIndex[i];//转为基址
			dwendtime = m_stBM.ChunksMap[nindex].stHead.unCTime;
			if(dwendtime > 0)
			{
				unsigned int unChunkID = m_stBM.ChunksMap[nindex].stHead.unChunkID;
				if(unChunkID > 0 && !m_stBM.ChunksMap[nindex].bHaveData)
				{//数据块合法并且现在没数据，应该请求
					if(m_bNoData && btmp)
					{//将播放指针放到第一个请求的位置
						btmp = FALSE;
						m_stBM.nNeedPlayIndex = nindex;
						m_stBM.unNeedPlayID = unChunkID;
						m_stBM.bNeedI = TRUE;
						m_stBM.bNeedA = FALSE;
					}
					
					STREQ streq;
					streq.bAnswer = FALSE;
					streq.dwStartTime = 0;
					streq.bNEED = TRUE;//FALSE;
					streq.unChunkID = unChunkID;
					stpullreqs.push_back(streq);
					
					ncount++;
//////////////////////////////////////////////////////////////////////////
//if(m_nChannel == 2)
//{
//	char ch[100]={0};
//	sprintf(ch,"first req ID:%d\n",streq.unChunkID);
//	OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
					
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		
		
		if(ncount > 0)
		{
			m_stSTAT.unBeginReqID = stpullreqs[0].unChunkID;

//			char ch[100]={0};
//			sprintf(ch,"first req ID:%d\n",stpullreqs[0].unChunkID);
//			OutputDebugString(ch);
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif

	if(ncount > 0)
	{
		if(m_nRate == 100)
		{//缓冲充足，进行随机获取，否则进行顺序获取
			std::random_shuffle(stpullreqs.begin(), stpullreqs.end());
		}
		
		return TRUE;
	}
	return FALSE;
}


BOOL CCMultiBufferCtrl::ReadREQData(unsigned int unChunkID, BYTE *pBuffer, int &nLen)
{
	if(unChunkID <= 0 || pBuffer == NULL)
	{
		return FALSE;
	}

#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	for(int i=0; i<m_nChunkNUM; i++)
	{//从最新数据块向后遍历，看是否能找到指定数据块
		if(unChunkID == m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID)
		{//找到	
			if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData 
				&& m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset > 0 
				&& m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset <= JVN_CHUNK_LEN)
			{//数据有效
				//数据类型(1)+数据长度(4)+数据块头(?)+数据片内容(?)
				STCHUNKHEAD stHead;
				stHead.bHaveI = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.bHaveI;
				stHead.lWriteOffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset;
				stHead.unChunkID = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID;
				stHead.nIOffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.nIOffset;
				stHead.nNOffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.nNOffset;
				stHead.unCTime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime;

				pBuffer[0] = JVN_RSP_BMD;
				nLen = sizeof(STCHUNKHEAD) + stHead.lWriteOffset;
				memcpy(&pBuffer[1], &nLen, 4);
				memcpy(&pBuffer[5], &(stHead), sizeof(STCHUNKHEAD));
				memcpy(&pBuffer[5+sizeof(STCHUNKHEAD)], &m_stBM.pBuffer[m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].lBeginPos], stHead.lWriteOffset);
				
				nLen += 5;

			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return TRUE;
			}
			break;//数据无效，跳出遍历
		}
	}

	//请求失败
	//数据类型(1) + 数据总长度(4) + nChunkID(4)
	pBuffer[0] = JVN_RSP_BMDNULL;
	nLen = 4;
	memcpy(&pBuffer[1], &nLen, 4);
	memcpy(&pBuffer[5], &(unChunkID), 4);
	nLen = 9;

	//更新对方的缓存，可避免对方再次请求本地没有的数据
	BYTE uchtemp=0x01;
	BYTE uchtempFF=0xFF;
	int ncount = 0;
	int ntotal = ((m_stBM.nOldestChunk>(m_nChunkNUM-1-JVN_CHUNK_PRIVATE))?(m_nChunkNUM-1-JVN_CHUNK_PRIVATE):m_stBM.nOldestChunk);
	int nlastindex = m_stBM.pnChunkIndex[ntotal];//最后一个数据块的基址
	unChunkID = m_stBM.ChunksMap[nlastindex].stHead.unChunkID;
	if(unChunkID > 0)
	{
		//类型(1)+总长度(4)+CHUNKID(4)+chunk总数(4)+[BM(?)]
		pBuffer[9] = JVN_CMD_BM;
		memcpy(&pBuffer[14], &unChunkID, 4);
		memcpy(&pBuffer[18], &ntotal, 4);
		for(int i=ntotal; i>0; i--)
		{
			if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].bHaveData && m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.lWriteOffset > 0)
			{//有数据 对应位置1
				pBuffer[22+(ncount/8)] = (pBuffer[22+(ncount/8)] | (uchtemp<<(7-ncount%8)));
			}
			else
			{//无数据 对应位置0
				pBuffer[22+(ncount/8)] = (pBuffer[22+(ncount/8)] & (uchtempFF^(uchtemp<<(7-ncount%8))));
			}
			ncount ++;
		}
		
		int nByte = ncount/8 + ((ncount%8)?1:0);
		nLen = 8+nByte;
		memcpy(&pBuffer[10], &nLen, 4);
		
		nLen = 22+nByte;
	}	

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}

//添加新BM
void CCMultiBufferCtrl::AddNewBM(unsigned int unChunkID, int ncount, DWORD dwCTime[10], unsigned int &unNewID, unsigned int &unOldID, BOOL bA)
{
    if(unChunkID < 1 || ncount <= 0)
	{
		return;
	}

//////////////////////////////////////////////////////////////////////////
//char ch2[200]={0};
//sprintf(ch2,"addnewbm ID:%d unchunkid:%d, ncount:%d, chunk0:%d, playid:%d, needplayid:%d\n",
//		unChunkID+ncount-1,
//		unChunkID,ncount,m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID,m_stBM.unPlayedID,m_stBM.unNeedPlayID);
//if(m_nChannel == 2)
//{
//	OutputDebugString(ch2);
//}

//m_pLog->SetRunInfo(m_nChannel,"addnewbm!",__FILE__,__LINE__,ch2);
//////////////////////////////////////////////////////////////////////////

#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
		
	if((unChunkID+ncount-1) > m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID)
	{//新区间最新数据比本地新，可能需要更新			
		//计算新到数据块的数目
		int nNumNew = (unChunkID+ncount-1) - m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
		//缓存中空闲出的数据块数目
		int nNumBUF = 0;
		//实际可以增加的数据块数目
		int nNumWR = 0;
		if(nNumNew > 50000 && m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID > 0)
		{//新数据块来了超多，不正常，放弃更新
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
m_pLog->SetRunInfo(m_nChannel,"addnewbm ERROR! nNumNew>10000",__FILE__,__LINE__);
			return;
		}
		else
		{//新数据初步判断无异常

			//寻找当前播放点，做记录
			BOOL blast = FALSE;
			//从本地最后节点向前寻找，找到最早的那个有数据但没播放的数据块，之前的都可以替换
			int nindex = 0;
			nindex = m_stBM.pnChunkIndex[m_nChunkNUM-1];
			if(nindex == m_stBM.nNeedPlayIndex)
			{//找到播放指针
				blast = TRUE;
			}
//////////////////////////////////////////////////////////////////////////
/*			DWORD dw0=m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;
			DWORD dw1=m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
			if(dw0 > dw1 + 10000)
			{
				int nnext = NextIndex(m_stBM.nNeedPlayIndex);
				
				char ch[200]={0};
				if(nnext>=0)
				{
					sprintf(ch,"long!!!!!!!!!!!!!!![NeedID:%d][haove:%d][nexthave:%d]\n",
						m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID,
						m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].bHaveData,
						m_stBM.ChunksMap[nnext].bHaveData);
				}
				else
				{
					sprintf(ch,"long!!!!!!!!!!!!!!![NeedID:%d][haove:%d][000]\n",
						m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID,
						m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].bHaveData);
				}
				
				OutputDebugString(ch);
			}
*/
//////////////////////////////////////////////////////////////////////////

			//计算有多少数据需要被移除
			nNumBUF = m_nChunkNUM;
			if(nNumNew >= nNumBUF)
			{//新数据总数比当前缓存总数大，依照留新弃旧原则，旧数据将统统被移除
				//需要更新到缓存中的实际数据块数就是两者中的较小者 
				nNumWR = nNumBUF;
				m_stBM.nOldestChunk = -1;
				m_bNoData = TRUE;//重新开启首次接收数据特殊流程
				
				//向缓存中新增数据块
				for(int i=0; i<nNumWR; i++)
				{
					//清空当前缓存
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].ResetChunk();
					//新数据块
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID = unChunkID + ncount - 1 - i;

					//首数据块有明确时间
					for(int k=0; k<10; k++)
					{
						if(i==k)
						{
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime = dwCTime[k];
							//////////////////////////////////////////////////////////////////////////
//							char ch[100]={0};
//							sprintf(ch,"[ADDBM ID:%d][ct:%d]\n",m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID,dwCTime[k]);
//							OutputDebugString(ch);
							//////////////////////////////////////////////////////////////////////////
							
						}
					}
					
					//更新最早数据块位置
					m_stBM.nOldestChunk = ((m_stBM.nOldestChunk>=(m_nChunkNUM-1))?(m_nChunkNUM-1):(m_stBM.nOldestChunk+1));
				}
				
				//寻找新的关键帧
				m_stBM.bNeedI = TRUE;
				m_stBM.bNeedA = FALSE;
				
				//从最旧数据播放
				int tmp = ((m_stBM.nOldestChunk>=0)?m_stBM.nOldestChunk:(m_nChunkNUM-1));
				m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[tmp];
				m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			}
			else
			{//新数据总数比当前缓存总数小，旧数据有一部分是可以保留的，追加新数据
				//需要更新到缓存中的实际数据块数就是两者中的较小者 
				nNumWR = nNumNew;
				
				unsigned untmpid = 0;
				//向缓存中新增数据块
				while(nNumWR > 0)
				{
					untmpid = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
					//old舍弃 new 降级
					int ntmp = m_stBM.pnChunkIndex[m_nChunkNUM-1];
					for(int i=m_nChunkNUM-1; i>0; i--)
					{
						m_stBM.pnChunkIndex[i] = m_stBM.pnChunkIndex[i-1];
					}
					m_stBM.pnChunkIndex[0] = ntmp;
					
					//最后更新new为新的BM
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].ResetChunk();
					m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID = untmpid+1;
//					if(untmpid+1 == unChunkID+ncount-1)
//					{
//						m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime = dwCTime1;
//					}
					
					for(int k=0; k<10; k++)
					{
						if(untmpid+1 == unChunkID+ncount-1-k)
						{
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime = dwCTime[k];
							//////////////////////////////////////////////////////////////////////////
//							char ch[100]={0};
//							sprintf(ch,"[ADDBM ID:%d][ct:%d]\n",m_stBM.ChunksMap[m_stBM.pnChunkIndex[k]].stHead.unChunkID,dwCTime[k]);
//							OutputDebugString(ch);
							//////////////////////////////////////////////////////////////////////////
						}
					}

					
					nNumWR--;
					
					m_stBM.nOldestChunk = ((m_stBM.nOldestChunk>=(m_nChunkNUM-1))?(m_nChunkNUM-1):(m_stBM.nOldestChunk+1));
				}
				
				//从最旧数据播放
				if(blast)
				{
					int tmp = ((m_stBM.nOldestChunk>0)?m_stBM.nOldestChunk:(m_nChunkNUM-1));
					m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[tmp];
					m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
					//寻找新的关键帧
					m_stBM.bNeedI = TRUE;
					m_stBM.bNeedA = FALSE;
				}
			}
//char ch[1000];
//sprintf(ch,"addnewbm notfind p,...........  [old:%d  nowshouldp:%d]\n",
//		m_stBM.unNeedPlayID, 
//		m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID);
//OutputDebugString(ch);
			
			unNewID = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
			unOldID = m_stBM.ChunksMap[m_stBM.pnChunkIndex[m_nChunkNUM-1]].stHead.unChunkID;
			
//			m_bBMfromA = (bA?bA:m_bBMfromA);//只记录主控BM
		
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return;
		}
	}

	//将缓存中实际范围返回
	unNewID = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID;
	unOldID = m_stBM.ChunksMap[m_stBM.pnChunkIndex[m_nChunkNUM-1]].stHead.unChunkID;

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

int CCMultiBufferCtrl::NextIndex(int ncurindex)
{
	for(int i=m_nChunkNUM-1; i>0; i--)
	{
		if(m_stBM.pnChunkIndex[i] == ncurindex)
		{//找到当前位置
			return m_stBM.pnChunkIndex[i-1];
		}
	}

	//没找到当前索引，严重异常，从头开始
	return -1;
}

int CCMultiBufferCtrl::GetBufferPercent()
{
	DWORD dwPlayWait=0;//播放实际等待的时间
	DWORD dwCWait = 0;//播放画面等待的时间
	BOOL bHaveOldData=FALSE;//含有很老的数据，立即播放
	BOOL bHaveSomeData=FALSE;//没缓冲慢，但也有超过5s的数据了，立即播放
	m_dwSysNow = JVGetTime();
	if(m_dwSysNow < m_dwLastPlayTime)
	{
		m_dwLastPlayTime = m_dwSysNow;
	}

	//计算应该缓冲数据片个数
	if(m_unChunkTimeSpace > 0)
	{
		int nt = JVN_TIME_FIRSTWAIT/m_unChunkTimeSpace;
		if(nt>0 && nt<JVN_PLAYNOW && (abs(nt - m_nLastWaitCC) > 5))
		{
			m_nLastWaitCC = nt;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	int i=0;
	for(i=0; i<m_nChunkNUM; i++)
	{
		if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
		{
			DWORD dwendtime = 0;
			DWORD dwbegintime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unCTime;
			int nc = 0;
			for(int j=i; j>=0; j--)
			{
				dwendtime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime;
				if(dwbegintime > 0 && dwendtime > 0)
				{
					nc++;
				}
				
				if(dwendtime > dwbegintime + 500)//1500//2000//5000
				{
					break;
				}
			}
			
			if(nc > 0)
			{
				m_nLastWaitCC = nc;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	

	int nrate = 0;
	
	for(i=0; i<m_nChunkNUM; i++)
	{
		if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
		{
//			char chcc[200]={0};
//			sprintf(chcc,"*************************************************************[needplayID:%d][curID:%d]\n",m_stBM.unNeedPlayID,m_stBM.ChunksMap[m_stBM.pnChunkIndex[i]].stHead.unChunkID);
//			OutputDebugString(chcc);
			int nnn=0;
			int nRecv = 0;
			int ncheckcount = 0;
			DWORD dwlastbuftime = 0;
			DWORD dwcurbuftime = 0;
			for(int j=i; j>=0; j--)
			{
				unsigned int uid = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unChunkID;
				unsigned int uid1 = 0;
				unsigned int uid2 = 0;
				if(j>=1)
				{
					uid1 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j-1]].stHead.unChunkID;
				}
				if(j>=2)
				{
					uid2 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j-2]].stHead.unChunkID;
				}
				
				if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].bHaveData)
				{
//				nnn++;
//				char chh[100]={0};
//				sprintf(chh,"[ID:%d],nnn=%d, j=%d.............\n",uid,nnn,j);
//				OutputDebugString(chh);
					dwcurbuftime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime;

					nRecv++;
					if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime < m_dwLastCTime)
					{
						m_dwLastCTime = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime;
					}
					if(m_dwSysNow < m_dwLastPlayTime)
					{
						m_dwLastPlayTime = m_dwSysNow;
					}
					dwPlayWait = m_dwSysNow - m_dwLastPlayTime;
					dwCWait = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime - m_dwLastCTime;
					//数据已经到来了，只是整体没缓冲够，这种情况下已到达的数据不允许缓冲太久
					if(m_dwLastCTime > 0)
					{
						if(dwCWait >= m_unMaxWaitTime || dwPlayWait > 180000)
						{//卡了太久了(用数据片标准时间来计算时间，即图像内容为准)
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
							bHaveOldData = TRUE;
						}
						else
						{
//							if(dwlastbuftime > 0)
//							{
//								char ch[200]={0};
//								sprintf(ch,"[1count:%d][ID:%d, %d, %d][spt:%d][%d][%d]\n",
//									nnn,uid,uid1,uid2,dwcurbuftime-dwlastbuftime,dwlastbuftime,dwcurbuftime);
//								OutputDebugString(ch);
//							}
							
							if(dwlastbuftime > 0 && dwcurbuftime > dwlastbuftime + 2000)//4000
							{//已经缓存了超过5s的数据
								m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
								bHaveSomeData = TRUE;
							}
						}
					}
					else
					{
						if(dwPlayWait >= m_unMaxWaitTime)
						{//卡了太久了(用数据片标准时间来计算时间，即图像内容为准)
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
							bHaveOldData = TRUE;
						}
//						if(dwlastbuftime > 0)
//						{
//							char ch[200]={0};
//							sprintf(ch,"[2count:%d][ID:%d, %d, %d][spt:%d][%d][%d]\n",
//								nnn,uid,uid1,uid2,dwcurbuftime-dwlastbuftime,dwlastbuftime,dwcurbuftime);
//							OutputDebugString(ch);
//						}
						
						if(dwlastbuftime > 0 && dwcurbuftime > dwlastbuftime + 2000)
						{//已经缓存了超过2s的数据
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
							bHaveSomeData = TRUE;
						}
					}
					if(dwlastbuftime == 0)
					{//记录第一个连续的缓存包
						//////////////////////////////////////////////////////////////////////////
//						char ch[200]={0};
//						sprintf(ch,"dwlastbuftime==0 [count:%d][ID:%d, %d, %d][spt:%d][%d][%d]\n",
//							    nnn,uid,uid1,uid2,dwcurbuftime-dwlastbuftime,dwlastbuftime,dwcurbuftime);
//    					OutputDebugString(ch);
						
						//////////////////////////////////////////////////////////////////////////
						
						dwlastbuftime = dwcurbuftime;
					}
					
/**/
/*					//延时太长了也不允许再缓冲
					if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime > 0 && m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime > 0)
					{
						dwCWait = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime - m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime;
						if(dwCWait > 10000)
						{
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
							bHaveOldData = TRUE;
char ch[100]={0};
sprintf(ch,"GetBufferPercent relay.........[%d][%d,%d][%d,%d]\n",
		dwCWait,
		m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unChunkID,m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unChunkID,
		m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.unCTime, m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime);
OutputDebugString(ch);
						}
					}
*/
				}
				else
				{
//					if(dwlastbuftime > 0 || nnn > 0)
//					{
//						char ch[100]={0};
//						sprintf(ch,"dwlastbuftime>0 NULL!!! [count:%d][ID:%d, %d, %d][spt:%d]\n",nnn,uid,uid1,uid2,dwcurbuftime-dwlastbuftime);
//						OutputDebugString(ch);
//					}
					
					dwlastbuftime = 0;
				}
				
				ncheckcount++;
				if(ncheckcount >= m_nLastWaitCC)
				{//已经检查了从当前数据块开始的连续的JVN_PLAYNOW个
					break;
				}
			}
			
			nrate = (nRecv*100)/m_nLastWaitCC;
			
			if(nrate == 100)
			{
				//缓冲过的数据块更改缓冲标志
				ncheckcount = 0;
				for(int j=i; j>=0; j--)
				{
					if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].bHaveData)
					{
						m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
					}
					ncheckcount++;
					if(ncheckcount >= JVN_PLAYNOW)
					{//已经检查了从当前数据块开始的连续的JVN_PLAYNOW个
						break;
					}
				}

				m_stSTAT.finish();//缓冲结束
			}
			else
			{//缓冲中
				if(!bHaveOldData)
				{
					m_stSTAT.begin(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID);
				}
				if(bHaveSomeData)
				{//没缓冲满，但也有了超过5s的连续数据了，可以播
//					OutputDebugString("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
					for(int j=i; j>=0; j--)
					{
						if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].bHaveData && m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK)
						{
							 break;
						}
						if(m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].bHaveData)
						{
							m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bRateOK = TRUE;
						}
					}
		
					nrate = 100;//缓冲结束
					m_stSTAT.finish();//缓冲结束
				}
			}

			m_nRate = nrate;

			return nrate;
		}
	}

	m_nRate = 0;

	return -1;
}

void CCMultiBufferCtrl::ReadDataOnce(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead)
{
	memcpy(pBuffer, &m_stBM.pBuffer[nchunkoffset+nframeoffset+sizeof(STFRAMEHEAD)], stframehead.nSize);
	
	//当前数据块没有播完，只更改播放位置即可
	m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset += sizeof(STFRAMEHEAD);
	m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset += stframehead.nSize;

	long lleftcanuse = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - sizeof(STFRAMEHEAD);
	if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset > lleftcanuse)
	{//当前数据块已经完全播完，播放索引前移
//char ch[100];
//sprintf(ch,"readdataonce next play id  [%d -> %d]",m_stBM.unNeedPlayID, m_stBM.unNeedPlayID+1);
//OutputDebugString(ch);
	    if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset != m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset)
		{//数据异常，正常的数据应该是长度吻合的，下次要从关键帧开始播放
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;
m_pLog->SetRunInfo(m_nChannel,"readdataonce ERROR!",__FILE__,__LINE__);
		}
		m_stSTAT.jump(m_stBM.unPlayedID, m_stBM.unNeedPlayID);
		if(m_stBM.unPlayedID + 1 == m_stBM.unNeedPlayID)
		{
			m_unChunkTimeSpace = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime - m_dwLastCTime;
		}
		m_dwLastCTime = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
		m_stBM.unPlayedID = m_stBM.unNeedPlayID;
		m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bPlayed = TRUE;
		int ntmp = NextIndex(m_stBM.nNeedPlayIndex);
		if(ntmp >= 0)
		{
			m_stBM.nNeedPlayIndex = ntmp;
			m_stBM.unNeedPlayID++;
		}
		else if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
		{//没找到索引，异常，从开始处播放
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = FALSE;
			m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			m_stSTAT.unBufErrCount++;
m_pLog->SetRunInfo(m_nChannel,"readdataonce ERROR 2!",__FILE__,__LINE__);
		}
	}
	
	if(stframehead.uchType != JVN_DATA_A)
	{
		m_dwLastPlayTime = JVGetTime();//播放的数据块有变 更新时间
		m_dwLastFTime = stframehead.unFTime;
	}
//	m_dwLastPlayTime = JVGetTime();//播放的数据块有变 更新时间
//	m_dwLastFTime = stframehead.unFTime;
	m_unMaxWaitTime = JVN_TIME_MAXWAIT;//置为运行中缓冲允许最大时间
}
BOOL CCMultiBufferCtrl::ReadDataMore(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead)
{
	int nremain = stframehead.nSize;
	int nreadlen = 0;
	int ncurindex = m_stBM.nNeedPlayIndex;
	unsigned int uncurid = m_stBM.unNeedPlayID;
//unsigned int uncurid0=uncurid;	
	//计算第一数据块中可以读取多少数据
	int npartsize = m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset - sizeof(STFRAMEHEAD) - nframeoffset;
//int npartsize0 = npartsize;
	//读取数据
	memcpy(&pBuffer[nreadlen], &m_stBM.pBuffer[nchunkoffset+nframeoffset+sizeof(STFRAMEHEAD)], npartsize);
	//更新长度记录
	nreadlen += npartsize;
//int nreadlen0=nreadlen;
	nremain = stframehead.nSize - nreadlen;
	//读位置前移
	int ntmpindex = NextIndex(ncurindex);
	if(ntmpindex >= 0)
	{
		ncurindex = ntmpindex;
		uncurid++;
	}
	else
	{//异常，从头开始播放，本次失败
		if(ncurindex != m_stBM.pnChunkIndex[0])
		{
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = FALSE;
			m_stSTAT.unBufErrCount++;
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR 0!",__FILE__,__LINE__);
		}
		m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
		m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
	
		return FALSE;
	}
	
	//读取后续部分
	while(nremain > 0)
	{
		if(m_stBM.ChunksMap[ncurindex].stHead.unChunkID != uncurid)
		{//下一个数据块不是连续的，数据肯定丢了，寻找下一个关键帧
char ch[100]={0};
sprintf(ch,"need:%d, real:%d",uncurid,m_stBM.ChunksMap[ncurindex].stHead.unChunkID);
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR 0!",__FILE__,__LINE__,ch);

			m_stBM.nNeedPlayIndex = ncurindex;
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;//uncurid;
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;

			return FALSE;
		}

		if(m_stBM.ChunksMap[ncurindex].stPHead.bPlayed)
		{//已播放过的数据块，严重异常，放弃当前帧，从头开始播放
//char ch[1000];
//sprintf(ch,"readdatamore err!!! next play id  [%d -> %d]",m_stBM.unNeedPlayID, m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID);
//OutputDebugString(ch);
//m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR!",__FILE__,__LINE__);

			ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);//m_stBM.pnChunkIndex[0];
			if(ntmpindex < 0)
			{
				if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
				{
					m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
					m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR index=-1!",__FILE__,__LINE__);
				}
				m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
			}
			else
			{
				m_stBM.nNeedPlayIndex = ntmpindex;
			}
			
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = FALSE;
			
			return FALSE;
		}
		
		if(!m_stBM.ChunksMap[ncurindex].bHaveData)
		{//数据块没有数据，跳出，等完整了再播放
			DWORD dwt0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;
			DWORD dwt1 = m_stBM.ChunksMap[ncurindex].stHead.unCTime;
			if(dwt0 > dwt1 + 20000)
			{//延时了20秒以上，不等了
				m_stBM.nNeedPlayIndex = ncurindex;
				m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;//uncurid;
				m_stBM.bNeedI = TRUE;
				m_stBM.bNeedA = TRUE;
				return FALSE;
			}
			break;
		}
		
		if(m_stBM.ChunksMap[ncurindex].bHaveData
			&& (m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset <= 0 || m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset > JVN_CHUNK_LEN))
		{//有数据，但尺寸异常，属于严重异常，放弃当前帧，从头开始播放
//char ch[1000];
//sprintf(ch,"readdatamore err2!!! next play id  [%d -> %d]",m_stBM.unNeedPlayID, m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID);
//OutputDebugString(ch);
//m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR2!",__FILE__,__LINE__);

			ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);//m_stBM.pnChunkIndex[0];
			if(ntmpindex < 0)
			{
				if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
				{
					m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
					m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR2 index=-1!",__FILE__,__LINE__);
				}
				m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
			}
			else
			{
				m_stBM.nNeedPlayIndex = ntmpindex;
			}
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;
			
			return FALSE;
		}
		
		//有数据并且尺寸正常
		//计算本数据块中可以读取多少数据
		int npartsize = (m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset >= nremain)?(nremain):(m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset);
		
		//读取数据
		memcpy(&pBuffer[nreadlen], &m_stBM.pBuffer[m_stBM.ChunksMap[ncurindex].lBeginPos], npartsize);
		//更新长度记录
		nreadlen += npartsize;
		nremain = stframehead.nSize - nreadlen;
		
		//计算剩余多少数据需要读，如果读完，则更新播放位置，直接播放
		if(nreadlen >= stframehead.nSize)
		{//读取完毕
			m_stSTAT.jump(m_stBM.unPlayedID, m_stBM.unNeedPlayID);
			
			if(m_stBM.unPlayedID + 1 == uncurid-1)
			{
				m_unChunkTimeSpace = m_stBM.ChunksMap[ncurindex].stHead.unCTime - m_dwLastCTime;
			}

			if(stframehead.uchType != JVN_DATA_A)
			{
				m_dwLastPlayTime = JVGetTime();//播放的数据块更新时间
				m_dwLastFTime = stframehead.unFTime;
			}
//			m_dwLastPlayTime = JVGetTime();//播放的数据块更新时间
			m_dwLastCTime = m_stBM.ChunksMap[ncurindex].stHead.unCTime;
//			m_dwLastFTime = stframehead.unFTime;
			m_unMaxWaitTime = JVN_TIME_MAXWAIT;//置为运行中缓冲允许最大时间
			
			m_stBM.unPlayedID = uncurid-1;

			//将之前的数据块都置为已播放
			SetPlayed(ncurindex);
			
			//更改播放位置
			m_stBM.nNeedPlayIndex = ncurindex;
			m_stBM.unNeedPlayID = uncurid;

//////////////////////////////////////////////////////////////////////////
			if(m_stBM.ChunksMap[ncurindex].stPHead.lNextOffset != npartsize && (npartsize < m_stBM.ChunksMap[ncurindex].stHead.lWriteOffset))
			{
//				int l1=m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset;
//				int l2=m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset;
				STCHUNKHEAD stchead;
				stchead.bHaveI = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.bHaveI;
				stchead.lWriteOffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset;
				stchead.nIOffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nIOffset;
				stchead.nNOffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nNOffset;
				stchead.unChunkID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;

/*				STFRAMEHEAD stfhead;
				memcpy(&stfhead,&m_stBM.pBuffer[m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].lBeginPos + stchead.nNOffset], sizeof(STFRAMEHEAD));
               
				char ch[2000];
				sprintf(ch,"readdatamore ERROR3! \n[firstID:%d, firstptsize:%d, nextoffset:%d,Noffset:%d, bHaveI0:%d],\n[curID:%d,curptsize:%d,nextoffset:%d,Noffset:%d, bHaveI0:%d] \nps:%d \n[frameid1:%d framesize:%d frametype1:%d frameid2:%d framesize2:%d frametype2:%d]",
					    uncurid0,stframehead.nSize,0,0,0,
						uncurid,l2,l1,stchead.nNOffset,stchead.bHaveI,npartsize,
						stframehead.nFrameID,stframehead.nSize,stframehead.uchType,
						stfhead.nFrameID,stfhead.nSize,stfhead.uchType);
				m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR3!",__FILE__,__LINE__,ch);
*/
				if(npartsize == stchead.nNOffset)
				{
					m_stBM.ChunksMap[ncurindex].stPHead.lNextOffset = npartsize;
//m_pLog->SetRunInfo(m_nChannel,"readdatamore npartsize == noffset!!!!!!!!",__FILE__,__LINE__);
				}
			}
//////////////////////////////////////////////////////////////////////////
			
			long lleftcanuse = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - sizeof(STFRAMEHEAD);
			if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset > lleftcanuse)
			{//当前数据块已经完全播完，播放索引前移
				m_stBM.unPlayedID = uncurid;
				if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset != m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset)
				{//数据异常，正常的数据应该是长度吻合的，下次要从关键帧开始播放
//					int l1=m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset;
//					int l2=m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset;
					m_stBM.bNeedI = TRUE;
					m_stBM.bNeedA = TRUE;
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR4!",__FILE__,__LINE__);
				}
				m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bPlayed = TRUE;
				ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
				if(ntmpindex >= 0)
				{
					m_stBM.nNeedPlayIndex = ntmpindex;
					m_stBM.unNeedPlayID++;
				}
				else
				{//异常
					if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
					{
						m_stBM.bNeedI = TRUE;
						m_stBM.bNeedA = TRUE;
						m_stSTAT.unBufErrCount++;
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR4! index=-1",__FILE__,__LINE__);
					}
					m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
					m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
				}
			}
			
			return TRUE;
		}
		
		//计算剩余数据
		nremain = stframehead.nSize - nreadlen;
		//当前数据块仍未读完整，读位置前移，继续读下一数据块
		ntmpindex = NextIndex(ncurindex);
		if(ntmpindex < 0)
		{
			if(ncurindex == m_stBM.pnChunkIndex[0])
			{//已是第一个数据块，没有后续数据，数据不完整，跳出继续接受
				break;
			}

			m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = FALSE;
			m_stSTAT.unBufErrCount++;
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR5! index=-1",__FILE__,__LINE__);			
			return FALSE;
		}
		else
		{
			ncurindex = ntmpindex;
			uncurid++;
		}
	}
	
	//没有读取完整，后半部分未收全，将当前数据块置为需要缓冲
	m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bRateOK = FALSE;

	//检查是否进入危险区，危险区的数据不缓冲，没有的直接跳到下一块
//	for(int i=m_nChunkNUM-1; i>=max(m_nChunkNUM-JVN_PLAYIMD,0); i--)
	for(int i=m_nChunkNUM-1; i>=jvs_max(m_nChunkNUM-m_nPLAYIMD,0); i--)
	{
		if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
		{//当前块已进入危险区，不需要等待，直接跳过，播放下一个数据块
//OutputDebugString("p................!bhavedata  should play imd!!!\n");
			ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
			if(ntmpindex < 0)
			{
				if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
				{
					m_stSTAT.unBufErrCount++;
m_pLog->SetRunInfo(m_nChannel,"readdatamore ERROR6! index=-1",__FILE__,__LINE__);
				}
				m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
			}
			else
			{
				m_stBM.nNeedPlayIndex = ntmpindex;
			}
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;
			
			return FALSE;
		}
	}

	return FALSE;
}

void CCMultiBufferCtrl::SetPlayed(int ncurindex)
{
	for(int i=0; i<m_nChunkNUM; i++)
	{
		if(ncurindex == m_stBM.pnChunkIndex[i])
		{//找到当前数据块，这之前的所有数据都应置为已播放
			for(int j=i+1; j<m_nChunkNUM; j++)
			{
				//设置
				m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.bPlayed = TRUE;
				m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stPHead.lNextOffset = m_stBM.ChunksMap[m_stBM.pnChunkIndex[j]].stHead.lWriteOffset;
				
				if(m_stBM.pnChunkIndex[j] == m_stBM.nNeedPlayIndex)
				{//不需要全部设置，达到当前播放点就可以退出，因为之前的肯定已经播放过
					return;
				}
			}
		}
	}
}

//读取一帧数据播放
BOOL CCMultiBufferCtrl::ReadPlayBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize, int &nBufferRate)
{
	if(m_bNoData)
	{
		nBufferRate = 0;
		return FALSE;
	}

	if(m_bFirstWait)
	{
		if(JVGetTime() < m_dwBeginBFTime + 4000)//6000
		{
			nBufferRate = 0;
			return FALSE;//第一次播放必须要在3秒钟以后
		}
		m_bFirstWait = FALSE;
	}

	int ntmpindex = 0;
	static DWORD dws=0;
	nBufferRate = -1;
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
//OutputDebugString("ReadPlayBuffer...........\n");	
	if(m_stBM.nNeedPlayIndex >= 0 && m_stBM.nNeedPlayIndex < m_nChunkNUM)
	{//根据播放索引，直接找到对应的数据块
		if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID == m_stBM.unNeedPlayID)
		{//校验一致，如果没播过可以进入播放环节

			if(!m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bPlayed && m_stBM.unNeedPlayID >= m_stBM.unPlayedID)
			{//未播放过的数据块

				if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].bHaveData
					&& m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset > 0
					&& m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset <= JVN_CHUNK_LEN)
				{//数据块有数据，读取待播放帧头

					long lleftcanuse = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - sizeof(STFRAMEHEAD);
					if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset > lleftcanuse)
					{//当前数据块已经完全播放完毕

						ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);//播放下一数据块
						if(ntmpindex >= 0)
						{
							m_stBM.nNeedPlayIndex = ntmpindex;
							m_stBM.unNeedPlayID++;//下一个数据块的校验ID
						}
						else
						{
							if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
							{
								m_stBM.bNeedI = TRUE;
								m_stBM.bNeedA = TRUE;
								m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
								m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
//								OutputDebugString("readplaybuffer ERROR! index=-1\n");
							}
							
							m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
							m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
						}
						
					#ifndef WIN32
						pthread_mutex_unlock(&m_ct);
					#else
						LeaveCriticalSection(&m_ct);
					#endif
						
						return FALSE;
					}

					STFRAMEHEAD stframehead;
					int nchunkoffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].lBeginPos;
					int nframeoffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset;
					int nnoffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nNOffset;

					
					memcpy(&stframehead, &m_stBM.pBuffer[nchunkoffset+nframeoffset], sizeof(STFRAMEHEAD));
					if(nnoffset < 0 || stframehead.nSize < 0 || stframehead.nSize > 500000)
					{//当前数据块没有帧头，读不出有效帧，放弃该帧，需要从下个关键帧播放
					 //帧尺寸过大(超过500K，不是严格值)，严重异常，从下个关键帧开始播放
						m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
//						char ch[500]={0};
//						sprintf(ch,"size:%d, fid:%d, type:%d, time:%d, coffset:%d, foffset:%d, noffset:%d",
//							    stframehead.nSize,stframehead.nFrameID,stframehead.uchType,stframehead.unFTime,
//								nchunkoffset, nframeoffset, nnoffset);
//						m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! >500k",__FILE__,__LINE__,ch);

						ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
						if(ntmpindex < 0)
						{
							if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
							{
								m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
								m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
							}
							
							m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
						}
						else
						{
							m_stBM.nNeedPlayIndex = ntmpindex;
						}
						m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
						m_stBM.bNeedI = TRUE;
						m_stBM.bNeedA = TRUE;

					#ifndef WIN32
						pthread_mutex_unlock(&m_ct);
					#else
						LeaveCriticalSection(&m_ct);
					#endif
						
						return FALSE;
					}
					
					if(!m_stBM.bNeedI)
					{//不需要寻找关键帧，终于确定这就是要播的帧

						if(!m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bRateOK)
						{//当前数据块不是缓冲好的数据，需要先进行缓冲

							BOOL bInDen = FALSE;

							//检查是否进入危险区，危险区的数据不缓冲，没有的直接跳到下一块
							//for(int i=m_nChunkNUM-1; i>=max(m_nChunkNUM-JVN_PLAYIMD,0); i--)
							for(int i=m_nChunkNUM-1; i>=jvs_max(m_nChunkNUM-m_nPLAYIMD,0); i--)
							{
								if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
								{//当前块已进入危险区，不需要等待，直接跳过，播放下一个数据块

									bInDen = TRUE;
									break;
								}
							}
							
							//////////////////////////////////////////////////////////////////////////
							DWORD dwt0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;
							DWORD dwt1 = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
							if(dwt0 > dwt1 + 20000)
							{
								bInDen = TRUE;
//								OutputDebugString("20s.........................\n");
							}
							//////////////////////////////////////////////////////////////////////////

							if(!bInDen)
							{
								//不在危险区，正常缓冲
								nBufferRate = GetBufferPercent();
								if(nBufferRate != 100)
								{//缓冲的不够，暂不播放
									if(nBufferRate < 0)
									{//没找到当前索引，严重异常，重头开始播放
										m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
										m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
										m_stBM.bNeedI = TRUE;
										m_stBM.bNeedA = FALSE;
										m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
										m_pLog->SetRunInfo(m_nChannel, "有数据 不需要关键帧 缓冲的不够 索引异常！", __FILE__,__LINE__);
									}

								#ifndef WIN32
									pthread_mutex_unlock(&m_ct);
								#else
									LeaveCriticalSection(&m_ct);
								#endif
									
									return FALSE;
								}
							}
							m_stSTAT.finish();//危险区的数据不缓冲直接播放，因此缓冲过程应该立即结束
						}

						//当前数据块已是缓冲好的数据
						m_dwSysNow = JVGetTime();
					  //DWORD dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime, m_nFrameTime*3/2);//图像本身的时间戳
						DWORD dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-9, 200);//70//图像本身的时间戳100
						DWORD dwt1 = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
						DWORD dwt0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;

						if(dwt0 > dwt1 + 40000)//60000
						{//延时大于40s，快进追赶
							dwtmp = 1;
						}
						else if(dwt0 > dwt1 + 20000)
						{//延时大于20s，快进追赶
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-20, 70);
						}
						else if(dwt0 > dwt1 + 15000)//6000
						{//延时大于15s，快进追赶
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-18, 70);
						}
						else if(dwt0 > dwt1 + 6000)
						{//延时大于6s，普通加速播放
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-15, 70);
						}
						else if(m_bLan2A && dwt0 > dwt1 + 5000)//5000
						{//局域网里延时大于4s就要提速
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-14, 70);
						}
						else if(m_bLan2A && dwt0 > dwt1 + 4000)//5000
						{//局域网里延时大于3s就要提速
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-13, 70);
						}
						else if(!m_bLan2A && dwt0 > dwt1 + 5000)//5000
						{//外网里延时大于5s要提速
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-11, 70);
						}

//						if(JVGetTime() < m_dwBeginBFTime + 16000)
//						{//最初的15秒不进行提速
//							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime-8, 70);
//						}
						if(dwt0 <= dwt1 + 1000)
						{//延时小于1s，放慢
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime+5, 200);
						}
						else if(dwt0 <= dwt1 + 2000)//60000
						{//延时小于2s，放慢
							dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime+4, 200);
						}

						dwtmp = jvs_max(dwtmp, 1);//图像本身的时间戳
						
						if(stframehead.uchType == JVN_DATA_A || m_dwSysNow >= m_dwLastPlayTime+dwtmp)
						{//播放时间到达，直接播放，更新下次播放位置
							uchType = stframehead.uchType;
							nSize = stframehead.nSize;

							if((stframehead.nSize + sizeof(STFRAMEHEAD)) <= (m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - nframeoffset))
							{//当前帧全部在当前数据块,一次读取
								ReadDataOnce(pBuffer, nchunkoffset, nframeoffset, stframehead);

							#ifndef WIN32
								pthread_mutex_unlock(&m_ct);
							#else
								LeaveCriticalSection(&m_ct);
							#endif
								
//////////////////////////////////////////////////////////////////////////
//if(m_nChannel == 2)
//{
//char ch[200]={0};
//sprintf(ch,"ID:%d, frameID:%d, framesize:%d, frametype:%d, stftime:%u\n",m_stBM.unNeedPlayID,stframehead.nFrameID,stframehead.nSize,stframehead.uchType,stframehead.unFTime);
//OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
								return TRUE;
							}
							else
							{//当前帧只有一部分在当前数据块，需多次读取
								BOOL bret = ReadDataMore(pBuffer, nchunkoffset, nframeoffset, stframehead);

							#ifndef WIN32
								pthread_mutex_unlock(&m_ct);
							#else
								LeaveCriticalSection(&m_ct);
							#endif
								
//////////////////////////////////////////////////////////////////////////
//if(bret && m_nChannel == 2)
//{
//	char ch[200]={0};
//	sprintf(ch,"ID:%d, frameID:%d, framesize:%d, frametype:%d, stftime:%u\n",m_stBM.unNeedPlayID,stframehead.nFrameID,stframehead.nSize,stframehead.uchType,stframehead.unFTime);
//	OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
								return bret;
							}
						}
						else
						{//播放时间未到，延时等待
						#ifndef WIN32
							pthread_mutex_unlock(&m_ct);
						#else
							LeaveCriticalSection(&m_ct);
						#endif
							
							return FALSE;
						}
					}
					else
					{//需要寻找关键帧

						if(stframehead.uchType == JVN_DATA_I)
						{//当前帧是关键帧,终于确定这就是要播的帧

							if(!m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.bRateOK)
							{//当前数据块不是缓冲好的数据，需要先进行缓冲

								BOOL bInDen = FALSE;
								//检查是否进入危险区，危险区的数据不缓冲，没有的直接跳到下一块
								//for(int i=m_nChunkNUM-1; i>=max(m_nChunkNUM-JVN_PLAYIMD,0); i--)
								for(int i=m_nChunkNUM-1; i>=jvs_max(m_nChunkNUM-m_nPLAYIMD,0); i--)
								{
									if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
									{//当前块已进入危险区，不需要等待，直接跳过，播放下一个数据块
//OutputDebugString("p................!bhavedata  should play imd!!!\n");
										bInDen = TRUE;
										break;
									}
								}

								//////////////////////////////////////////////////////////////////////////
								DWORD dwt0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;
								DWORD dwt1 = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
								if(dwt0 > dwt1 + 20000)
								{
									bInDen = TRUE;
//OutputDebugString("20sI.........................\n");
								}
								//////////////////////////////////////////////////////////////////////////

								if(!bInDen)
								{
									nBufferRate = GetBufferPercent();
									if(nBufferRate != 100)
									{//缓冲的不够，暂不播放
										if(nBufferRate < 0)
										{//没找到当前索引，严重异常，重头开始播放
											m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
											m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
											m_stBM.bNeedI = TRUE;
											m_stBM.bNeedA = FALSE;
											m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
											m_pLog->SetRunInfo(m_nChannel, "有数据 需要关键帧 缓冲的不够 索引异常！", __FILE__,__LINE__);
										}
									#ifndef WIN32
										pthread_mutex_unlock(&m_ct);
									#else
										LeaveCriticalSection(&m_ct);
									#endif
										
										return FALSE;
									}
								}
								m_stSTAT.finish();//危险区的数据不缓冲直接播放，因此缓冲过程应该立即结束
							}

							//当前数据块已是缓冲好的数据
							m_dwSysNow = JVGetTime();
							DWORD dwtmp = jvs_min(stframehead.unFTime-m_dwLastFTime, m_nFrameTime);//图像本身的时间戳//0
							dwtmp = jvs_max(dwtmp, 1);
//							if(dwtmp > 10000)
//							{
//								OutputDebugString("playerror   timeerror\n");
//							}

							if(m_dwSysNow >= m_dwLastPlayTime+dwtmp)
							{//播放时间到达，直接播放，更新下次播放位置
								uchType = stframehead.uchType;
								nSize = stframehead.nSize;
								
								if((stframehead.nSize + sizeof(STFRAMEHEAD)) <= (m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - nframeoffset))
								{//当前帧全部在当前数据块,一次读取
									ReadDataOnce(pBuffer, nchunkoffset, nframeoffset, stframehead);
									
									//寻找关键帧标识复位，进入正常流程
									m_stBM.bNeedI = FALSE;
									m_stBM.bNeedA = FALSE;

								#ifndef WIN32
									pthread_mutex_unlock(&m_ct);
								#else
									LeaveCriticalSection(&m_ct);
								#endif

//////////////////////////////////////////////////////////////////////////
//if(m_nChannel == 2)
//{									
//char ch[200]={0};
//sprintf(ch,"ID:%d, frameID:%d, framesize:%d, frametype:%d, stftime:%u\n",m_stBM.unNeedPlayID,stframehead.nFrameID,stframehead.nSize,stframehead.uchType,stframehead.unFTime);
//OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
									
									return TRUE;
								}
								else
								{//当前帧只有一部分在当前数据块，需多次读取
									BOOL bret = ReadDataMore(pBuffer, nchunkoffset, nframeoffset, stframehead);
									if(bret)
									{
										//寻找关键帧标识复位，进入正常流程
										m_stBM.bNeedI = FALSE;
										m_stBM.bNeedA = FALSE;
									}
									else
									{
										nBufferRate = -1;//数据不足，再次缓冲
									}
								#ifndef WIN32
									pthread_mutex_unlock(&m_ct);
								#else
									LeaveCriticalSection(&m_ct);
								#endif

//////////////////////////////////////////////////////////////////////////
//if(bret && m_nChannel == 2)
//{
//	char ch[200]={0};
//	sprintf(ch,"ID:%d, frameID:%d, framesize:%d, frametype:%d, stftime:%u\n",m_stBM.unNeedPlayID,stframehead.nFrameID,stframehead.nSize,stframehead.uchType,stframehead.unFTime);
//	OutputDebugString(ch);
//}
//////////////////////////////////////////////////////////////////////////
									
									return bret;
								}
							}
							else
							{//播放时间未到，延时等待
							#ifndef WIN32
								pthread_mutex_unlock(&m_ct);
							#else
								LeaveCriticalSection(&m_ct);
							#endif
								
								return FALSE;
							}
						}
						else
						{//当前不是关键帧
							if(m_stBM.bNeedA && stframehead.uchType == JVN_DATA_A)
							{//如果需要保留音频帧，且当前就是音频帧，则直接播放音频，更新下次播放位置
								uchType = stframehead.uchType;
								nSize = stframehead.nSize;
								
								if((stframehead.nSize + sizeof(STFRAMEHEAD)) <= (m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.lWriteOffset - nframeoffset))
								{//当前帧全部在当前数据块,一次读取
									ReadDataOnce(pBuffer, nchunkoffset, nframeoffset, stframehead);

								#ifndef WIN32
									pthread_mutex_unlock(&m_ct);
								#else
									LeaveCriticalSection(&m_ct);
								#endif
									
									return TRUE;
								}
								else
								{//当前帧只有一部分在当前数据块，需多次读取
									BOOL bret = ReadDataMore(pBuffer, nchunkoffset, nframeoffset, stframehead);
									if(!bret)
									{//读取失败，不播放，需要继续向下寻找关键帧
										nBufferRate = 0;
									}
									
								#ifndef WIN32
									pthread_mutex_unlock(&m_ct);
								#else
									LeaveCriticalSection(&m_ct);
								#endif
									
									return bret;
								}
							}
							else
							{//不播放，需要继续向下寻找关键帧
								if(m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.bHaveI 
									&& m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nIOffset >= 0
									&& m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset < m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nIOffset)
								{//本数据块含有没读取的关键帧,将播放偏移移到关键帧处
									m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stPHead.lNextOffset = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.nIOffset;
								}
								else
								{//本数据块没有关键帧,寻找下一数据块
									m_stSTAT.unNoICount++;
									
									ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
									if(ntmpindex >= 0)
									{
										m_stBM.nNeedPlayIndex = ntmpindex;
										m_stBM.unNeedPlayID++;
									}
									else
									{
										if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
										{
											m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
											m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
										}
										m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
										m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
										m_stBM.bNeedI = TRUE;
										m_stBM.bNeedA = TRUE;
									}
								}

								nBufferRate = 0;
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
				else
				{//数据块无数据
					//检查是否进入危险区，危险区的数据不缓冲，没有的直接跳到下一块
					//for(int i=m_nChunkNUM-1; i>=max(m_nChunkNUM-JVN_PLAYIMD,0); i--)
					for(int i=m_nChunkNUM-1; i>=jvs_max(m_nChunkNUM-m_nPLAYIMD,0); i--)
					{
						if(m_stBM.nNeedPlayIndex == m_stBM.pnChunkIndex[i])
						{//当前块已进入危险区，不需要等待，直接跳过，播放下一个数据块
							m_stSTAT.finish();//危险区的数据不缓冲直接播放，因此缓冲过程应该立即结束
							
							nBufferRate = GetBufferPercent();
							m_dwSysNow = JVGetTime();
							if(nBufferRate == 0 && (m_dwSysNow > m_dwLastPlayTime + JVN_TIME_MAXWAIT))
							{//在危险区缓冲为0 持续了很长时间 很可能数据延迟太多 播放向后移 舍弃一部分数据
								m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[(m_nChunkNUM-1)/2];
								m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;

								m_stBM.bNeedI = TRUE;
								m_stBM.bNeedA = TRUE;
								
								nBufferRate = 0;//-1;

							#ifndef WIN32
								pthread_mutex_unlock(&m_ct);
							#else
								LeaveCriticalSection(&m_ct);
							#endif
								

//////////////////////////////////////////////////////////////////////////
m_pLog->SetRunInfo(m_nChannel,"readplaybuffer wait long time rate0 jump1/2! ",__FILE__,__LINE__);
								
								return FALSE;
							}
							else
							{
								ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
								if(ntmpindex >= 0)
								{
									m_stBM.nNeedPlayIndex = ntmpindex;
									m_stBM.unNeedPlayID++;
								}
								else
								{
									if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
									{
										m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
										m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
									}
									m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
									m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
								}
								
								m_stBM.bNeedI = TRUE;
								m_stBM.bNeedA = TRUE;
								
								nBufferRate = 0;//-1;
								
								m_stSTAT.unNoDataCount++;
								
							#ifndef WIN32
								pthread_mutex_unlock(&m_ct);
							#else
								LeaveCriticalSection(&m_ct);
							#endif
								
								return FALSE;
							}
						}
					}

					//////////////////////////////////////////////////////////////////////////
					DWORD dwt0 = m_stBM.ChunksMap[m_stBM.pnChunkIndex[0]].stHead.unCTime;
					DWORD dwt1 = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unCTime;
					if(dwt0 > dwt1 + 20000)
					{
//OutputDebugString("000000000000000\n");
						ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
						if(ntmpindex >= 0)
						{
							m_stBM.nNeedPlayIndex = ntmpindex;
							m_stBM.unNeedPlayID++;
						}
						else
						{
							if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
							{
								m_stSTAT.unBufErrCount++;
								//////////////////////////////////////////////////////////////////////////
								m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
							}
							m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
							m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
						}
						
						m_stBM.bNeedI = TRUE;
						m_stBM.bNeedA = TRUE;
						
						nBufferRate = 0;//-1;
						
						m_stSTAT.unNoDataCount++;
						
					#ifndef WIN32
						pthread_mutex_unlock(&m_ct);
					#else
						LeaveCriticalSection(&m_ct);
					#endif
						return FALSE;
					}
					//////////////////////////////////////////////////////////////////////////

					m_dwSysNow = JVGetTime();
					if(m_dwLastPlayTime > 0 && (m_dwSysNow > m_dwLastPlayTime + JVN_TIME_MAXWAIT))
					{//缓冲了太久了，不能再等待了，否则影响用户体验，开始往后跳
						//不在危险区，则肯定需要缓冲等待，提示缓冲进度
						nBufferRate = GetBufferPercent();
						if((nBufferRate == 0 && (m_dwSysNow > m_dwLastJump + JVN_TIME_MAXWAIT)))
						{//缓冲了很久，缓冲进度一直为0，很可能到了临界状态，果断往后跳
							m_dwLastJump = m_dwSysNow;

							m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
							m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
							m_stBM.bNeedI = TRUE;
							m_stBM.bNeedA = TRUE;
							
							nBufferRate = 0;//-1;
							m_stSTAT.unNoDataCount++;

						#ifndef WIN32
							pthread_mutex_unlock(&m_ct);
						#else
							LeaveCriticalSection(&m_ct);
						#endif
							
//////////////////////////////////////////////////////////////////////////
m_pLog->SetRunInfo(m_nChannel,"readplaybuffer wait long time rate0 jump0! ",__FILE__,__LINE__);
							return FALSE;
						}
/*
						if(m_dwSysNow > m_dwLastPlayTime + 180000)
						{//如果超过3分钟仍未播放，估计很难继续进行了
							ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
							if(ntmpindex >= 0)
							{
								m_stBM.nNeedPlayIndex = ntmpindex;
								m_stBM.unNeedPlayID++;
							}
							else
							{
								if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
								{
									m_stSTAT.unBufErrCount++;
									//////////////////////////////////////////////////////////////////////////
									m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
								}
								m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
								m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
							}
							
							m_stBM.bNeedI = TRUE;
							m_stBM.bNeedA = TRUE;
							
							nBufferRate = 0;//-1;
							
							m_stSTAT.unNoDataCount++;
							
							LeaveCriticalSection(&m_ct);
//////////////////////////////////////////////////////////////////////////
m_pLog->SetRunInfo(m_nChannel,"readplaybuffer wait > 3min, no hope! jumpnext.",__FILE__,__LINE__);
							return FALSE;
							
						}
*/
					}

					//不在危险区，则肯定需要缓冲等待，提示缓冲进度
					nBufferRate = GetBufferPercent();
					if(nBufferRate < 0)
					{//没找到当前索引，严重异常，重头开始播放
						m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
						m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
						m_stBM.bNeedI = TRUE;
						m_stBM.bNeedA = FALSE;
						m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
						m_pLog->SetRunInfo(m_nChannel, "无数据 在危险区 缓冲进度 没有找到索引", __FILE__,__LINE__);
					}
					
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return FALSE;
				}
			}
			else
			{//已播放过该数据块，需要从当前块向前找到一个没播过的关键帧开始播放，重新缓冲，不留音频帧
				m_stBM.bNeedI = TRUE;
				m_stBM.bNeedA = FALSE;

				ntmpindex = NextIndex(m_stBM.nNeedPlayIndex);
				if(ntmpindex < 0)
				{
					if(m_stBM.nNeedPlayIndex != m_stBM.pnChunkIndex[0])
					{
						m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
						m_pLog->SetRunInfo(m_nChannel,"readplaybuffer ERROR! index=-1",__FILE__,__LINE__);
					}
					m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
				}
				else
				{
					m_stBM.nNeedPlayIndex = ntmpindex;
				}
				m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;

			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
		else
		{//不一致，已出现异常或丢数据，需要从当前块向前找到一个没播过的关键帧开始播放，重新缓冲，保留音频帧
			m_stBM.bNeedI = TRUE;
			m_stBM.bNeedA = TRUE;

			//重新校验，从当前开始寻找关键帧
			m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;

		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return FALSE;
		}
	}
	else
	{//播放索引无效, 没找到当前索引，严重异常，重头开始播放
		m_stBM.nNeedPlayIndex = m_stBM.pnChunkIndex[0];
		m_stBM.unNeedPlayID = m_stBM.ChunksMap[m_stBM.nNeedPlayIndex].stHead.unChunkID;
		m_stBM.bNeedI = TRUE;
		m_stBM.bNeedA = FALSE;
		m_stSTAT.unBufErrCount++;
//////////////////////////////////////////////////////////////////////////
		m_pLog->SetRunInfo(m_nChannel, "索引无效 严重异常", __FILE__,__LINE__);		
		
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
	
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//单播缓存类
CCSingleBufferCtrl::CCSingleBufferCtrl():CCBaseBufferCtrl(){}
CCSingleBufferCtrl::CCSingleBufferCtrl(int nLChannel,BOOL bTURN):CCBaseBufferCtrl(bTURN)
{
	m_bJVP2P = FALSE;
	
	m_lTLENGTH = JVN_BUF_MINS;
	m_lSLENGTH = m_lTLENGTH/2;
	
	m_nHEADSIZE = sizeof(STBNODE);
	m_nFrameTime = 40;
	m_nFrames = 50;

	m_nSpeedup = 20;//5;

	m_dStart=0;//视频数据计时
	m_dEnd=0;
	m_dTimeUsed=0;

	m_bFirstWrite = TRUE;
	m_bFirstRead = TRUE;

	m_pBuffer[0] = NULL;
	m_pBuffer[1] = NULL;
	
	m_stTMP[0].lReadPos = 0;
	m_stTMP[1].lReadPos = 0;
	m_stTMP[0].lWritePos = 0;
	m_stTMP[1].lWritePos = 0;
	m_stTMP[0].unIID = 0;
	m_stTMP[1].unIID = 0;
	m_stTMP[0].nTotalFrames = 0;
	m_stTMP[1].nTotalFrames = 0;
	m_stTMP[0].nTotalI = 0;
	m_stTMP[1].nTotalI = 0;

	m_nWait = 1;
	m_nSend = 0;
	m_nWrite = 0;

	if(m_lTLENGTH > 0)
	{
		//分配内存
		(m_pBuffer[0]) = new BYTE[m_lSLENGTH];
		(m_pBuffer[1]) = new BYTE[m_lSLENGTH];	
	}

	m_nLocalChannel = nLChannel;
}
CCSingleBufferCtrl::~CCSingleBufferCtrl()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	if(m_pBuffer[0] != NULL)
	{
		delete[] m_pBuffer[0];
		m_pBuffer[0] = NULL;
	}
	if(m_pBuffer[1] != NULL)
	{
		delete[] m_pBuffer[1];
		m_pBuffer[1] = NULL;
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
}

//读取一帧数据 按照新的缓冲规则
BOOL CCSingleBufferCtrl::ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	if(m_stTMP[m_nSend].lReadPos >= m_stTMP[m_nSend].lWritePos)
	{
		if(m_stTMP[m_nWait].lWritePos > 0)
		{//若有数据，发送缓冲与等待缓冲互换,开始发送最新数据  
			int nTemp = m_nSend;
			m_nSend = m_nWait; 
			m_nWait = nTemp;
			
			m_stTMP[m_nWait].lReadPos = 0;
			m_stTMP[m_nWait].lWritePos = 0;
			m_stTMP[m_nWait].nTotalI = 0;

			//更新读位置 
			m_stTMP[m_nSend].lReadPos = 0;
		}

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//无数据
	}

	long lrpos = m_stTMP[m_nSend].lReadPos;
	if(lrpos >= m_lSLENGTH)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//读指针无效
	}
/*
	if(m_bFirstRead)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//缓存帧数不足
	}
*/

	memcpy(&m_stNode, &m_pBuffer[m_nSend][lrpos], m_nHEADSIZE);

	BYTE *pData = (BYTE *)&(m_pBuffer[m_nSend][lrpos + m_nHEADSIZE]);
	
	if( pData == NULL 
		|| (m_stNode.uchType != JVN_DATA_I && m_stNode.uchType != JVN_DATA_P && m_stNode.uchType != JVN_DATA_B && m_stNode.uchType != JVN_DATA_SKIP && m_stNode.uchType != JVN_DATA_A))
	{
		m_stTMP[m_nSend].lReadPos = m_lSLENGTH;

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//数据无效
	}

	uchType = m_stNode.uchType;
	nSize = m_stNode.nLen;
	if(nSize >= m_lSLENGTH || nSize <= 0)
	{
		m_stTMP[m_nSend].lReadPos = m_lSLENGTH;

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//数据无效
	}
	memcpy(pBuffer, pData, nSize);


	////////////////////////////////////////计时
#ifndef WIN32
	struct timeval start;
	gettimeofday(&start,NULL);
	
	m_dEnd = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
#else
	m_dEnd = GetTickCount();
#endif
//DWORD dwNeed = 0;
//DWORD dwReal = 0;
//DWORD dwLFT = m_dwLastFTime;

	if(!m_bFirstRead)
	{
		DWORD dwNeed = m_stNode.unFTime - m_dwLastFTime;
		DWORD dwReal = m_dEnd - m_dwLastPlayTime + m_nSpeedup;
		if(m_stNode.unFTime < m_dwLastFTime || m_dEnd < m_dwLastPlayTime || dwNeed > 10000)
		{//无效的时间，重新计时
			m_bFirstRead = FALSE;
			m_dwLastFTime = m_stNode.unFTime;
			m_dwLastPlayTime = m_dEnd;
		}
		else
		{
			if(dwReal < dwNeed && dwNeed < 80)
			{//时间不够，继续等待
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			else
			{//本地已经等待了足够时间
/*				if(dwReal > dwNeed + m_nSpeedup + 5)
				{//如果实际读取时间总是比理论时间长，可能是应用层浪费了时间，需要提速补充一下
					m_nSpeedup ++;
				}
				else if(dwReal + 2 < dwNeed + m_nSpeedup)
				{//如果实际读取时间总是比理论时间短，提速可适当恢复
					m_nSpeedup --;
				}
*/				m_dwLastFTime = m_stNode.unFTime;
				m_dwLastPlayTime = m_dEnd;
			}
		}
	}
	else
	{
		m_bFirstRead = FALSE;
		m_dwLastFTime = m_stNode.unFTime;
		m_dwLastPlayTime = m_dEnd;
	}

//if(m_stNode.uchType == JVN_DATA_I)
//{
//char ch[500]={0};
//sprintf(ch,"%d.%d.%d[%d][%d][%d]............read..............\n" ,m_stTMP[m_nSend].unIID,m_stNode.nFID, m_stNode.unFTime,dwNeed,dwReal, dwLFT);
//OutputDebugString(ch);
//}
	m_stTMP[m_nSend].lReadPos += m_stNode.nLen + m_nHEADSIZE;//直接删除当前帧，读位置下移
	
	if(m_stTMP[m_nSend].lReadPos >= m_stTMP[m_nSend].lWritePos)
	{//若当前缓冲区内写入的帧已发送完，判断等待缓冲区里是否有数据
		if(m_stTMP[m_nWait].lWritePos > 0)
		{//若有数据，发送缓冲与等待缓冲互换,开始发送最新数据  
			int nTemp = m_nSend;
			m_nSend = m_nWait; 
			m_nWait = nTemp;
			
			m_stTMP[m_nWait].lReadPos = 0;
			m_stTMP[m_nWait].lWritePos = 0;

			m_stTMP[m_nWait].nTotalI = 0;
			//更新读位置 
			m_stTMP[m_nSend].lReadPos = 0;
		}
	}
	
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

//写入新帧
BOOL CCSingleBufferCtrl::WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize,unsigned int unIID, int nFID, unsigned int unFTime)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	//判断写入类型，I B P
	if(uchType == JVN_DATA_I)
	{//若是I帧
		long lwpos = m_stTMP[m_nWrite].lWritePos;
		//if((lwpos*2 < m_lSLENGTH) && m_stTMP[m_nWrite].nTotalI < 3)
		if((lwpos*3 < m_lSLENGTH*2) && m_stTMP[m_nWrite].nTotalI < 3 && !m_bFirstWrite && m_nSend != m_nWrite)
		{//正在发送的缓冲区使用不到一半，此时继续往发送缓冲里写数据
			if((lwpos*2 < m_lSLENGTH) && m_stTMP[m_nWrite].nTotalI < 3)
			{//缓存超过了一半，应该加快播放了
				m_nSpeedup += 10;
			}

			/*判断写缓冲剩余长度是否小于nSize,小于的话不做任何操作*/
			if(m_lSLENGTH - lwpos > nSize + m_nHEADSIZE)
			{//将新帧写入m_Write缓冲区的m_lWritePos位置，		
				BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][lwpos + m_nHEADSIZE]);//..
				if(pData == NULL)
				{
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return FALSE;
				}
				m_stNode.uchType = uchType;
				m_stNode.lStartPos = lwpos;
				m_stNode.nLen = nSize;
				m_stNode.unFTime = unFTime;
				m_stNode.nFID = nFID;
				m_stNode.unIID = unIID;
				
				m_stTMP[m_nWrite].unIID = unIID;
				
				//memcpy(&(m_pBuffer[m_nWrite][m_lWritePos]), &(Node.uchType), 1);
				//memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 1]), &(Node.lStartPos), 4);
				//memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 5]), &(Node.nLen), 4);
				//memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 9]), &(Node.nNeedDelay), 4);
				memcpy(&m_pBuffer[m_nWrite][lwpos], &m_stNode, m_nHEADSIZE);
				memcpy(pData, pBuffer, nSize);
				
				//更新下次写入位置
				m_stTMP[m_nWrite].lWritePos += nSize + m_nHEADSIZE;
				m_stTMP[m_nWrite].nTotalI++;
				m_stTMP[m_nWrite].nTotalFrames = 0;

				m_bFirstWrite = FALSE;

			#ifndef WIN32
				struct timeval start;
				gettimeofday(&start,NULL);
				
				DWORD dw = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
			#else
				DWORD dw = GetTickCount();
			#endif
				DWORD dwu = dw-m_dwLastWTime;
				m_dwLastWTime = dw;
				DWORD dwfu = unFTime-m_dwLastWFTime;
				m_dwLastWFTime = unFTime;
				if(dwu > dwfu + 20)
				{//数据过来的慢了 应该缓放
					m_nSpeedup = max(m_nSpeedup-1, 10);//5);
				}
				
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return TRUE;
			}
			else
			{//剩余缓冲不足 溢出
				m_bFirstWrite = TRUE;
			}
		}
		else
		{
			if(m_stTMP[m_nWait].lWritePos > 0)// && m_stTMP[m_nWait].lReadPos == 0)
			{//读的一直没读完，写的已经写满，正准备覆盖重写
				m_nSpeedup += 20;
				
				if((m_stTMP[m_nSend].lWritePos > 0) && (m_stTMP[m_nSend].lReadPos > 0)
					&& (m_stTMP[m_nSend].lReadPos < m_stTMP[m_nSend].lWritePos))
				{//若有数据，发送缓冲与等待缓冲互换,开始发送最新数据
					int nTemp = m_nSend;
					m_nSend = m_nWait; 
					m_nWait = nTemp;
					
					m_stTMP[m_nWait].lReadPos = 0;
					m_stTMP[m_nWait].lWritePos = 0;
					m_stTMP[m_nWait].nTotalI = 0;
					m_stTMP[m_nWait].nTotalFrames = 0;
					
					//更新读位置 
					m_stTMP[m_nSend].lReadPos = 0;
				}
			}
			
			/*新数据往等待缓冲区里写入*/
			m_nWrite=m_nWait;//要写入的缓冲区更新为等待缓冲区 
			m_stTMP[m_nWrite].lReadPos = 0;
			m_stTMP[m_nWrite].lWritePos = 0;
			m_stTMP[m_nWrite].nTotalI = 1;
			m_stTMP[m_nWrite].nTotalFrames = 1;
			//判断写缓冲剩余长度是否小于nSize,小于的话返回失败
			if(m_lSLENGTH < nSize + m_nHEADSIZE || nSize <= 0)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			
			//不小于的话将新帧写入m_nWrite缓冲区的m_lWritePos位置
			long lwpos = m_stTMP[m_nWrite].lWritePos;
			BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][lwpos + m_nHEADSIZE]);//..
			if( pData == NULL)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			
			m_stNode.uchType = uchType;
			m_stNode.lStartPos = lwpos;
			m_stNode.nLen = nSize;
			m_stNode.unFTime = unFTime;
			m_stNode.nFID = nFID;
			m_stNode.unIID = unIID;
			
			m_stTMP[m_nWrite].unIID = unIID;
			//memcpy(&(m_pBuffer[m_nWrite][lwpos]), &(m_stNode.uchType), 1);
			//memcpy(&(m_pBuffer[m_nWrite][lwpos + 1]), &(m_stNode.lStartPos), 4);
			//memcpy(&(m_pBuffer[m_nWrite][lwpos + 5]), &(m_stNode.nLen), 4);
			//memcpy(&(m_pBuffer[m_nWrite][lwpos + 9]), &(m_stNode.nNeedDelay), 4);
			memcpy(&m_pBuffer[m_nWrite][lwpos], &m_stNode, m_nHEADSIZE);
			memcpy(pData, pBuffer, nSize);
			
			//更新下次写入位置
			m_stTMP[m_nWrite].lWritePos += nSize + m_nHEADSIZE;
			
			m_bFirstWrite = FALSE;

		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);
			
			DWORD dw = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			DWORD dw = GetTickCount();
		#endif
			DWORD dwu = dw-m_dwLastWTime;
			m_dwLastWTime = dw;
			DWORD dwfu = unFTime-m_dwLastWFTime;
			m_dwLastWFTime = unFTime;
			if(dwu > dwfu + 20)
			{//数据过来的慢了 应该缓放
				m_nSpeedup = max(m_nSpeedup-1, 10);//5);
			}
			
//char ch[500]={0};
//sprintf(ch,"%d.%d.%d[%d][%d]......................Write....................\n" ,m_stTMP[m_nWrite].unIID,m_stNode.nFID, m_stNode.unFTime,dwu,dwfu);
//OutputDebugString(ch);
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
	}
	else if(uchType == JVN_DATA_P || uchType == JVN_DATA_B || uchType == JVN_DATA_SKIP || uchType == JVN_DATA_A)
	{//若是B或P或S帧,要写入的缓冲m_nWrite和写入位置m_lWritePos[m_nWrite]不变
		if(m_bFirstWrite || unIID != m_stTMP[m_nWrite].unIID)
		{//首次运行，非I帧丢弃/上一帧序列发生过溢出，需等下一个I帧，防止出现马赛克
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
		
		long lwpos = m_stTMP[m_nWrite].lWritePos;
		//判断写缓冲剩余长度是否小于nSize,小于的话不做任何操作；
		if(m_lSLENGTH - lwpos > nSize + m_nHEADSIZE && lwpos != 0)
		{
			//不小于的话将新帧写入m_Write缓冲区的m_lWritePos位置，	
			BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][lwpos + m_nHEADSIZE]);//..
			if(pData == NULL)
			{
				m_bFirstWrite = TRUE;
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			m_stNode.uchType = uchType;
			m_stNode.lStartPos = lwpos;
			m_stNode.nLen = nSize;
			m_stNode.unFTime = unFTime;
			m_stNode.nFID = nFID;
			m_stNode.unIID = unIID;

			memcpy(&m_pBuffer[m_nWrite][lwpos], &m_stNode, m_nHEADSIZE);
			memcpy(pData, pBuffer, nSize);

			//更新下次写入位置
			m_stTMP[m_nWrite].lWritePos += nSize + m_nHEADSIZE;

			m_stTMP[m_nWrite].nTotalFrames++;
//char ch[500]={0};
//sprintf(ch,"%d.%d.%d.........O......\n" ,m_stTMP[m_nWrite].unIID,m_stNode.nFID, m_stNode.unFTime);
//OutputDebugString(ch);
			
		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);
			
			DWORD dw = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			DWORD dw = GetTickCount();
		#endif
			DWORD dwu = dw-m_dwLastWTime;
			m_dwLastWTime = dw;
			DWORD dwfu = unFTime-m_dwLastWFTime;
			m_dwLastWFTime = unFTime;
			if(dwu > dwfu + 20)
			{//数据过来的慢了 应该缓放
				m_nSpeedup = max(m_nSpeedup-1, 10);//5);
			}
			
//char ch[500]={0};
//DWORD dw = GetTickCount();
//DWORD dwu = dw-m_dwLastWTime;
//m_dwLastWTime = dw;
//DWORD dwfu = unFTime-m_dwLastWFTime;
//m_dwLastWFTime = unFTime;
//sprintf(ch,"%d.%d.%d[%d][%d]......................Write....................\n" ,m_stTMP[m_nWrite].unIID,m_stNode.nFID, m_stNode.unFTime,dwu,dwfu);
//OutputDebugString(ch);			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
		else
		{
			m_bFirstWrite = TRUE;
		}

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return TRUE;
	}
	else
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return TRUE;
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

void CCSingleBufferCtrl::ClearBuffer()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	m_dStart=0;//视频数据计时
	m_dEnd=0;
	m_dTimeUsed=0;
	
	m_bFirstWrite = TRUE;
	m_bFirstRead = TRUE;
	
	m_nWait = 1;
	m_nSend = 0;
	m_nWrite = 0;
	
	m_stTMP[0].lReadPos = 0;
	m_stTMP[1].lReadPos = 0;
	m_stTMP[0].lWritePos = 0;
	m_stTMP[1].lWritePos = 0;
	m_stTMP[0].unIID = 0;
	m_stTMP[1].unIID = 0;
	
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCOldBufferCtrl::CCOldBufferCtrl():CCBaseBufferCtrl(){}
CCOldBufferCtrl::CCOldBufferCtrl(int nLChannel,BOOL bTURN):CCBaseBufferCtrl(bTURN)
{
	m_bJVP2P = FALSE;
	
	m_lTLENGTH = JVN_BUF_MINS;

	m_bOver = FALSE;
	m_nFrameTime = 40;
	m_nFrames = 50;

	m_dStart=0;//视频数据计时
	m_dEnd=0;
	m_dTimeUsed=0;

	m_bFirstWrite = TRUE;
	m_bFirstRead = TRUE;

	m_lLength = m_lTLENGTH/2;
	m_pBuffer[0] = NULL;
	m_pBuffer[1] = NULL;
	
	m_nWriteTotal[0] = 0;
	m_nWriteTotal[1] = 0;

	m_nSendTotalL[0] = 0;
	m_nSendTotalL[1] = 0;
	
	m_nWait = 1;
	m_nSend = 0;
	m_nWrite = 0;
	
	m_lWritePos = 0;
	m_lReadPos = 0;

	m_nWaitFrame = 1;

	if(m_lLength > 0)
	{
		//分配内存
		(m_pBuffer[0]) = new BYTE[m_lLength];
		(m_pBuffer[1]) = new BYTE[m_lLength];	
	}

	m_nLocalChannel = nLChannel;
}
CCOldBufferCtrl::~CCOldBufferCtrl()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	if(m_pBuffer[0] != NULL)
	{
		delete[] m_pBuffer[0];
		m_pBuffer[0] = NULL;
	}
	if(m_pBuffer[1] != NULL)
	{
		delete[] m_pBuffer[1];
		m_pBuffer[1] = NULL;
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
}

//读取一帧数据
BOOL CCOldBufferCtrl::ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	if(m_nSendTotalL[m_nSend] >= m_nWriteTotal[m_nSend])
	{
		if(m_nWriteTotal[m_nWait] > 0)
		{//若有数据，发送缓冲与等待缓冲互换,开始发送最新数据  
			int nTemp = m_nSend;
			m_nSend = m_nWait; 
			m_nWait = nTemp;
			
			m_nSendTotalL[m_nWait] = 0;//
			m_nWriteTotal[m_nWait] = 0;//
			//更新读位置 
			m_lReadPos=0;
		}

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//无数据
	}

	if(m_lReadPos >= m_lLength)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//读指针无效
	}

	if(m_bFirstRead && m_nWriteTotal[m_nSend]-m_nSendTotalL[m_nSend] + m_nWriteTotal[m_nWait]-m_nSendTotalL[m_nWait] <= m_nWaitFrame)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//缓存帧数不足
	}

	memcpy(&(Node.uchType),&(m_pBuffer[m_nSend][m_lReadPos]),1);
	memcpy(&(Node.lStartPos),&(m_pBuffer[m_nSend][m_lReadPos + 1]),4);
	memcpy(&(Node.nLen),&(m_pBuffer[m_nSend][m_lReadPos + 5]),4);
	memcpy(&(Node.nNeedDelay),&(m_pBuffer[m_nSend][m_lReadPos + 9]),4);

	BYTE *pData = (BYTE *)&(m_pBuffer[m_nSend][m_lReadPos + BUFHEADSIZE]);
	
	if( pData == NULL 
		|| (Node.uchType != JVN_DATA_I && Node.uchType != JVN_DATA_P && Node.uchType != JVN_DATA_B && Node.uchType != JVN_DATA_SKIP))
	{
		m_lReadPos = m_lLength;

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//数据无效
	}

	uchType = Node.uchType;
	nSize = Node.nLen-1;
	if(nSize >= m_lLength || nSize <= 0)
	{
		m_lReadPos = m_lLength;

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;//数据无效
	}
	memcpy(pBuffer, &pData[1], nSize);


	////////////////////////////////////////计时
#ifndef WIN32
	struct timeval start;
	gettimeofday(&start,NULL);
	
	m_dEnd = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
#else
	m_dEnd = GetTickCount();
#endif
	
	if(!m_bFirstRead)
	{
        m_dTimeUsed = m_dEnd-m_dStart;
	}
	else
	{//首次运行 开始计时
		m_dTimeUsed = 0;//m_nFrameTime;
		m_bFirstRead = FALSE;
		
		m_dStart = m_dEnd;
	}

	int nTemp = (m_nWriteTotal[m_nSend]-m_nSendTotalL[m_nSend] + m_nWriteTotal[m_nWait]-m_nSendTotalL[m_nWait])*100/(m_nWriteTotal[m_nSend] + m_nWriteTotal[m_nWait]);

	if(Node.nNeedDelay < m_nFrameTime/3)
	{//帧间隔过小，一般是由於阻塞引起，需要延后播放，使图像流畅
		if(nTemp <= 15)
		{//普通延时播放
			if(m_dTimeUsed < m_nFrameTime)//m_nFrameTime/2+6)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
		else if(nTemp <= 30)
		{//缓存了较多帧，防止图像延后过多，加快播放
			if(m_dTimeUsed < m_nFrameTime/2+6)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
		else
		{//缓存了太多帧，防止图像延后过多，加快播放
			if(m_dTimeUsed < m_nFrameTime/3)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
	}
	else
	{//正常帧间隔
		if(nTemp <= 15)
		{
			if(m_dTimeUsed < jvs_min(Node.nNeedDelay, m_nFrameTime))
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
		else if(nTemp <= 25)
		{//缓存了太多帧，防止图像延后过多，加快播放
			if(m_dTimeUsed < jvs_min(Node.nNeedDelay, m_nFrameTime/2))
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
		else
		{//缓存了太多帧，防止图像延后过多，加快播放
			if(m_dTimeUsed < jvs_min(Node.nNeedDelay, m_nFrameTime/3))
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
		}
	}

	m_dStart = m_dEnd;
	
	m_lReadPos += Node.nLen + BUFHEADSIZE;//直接删除当前帧，读位置下移
	m_nSendTotalL[m_nSend] ++;//更新逻辑发送总帧数
	
	if(m_nSendTotalL[m_nSend] >= m_nWriteTotal[m_nSend])
	{
		//若当前缓冲区内写入的帧已发送完，判断等待缓冲区里是否有数据
		if(m_nWriteTotal[m_nWait] > 0)
		{
			//若有数据，发送缓冲与等待缓冲互换,开始发送最新数据  
			int nTemp = m_nSend;
			m_nSend = m_nWait; 
			m_nWait = nTemp;
			
			m_nSendTotalL[m_nWait] = 0;//
			m_nWriteTotal[m_nWait] = 0;//
			//更新读位置 
			m_lReadPos=0;
		}
	}
	
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

//写入新帧
BOOL CCOldBufferCtrl::WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, int nNeedDelay)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	//判断写入类型，I B P
	if(uchType == JVN_DATA_I)
	{//若是I帧
		if(m_lWritePos*2 < m_lLength)
		{//正在发送的缓冲区使用不到一半，此时继续往发送缓冲里写数据
			/*判断写缓冲剩余长度是否小于nSize,小于的话不做任何操作*/
			if(m_lLength - m_lWritePos > nSize + sizeof(STBUFNODE))// && m_lWritePos != 0)
			{//将新帧写入m_Write缓冲区的m_lWritePos位置，		
				BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][m_lWritePos + BUFHEADSIZE]);//..
				if(pData == NULL)
				{
					m_bOver = TRUE;
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return FALSE;
				}
				Node.uchType = uchType;
				Node.lStartPos = m_lWritePos;
				Node.nLen = nSize;
				Node.nNeedDelay = nNeedDelay;
				
				memcpy(&(m_pBuffer[m_nWrite][m_lWritePos]), &(Node.uchType), 1);
				memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 1]), &(Node.lStartPos), 4);
				memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 5]), &(Node.nLen), 4);
				memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 9]), &(Node.nNeedDelay), 4);
				memcpy(pData, pBuffer, nSize);
				
				//更新下次写入位置
				m_lWritePos += nSize + BUFHEADSIZE;
				
				//更新当前缓冲的总帧数
				m_nWriteTotal[m_nWrite] ++;


				if(m_bFirstWrite)
				{
					m_bFirstWrite = FALSE;//收到I帧，去除首次运行标志，允许接收非I帧数据
				}
			
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return TRUE;
			}
			else
			{//剩余缓冲不足 溢出
				m_bOver = TRUE;
			}
		}
		else
		{
			/*新数据往等待缓冲区里写入*/
			m_lWritePos=0;//更新下次写入位置
			m_nWrite=m_nWait;//要写入的缓冲区更新为等待缓冲区 
			//判断写缓冲剩余长度是否小于nSize,小于的话返回失败
			if(m_lLength < nSize + sizeof(STBUFNODE) || nSize <= 0)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			
			//不小于的话将新帧写入m_nWrite缓冲区的m_lWritePos位置	
			BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][m_lWritePos + BUFHEADSIZE]);//..
			if( pData == NULL)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			
			Node.uchType = uchType;
			Node.lStartPos = m_lWritePos;
			Node.nLen = nSize;
			Node.nNeedDelay = nNeedDelay;
			
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos]), &(Node.uchType), 1);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 1]), &(Node.lStartPos), 4);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 5]), &(Node.nLen), 4);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 9]), &(Node.nNeedDelay), 4);
			memcpy(pData, pBuffer, nSize);
			
			//更新下次写入位置
			m_lWritePos += nSize + BUFHEADSIZE;
			//写入缓冲区的总帧数归0 
			m_nSendTotalL[m_nWrite] = 0;
			m_nWriteTotal[m_nWrite] = 0;
			
			//更新写入缓冲的总帧数
			m_nWriteTotal[m_nWrite] ++;
			
			m_bOver = FALSE;
					
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
	}
	else if(uchType == JVN_DATA_P || uchType == JVN_DATA_B || uchType == JVN_DATA_SKIP)
	{//若是B或P或S帧,要写入的缓冲m_nWrite和写入位置m_lWritePos[m_nWrite]不变
		if(m_bFirstWrite || m_bOver)
		{//首次运行，非I帧丢弃/上一帧序列发生过溢出，需等下一个I帧，防止出现马赛克
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
		
		//判断写缓冲剩余长度是否小于nSize,小于的话不做任何操作；
		if(m_lLength - m_lWritePos > nSize + sizeof(STBUFNODE) && m_lWritePos != 0)
		{
			//不小于的话将新帧写入m_Write缓冲区的m_lWritePos位置，	
			BYTE *pData = (BYTE *)&(m_pBuffer[m_nWrite][m_lWritePos + BUFHEADSIZE]);//..
			if(pData == NULL)
			{
				m_bOver = TRUE;
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return FALSE;
			}
			Node.uchType = uchType;
			Node.lStartPos = m_lWritePos;
			Node.nLen = nSize;
			Node.nNeedDelay = nNeedDelay;

			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos]), &(Node.uchType), 1);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 1]), &(Node.lStartPos), 4);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 5]), &(Node.nLen), 4);
			memcpy(&(m_pBuffer[m_nWrite][m_lWritePos + 9]), &(Node.nNeedDelay), 4);
			memcpy(pData, pBuffer, nSize);

			//更新下次写入位置
			m_lWritePos += nSize + BUFHEADSIZE;

			//更新当前缓冲的总帧数
			m_nWriteTotal[m_nWrite] ++;
	
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
		else
		{
			m_bOver = TRUE;
		}

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return TRUE;
	}
	else
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return TRUE;
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

void CCOldBufferCtrl::ClearBuffer()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	m_bOver = FALSE;
	
	m_dStart=0;//视频数据计时
	m_dEnd=0;
	m_dTimeUsed=0;
	
	m_bFirstWrite = TRUE;
	m_bFirstRead = TRUE;
		
	m_nWriteTotal[0] = 0;
	m_nWriteTotal[1] = 0;
	
	m_nSendTotalL[0] = 0;
	m_nSendTotalL[1] = 0;
	
	m_nWait = 1;
	m_nSend = 0;
	m_nWrite = 0;
	
	m_lWritePos = 0;
	m_lReadPos = 0;
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}









