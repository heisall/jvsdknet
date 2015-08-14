//----------------------------------------------------------------------
// RtmpStream.cpp
// Rtmp视频流
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#include "JvRtmpClient.h"
#include "RtmpStream.h"
#include <cassert>
#include <cstring>

namespace JMS
{
	RtmpStreamBase::RtmpStreamBase(void *pUserData, bool bWriteMode)
		: m_hRtmp(NULL), m_pUserData(pUserData), m_bWriteMode(bWriteMode),
		m_ssState(SS_Closed)
	{
		memset(m_strLastError,0,sizeof(m_strLastError));
		
		m_bExitSignal = false;//线程已结束标志 手机用
		m_bExit = false;//开始结束线程 手机用
	}

	RtmpStreamBase::~RtmpStreamBase()
	{
	}

	bool RtmpStreamBase::_RtmpConnect(bool bWriteMode)
	{
		if(m_hRtmp != NULL)
		{
			sprintf(m_strLastError,"_RtmpConnect error m_hRtmp == NULL line = %d",__LINE__);
			return true;
		}

		m_hRtmp = srs_rtmp_create((char*)m_strUrl.c_str());
		if(m_hRtmp == NULL)
		{
			sprintf(m_strLastError,"_RtmpConnect error srs_rtmp_create error line = %d",__LINE__);
			return false;
		}

		if(srs_simple_handshake(m_hRtmp) != 0)
		{
			sprintf(m_strLastError,"_RtmpConnect error srs_simple_handshake error line = %d",__LINE__);
			_RtmpDisconnect();
			return false;
		}

		if(srs_connect_app(m_hRtmp) != 0)
		{
			sprintf(m_strLastError,"_RtmpConnect error srs_connect_app error line = %d",__LINE__);
			_RtmpDisconnect();
			return false;
		}

		if(bWriteMode)
		{
			if(srs_publish_stream(m_hRtmp) != 0)
			{
				sprintf(m_strLastError,"_RtmpConnect error srs_publish_stream error line = %d",__LINE__);
				_RtmpDisconnect();
				return false;
			}
		}
		else
		{
			if(srs_play_stream(m_hRtmp) != 0)
			{
				sprintf(m_strLastError,"_RtmpConnect error srs_play_stream error line = %d",__LINE__);
				_RtmpDisconnect();
				return false;
			}
		}
		m_ssState = SS_Connected;

		return true;
	}

	void RtmpStreamBase::_RtmpDisconnect()
	{
		if(m_hRtmp != NULL)
		{
			m_ssState = SS_Clossing;
			srs_rtmp_destroy(m_hRtmp);
			m_hRtmp = NULL;
		}
		m_ssState = SS_Closed;
	}

}
