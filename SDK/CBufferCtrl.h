// CBufferCtrl.h: interface for the CCBufferCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_)
#define AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "RunLog.h"
#include "CPartner.h"
#include <map>
#include <algorithm>

#define JVN_CHUNK_PRIVATE  20//保留的数据块数目，这些块不对外公布，仅用于补救那些太落后的请求
#define JVN_CHUNK_LEN      16384//每个数据块的大小 <<<注：主分控必须统一>>>
//#define JVN_BUF_MINM       10240000//多播方式时 内存方式需要的最小内存大小
#define JVN_BUF_MINS       4096000//800000//819200//普通方式时 需要的最小内存大小
#ifndef WIN32
	#define JVN_BUF_MINM       1024000//10240000//多播方式时 内存方式需要的最小内存大小
#else
	#define JVN_BUF_MINM       4096000//8192000//10240000//多播方式时 内存方式需要的最小内存大小
#endif

#define JVN_REQBEGINMAX  100//默认请求保留阀值，最旧的这些数据块不进行请求，防止数据延期到达造成浪费
#define JVN_PLAYIMD   40//默认危险阀值，进入该区域后有被覆盖造成浪费的危险，需要立即播放
#define JVN_PLAYNOW   20//50//缓冲阀值，缓冲连续多少个数据块开始播放，当播放指针进入危险区后，该值无效

#define JVN_FIRSTREQ  50//第一次请求数据的相对起始点

#define JVN_TIME_FIRSTWAIT 15000//初始缓冲最大时间
#define JVN_TIME_MAXWAIT   60000//运行中单次最大缓冲时间

#define BM_CHUNK_TIMEOUT5   5000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT10  10000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT20  20000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT40  40000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT60  60000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT100 100000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT120 120000//请求数据片超时时间
#define BM_CHUNK_TIMEOUT240 240000//请求数据片超时时间
#define BM_ARRIVETIMEOUT   60000//BM超时时间

//////////////////////////////////////////////////////////////////////////旧版兼容
#define BUFHEADSIZE 13
typedef struct 
{
	BYTE uchType;//int nType;//类型：I P B
	long lStartPos;
	int nLen;
	int nNeedDelay;
}STBUFNODE;
//////////////////////////////////////////////////////////////////////////

typedef struct STFRAMEHEAD
{
	BYTE  uchType;//关键帧，普通视频帧，音频帧
	int   nSize;  //帧净载长度
	unsigned int unFTime;//当前帧与软件运行的相对时间
	int   nFrameID;//视频帧编号，相对于帧序列，I帧编号为0，用于在播放时控制跳跃时间
}STFRAMEHEAD;//帧头

typedef struct STCHUNKHEAD
{
	unsigned int unChunkID;//数据片编号，从1开始 顺序累加
	long lWriteOffset;//目前缓存中实际存入长度,即下次写入位置,相对于lBeginPos.也是当前一些如的数据长度
	BOOL bHaveI;//该数据片中是否有关键帧
	int nIOffset;//如有关键帧时第一个关键帧的位置，相对于lBeginPos
	int nNOffset;//第一个帧(关键帧或其他帧)的位置，相对于lBeginPos
	unsigned int unCTime;//当前数据片与软件运行的相对时间
}STCHUNKHEAD;//数据块头信息

typedef struct STPLAYHEAD
{
	BOOL bPlayed;//当前数据块是否已经播放过
	BOOL bRateOK;//当前数据块是否已经过缓冲
	long lNextOffset;//当前数据块中下次需要播放的起始位置(块中某个帧的起始位置)
}STPLAYHEAD;

typedef struct STCHUNK
{
	STCHUNKHEAD stHead;//数据块头
	STPLAYHEAD stPHead;//播放控制

	long lBeginPos;//每个chunk在总缓存的起始位置,相对于总缓存
	BOOL bHaveData;//该数据块是否完整
	BOOL bNeedPush;//是否需要推送
	STCHUNK()
	{
		stHead.unChunkID = 0;
		stHead.bHaveI = FALSE;
		stHead.nIOffset = -1;
		stHead.nNOffset = -1;
		stHead.lWriteOffset = 0;
		stHead.unCTime = 0;
	
		stPHead.bPlayed = FALSE;
		stPHead.bRateOK = FALSE;
		stPHead.lNextOffset = -1;

		lBeginPos = 0;
		bHaveData = FALSE;
		bNeedPush = FALSE;
	}
	
	void ResetChunk()
	{
		stHead.unChunkID = 0;
		stHead.bHaveI = FALSE;
		stHead.nIOffset = -1;
		stHead.nNOffset = -1;
		stHead.lWriteOffset = 0;
		stHead.unCTime = 0;

		stPHead.bPlayed = FALSE;
		stPHead.bRateOK = FALSE;
		stPHead.lNextOffset = -1;
		bHaveData = FALSE;
		bNeedPush = FALSE;
	}
}STCHUNK;//一个数据片单位(一个帧序列)

typedef struct STBM
{
	BYTE *pBuffer;//数据实际存储缓冲
	::std::map<int, STCHUNK> ChunksMap;//数据块集合
	
	BOOL bBMChanged;//数据片是否有变化
	
	int *pnChunkIndex;//索引，循环覆盖时只切换索引值，维护缓存的顺序号和基址的对应关系
	                  //例如：pnChunkIndex[0] 代表的是永远是最新数据块，但该数据块的基址是循环覆盖的，是动态变化的。
	
	int nOldestChunk;//当前缓存中最早数据块的顺序地址，非索引和ID，理论上恒为m_nChunkNUM-1，这里应考虑初始数据不满的情形

	int nNeedPlayIndex;//下次需要播放的数据块基址位置
	unsigned int unNeedPlayID;//下次需要播放的数据块ID
	BOOL bNeedI;//是否需要寻找下个关键帧
	BOOL bNeedA;//寻找关键帧过程中是否保留音频帧
	unsigned int unPlayedID;//已播过的最新ID

	STBM()
	{
		pBuffer = NULL;
		bBMChanged = FALSE;
		ChunksMap.clear();
		pnChunkIndex = NULL;
		nOldestChunk = -1;
	
		nNeedPlayIndex = -1;
		unNeedPlayID = 0;
		bNeedI = TRUE;
		bNeedA = FALSE;
		unPlayedID = 0;
	}
}STBM;//缓存单位(内存交互的单位)

typedef struct STSTAT
{
	char chBegainTime[MAX_PATH];//起始时间
	char chEndTime[MAX_PATH];//结束时间
	DWORD dwBeginTime;
	DWORD dwEndTime;
	DWORD dwTimeTotal;
	unsigned int unWaitCount;//缓冲总次数
	unsigned int unWaitTimeTotal;//缓冲总时间
	unsigned int unWaitTimeAvg;//缓冲平均时间
	unsigned int unBufErrCount;//解析不出数据的数据块总个数
	unsigned int unBufJumpCount;//因为数据不连续而跳过的数据块总数
	unsigned int unWaitFrequence;//缓冲频率

	unsigned int unDelayCount;

	unsigned int unBeginReqID;
	unsigned int unFirstRcvID;
	unsigned int unBeginPlayID0;
	unsigned int unBeginPlayID1;

	unsigned int unNobufCount;
	unsigned int unRepeatCount;
	unsigned int unNotfindCount;

	unsigned int unNoDataCount;
	unsigned int unNoICount;

	unsigned int unbeginid;
	DWORD dwbegin;
	DWORD dwend;
	STSTAT()
	{
		memset(chBegainTime, 0, MAX_PATH);
		memset(chEndTime, 0, MAX_PATH);
		unWaitCount = 0;
		unWaitTimeTotal = 0;
		unWaitTimeAvg = 0;
		unBufErrCount = 0;
		unBufJumpCount = 0;
		unDelayCount = 0;

		dwBeginTime = 0;
		dwEndTime = 0;
		dwTimeTotal = 0;
		unWaitFrequence = 0;

		unBeginReqID = 0;
		unFirstRcvID = 0;
		unBeginPlayID0 = 0;
		unBeginPlayID1 = 0;

		unNobufCount = 0;
		unRepeatCount = 0;
		unNotfindCount = 0;

		unNoDataCount = 0;
		unNoICount = 0;
	}

	void begin(unsigned int uncurid)
	{
		if(uncurid != unbeginid)
		{//不重复缓冲，起始点不变则说明是同一次缓冲
//			finish();//结束上次缓冲，这里不再执行，因为结束缓冲的过程有自由的流程，这里只负责缓冲，没有走正常结束缓冲流程的不应该计时

			unbeginid = uncurid;
		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);	
			dwbegin = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			dwbegin = GetTickCount();
		#endif
		}
	}
	void finish()
	{
		if(unbeginid > 0 && dwbegin > 0)
		{
			unWaitCount++;
			
		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);	
			dwend = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			dwend = GetTickCount();
		#endif
			if(dwend >= dwbegin)
			{
				unWaitTimeTotal += ((dwend-dwbegin)/1000);
			}
		}

		unbeginid = 0;
		dwbegin = 0;
		dwend = 0;
	}
	void jump(unsigned int unplayedid, unsigned int unneedid)
	{
		if(unneedid > (unplayedid+1))
		{
			int ntmp = unneedid - unplayedid - 1;
			unBufJumpCount += ntmp;
		}
	}

}STSTAT;//整体效果参数


class CCBaseBufferCtrl  
{
public:
	CCBaseBufferCtrl();
	CCBaseBufferCtrl(BOOL bTURN);
	virtual ~CCBaseBufferCtrl();
	
	/*多播*/
	virtual BOOL ReadPlayBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize, int &nBufferRate){return FALSE;};
	virtual BOOL WriteBuffer(STCHUNKHEAD stHead, BYTE *pBuffer){return FALSE;};
	
	virtual BOOL GetBM(BYTE *pBuffer, int &nSize){return FALSE;};//检查是否BM有变化，即是否收到chunk数据，是的话可发送BM
	virtual BOOL ReadChunkLocalNeed(STREQS &stpullreqs, int &ncount){return FALSE;};//逐个检查chunk，未超时的跳过，超时的复位重新请求
	virtual BOOL ReadREQData(unsigned int unChunkID, BYTE *pBuffer, int &nLen){return FALSE;};
	virtual void AddNewBM(unsigned int unChunkID, int ncount, DWORD dwCTime[10], unsigned int &unNewID, unsigned int &unOldID, BOOL bA=FALSE){};//添加新BM
	
	virtual BOOL WaitHighFrequency(){return FALSE;};//是否缓冲过于频繁，这是判断效果好坏的标准

	virtual void ClearBuffer(){};
	DWORD JVGetTime();

	virtual void SetPlayed(int ncurindex){};
	virtual BOOL ReadDataMore(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead){return FALSE;};
	virtual void ReadDataOnce(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead){};
	virtual int GetBufferPercent(){return 0;};
	virtual int NextIndex(int ncurindex){return 0;};


	/*单播*/
	virtual BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, unsigned int unIID, int nFID, unsigned int unFTime){return FALSE;};
	virtual BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, int nNeedDelay){return FALSE;};
	virtual BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize){return FALSE;};
	
public:
	int m_nFrameTime;//帧时间间隔
	int m_nFrames;//关键帧周期
	
	unsigned int m_unMaxWaitTime;//允许的最大缓冲时间

	BOOL m_bFirstWait;//是否还从未播放过
	DWORD m_dwBeginBFTime;//连接建立时间

	CRunLog *m_pLog;
	int m_nChannel;
 
	BOOL m_bTURN;
	BOOL m_bJVP2P;
	BOOL m_bLan2A;
	int m_nClientCount;

	BOOL m_bFirstBMDREQ;//是否是第一次请求数据，用于确定请求起始点
	BOOL m_bNoData;//当前缓存是否收到数据，用于确定播放起始点

	long m_lTLENGTH;//缓冲区总长度
	int m_nChunkNUM;//可以缓存的数据块总数(根据缓冲总长度计算获得)
	
#ifndef WIN32
	pthread_mutex_t m_ct;
#else
	CRITICAL_SECTION m_ct;
#endif

	DWORD m_dwLastPlayTime;//上次播放绝对时间
	DWORD m_dwLastFTime;//上一帧相对时间
	DWORD m_dwLastCTime;//上一数据片相对时间
	DWORD m_dwSysNow;
	unsigned int m_unChunkTimeSpace;//最新的相邻数据片时间差，用于动态估算时间
	int m_nLastWaitCC;//上次计算出的缓冲数据片个数,用于在缓冲时确定需要缓冲多少数据片
	DWORD m_dwLastJump;//上一次强制跳帧的时间

	STSTAT m_stSTAT;//效果统计

	int m_nRate;//最新缓冲进度
	int m_nReqBegin;//保留数据块数目不进行请求
	int m_nPLAYIMD;//立即播放阀值
};

class CCMultiBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCMultiBufferCtrl();
	CCMultiBufferCtrl(BOOL bTURN, BOOL bJVP2P=TRUE);
	virtual ~CCMultiBufferCtrl();
	
	BOOL ReadPlayBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize, int &nBufferRate);
	BOOL WriteBuffer(STCHUNKHEAD stHead, BYTE *pBuffer);
	
	BOOL GetBM(BYTE *pBuffer, int &nSize);//检查是否BM有变化，即是否收到chunk数据，是的话可发送BM
	BOOL ReadChunkLocalNeed(STREQS &stpullreqs, int &ncount);//逐个检查chunk，未超时的跳过，超时的复位重新请求
	BOOL ReadREQData(unsigned int unChunkID, BYTE *pBuffer, int &nLen);
	void AddNewBM(unsigned int unChunkID, int ncount, DWORD dwCTime[10], unsigned int &unNewID, unsigned int &unOldID, BOOL bA=FALSE);//添加新BM
	
	BOOL WaitHighFrequency();//是否缓冲过于频繁，这是判断效果好坏的标准

	void ClearBuffer();

	void SetPlayed(int ncurindex);
	BOOL ReadDataMore(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead);
	void ReadDataOnce(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead);
	int GetBufferPercent();
	int NextIndex(int ncurindex);

private: 
	STBM m_stBM;//本地缓存
};

typedef struct STBNODE
{
	BYTE uchType;//int nType;//类型：I P B
	long lStartPos;
	int nLen;
	unsigned int unIID;
	int nFID;
	unsigned int unFTime;//当前帧与软件运行的相对时间
	STBNODE()
	{
		unIID = 0;
		uchType = 0;
		lStartPos = 0;
		nLen = 0;
		nFID = 0;
		unFTime = 0;
	}
}STBNODE;//TCP TURN数据节点 共享数据
typedef struct STBTMP
{
	unsigned int unIID;//关键帧编号
	long lWritePos;//缓存对应的写指针
	long lReadPos;//两组缓存分别对应的读指针
	int nTotalFrames;
	int nTotalI;//帧序列个数
}STBTMP;//single 帧序列其他参数

class CCSingleBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCSingleBufferCtrl();
	CCSingleBufferCtrl(int nLChannel,BOOL bTURN);
	virtual ~CCSingleBufferCtrl();
	
	BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize);//读取一帧数据
	BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, unsigned int unIID, int nFID, unsigned int unFTime);
	
	void ClearBuffer();
public:
	CRunLog *m_pLog;
private: 
	long m_lSLENGTH;//单个缓冲区长度
	
	BYTE *m_pBuffer[2];//两组缓存
	STBTMP m_stTMP[2];
	
	bool m_bFirstWrite;//首次写数据
	bool m_bFirstRead;//首次读数据
	
	int m_nWait;//空闲缓冲区编号
	int m_nSend;//发送缓冲区编号
	int m_nWrite;//新数据需写入的缓冲区编号
	
	STBNODE m_stNode;

	int m_nSpeedup;//加速播放
	
	//计时相关
	DWORD m_dStart;
	DWORD m_dEnd;
	DWORD m_dTimeUsed;
	
	DWORD m_dwLastFTime;
	DWORD m_dwLastPlayTime;
	int m_nLocalChannel;
	
	DWORD m_dwLastWTime;//实际写操作的时间
	DWORD m_dwLastWFTime;//此帧和上一帧之间的理论时间
	int m_nHEADSIZE;
	
};

//////////////////////////////////////////////////////////////////////////旧版兼容
class CCOldBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCOldBufferCtrl();
	CCOldBufferCtrl(int nLChannel,BOOL bTURN);
	virtual ~CCOldBufferCtrl();
	
	BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize);
	//读取一帧数据
	
	BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, int nNeedDelay);
	
	void ClearBuffer();
public:
	CRunLog *m_pLog;
private: 
	int m_nWaitFrame;//播放延后帧数
	long m_lLength;//单个缓冲区长度
	
	BYTE *m_pBuffer[2];//两组缓存
	
	int m_nWriteTotal[2];//已写入的帧数
	int m_nSendTotalL[2];//逻辑已发送的帧数
	
	bool m_bFirstWrite;//首次写数据
	bool m_bFirstRead;//首次读数据
	
	int m_nWait;//空闲缓冲区编号
	int m_nSend;//发送缓冲区编号
	int m_nWrite;//新数据需写入的缓冲区编号
	
	long m_lWritePos;//缓存对应的写指针
	long m_lReadPos;//两组缓存分别对应的读指针
	
	STBUFNODE Node;
	
	//计时相关
	DWORD m_dStart;
	DWORD m_dEnd;
	DWORD m_dTimeUsed;
	
	BOOL m_bOver;//发生了溢出

	int m_nLocalChannel;
	
};

//////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_)
