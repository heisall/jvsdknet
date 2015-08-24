// COldChannel.h: interface for the CCOldChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COLDCHANNEL_H__E45EAC5E_523B_4B4A_A3F9_C97D11DCD672__INCLUDED_)
#define AFX_COLDCHANNEL_H__E45EAC5E_523B_4B4A_A3F9_C97D11DCD672__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "UpnpImpl.h"
#include "CChannel.h"
#include "CBufferCtrl.h"

#include "JVNSDKDef.h"
#include "CStream.h"
#include<stdlib.h>
#include<time.h>
#define random() (9200 + rand()%50000)

class CCWorker;
class CCChannel;

class CCOldChannel
{
public:
    CCOldChannel();
    CCOldChannel(CCWorker *pWorker, CCChannel *pChannel, BOOL bIMMO=FALSE);
    virtual ~CCOldChannel();
    
    BOOL SendData(BYTE uchType, BYTE *pBuffer,int nSize);
    
    BOOL DisConnect();
    BOOL StopConnect();
    
    void ClearBuffer();
    void SetNoChannel();
    
public:
    int m_nLocalChannel;//本地通道号 >=1
    int m_nChannel;//服务通道号 >=1
    
    BOOL m_bClose;//
    
    int m_nStatus;//主连接当前状态
    DWORD m_dwStartTime;//主连接关键计时
    
    BOOL m_bCanDelS;//因为流媒体服务器的存在是否可以清理
    BOOL m_bByStreamServer;//是否是流媒体服务器方式连接
    CCChannel *m_pChannel;
    
    //	int m_nFYSTVER;//远端云视通协议版本
    BOOL m_recvThreadExit ;//
    BOOL m_connectThreadExit ;//
    BOOL m_playProExit ;
    
private:
    
    BOOL SendDataTCP(BYTE uchType, BYTE *pBuffer,int nSize);
    
    void ProcessDisConnect();
    
#ifndef WIN32
    static void* ConnProc(void* pParam);//发送线程，用于处理主连接
    static void* RecvProc(void* pParam);
    static void* RecvMsgProc(void* pParam);
    static void* RecvProcTCP(void* pParam);
    static void* PlayProc(void* pParam);
#else
    static UINT WINAPI ConnProc(LPVOID pParam);//发送线程，用于处理主连接
    static UINT WINAPI RecvProc(LPVOID pParam);
    static UINT WINAPI RecvMsgProc(LPVOID pParam);
    static UINT WINAPI RecvProcTCP(LPVOID pParam);
    static UINT WINAPI PlayProc(LPVOID pParam);
#endif
    
    //
    BOOL SendPWCheck();
    int RecvPWCheck(int &nPWData);
    BOOL StartWorkThread();
private:
    int m_nProtocolType;//连接协议类型 TYPE_PC_UDP/TYPE_PC_TCP/TYPE_MO_TCP
    BOOL m_bAcceptChat;//接受语音请求
    BOOL m_bAcceptText;//接受文本请求
    BOOL m_bDAndP;//下载回放中
    
    BOOL m_bPass;//身份验证成功
    CCWorker *m_pWorker;
    
    int m_nPACKLEN;
    
    BOOL m_bShouldSendMOType;//是否需要发送手机类型，当手机连接新主控时需要发送
    
    BOOL m_bExit;
    
#ifndef WIN32
    pthread_t m_hConnThread;//线程句柄
    pthread_t m_hRecvThread;//线程句柄
    pthread_t m_hPlayThread;//线程句柄
    BOOL m_bEndC;
    BOOL m_bEndR;
    BOOL m_bEndP;
#else
    HANDLE m_hConnThread;//线程句柄
    HANDLE m_hStartEventC;
    HANDLE m_hEndEventC;
    
    HANDLE m_hRecvThread;//线程句柄
    HANDLE m_hStartEventR;
    HANDLE m_hEndEventR;
    
    HANDLE m_hPlayThread;//线程句柄
    HANDLE m_hStartEventP;
    HANDLE m_hEndEventP;
#endif
    BYTE *m_preadBuf;
    BYTE *m_precBuf;
    
    int m_nOCount;//收到的帧数
    CCBaseBufferCtrl *m_pBuffer;
    
    CCStream *m_pStream;
};

#endif // !defined(AFX_COLDCHANNEL_H__E45EAC5E_523B_4B4A_A3F9_C97D11DCD672__INCLUDED_)
