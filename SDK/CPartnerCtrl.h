// CPartnerCtrl.h: interface for the CCPartnerCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_)
#define AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CPartner.h"  

#define PT_LCONNECTMAX 5//10//5//本地内网活动连接上限(主动发起连接
#define PT_NCONNECTMAX 5//10//5//本地外网活动连接上限(主动发起连接)
#define PT_LACCEPTMAX  10//10//5//本地接受内网连接数上限
#define PT_NACCEPTMAX  10//10//5//本地接受外网连接数上限

#define PT_TIME_CNNEWPT   5000//尝试连接新的伙伴周期 
#define PT_TIME_DISBADPT  120000//断开最差伙伴连接
#define PT_TIME_REQPTLIST 60000//定期获取最新的伙伴列表
#define PT_TIME_NODATA    20000//60000//超过该时间未收到数据，认为是极差节点

#define JVC_CLIENTS_BM3 3

class CCChannel;
class CCWorker;


typedef ::std::vector<STPTLI> PartnerIDList;//伙伴ID列表
typedef ::std::vector<CCPartner*> PartnerList;//伙伴列表
class CCPartnerCtrl  
{
public:
	CCPartnerCtrl();
	virtual ~CCPartnerCtrl();

	BOOL SetPartnerList(PartnerIDList partneridlist);//收到伙伴列表后更新本地列表

	void GetPartnerInfo(char *pMsg, int &nSize, DWORD dwend);//获取伙伴状态信息
	void ClearPartner();
	void DisConnectPartners();

	BOOL CheckAndSendChunk(unsigned int unChunkID, BOOL bTimeOut);//判断是否有有效的提供者

	void SetReqStartTime(BOOL bA, unsigned int unChunkID, DWORD dwtime);
	//------------------------------------
	void AddTurnCachPartner();//需要提速转发连接 后续判断处理
	void AcceptPartner(UDTSOCKET socket, SOCKADDR_IN clientaddr, int nDesID, BOOL bTCP=FALSE);//接收到一个伙伴连接 后续判断处理
	BOOL CheckPartnerLinks(BOOL bExit);//检查伙伴连接情况 进行伙伴连接

	BOOL PartnerLink(BOOL bExit);//进行伙伴连接

	BOOL RecvFromPartners(BOOL bExit, HANDLE hEnd);
	
	BOOL SendBM2Partners(BYTE *puchBuf, int nSize,BOOL bExit, HANDLE hEnd);//读取BM并向全部或部分伙伴发送BM
	BOOL SendBMDREQ2Partner(STREQS streqs, int ncount, BOOL bExit);

	void CheckIfNeedSetBuf(unsigned int unChunkID, int ncount, DWORD dwCTime[10], BOOL bA=FALSE);

	void ResetProxy2();
	//------------------------------------

	void CheckGarbage();//无效节点清理

	BOOL SendBMD();

	static BOOL CheckInternalIP(const unsigned int ip_addr);
	static int tcpreceive(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec);
	static int tcpsend(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec, int &ntimeout);
	static int connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec);

private:
	DWORD m_dwLastREQListTime;
	DWORD m_dwLastTryNewLTime;
	DWORD m_dwLastTryNewNTime;
	DWORD m_dwLastDisOldLTime;
	DWORD m_dwLastDisOldNTime;

//	::std::map<unsigned int, unsigned int> m_PTaddrMap;
public:
	PartnerList m_PList;//最新所有伙伴列表
	PartnerList m_PListTMP;//所有伙伴列表备份
	PartnerList m_PLINKList;//所有待连伙伴

	CCChannel *m_pChannel;
	CCWorker *m_pWorker;

	BOOL m_bClearing;//加快清理速度

	DWORD m_dwLastPTData;//上次得到伙伴数据的时间，如果长时间得不到伙伴数据，需要考虑从主控索取

#ifndef WIN32
	pthread_mutex_t m_ct;
	pthread_mutex_t m_ctCONN;
	pthread_mutex_t m_ctPTINFO;
#else
	CRITICAL_SECTION m_ct;
	CRITICAL_SECTION m_ctCONN;
	CRITICAL_SECTION m_ctPTINFO;
#endif

	unsigned int m_unChunkIDNew;//最新的数据块ID
	unsigned int m_unChunkIDOld;//最旧的数据块ID

	::std::map<unsigned int, DWORD> m_ChunkBTMap;//某数据块开始请求时间记录
};

#endif // !defined(AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_)
