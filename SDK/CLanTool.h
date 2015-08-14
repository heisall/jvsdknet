// CLanSerch.h: interface for the CCLanSerch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLANTOOL_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_)
#define AFX_CLANTOOL_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CLanSerch.h"

class CCWorker;
class CCLanTool  
{
public:
	CCLanTool();
	CCLanTool(int nLPort, int nDesPort, CCWorker *pWorker);
	virtual ~CCLanTool();

	BOOL LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut);

private:
	void GetAdapterInfo();
	
	#ifndef WIN32
		static void* LANTRcvProc(void* pParam);
		static void* LANTSndProc(void* pParam);
	#else
		static UINT WINAPI LANTRcvProc(LPVOID pParam);
		static UINT WINAPI LANTSndProc(LPVOID pParam);
	#endif
	
public:
	BOOL m_bOK;
private:
	CCWorker *m_pWorker;
	unsigned int m_unLANTID;//
	SOCKET m_SocketLANT;
	int m_nLANTPort;
	int m_nDesPort;
	
	int m_nTimeOut;
	DWORD m_dwBeginTool;
	BOOL m_bTimeOut;
	BOOL m_bLANTooling;
	
	BOOL m_bNewTool;//是否有新的搜索
	BOOL m_bStopToolImd;//是否立即停止当前搜索发送

	AdapterList m_IpList;

	BYTE m_uchData[10340];
	int m_nNeedSend;
	
#ifndef WIN32
	pthread_t m_hLANToolRcvThread;
	pthread_t m_hLANToolSndThread;
	BOOL m_bRcvEnd;
	BOOL m_bSndEnd;
	pthread_mutex_t m_ct;
#else
	HANDLE m_hLANToolRcvThread;
	HANDLE m_hLANToolRcvStartEvent;//
	HANDLE m_hLANToolRcvEndEvent;	
	
	HANDLE m_hLANToolSndThread;
	HANDLE m_hLANToolSndStartEvent;//
	HANDLE m_hLANToolSndEndEvent;
	CRITICAL_SECTION m_ct;
#endif
};

#endif // !defined(AFX_CLANSERCH_H__8B4A509B_8997_41EC_AC0A_399C4EC178E0__INCLUDED_)
