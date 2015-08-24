// CConnectCtrl.h: interface for the CCConnectCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCONNECTCTRL_H__7E04509A_5130_433B_A7A0_D6E67344FEF0__INCLUDED_)
#define AFX_CCONNECTCTRL_H__7E04509A_5130_433B_A7A0_D6E67344FEF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CChannel.h"


class CChannel;
typedef ::std::vector<CCChannel*> Channels;
typedef ::std::vector<STCONNECTINFO> ConnList;
class CCConnectCtrl  
{
public:
	CCConnectCtrl();
	virtual ~CCConnectCtrl();
public:
	void InitPortCtrl(int nStartPort);//初始化端口控制
	int GetFreePort();//获取空闲端口
	void DeletePort(int nPort);//删除某端口，<0时删除所有
	
	
private:
	int m_nStartPort;
	::std::vector<int> m_nLocalPortUseds;//本地端口记录

#ifndef WIN32
	pthread_mutex_t m_criticalsectionPort;
#else
	CRITICAL_SECTION m_criticalsectionPort;//
#endif
};

#endif // !defined(AFX_CCONNECTCTRL_H__7E04509A_5130_433B_A7A0_D6E67344FEF0__INCLUDED_)
