// CRtmpChannel.cpp: implementation of the CCRtmpChannel class.
//
//////////////////////////////////////////////////////////////////////

#include "CRtmpChannel.h"


#include <memory.h>
#include <stdio.h>
#include <string.h>


#ifdef WIN32
#include "..\RTMP\JvRtmpClient.h"
#include "..\RTMP\RtmpStream.h"
#include "..\JVNC\RtmpPlayerStream.h"
#else
#include "../RTMP/JvRtmpClient.h"
#include "../RTMP/RtmpStream.h"
#include "../JVNC/RtmpPlayerStream.h"
#endif

// #ifndef WIN32
// #include "../JVNS/SRtmpChannel.h"
// #else
// #include "..\JVNS\SRtmpChannel.h"
// #endif
using namespace JMS;

#include "Def.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CCRtmpChannel::CCRtmpChannel()
{
    m_nLocalChannel = -1;
  //  memset(m_strURL,0,sizeof(m_strURL));
    
    m_RtmpConnectCallBack = NULL;
    m_RtmpNormalDataCallBack = NULL;
    
    m_hRtmpClient = NULL;
}

CCRtmpChannel::~CCRtmpChannel()
{
    ShutDown();
}

bool CCRtmpChannel::ConnectServer(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout)
{
	m_nLocalChannel = nLocalChannel;
//	strcpy(m_strURL,strURL);
	m_RtmpConnectCallBack = rtmpConnectChange;
	m_RtmpNormalDataCallBack = rtmpNormalData;

//	if(m_hRtmpClient != NULL)
//	{
//		RtmpPlayerStream *pStream = (RtmpPlayerStream *)m_hRtmpClient;
//		delete pStream;
//		m_hRtmpClient = NULL;
//	}

	RtmpPlayerStream *pStream = NULL;

	pStream = new RtmpPlayerStream(this);
	if(pStream == NULL)
	{
		return false;
	}
#ifdef WIN32
	OutputDebug("分控连接流媒体....%s",strURL);
#endif
	if(!pStream->Connect(strURL, 512 * 1024,nTimeout))
	{
		pStream->Disconnect();
		delete pStream;
		return false;
	}
	
	m_hRtmpClient = pStream;

	return true;
}

void CCRtmpChannel::ShutDown()
{
	if(m_hRtmpClient != NULL)
	{
		RtmpPlayerStream *pStream = (RtmpPlayerStream* )m_hRtmpClient;
		//JVRTMP_Close(m_hRtmpClient);
		pStream->Disconnect();
		delete pStream;
		m_hRtmpClient = NULL;
		
	}
}

void CCRtmpChannel::Rtmp_ConnectChange(unsigned char uchType,const char *pMsg, int nPWData)
{
	char strError[1000] = {0};
	if(uchType == 2)
	{
		RtmpStreamBase *pStream = (RtmpStreamBase* )m_hRtmpClient;

		sprintf(strError,"%s  %s",pMsg,pStream->m_strLastError);
	}
	else
	{
		sprintf(strError,"%s",pMsg);

	}
	m_RtmpConnectCallBack(m_nLocalChannel,uchType,strError,nPWData);
}

void CCRtmpChannel::RTMP_NormaData(unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp)
{
    if(NULL == pBuffer){
        return;
    }
    m_RtmpNormalDataCallBack(m_nLocalChannel,uchType,pBuffer,nSize,nTimestamp);
}


