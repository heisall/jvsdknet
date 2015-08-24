// CPartner.h: interface for the CCPartner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_)
#define AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include <map>

#define JVN_TIMEOUTCOUNT 40//15//连续JVN_TIMEOUTCOUNT个请求超时，该节点有问题，直接关闭

#define JVN_TIME_WAITLRECHECK  10000//等待内网验证消息
#define JVN_TIME_WAITNRECHECK  15000//等待外网验证消息

#define JVN_TIME_WAIT          10000//等待

#define JVN_PTBUFSIZE   102400//伙伴间数据块大小
#define JVN_PACKTIMEOUT 10000

#define PNODE_STATUS_NEW           1//新节点
#define PNODE_STATUS_CONNECTING    2//连接中
#define PNODE_STATUS_CONNECTED     4//连接成功
#define PNODE_STATUS_WAITLCHECK    5//内网探测待验证(需发送验证)
#define PNODE_STATUS_WAITNCHECK    7//外网待验证(需发送验证)
#define PNODE_STATUS_FAILD         8//连接失败
#define PNODE_STATUS_ACCEPT        9//收到连接(需等待验证)

#define PNODE_STATUS_TCCONNECTING  10//提速连接中
#define PNODE_STATUS_TCWAITTS      11//提速连接正在等待服务器地址
#define PNODE_STATUS_TCWAITRECHECK 12//提速连接正等待预验证
#define PNODE_STATUS_TCWAITPWCHECK 13//提速连接正等待身份验证

#define PTYPE_N      0//外网伙伴
#define PTYPE_L      1//内网伙伴
#define PTYPE_A      2//本地被动连接
#define PTYPE_NULL   3//初始状态

typedef struct STPTLI
{
	int nLinkID;
	BOOL bIsSuper;
	BOOL bIsLan2A;
	BOOL bIsLan2B;
	BOOL bIsCache;
	BOOL bIsTC;
	SOCKADDR_IN sAddr;
}STPTLI;

typedef struct STREQ
{
	unsigned int unChunkID;//数据片编号，该编号从0开始
	DWORD dwStartTime;//开始请求时间
	BOOL bAnswer;//是否进行过回复
	BOOL bNEED;//处在缓冲窗口，急需，优先请求
//	int nNeed;//紧迫性 该值越大 说明它后面收到的越多 当前越急需
}STREQ;//本地需要请求哪些数据 对方请求了哪些数据

typedef struct STMAPINFO
{
	unsigned int unBeginChunkID;//数据块起始ID
	int nChunkCount;
	BYTE uchMAP[5000];//缓存映射
	::std::map<unsigned int, DWORD> ChunkBTMap;//某数据块开始请求时间记录
	
	STMAPINFO()
	{
		unBeginChunkID = 0;
		nChunkCount = 0;
		memset(uchMAP, 0, 5000);
		ChunkBTMap.clear();
	}
}STMAPINFO;

typedef ::std::vector<STREQ> STREQS;

class CCPartnerCtrl;
class CCChannel;
class CCWorker;

class CCPartner  
{
public:
	CCPartner();
	CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel);
	CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel,SOCKADDR_IN addr, UDTSOCKET socket, BOOL bTCP);

	virtual ~CCPartner();

	int CheckLink(CCPartnerCtrl *pPCtrl, BOOL bNewConnect, DWORD &dwlasttime);//检查伙伴连接情况 进行伙伴连接

	int PartnerLink(CCPartnerCtrl *pPCtrl);//进行伙伴连接

	BOOL BaseRecv(CCPartnerCtrl *pPCtrl);
	BOOL BaseRecvTCP(CCPartnerCtrl *pPCtrl);

	void DisConnectPartner();

	int Send2Partner(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete=TRUE);//发送普通数据
	int Send2PartnerTCP(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete=TRUE);//发送普通数据
	
	BOOL SendBM(BYTE *puchBuf, int nSize);//发送BM
	BOOL SendBMD(CCPartnerCtrl *pPCtrl);//发送数据片
	
	BOOL SendBMDREQ(unsigned int unChunkID, CCPartnerCtrl *pPCtrl);//发送数据片请求

	BOOL CheckREQ(unsigned int unChunkID, BOOL bTimeOut);//检查是否有该数据片，是否向该伙伴请求过当前数据片，是的话重置该请求，并更新伙伴性能，并返回性能值

	void GetPInfo(char *pMsg, int &nSize, DWORD dwend);//获取伙伴信息
	float GetPower();//获取当前伙伴的性能(下载速度)

private:
	void SetBM(unsigned int unChunkID, int nCount, BYTE *pBuffer, int nLen, CCPartnerCtrl *pPCtrl);//更新当前伙伴BM
	void RefreshPartner(unsigned int unChunkID, int nLen, BOOL bNULL=FALSE);//刷新当前伙伴性能参数

	BOOL ParseMsg(CCPartnerCtrl *pPCtrl);//解析接收到的数据包，主要作用是检查是否接收完毕，以及区分粘包

	void ResetPack(BOOL bAccept=FALSE);//重置数据收发参数

	BOOL SendSTURN(SOCKADDR_IN addrs);
	int RecvSTURN();
	BOOL ConnectTURN();
	BOOL SendReCheck(BOOL bYST);
	BOOL SendPWCheck();
public:
	BOOL m_bTCP;
	int m_ndesLinkID;//在主控端的连接ID，
	UDTSOCKET m_socket;//本地套接字
	SOCKET    m_socketTCP;
	SOCKET    m_socketTCtmp;
	SOCKADDR_IN m_addr;//主控端记录的该节点地址
	int m_nstatus;//节点状态(新节点，请求地址中；请求地址成功；连接成功；内网待验证；内网探测验证失败；外网待验证；连接失败)

	BOOL m_bTryNewLink;//是否是新的尝试节点

	DWORD m_dwstart;//超时计时起始时间

	BOOL m_bTURNC;//是否是提速转发节点
	BOOL m_bCache;//是否是高速缓存节点
	BOOL m_bAccept;//是否是被动连接
	BOOL m_bSuperProxy;//是否是超级节点 超级节点具有优先连接权
	BOOL m_bProxy2;//是否是二级代理节点 本地是高速缓存时这种节点具有优先传输权
	BOOL m_bLan2A;//是否是主控的内网伙伴
	BOOL m_bLan2B;
	unsigned int m_unADDR;
	
	int m_nConnFCount;//连接累计失败次数

	//------伙伴网络性能-----------
	DWORD m_dwRUseTTime;
	int m_nLPSAvage;//本地下行带宽/该伙伴的上行带宽(KB/S)(请求数据量小,下载回复数据量大,所以下载为主)
	int m_nRealLPSAvage;
	int m_nDownLoadTotalB;
	int m_nDownLoadTotalKB;
	int m_nDownLoadTotalMB;
	int m_nUpTotalB;
	int m_nUpTotalKB;
	int m_nUpTotalMB;
	int m_nLastDownLoadTotalB;
	int m_nLastDownLoadTotalKB;
	int m_nLastDownLoadTotalMB;
	DWORD m_dwLastInfoTime;

	DWORD m_dwLastDataTime;//最后一次收到数据的时间

	int m_nTimeOutCount;//连续请求超时不良记录，超过阀值关闭该连接

	int m_nConnTotalSpeed;
	DWORD m_dwLastConnTime;
	float m_fLastConnTotal;
	//------伙伴最新BM-------------
	STMAPINFO m_stMAP;

	STREQS m_PushQueue;
	
private:
	CCChannel *m_pChannel;
	CCWorker *m_pWorker;

	BYTE m_chdata[1000];
	BYTE *m_puchSendBuf;//发送临时缓存，用于控制多次发送
	int m_nSendPos;//临时缓存数据包已发送长度
	int m_nSendPackLen;//临时缓存数据包总长度
	DWORD m_dwSendPackLast;

	BYTE *m_puchRecvBuf;//接收临时缓存，用于控制多次发送
	int m_nRecvPos;//临时缓存数据包已发送长度
	int m_nRecvPackLen;//临时缓存数据包总长度
	DWORD m_dwRecvPackLast;
};

#endif // !defined(AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_)
