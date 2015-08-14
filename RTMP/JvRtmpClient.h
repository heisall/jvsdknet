//----------------------------------------------------------------------
// JvRtmpClient.h
// Rtmp¿Í»§¶Ë
//
// ×÷Õß£º³ÌÐÐÍ¨ Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

typedef void* JVRTMP_HANDLE;

#define JVRTMP_CONNECTEVENTTYPE_CONNECTED		0x01 //Á¬½ÓÒÑ½¨Á¢
#define JVRTMP_CONNECTEVENTTYPE_CONNECTFAILED	0x02 //Á¬½ÓÊ§°Ü
#define JVRTMP_CONNECTEVENTTYPE_DISCONNECTED	0x03 //Á¬½ÓÒÑ¶Ï¿ª
#define JVRTMP_CONNECTEVENTTYPE_DISCONNECTEDFAILED 0x04//异常断开
#define JVRTMP_CONNECTEVENTTYPE_NODATATIMEOUT      0x05//长时间没有收到数据

#define JVRTMP_DATATYPE_METADATA				0x00
#define JVRTMP_DATATYPE_H264_I					0x01
#define JVRTMP_DATATYPE_H264_BP					0x02
#define JVRTMP_DATATYPE_ALAW					0x11
#define JVRTMP_DATATYPE_ULAW					0x12

typedef struct _JVRTMP_Metadata_t
{
	int nVideoWidth;
	int nVideoHeight;
	int nVideoFrameRateNum;
	int nVideoFrameRateDen;
	int nAudioDataType;
	int nAudioSampleRate;
	int nAudioSampleBits;
	int nAudioChannels;
}
JVRTMP_Metadata_t;

typedef void (*FunJvRtmpConnectCallback_tS)(JVRTMP_HANDLE hHandle, void *pUserData, int nEvent, const char *szMsg);
typedef void (*FunJvRtmpConnectCallback_tC)(JVRTMP_HANDLE hHandle, void *pUserData, int nEvent, const char *szMsg);

typedef void (*FunJvRtmpMediaDataCallback_tC)(JVRTMP_HANDLE hHandle, void *pUserData, int nType, void *pData, int nSize, int nTimestamp);
