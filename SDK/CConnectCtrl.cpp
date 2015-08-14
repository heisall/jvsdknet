// CConnectCtrl.cpp: implementation of the CCConnectCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "CConnectCtrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCConnectCtrl::CCConnectCtrl()
{
	m_nLocalPortUseds.clear();
	m_nStartPort=9200;
#ifndef WIN32
	pthread_mutex_init(&m_criticalsectionPort, NULL); //初始化临界区
#else
	InitializeCriticalSection(&m_criticalsectionPort); //初始化临界区
#endif
}

CCConnectCtrl::~CCConnectCtrl()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_criticalsectionPort);
#else
	DeleteCriticalSection(&m_criticalsectionPort); //释放临界区
#endif

}


/*端口管理*/////////////////////////////////////////////////////////////////////////
//初始化端口控制
void CCConnectCtrl::InitPortCtrl(int nStartPort)
{
#ifndef WIN32
	pthread_mutex_lock(&m_criticalsectionPort);
#else
	EnterCriticalSection(&m_criticalsectionPort);
#endif

	m_nStartPort=nStartPort;
	m_nLocalPortUseds.clear();

#ifndef WIN32
	pthread_mutex_unlock(&m_criticalsectionPort);
#else
	LeaveCriticalSection(&m_criticalsectionPort);
#endif
}

//获取空闲端口
int CCConnectCtrl::GetFreePort()
{
#ifndef WIN32
	pthread_mutex_lock(&m_criticalsectionPort);
#else
	EnterCriticalSection(&m_criticalsectionPort);
#endif

	int nLocalPort = m_nStartPort;
	if(nLocalPort > 0)
	{
		for(int i=0; i<m_nLocalPortUseds.size(); )
		{
			if(nLocalPort == m_nLocalPortUseds[i])
			{//该端口已被使用
				nLocalPort++;
				i=0;
				continue;
			}
			i++;
		}
		m_nLocalPortUseds.push_back(nLocalPort);
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_criticalsectionPort);
#else
	LeaveCriticalSection(&m_criticalsectionPort);
#endif

	return nLocalPort;
}

//删除某端口，<0时删除所有
void CCConnectCtrl::DeletePort(int nPort)
{
#ifndef WIN32
	pthread_mutex_lock(&m_criticalsectionPort);
#else
	EnterCriticalSection(&m_criticalsectionPort);
#endif

	if(nPort < 0)
	{
		m_nLocalPortUseds.clear();
	}
	else
	{
		for(int i=0; i<m_nLocalPortUseds.size(); i++)
		{
			if(nPort == m_nLocalPortUseds[i])
			{//该端口已被使用
				m_nLocalPortUseds.erase(m_nLocalPortUseds.begin()+i);
				break;
			}
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_criticalsectionPort);
#else
	LeaveCriticalSection(&m_criticalsectionPort);
#endif

}
