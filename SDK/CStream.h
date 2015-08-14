#ifndef CSSTREAM_H
#define CSSTREAM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CBufferCtrl.h"

//#include "UpnpImpl.h"
#include<stdlib.h>
#include<time.h>
#define random() (9200 + rand()%50000)

typedef struct STJVID
{
	unsigned int idH;
	unsigned int idL;
}STJVID;
#define ISIDZERO(id) ((id.idH==0)&&(id.idL==0))						//??ID???0
#define CMPID(id1,id2) ((id1.idH==id2.idH)&&(id1.idL==id2.idL))		//??ID???? ????TRUE
#define COPYID(tar,src)  {tar.idH=src.idH;tar.idL=src.idL;}         //ID??
#define ZEROID(id)  {id.idH=0;id.idL=0;}

//#pragma pack(1)
//??????????????
typedef struct ReqConnectPacket
{
	BYTE cType;
	int nLen;
	int nChannel;
	int nProto;
	unsigned int idSID;
	STJVID idCID;
	char pYST[16];
}ReqConnectPacket;

class CCWorker;
class CCOldChannel;
class CCStream  
{
public:
	CCStream();
	CCStream(int nChannel,int nLocalChannel,int nProtocolType,CCWorker *pWorker,CCOldChannel *pParentr);

	virtual ~CCStream();
	BOOL SendData(int sock,BYTE uchType, BYTE *pBuffer,int nSize);  //???????????(???ID)
	BOOL SendData(BYTE uchType, BYTE *pBuffer,int nSize); //???????????(??ID)
	BOOL DisConnectFromStream();  //????????????
	void ClearBuffer();

	BOOL ConnectStreamServer();     //????????
private:
	BOOL SendDataTCP(int sock,BYTE uchType, BYTE *pBuffer,int nSize);

	#ifndef WIN32
		static void* RecvFromSServer(void* pParam);
		static void* RecvFromSServerTCP(void* pParam);
	#else
		static UINT WINAPI RecvFromSServer(LPVOID pParam);
		static UINT WINAPI RecvFromSServerTCP(LPVOID pParam);
	#endif
	
	int RecvProc();
public:
	int m_nLocalChannel;//????? >=1
	int m_nChannel;//????? >=1
	int m_SServerSocket;//??????socket
	BOOL m_bPass;

	unsigned long m_nSServerIP;
	int m_nSServerPort;
	unsigned int m_idSServerID;  //????ID
	STJVID m_idMyID;       //??ID
private:	
	int m_nProtocolType;//?????? TYPE_PC_UDP/TYPE_PC_TCP/TYPE_MO_TCP
	BOOL m_bAcceptChat;//??????
	BOOL m_bAcceptText;//??????
	BOOL m_bDAndP;//?????

	CCWorker *m_pWorker;
	CCOldChannel *m_pParent;
	int m_nPACKLEN;

	BOOL m_bExit;
	
#ifndef WIN32
	pthread_t m_hSRecvThread;//????
#else
	HANDLE m_hSRecvThread;//????
#endif
	
	int m_nOCount;//?????
	CCOldBufferCtrl *m_pBuffer;
};

#endif




