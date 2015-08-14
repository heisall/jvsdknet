//----------------------------------------------------------------------
// RtmpPlayerStream.cpp
// Rtmp播放视频流
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#include "RtmpPlayerStream.h"
#include "Def.h"
#ifndef MOBILE_CLIENT
#include <malloc.h>
#endif

#ifdef WIN32
#include "..\RTMP\AmfSerializer.h"
#else
#include "../RTMP/AmfSerializer.h"
#endif
#include "CRtmpChannel.h"




namespace JMS
{
	RtmpPlayerStream::RtmpPlayerStream(void *pUserData)
		: RtmpStreamBase(pUserData, false), m_nFrameBufSize(0),
		m_nFrameBufDataSize(0), m_pFrameBuf(NULL)
	{
        m_nLastRecvTime  = JVRTMPGetTime();
        m_nTimeOut =60*1000;
	}

	RtmpPlayerStream::~RtmpPlayerStream()
	{
		if(m_pFrameBuf != NULL)
		{
			free(m_pFrameBuf);
		}
	}

    unsigned int RtmpPlayerStream::JVRTMPGetTime()
    {
        unsigned int  dwresult=0;
#ifndef WIN32
        struct timeval start;
        gettimeofday(&start,NULL);
        
        dwresult = (unsigned int )(start.tv_sec*1000 + start.tv_usec/1000);
#else
        dwresult = GetTickCount();
#endif
        
        return dwresult;
    }
	bool RtmpPlayerStream::Connect(const char *szUrl, int nBufSize,int nTimeout)
	{
		m_strUrl = szUrl;
		m_ssState = SS_Connecting;
		m_bThreadRun = true;
        m_nTimeOut = nTimeout;
         m_nLastRecvTime  = JVRTMPGetTime();
		if(!m_hThread.Run(WorkerThreadProc, this))
		{
			sprintf(m_strLastError,"m_hThread.Run error line = %d",__LINE__);
			return false;
		}
		
		return true;
	}

	void RtmpPlayerStream::Disconnect()
	{
		m_bThreadRun = false;
		if(!m_hThread.WaitForEnd(3000))
		{
			m_hThread.KillThread();
		}
	}

	bool RtmpPlayerStream::_LoopProc(int* nNoDataTimeOut)
	{
        *nNoDataTimeOut = 0;
		int nType = 0, nPacketLen = 0;
		uint32_t dwTimestamp = 0;
		char *pPacket = NULL;
		int nR = srs_read_packet(m_hRtmp, &nType, &dwTimestamp, &pPacket, &nPacketLen);
		if(nR == -999)//没有读出数据来
		{
#ifdef WIN32
			Sleep(100);
#else
			usleep(100 * 1000);
#endif
            unsigned int nTime = JVRTMPGetTime();//
            if((nTime - m_nLastRecvTime) > m_nTimeOut)
            {
                *nNoDataTimeOut = 1;
                return false;
            }
            return true;
		}
        m_nLastRecvTime =JVRTMPGetTime();
		if(nR == -9999)//已断开
		{
			return false;
		}
		if(nR != 0)
		{
			return false;
		}

		bool bRet = true;
		switch(nType)
		{
		case SRS_RTMP_TYPE_AUDIO:
			bRet = _OnAudioData((uint8_t*)pPacket, nPacketLen, dwTimestamp);
			break;

		case SRS_RTMP_TYPE_VIDEO:
			bRet = _OnVideoData((uint8_t*)pPacket, nPacketLen, dwTimestamp);
			break;

		case SRS_RTMP_TYPE_SCRIPT:
			bRet = _OnScriptData((uint8_t*)pPacket, nPacketLen, dwTimestamp);
			break;

		default:
			assert(false);
			bRet = false;
			break;
		}

		free(pPacket);

		return bRet;
	}

	bool RtmpPlayerStream::_AccessFrameBuf(int nSize)
	{
		if(m_nFrameBufSize >= nSize && m_pFrameBuf != NULL)
		{
			return true;
		}

		if(m_pFrameBuf == NULL)
		{
			m_pFrameBuf = (uint8_t*)malloc(nSize);
		}
		else
		{
			m_pFrameBuf = (uint8_t*)realloc(m_pFrameBuf, nSize);
		}
		if(m_pFrameBuf == NULL)
		{
			return false;
		}
		m_nFrameBufSize = nSize;

		return true;
	}

	bool RtmpPlayerStream::_OnVideoData(uint8_t *pData, int nSize, uint32_t dwTimestamp)
	{
//		printf("%2.2X %2.2X %2.2X %2.2X %2.2X %d\n", pData[0], pData[1], pData[2], pData[3], pData[4], nSize);

		if((pData[0] & 0x0F) != 0x07)
		{
			return false;
		}

		if(pData[1] == 0)
		{
			if(!_AccessFrameBuf(1024))
			{
				return false;
			}
			
			int nLen, nNum;
			uint8_t *pDest = m_pFrameBuf, *pSrc = pData + 11;
			nLen = *(pSrc++);
			nLen <<= 8;
			nLen |= *(pSrc++);
			*(pDest++) = 0;
			*(pDest++) = 0;
			*(pDest++) = 0;
			*(pDest++) = 1;
			memcpy(pDest, pSrc, nLen);
			pSrc += nLen;
			pDest += nLen;
			nNum = *(pSrc++);
			for(int i = 0; i < nNum; ++i)
			{
				nLen = *(pSrc++);
				nLen <<= 8;
				nLen |= *(pSrc++);
				*(pDest++) = 0;
				*(pDest++) = 0;
				*(pDest++) = 0;
				*(pDest++) = 1;
				memcpy(pDest, pSrc, nLen);
				pSrc += nLen;
				pDest += nLen;
			}
			*(pDest++) = 0;
			*(pDest++) = 0;
			*(pDest++) = 0;
			*(pDest++) = 1;
			m_nFrameBufDataSize = pDest - m_pFrameBuf;
		}
		else
		{
			int nLen = 0;
			nLen = pData[5];
			nLen <<= 8;
			nLen |= pData[6];
			nLen <<= 8;
			nLen |= pData[7];
			nLen <<= 8;
			nLen |= pData[8];
			int nOffset = 0;
			if((pData[9] & 0x1F) == 6)
			{
				//跳过SEI
				nOffset = nLen + 4;
				nLen = pData[5 + nOffset];
				nLen <<= 8;
				nLen |= pData[6 + nOffset];
				nLen <<= 8;
				nLen |= pData[7 + nOffset];
				nLen <<= 8;
				nLen |= pData[8 + nOffset];
				//_LogPrint("Skeep sei------------------------------------\n");
			}
			bool bKeyFrame = (pData[9 + nOffset] & 0x1F) == 5;
			if(!_AccessFrameBuf(m_nFrameBufDataSize + nLen))
			{
				return false;
			}
			memcpy(m_pFrameBuf + m_nFrameBufDataSize, pData + 9 + nOffset, nLen);
			CCRtmpChannel* pRTMP = (CCRtmpChannel* )m_pUserData;

			if(bKeyFrame)
			{
				pRTMP->RTMP_NormaData(JVRTMP_DATATYPE_H264_I, m_pFrameBuf, m_nFrameBufDataSize + nLen, dwTimestamp);
			}
			else
			{
				if(m_nFrameBufDataSize > 4)
					pRTMP->RTMP_NormaData(JVRTMP_DATATYPE_H264_BP, m_pFrameBuf + m_nFrameBufDataSize - 4, nLen + 4, dwTimestamp);
			}
		}

		return true;
	}

	bool RtmpPlayerStream::_OnAudioData(uint8_t *pData, int nSize, uint32_t dwTimestamp)
	{
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )m_pUserData;

		switch(pData[0] & 0xF0)
		{
		case 0x70:
			pRTMP->RTMP_NormaData(JVRTMP_DATATYPE_ALAW, pData + 1, nSize - 1, dwTimestamp);
			break;

		case 0x80:
			pRTMP->RTMP_NormaData(JVRTMP_DATATYPE_ULAW, pData + 1, nSize - 1, dwTimestamp);
			break;
		}
		
		return true;
	}

	bool RtmpPlayerStream::_OnScriptData(uint8_t *pData, int nSize, uint32_t dwTimestamp)
	{
		if(*(pData++) != 2)
		{
			return true;
		}
		--nSize;

		std::string strValue;
		int nLen = AmfSerializer::ReadString(pData, nSize, strValue, false);
		if(nLen <= 0)
		{
			return true;
		}
		if(strValue != "onMetaData")
		{
			return true;
		}
		pData += nLen;
		nSize -= nLen;

		++pData;

		JVRTMP_Metadata_t metadata = {0};
		while(true)
		{
			nLen = AmfSerializer::ReadString(pData, nSize, strValue, false);
			if(nLen <= 0)
			{
				break;
			}
			pData += nLen;
			nSize -= nLen;

			if(strValue == "width" || strValue == "height" || strValue == "framerate" || strValue == "audiosamplerate" ||
				strValue == "audiosamplesize" || strValue == "audiocodecid" || strValue == "audiochannels")
			{
				double dbValue;
				nLen = AmfSerializer::ReadNumber(pData, nSize, dbValue, true);
				if(nLen <= 0)
				{
					break;
				}
				pData += nLen;
				nSize -= nLen;

				if(strValue == "width")
				{
					metadata.nVideoWidth = (int)dbValue;
				}
				else if(strValue == "height")
				{
					metadata.nVideoHeight = (int)dbValue;
				}
				else if(strValue == "framerate")
				{
					metadata.nVideoFrameRateNum = (int)(1000.0f * dbValue);
					metadata.nVideoFrameRateDen = 1000;
				}
				else if(strValue == "audiosamplerate")
				{
					metadata.nAudioSampleRate = (int)dbValue;
				}
				else if(strValue == "audiosamplesize")
				{
					metadata.nAudioSampleBits = 8 * (int)dbValue;
				}
				else if(strValue == "audiocodecid")
				{
					if(dbValue == 7.0f)
					{
						metadata.nAudioDataType = JVRTMP_DATATYPE_ALAW;
					}
					else if(dbValue == 8.0f)
					{
						metadata.nAudioDataType = JVRTMP_DATATYPE_ULAW;
					}
					else
					{
						metadata.nAudioDataType = 0;
					}
				}
				else if(strValue == "audiochannels")
				{
					metadata.nAudioChannels = (int)dbValue;
				}
			}
			else
			{
				nLen = AmfSerializer::SkeepValue(pData, nSize);
				if(nLen <= 0)
				{
					break;
				}
				pData += nLen;
				nSize -= nLen;
			}
		}
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )m_pUserData;

		pRTMP->RTMP_NormaData(JVRTMP_DATATYPE_METADATA, (unsigned char* )&metadata, sizeof(metadata), 0);

		return true;
	}


	
	void RtmpPlayerStream::WorkerThreadProc(void *pParam)
	{
		RtmpPlayerStream *pInstance = (RtmpPlayerStream*)pParam;
		pInstance->_WorkerThreadProc();
	}
	
	void RtmpPlayerStream::_WorkerThreadProc()
	{
		CCRtmpChannel* pRTMP = (CCRtmpChannel* )m_pUserData;
		if(_RtmpConnect(m_bWriteMode))
		{
			pRTMP->Rtmp_ConnectChange(JVRTMP_CONNECTEVENTTYPE_CONNECTED, "connect success",__LINE__);
		}
		else
		{
            m_bExitSignal = true;
			pRTMP->Rtmp_ConnectChange(JVRTMP_CONNECTEVENTTYPE_CONNECTFAILED, "connect failed",__LINE__);
			return;
		}
		
		 bool b=true;
		while(m_bThreadRun)
		{
            int nNoDataTimeOut = 0;
			if(!_LoopProc(&nNoDataTimeOut))
			{
				m_bThreadRun = false;
				m_ssState = SS_Clossing;
				 b = false;
			}
			if(m_bExit)
			{
				break;
			}
            if(nNoDataTimeOut == 1)
            {
                _RtmpDisconnect();

                pRTMP->Rtmp_ConnectChange(JVRTMP_CONNECTEVENTTYPE_NODATATIMEOUT, "no data time out",__LINE__);

                m_bExitSignal = true;

                return;
			}
			
		}
		
		_RtmpDisconnect();
		if (!b) {
			pRTMP->Rtmp_ConnectChange(JVRTMP_CONNECTEVENTTYPE_DISCONNECTEDFAILED, "disconnect exception",__LINE__);
		}else{
			pRTMP->Rtmp_ConnectChange(JVRTMP_CONNECTEVENTTYPE_DISCONNECTED, "disconnect success",__LINE__);
		}

		m_bExitSignal = true;
	}
	

}
