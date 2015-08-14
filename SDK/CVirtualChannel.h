// CVirtualChannel.h: interface for the CCVirtualChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_)
#define AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CChannel.h"

class CCHelpCtrl;
class CCVirtualChannel  
{
public:
	CCVirtualChannel();
	CCVirtualChannel(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker,BOOL bMobile = TRUE);
	virtual ~CCVirtualChannel();
	void ReConnect(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker);

	DWORD m_dwSenTime,m_dwRecvTime;
	BOOL SendKeep();//发送心跳成功
	BOOL RecvKeep();//
	BOOL DisConnect();
	BOOL ConnectStatus(SOCKET s,SOCKADDR_IN* addr,int nTimeOut,BOOL bFinish,UDTSOCKET uSocket);

	BOOL DealConnectIP(STCONNPROCP *pstConn);

public:
	BOOL m_bMobile;//手机连接
	int m_nLocalStartPort;
	UDTSOCKET m_ServerSocket;//对应服务的socket

	DWORD m_dwConnectTime;//记录本次发起连接的时间，用于小助手控制连接频率


	SOCKET m_SocketSTmp;//临时套接字，TCP连接及转发服务器交互使用
	SOCKADDR_IN m_addrAN;//主控外网地址
	SOCKADDR_IN m_addrAL;//主控内网地址
	SOCKADDR_IN m_addrTS;//转发服务器地址
	int m_nStatus;//主连接当前状态
	DWORD m_dwStartTime;//主连接关键计时
	SOCKADDR_IN m_addressA;
	BOOL m_bPassWordErr;//是否检测到身份验证回复

	SOCKADDR_IN m_addrLastLTryAL;//上次尝试过的内网探测地址

	BOOL m_bJVP2P;//是否多播方式(依赖于主控)

	BOOL m_bTURN;//是否基本转发
	BOOL m_bLocal;//是否内网探测

	int m_nFYSTVER;//远端云视通协议版本
	
	STCONNECTINFO m_stConnInfo;
	ServerList m_SList;//P2P连接时使用
	ServerList m_SListTurn;//TURN连接时使用
	ServerList m_SListBak;//备份暂无用
	
	ServerList m_ISList;//索引服务器列表

	BOOL m_bShowInfo;//是否已经对外提示过连接信息
	//实际连接成功后要求按照下面的参数直接连接
	BOOL m_bDirectConnect;
	SOCKET m_sSocket;
	char m_strIP[20];
	int m_nPort;

	int m_nNatTypeA;//主控的网络类型，通过第一个查询服务器得到

	DWORD m_dwStartConnectTime;
    DWORD m_dwAddTime;
private:
	BOOL SendData(BYTE uchType, BYTE *pBuffer, int nSize);

	//
	BOOL SendPWCheck();
	int RecvPWCheck(int &nPWData);
	BOOL SendSP2P(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL SendSP2PTCP(SOCKADDR_IN addrs, int nIndex, char *pchError);
	int RecvS(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL ConnectLocalTry(int nIndex, char *pchError);
	BOOL ConnectNet(int nIndex, char *pchError);
	BOOL ConnectNetTry(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL SendSTURN(SOCKADDR_IN addrs, int nIndex, char *pchError);
	int RecvSTURN(int nIndex, char *pchError);
	BOOL ConnectTURN(int nIndex, char *pchError);

	//打洞函数
	BOOL Punch(int nYSTNO,int sock,SOCKADDR_IN *addrA);
	//预打洞函数
	void PrePunch(int sock,SOCKADDR_IN addrA);

	/*连接处理函数*/
	void DealNewYST(STCONNPROCP *pstConn);

	void DealWaitSerREQ(STCONNPROCP *pstConn);
	void DealWaitSerRSP(STCONNPROCP *pstConn);
	
	void DealOK(STCONNPROCP *pstConn);
	void DealFAILD(STCONNPROCP *pstConn);
	void DealNEWP2P(STCONNPROCP *pstConn);
	void DealWaitS(STCONNPROCP *pstConn);
	void DealNEWP2PL(STCONNPROCP *pstConn);
	void DealWaitLWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitLPWCheck(STCONNPROCP *pstConn);
	void DealNEWP2PN(STCONNPROCP *pstConn);
	void DealWaitNWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitNPWCheck(STCONNPROCP *pstConn);

	void DealNEWTURN(STCONNPROCP *pstConn);
	void DealWaitTURN(STCONNPROCP *pstConn);
	void DealRecvTURN(STCONNPROCP *pstConn);
	void DealWaitTSWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitTSPWCheck(STCONNPROCP *pstConn);

	void DealWaitIndexSerREQ(STCONNPROCP *pstConn);
	void DealWaitIndexSerRSP(STCONNPROCP *pstConn);

	void DealWaitNatREQ(STCONNPROCP *pstConn);//查网络类型询主控
	void DealWaitNatRSP(STCONNPROCP *pstConn);//查网络类型询主控

	void StartConnThread();
	void GetSerAndBegin(STCONNPROCP *pstConn);

	void DealMakeHole(STCONNPROCP *pstConn);
#ifndef WIN32
	static void* ConnProc(void* pParam);//发送线程，用于处理主连接
#else
	static UINT WINAPI ConnProc(LPVOID pParam);//发送线程，用于处理主连接
#endif
	
public:
	void AddRemoteConnect(SOCKET s,char* pIP,int nPort);//实际连接成功后 通知助手去连接
//	static void WaitThreadExit(HANDLE &hThread);//强制退出线程
private:	
	BOOL m_bPass;//身份验证成功
	CCHelpCtrl *m_pHelp;
	CCWorker *m_pWorker;

	BOOL m_bExit;
	
	BOOL m_bDisConnectShow;//是否已经提示过连接断开

	::std::vector<NAT> m_NATListTEMP;//本地的IP 和NAT 列表

    NATList m_NatList;
	
#ifndef WIN32
	pthread_t m_hConnThread;//线程句柄
	BOOL m_bEndC;
	pthread_mutex_t m_ct;
#else
	HANDLE m_hConnThread;//线程句柄
	HANDLE m_hStartEventC;
	HANDLE m_hEndEventC;

	CRITICAL_SECTION m_ct;
#endif
};

#endif // !defined(AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_)
