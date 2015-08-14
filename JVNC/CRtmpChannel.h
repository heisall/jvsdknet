// CRtmpChannel.h: interface for the CCRtmpChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_)
#define AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 成功 2 失败 3 断开 4 异常断开
typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

class CCRtmpChannel  
{
public:
	CCRtmpChannel();
	virtual ~CCRtmpChannel();

	bool ConnectServer(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout);
	void ShutDown();

	void Rtmp_ConnectChange(unsigned char uchType,const char *pMsg, int nPWData);

	void RTMP_NormaData(unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

	int m_nLocalChannel;//本地通道号
    //std::string m_strURL;
    //char m_strURL[64];//服务器地址

	FUNC_CRTMP_CONNECT_CALLBACK m_RtmpConnectCallBack;//连接回调 成功 失败 断开
	FUNC_CRTMP_NORMALDATA_CALLBACK m_RtmpNormalDataCallBack;//视频数据

	void *m_hRtmpClient;
};

#endif // !defined(AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_)
