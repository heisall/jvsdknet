//----------------------------------------------------------------------
// RtmpPlayerStream.h
// Rtmp播放视频流
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#ifdef WIN32
#include "..\RTMP\RtmpStream.h"
#else
#include "../RTMP/RtmpStream.h"
#endif

namespace JMS
{
	class RtmpPlayerStream : public RtmpStreamBase
	{
	public:
		RtmpPlayerStream(void *pUserData);
		virtual ~RtmpPlayerStream();

	public:
		virtual bool Connect(const char *szUrl, int nBufSize,int nTimeout);
		virtual void Disconnect();

		static void WorkerThreadProc(void *pParam);
		void _WorkerThreadProc();
        unsigned int JVRTMPGetTime();
        unsigned int m_nLastRecvTime;
        unsigned int m_nTimeOut;
	protected:
		virtual bool _LoopProc(int* nNoDataTimeOut);
		bool _AccessFrameBuf(int nSize);
		bool _OnVideoData(uint8_t *pData, int nSize, uint32_t dwTimestamp);
		bool _OnAudioData(uint8_t *pData, int nSize, uint32_t dwTimestamp);
		bool _OnScriptData(uint8_t *pData, int nSize, uint32_t dwTimestamp);

	private:
		int m_nFrameBufSize;
		int m_nFrameBufDataSize;
		uint8_t *m_pFrameBuf;
	};
}
