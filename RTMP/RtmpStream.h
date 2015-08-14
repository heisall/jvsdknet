//----------------------------------------------------------------------
// RtmpStream.h
// Rtmp视频流
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#include "JRCDef.h"
#include "Thread.h"
#include <string>
#include "JvRtmpClient.h"
#include "srs_librtmp.h"


namespace JMS
{
	class RtmpStreamBase
	{
	public:
		RtmpStreamBase(void *pUserData, bool bWriteMode);
		virtual ~RtmpStreamBase();

		enum StreamState
		{
			SS_Closed,
			SS_Connecting,
			SS_Connected,
			SS_Clossing
		};

		char m_strLastError[64];

		bool _RtmpConnect(bool bWriteMode);
		void _RtmpDisconnect();

	public:

		//手机专用
		bool m_bExitSignal;//线程已结束标志 手机用
		bool m_bExit;//开始结束线程 手机用

	protected:

	protected:
		void *m_pUserData;//CSRtmpChannel
		srs_rtmp_t m_hRtmp;
		bool m_bWriteMode;
		Thread m_hThread;
		bool m_bThreadRun;
		StreamState m_ssState;
		std::string m_strUrl;
	};
}
