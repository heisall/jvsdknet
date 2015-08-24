// RunLog.h: interface for the CRunLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RUNLOG_H__435E72C0_429B_45A8_9CC5_4F138EFA5AA7__INCLUDED_)
#define AFX_RUNLOG_H__435E72C0_429B_45A8_9CC5_4F138EFA5AA7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

class CRunLog  
{
public:
	CRunLog();
	virtual ~CRunLog();

	bool EnableLog(bool bEnable, char chLocalPath[MAX_PATH]);
 	void SetRunInfo(int nLocalChannel, char *pchEvent, char *pchFile, int nLine, char *pchJUDT=NULL);
		
	bool m_bWriteLog;
	int m_nLanguage;
private:
	bool GetCurrentPath(char chLocalPath[MAX_PATH]);
private:
	char m_chPath[MAX_PATH];

#ifndef WIN32
	pthread_mutex_t m_ct;
#else
	CRITICAL_SECTION m_ct;
#endif
};

#endif // !defined(AFX_RUNLOG_H__435E72C0_429B_45A8_9CC5_4F138EFA5AA7__INCLUDED_)
