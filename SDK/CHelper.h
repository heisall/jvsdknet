// CHelper.h: interface for the CCHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_)
#define AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include "CChannel.h"
#include <map>

//新助手 记录所有搜索到的号码，以及可以连通的信息
typedef struct DATA_LOCAL
{
	char strGroup[4];//编组
	int nYST;//号码
	int nPort;//查询端口6666

	char strRemoteIP[20];//远程地址
	int nRemotePort;//远程端口
	int nChannelNum;//通道数量

	DWORD dwLastRecvTime;//最后通信时间 连通后 维持心跳
	BOOL bActive;//是否在线
}DATA_LOCAL;

typedef std::map<std::string, DATA_LOCAL > LOCAL_LIST;//缓存列表内网搜索到的 云视通 ->数据

//新助手 记录所有搜索到的号码，以及可以连通的信息
typedef struct DATA_OUTER
{
	char strGroup[4];//编组
	int nYST;//号码
	
	SOCKET sLocal;
	char strRemoteIP[20];//远程地址
	int nRemotePort;//远程端口
	
	DWORD dwLastRecvTime;//最后通信时间 连通后 维持心跳
	BOOL bActive;//是否在线
}DATA_OUTER;

typedef std::map<std::string, DATA_OUTER > OUTER_LIST;//缓存列表内网搜索到的 云视通 ->数据

class CCWorker;

#define JVN_REQ_NATA_A			0xB6		//查询主控的网络类型

class CCHelper  
{
public:
	CCHelper();
	virtual ~CCHelper();

	BOOL Start(CCWorker* pWorker);//启动助手

	CCWorker* m_pWorker;

#ifndef WIN32
	pthread_t m_hSendThread;//线程句柄
	pthread_t m_hReceivThread;//线程句柄
#else
	HANDLE m_hSendThread;//线程句柄
	HANDLE m_hReceivThread;//线程句柄
#endif
	BOOL m_bExit;//是否退出工作线程

	AdapterList m_IpList;//当前ip
	void GetAdapterInfo();
#ifdef WIN32	
	static UINT WINAPI LanRecvProc(LPVOID pParam);
	static UINT WINAPI LANSSndProc(LPVOID pParam);
#else
	static void*  LanRecvProc(void* pParam);
	static void*  LANSSndProc(void* pParam);
#endif	

	pthread_mutex_t m_LocalLock;//操作广播列表
	LOCAL_LIST m_LocalList;//缓存的列表

	pthread_mutex_t m_OuterLock;//操作外网
	OUTER_LIST m_OuterList;//缓存的列表 外网连接的号码

	SOCKET m_sBroadcast;//用于广播的套接字，用于内网使用
	DWORD m_dwLastBroadcastTime;
	int m_nLocalPort;//本地广播端口

	void SearchDevice(int* pIP = NULL,BOOL bIsMobile = FALSE);
	void Broadcast();
	void ReceiveBroadcast();

	int m_nIpNum[255];//可以使用的网关
	int m_nDestPort;//广播的端口 6666

	BOOL m_bisMobile;//是否是手机

	void KeepActive();
	BYTE m_ucSearchData[30];//查询命令
	int m_nSearchLen;//查询命令长度

	DWORD m_dwLastKeepActive;//最后发送查询时间

	BOOL GetLocalInfo(char* pGroup,int nYST,char* pIP,int &nPort);//查询是否是已经搜到了在线的设备

	void AddOkYST(char* pGroup,int nYST,char* pIP,int nPort);//添加一个号码，公网地址的号码

	void AddYST(char* pGroup,int nYST,int* pIP,BOOL bIsMobile);//添加一个号码
	

	BOOL GetOuterInfo(char* pGroup,int nYST,SOCKET &sLocal,char* pIP,int &nPort);//查询已经连接过的还在维持着连接的号码
	void UpdateTime(char* pGroup,int nYST,SOCKET sLocal,SOCKADDR_IN* addrRemote);

};
#endif // !defined(AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_)
