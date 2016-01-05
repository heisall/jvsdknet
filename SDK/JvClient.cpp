//#define JVCLIENT_API extern "C" __declspec(dllexport)

#ifdef WIN32
#include <process.h>
#endif

#include "JVNSDKDef.h"
#include "CWorker.h"
#include "JVN_DBG.h"
CCWorker *g_pCWorker = NULL;



extern FUNC_UDT_RECV_CALLBACK g_pfRecv;

void UDT_Recv(SOCKET sSocket,int nLocalPort,sockaddr* addrRemote,char *pMsg,int nLen,BOOL bNewUDP);//ªÿµ˜∫Ø ˝ ªÿµ˜UDTø‚ƒ⁄≤øΩ” ’µΩµƒ∑«UDT ˝æ›∞¸

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif

static void funcCconnectNULL(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData){};
static void funcCnormaldataNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight){};
static void funcCcheckresultNULL(int nLocalChannel,BYTE *pBuffer, int nSize){};
static void funcCchatdataNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize){};
static void funcCtextdataNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize){};
static void funcCdownloadNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize, int nFileLen){};
static void funcCplaydataNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame){};

static void funcCbufrateNULL(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize, int nRate){};
static void funcClansdataNULL(STLANSRESULT stLSResult){};
static void funcCBCdataNULL(int nBCID, BYTE *pBuffer, int nSize, char chIP[16], BOOL bTimeOut){};
static int  funcCLANTdataNULL(STLANTOOLINFO *pLANTData){return 0;};
static void  funcWriteReadNULL(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize){};
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                     //
//                                      ∑÷øÿ∂ÀΩ”ø⁄                                                     //
//                                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************
 *√˚≥∆  : JVC_InitSDK
 *π¶ƒ‹  : ≥ı ºªØSDK◊ ‘¥£¨±ÿ–Î±ªµ⁄“ª∏ˆµ˜”√
 *≤Œ ˝  : [IN] nLocalStartPort ±æµÿ¡¨Ω” π”√µƒ∆ º∂Àø⁄ ƒ¨»œ9200
 *∑µªÿ÷µ: TRUE     ≥…π¶
 FALSE     ß∞‹
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
#ifdef MOBILE_CLIENT
JVCLIENT_API int JVC_InitSDK(int nLocStartPort,char* pfWriteReadData)
#else
JVCLIENT_API int JVC_InitSDK(int nLocStartPort)
#endif
#else
JVCLIENT_API bool __stdcall	JVC_InitSDK(int nLocStartPort)
#endif
{
    g_pfRecv = UDT_Recv;
    
    if(nLocStartPort < 0)
    {
        nLocStartPort = 9200;
    }
    if(g_pCWorker == NULL)
    {
#ifdef MOBILE_CLIENT
        g_pCWorker = new CCWorker(nLocStartPort,pfWriteReadData);
#else
        g_pCWorker = new CCWorker(nLocStartPort);
#endif
        if(g_pCWorker == NULL)
        {
            return FALSE;
        }
        
        g_pCWorker->m_pfConnect = funcCconnectNULL;
        g_pCWorker->m_pfNormalData = funcCnormaldataNULL;
        g_pCWorker->m_pfCheckResult = funcCcheckresultNULL;
        g_pCWorker->m_pfChatData = funcCchatdataNULL;
        g_pCWorker->m_pfTextData = funcCtextdataNULL;
        g_pCWorker->m_pfDownLoad = funcCdownloadNULL;
        g_pCWorker->m_pfPlayData = funcCplaydataNULL;
        g_pCWorker->m_pfBufRate = funcCbufrateNULL;
        g_pCWorker->m_pfLANSData = funcClansdataNULL;
        g_pCWorker->m_pfBCData = funcCBCdataNULL;
        g_pCWorker->m_pfLANTData = funcCLANTdataNULL;
        g_pCWorker->m_pfWriteReadData = funcWriteReadNULL;
        
#ifndef MOBILE_CLIENT
        g_pCWorker->StartLANSerchServer(0, 6666);//ø™ ºæÕ∆Ù∂Ø
#endif
        
        
        
    }
    
    return TRUE;
}

/****************************************************************************
 *√˚≥∆  : JVC_ReleaseSDK
 *π¶ƒ‹  :  Õ∑≈SDK◊ ‘¥£¨±ÿ–Î◊Ó∫Û±ªµ˜”√
 *≤Œ ˝  : Œﬁ
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_ReleaseSDK()
#else
JVCLIENT_API void __stdcall	JVC_ReleaseSDK()
#endif
{
    if(g_pCWorker != NULL)
    {
        delete g_pCWorker;
        g_pCWorker = NULL;
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_RegisterSCallBack
 *π¶ƒ‹  : …Ë÷√∑÷øÿ∂Àªÿµ˜∫Ø ˝
 *≤Œ ˝  : [IN] ConnectCallBack   ”Î÷˜øÿ¡¨Ω”◊¥øˆªÿµ˜∫Ø ˝
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : ∑÷øÿ∂Àªÿµ˜∫Ø ˝∞¸¿®£∫
 ”Î÷˜øÿ∂ÀÕ®–≈◊¥Ã¨∫Ø ˝£ª      (¡¨Ω”◊¥Ã¨)
  µ ±º‡øÿ¥¶¿Ì                ( ’µΩ µ ±º‡øÿ ˝æ›)
 ¬ºœÒºÏÀ˜Ω·π˚¥¶¿Ì∫Ø ˝£ª      ( ’µΩ¬ºœÒºÏÀ˜Ω·π˚)
 ‘∂≥Ãªÿ∑≈∫Ø ˝£ª              ( ’µΩ‘∂≥Ãªÿ∑≈ ˝æ›)
 ”Ô“Ù¡ƒÃÏ/Œƒ±æ¡ƒÃÏ∫Ø ˝       (‘∂≥Ã”Ô“Ù∫ÕŒƒ±æ¡ƒÃÏ)
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_RegisterCallBack(FUNC_CCONNECT_CALLBACK ConnectCallBack,
                                       FUNC_CNORMALDATA_CALLBACK NormalData,
                                       FUNC_CCHECKRESULT_CALLBACK CheckResult,
                                       FUNC_CCHATDATA_CALLBACK ChatData,
                                       FUNC_CTEXTDATA_CALLBACK TextData,
                                       FUNC_CDOWNLOAD_CALLBACK DownLoad,
                                       FUNC_CPLAYDATA_CALLBACK PlayData)
#else
JVCLIENT_API void __stdcall	JVC_RegisterCallBack(FUNC_CCONNECT_CALLBACK ConnectCallBack,
                                                 FUNC_CNORMALDATA_CALLBACK NormalData,
                                                 FUNC_CCHECKRESULT_CALLBACK CheckResult,
                                                 FUNC_CCHATDATA_CALLBACK ChatData,
                                                 FUNC_CTEXTDATA_CALLBACK TextData,
                                                 FUNC_CDOWNLOAD_CALLBACK DownLoad,
                                                 FUNC_CPLAYDATA_CALLBACK PlayData)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(ConnectCallBack != NULL)
        {
            g_pCWorker->m_pfConnect = ConnectCallBack;
        }
        
        if(NormalData != NULL)
        {
            g_pCWorker->m_pfNormalData = NormalData;
        }
        
        if(CheckResult != NULL)
        {
            g_pCWorker->m_pfCheckResult = CheckResult;
        }
        
        if(ChatData != NULL)
        {
            g_pCWorker->m_pfChatData = ChatData;
        }
        
        if(TextData != NULL)
        {
            g_pCWorker->m_pfTextData = TextData;
        }
        
        if(DownLoad != NULL)
        {
            g_pCWorker->m_pfDownLoad = DownLoad;
        }
        
        if(PlayData != NULL)
        {
            g_pCWorker->m_pfPlayData = PlayData;
        }
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_Connect
 *π¶ƒ‹  : ¡¨Ω”ƒ≥Õ®µ¿Õ¯¬Á∑˛ŒÒ
 *≤Œ ˝  : [IN] nLocalChannel ±æµÿÕ®µ¿∫≈ >=1
 [IN] nChannel      ∑˛ŒÒÕ®µ¿∫≈ >=1
 [IN] pchServerIP   µ±nYSTNO<0 ±£¨∏√≤Œ ˝÷∏Õ®µ¿∑˛ŒÒIP£ªµ±nYSTNO>=0 ±Œﬁ–ß.
 [IN] nServerPort   µ±nYSTNO<0 ±£¨∏√≤Œ ˝÷∏Õ®µ¿∑˛ŒÒport£ªµ±nYSTNO>=0 ±Œﬁ–ß.
 [IN] pchPassName   ”√ªß√˚
 [IN] pchPassWord   √‹¬Î
 [IN] nYSTNO        ‘∆ ”Õ®∫≈¬Î£¨≤ª π”√ ±÷√-1
 [IN] chGroup       ±‡◊È∫≈£¨–Œ»Á"A" "AAAA"  π”√‘∆ ”Õ®∫≈¬Î ±”––ß
 [IN] bLocalTry      «∑ÒΩ¯––ƒ⁄Õ¯ÃΩ≤‚
 [IN] nTURNType     YST∑Ω Ω ±”––ß,◊™∑¢π¶ƒ‹¿‡–Õ(Ω˚”√◊™∑¢\∆Ù”√◊™∑¢(ƒ¨»œ÷µ)\Ωˆ”√◊™∑¢)
 *∑µªÿ÷µ: TRUE  ≥…π¶
 FALSE  ß∞‹
 *∆‰À˚  : nLocalChannel <= -2 «“ nChannel = -1 ø…¡¨Ω”∑˛ŒÒ∂ÀµƒÃÿ ‚Õ®µ¿£¨
 ø…±‹ø™ ”∆µ ˝æ›£¨”√”⁄ ’∑¢∆’Õ® ˝æ›
 *****************************************************************************/
#ifndef WIN32
#ifdef MOBILE_CLIENT
JVCLIENT_API void JVC_Connect(int nLocalChannel,int nChannel,
                              char *pchServerIP,int nServerPort,
                              char *pchPassName,char *pchPassWord,
                              int nYSTNO,char chGroup[4],
                              BOOL bLocalTry,
                              int nTURNType,
                              BOOL bCache,
                              int nConnectType,BOOL isBeRequestVedio,int nVIP
							  ,int nOnlyTCP
							  )
#else
JVCLIENT_API void JVC_Connect(int nLocalChannel,int nChannel,
                              char *pchServerIP,int nServerPort,
                              char *pchPassName,char *pchPassWord,
                              int nYSTNO,char chGroup[4],
                              BOOL bLocalTry,
                              int nTURNType,
                              BOOL bCache)
#endif
#else
JVCLIENT_API void __stdcall	JVC_Connect(int nLocalChannel,int nChannel,
                                        char *pchServerIP,int nServerPort,
                                        char *pchPassName,char *pchPassWord,
                                        int nYSTNO,char chGroup[4],
                                        BOOL bLocalTry,
                                        int nTURNType,
                                        BOOL bCache)
#endif
{
    if(g_pCWorker != NULL)
    {
        if((nLocalChannel <= 0 && nChannel != -1) || (nChannel <= 0 && nChannel != -1) || nLocalChannel > 65535 || nChannel > 65535)
        {//≤Œ ˝Œﬁ–ß
			char chMsg[] = "通道号参数无效,有效范围[1,65535]";
            g_pCWorker->ConnectChange(nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            return;
        }
        
        if(nYSTNO < 0)
        {//æ÷”ÚÕ¯
            if(pchServerIP == NULL || strlen(pchServerIP) < 7 || strlen(pchServerIP) > 18)
            {//≤Œ ˝Œﬁ–ß
				char chMsg[] = "参数无效,进行局域网连接,但未传入合法的IP地址!";
                g_pCWorker->ConnectChange(nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                return;
            }
#ifdef MOBILE_CLIENT
            g_pCWorker->m_dwConnectTime = CCWorker::JVGetTime();
            g_pCWorker->ConnectServerDirect(nLocalChannel, nChannel, pchServerIP, nServerPort,pchPassName, pchPassWord, bCache, nConnectType,isBeRequestVedio
			,nOnlyTCP
			);
#else
            g_pCWorker->ConnectServerDirect(nLocalChannel, nChannel, pchServerIP, nServerPort,pchPassName, pchPassWord, bCache);
            //			g_pCWorker->ConnectServerDirect(nLocalChannel, nChannel, pchServerIP, nServerPort,pchPassName, pchPassWord, bCache, TYPE_3GMO_UDP);
#endif
            return;
        }
        else
        {//YST
            if(strlen(chGroup) <= 0)
            {//±‡◊È∫≈Œﬁ–ß£¨Œﬁ–ßµƒ‘∆ ”Õ®∫≈¬Î£¨Ω· ¯¡¨Ω”
				char chMsg[] = "云视通号码无效,请确认";
                g_pCWorker->ConnectChange(nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
                return;
            }
#ifdef MOBILE_CLIENT
            g_pCWorker->m_dwConnectTime = CCWorker::JVGetTime();
            g_pCWorker->ConnectServerByYST(nLocalChannel, nChannel, nYSTNO, chGroup, pchPassName, pchPassWord, bLocalTry,nTURNType,FALSE,nConnectType,isBeRequestVedio,nVIP);
#else
            g_pCWorker->ConnectServerByYST(nLocalChannel, nChannel, nYSTNO, chGroup, pchPassName, pchPassWord, bLocalTry,nTURNType,bCache);
            //			g_pCWorker->ConnectServerByYST(nLocalChannel, nChannel, nYSTNO, chGroup, pchPassName, pchPassWord, bLocalTry,nTURNType,FALSE,TYPE_3GMOHOME_UDP);
#endif
            return;
        }
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_DisConnect
 *π¶ƒ‹  : ∂œø™ƒ≥Õ®µ¿∑˛ŒÒ¡¨Ω”
 *≤Œ ˝  : [IN] nLocalChannel ∑˛ŒÒÕ®µ¿∫≈ >=1
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  :
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_DisConnect(int nLocalChannel)
#else
JVCLIENT_API void __stdcall	JVC_DisConnect(int nLocalChannel)
#endif
{
    if(g_pCWorker != NULL)
    {
        g_pCWorker->DisConnect(nLocalChannel);
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_SendData
 *π¶ƒ‹  : ∑¢ÀÕ ˝æ›
 *≤Œ ˝  : [IN] nLocalChannel   ±æµÿÕ®µ¿∫≈ >=1
 [IN] uchType           ˝æ›¿‡–Õ£∫∏˜÷÷«Î«Û£ª∏˜÷÷øÿ÷∆£ª∏˜÷÷”¶¥
 [IN] pBuffer         ¥˝∑¢ ˝æ›ƒ⁄»›, ”∆µ/“Ù∆µ ±”––ß
 [IN] nSize           ¥˝∑¢ ˝æ›≥§∂», ”∆µ/“Ù∆µ ±”––ß
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : “‘Õ®µ¿Œ™µ•Œª£¨œÚÕ®µ¿¡¨Ω”µƒÀ˘”–∑÷øÿ∑¢ÀÕ ˝æ›
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API BOOL JVC_SendData(int nLocalChannel, unsigned char uchType, BYTE *pBuffer,int nSize)
#else
JVCLIENT_API void __stdcall	JVC_SendData(int nLocalChannel, unsigned char uchType, BYTE *pBuffer,int nSize)
#endif
{
    BOOL result = FALSE;
    if(g_pCWorker != NULL)
    {
        result = g_pCWorker->SendData(nLocalChannel, uchType, pBuffer, nSize);
        printf("SENDDATA: result: %d, uchType: %x\n",result,uchType);
    }
    return result;
}

/****************************************************************************
 *√˚≥∆  : JVN_EnableLog
 *π¶ƒ‹  : …Ë÷√–¥≥ˆ¥Ì»’÷æ «∑Ò”––ß
 *≤Œ ˝  : [IN] bEnable  TRUE:≥ˆ¥Ì ±–¥»’÷æ£ªFALSE:≤ª–¥»Œ∫Œ»’÷æ
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
bool isWriteLog;
#ifndef WIN32
JVCLIENT_API void JVC_EnableLog(bool bEnable)
#else
JVCLIENT_API void __stdcall	JVC_EnableLog(bool bEnable)
#endif
{
    if(g_pCWorker != NULL)
    {
        //g_pCWorker->EnableLog(bEnable);
        isWriteLog = bEnable;
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_SetLanguage
 *π¶ƒ‹  : …Ë÷√»’÷æ/Ã· æ–≈œ¢”Ô—‘(”¢Œƒ/÷–Œƒ)
 *≤Œ ˝  : [IN] nLgType  JVN_LANGUAGE_ENGLISH/JVN_LANGUAGE_CHINESE
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_SetLanguage(int nLgType)
#else
JVCLIENT_API void __stdcall	JVC_SetLanguage(int nLgType)
#endif
{
    if(g_pCWorker != NULL)
    {
        g_pCWorker->SetLanguage(nLgType);
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_TCPConnect
 *π¶ƒ‹  : TCP∑Ω Ω¡¨Ω”ƒ≥Õ®µ¿Õ¯¬Á∑˛ŒÒ
 *≤Œ ˝  : [IN] nLocalChannel ±æµÿÕ®µ¿∫≈ >=1
 [IN] nChannel      ∑˛ŒÒÕ®µ¿∫≈ >=1
 [IN] pchServerIP   µ±nYSTNO<0 ±£¨∏√≤Œ ˝÷∏Õ®µ¿∑˛ŒÒIP£ªµ±nYSTNO>=0 ±Œﬁ–ß.
 [IN] nServerPort   µ±nYSTNO<0 ±£¨∏√≤Œ ˝÷∏Õ®µ¿∑˛ŒÒport£ªµ±nYSTNO>=0 ±Œﬁ–ß.
 [IN] pchPassName   ”√ªß√˚
 [IN] pchPassWord   √‹¬Î
 [IN] nYSTNO        ‘∆ ”Õ®∫≈¬Î£¨≤ª π”√ ±÷√-1
 [IN] chGroup       ±‡◊È∫≈£¨–Œ»Á"A" "AAAA"  π”√‘∆ ”Õ®∫≈¬Î ±”––ß
 [IN] bLocalTry      «∑ÒΩ¯––ƒ⁄Õ¯ÃΩ≤‚
 [IN] nConnectType  ¡¨Ω”∑Ω Ω£∫TYPE_PC_TCP/TYPE_MO_TCP
 [IN] nTURNType     ◊™∑¢π¶ƒ‹¿‡–Õ(Ω˚”√◊™∑¢\∆Ù”√◊™∑¢\Ωˆ”√◊™∑¢)
 *∑µªÿ÷µ: TRUE  ≥…π¶
 FALSE  ß∞‹
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_TCPConnect(int nLocalChannel,int nChannel,
                                 char *pchServerIP,int nServerPort,
                                 char *pchPassName,char *pchPassWord,
                                 int nYSTNO,char chGroup[4],
                                 BOOL bLocalTry,
                                 int nConnectType,
                                 int nTURNType)
#else
JVCLIENT_API void __stdcall	JVC_TCPConnect(int nLocalChannel,int nChannel,
                                           char *pchServerIP,int nServerPort,
                                           char *pchPassName,char *pchPassWord,
                                           int nYSTNO,char chGroup[4],
                                           BOOL bLocalTry,
                                           int nConnectType,
                                           int nTURNType)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(nLocalChannel <= 0 || nChannel <= 0 || nLocalChannel > 65535 || nChannel > 65535)
        {//≤Œ ˝Œﬁ–ß
			char chMsg[] = "通道号参数无效,有效范围[1,65535]";
            g_pCWorker->ConnectChange(nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
        }
        
        if(nYSTNO < 0)
        {//æ÷”ÚÕ¯
            if(pchServerIP == NULL || strlen(pchServerIP) < 7 || strlen(pchServerIP) > 18)
            {//≤Œ ˝Œﬁ–ß
				char chMsg[] = "参数无效,进行局域网连接,但未传入合法的IP地址!";
                g_pCWorker->ConnectChange(nLocalChannel,JVN_CCONNECTTYPE_CONNERR,chMsg,0,__FILE__,__LINE__,__FUNCTION__);
            }
            
            g_pCWorker->ConnectServerDirect(nLocalChannel, nChannel, pchServerIP, nServerPort,pchPassName, pchPassWord,FALSE,nConnectType);
            return;
        }
        else
        {//YST
            g_pCWorker->ConnectServerByYST(nLocalChannel, nChannel, nYSTNO, chGroup, pchPassName, pchPassWord, bLocalTry,nTURNType,FALSE,nConnectType);
            return;
        }
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_GetPartnerInfo
 *π¶ƒ‹  : ªÒ»°ªÔ∞ÈΩ⁄µ„–≈œ¢
 *≤Œ ˝  : [IN] nLocalChannel   ±æµÿÕ®µ¿∫≈ >=1
 [OUT] pMsg   –≈œ¢ƒ⁄»›
 (ªÔ∞È◊‹∏ˆ ˝(4)+[IP(16) + port(4)+¡¨Ω”◊¥Ã¨(1)+À≤º‰œ¬‘ÿÀŸ∂»(4)+À≤º‰…œ¥´ÀŸ∂»(4)+œ¬‘ÿ◊‹¡ø(4)+…œ¥´◊‹¡ø(4)]
 +[...]...)
 [OUT] nSize  –≈œ¢◊‹≥§∂»
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : µ˜”√∆µ¬ —œΩ˚Ã´∏ﬂ£¨∑Ò‘Úª·”∞œÏ ”∆µ¥¶¿ÌÀŸ∂»£ª
 ∆µ∑±≥Ã∂»∂»≤ªƒ‹µÕ”⁄1√Î£¨◊Ó∫√‘⁄2√Î“‘…œªÚ∏¸≥§ ±º‰£¨ ±º‰‘Ω≥§”∞œÏ‘Ω–°°£
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_GetPartnerInfo(int nLocalChannel, char *pMsg, int &nSize)
#else
JVCLIENT_API void __stdcall	JVC_GetPartnerInfo(int nLocalChannel, char *pMsg, int &nSize)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(nLocalChannel <= 0 || nLocalChannel > 65535 || pMsg == NULL)
        {//≤Œ ˝Œﬁ–ß
            return;
        }
        
        g_pCWorker->GetPartnerInfo(nLocalChannel, pMsg,nSize);
    }
    return;
}

#ifndef WIN32
JVCLIENT_API void JVC_RegisterRateCallBack(FUNC_CBUFRATE_CALLBACK BufRate)
#else
JVCLIENT_API void __stdcall	JVC_RegisterRateCallBack(FUNC_CBUFRATE_CALLBACK BufRate)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(BufRate != NULL)
        {
            g_pCWorker->m_pfBufRate = BufRate;
        }
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_StartLANSerchServer
 *π¶ƒ‹  : ø™∆Ù∑˛ŒÒø…“‘À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
 *≤Œ ˝  : [IN] nLPort      ±æµÿ∑˛ŒÒ∂Àø⁄£¨<0 ±Œ™ƒ¨»œ9400
 [IN] nServerPort …Ë±∏∂À∑˛ŒÒ∂Àø⁄£¨<=0 ±Œ™ƒ¨»œ9103,Ω®“ÈÕ≥“ª”√ƒ¨»œ÷µ”Î∑˛ŒÒ∂À∆•≈‰
 [IN] LANSData    À—À˜Ω·π˚ªÿµ˜∫Ø ˝
 *∑µªÿ÷µ: TRUE/FALSE
 *∆‰À˚  : ø™∆Ù¡ÀÀ—À˜∑˛ŒÒ≤≈ø…“‘Ω” ’À—À˜Ω·π˚£¨À—À˜Ãıº˛Õ®π˝JVC_LANSerchDeviceΩ”ø⁄÷∏∂®
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_StartLANSerchServer(int nLPort, int nServerPort, FUNC_CLANSDATA_CALLBACK LANSData)
#else
JVCLIENT_API bool __stdcall	JVC_StartLANSerchServer(int nLPort, int nServerPort, FUNC_CLANSDATA_CALLBACK LANSData)
#endif
{//return true;
	if(g_pCWorker != NULL)
	{
		if(LANSData != NULL)
		{
			g_pCWorker->m_pfLANSData = LANSData;
		}
		if(g_pCWorker->StartLANSerchServer(nLPort, nServerPort))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_StopLANSerchServer
 *π¶ƒ‹  : Õ£÷πÀ—À˜∑˛ŒÒ
 *≤Œ ˝  : Œﬁ
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  :
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_StopLANSerchServer()
#else
JVCLIENT_API void __stdcall	JVC_StopLANSerchServer()
#endif
{//return;
    if(g_pCWorker != NULL)
    {
        g_pCWorker->StopLANSerchServer();
    }
}

#ifndef WIN32
JVCLIENT_API int JVC_MOStopLANSerchServer()
#else
JVCLIENT_API int __stdcall	JVC_MOStopLANSerchServer()
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->StopSearchThread();
    }
    return 0;
}

/****************************************************************************
 *√˚≥∆  : JVC_LANSerchDevice
 *π¶ƒ‹  : À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
 *≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
 [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
 [IN] nCardType   À—À˜ƒ≥–Õ∫≈µƒ…Ë±∏£¨>0”––ß,µ±nYSTNO>0 ±∏√≤Œ ˝Œﬁ–ß
 [IN] chDeviceNameÀ—À˜ƒ≥∏ˆ±√˚µƒ…Ë±∏£¨strlen>0”––ß£¨µ±nYSTNO>0 ±Œﬁ–ß
 [IN] nVariety    À—À˜ƒ≥∏ˆ¿‡±µƒ…Ë±∏£¨1∞Âø®;2DVR;3IPC;>0”––ß,µ±nYSTNO>0 ±∏√≤Œ ˝Œﬁ–ß
 [IN] nTimeOut    ±æµÿÀ—À˜”––ß ±º‰£¨µ•Œª∫¡√Î°£≥¨π˝∏√ ±º‰µƒΩ·π˚Ω´±ª…·∆˙£¨
 ≥¨ ± ±º‰µΩ¥Ô∫Ûªÿµ˜∫Ø ˝÷–Ω´µ√µΩ≥¨ ±Ã· æ◊˜Œ™À—À˜Ω· ¯±Í÷æ°£
 »Áπ˚≤ªœÎ π”√SDK≥¨ ±¥¶¿Ìø…“‘÷√Œ™0£¨¥À ±Ω·π˚«ø»´≤ø∑µªÿ∏¯µ˜”√’ﬂ°£
 [IN] unFrequence (µ•ŒªŒ™s )÷¥––ping Õ¯πÿµƒ∆µ¬ £¨ƒ¨»œ«ÎÃÓ»Î30sÀ—À˜“ª¥Œ£¨«∂»Î Ω…Ë±∏ø…∏˘æ›–Ë“™∏¸∏ƒ>0&&<24*3600”––ß.
 
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : µ±¡Ω≤Œ ˝Õ¨ ±Œ™0 ±£¨Ω´À—À˜æ÷”ÚÕ¯÷–À˘”–÷–Œ¨…Ë±∏
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence)
#else
JVCLIENT_API bool __stdcall	JVC_LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence )
#endif
{
    if(g_pCWorker != NULL)
    {
        BOOL b=g_pCWorker->LANSerchDevice(chGroup, nYSTNO, nCardType, nVariety, chDeviceName, nTimeOut,FALSE,unFrequence);
        if(b)
        {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_SetLocalFilePath
 *π¶ƒ‹  : ◊‘∂®“Â±æµÿŒƒº˛¥Ê¥¢¬∑æ∂£¨∞¸¿®»’÷æ£¨…˙≥…µƒ∆‰À˚πÿº¸Œƒº˛µ»
 *≤Œ ˝  : [IN] chLocalPath  ¬∑æ∂ –Œ»Á£∫C:\jovision  ∆‰÷–jovision «Œƒº˛º–
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : ≤Œ ˝ π”√ƒ⁄¥ÊøΩ±¥ ±«Î◊¢“‚≥ı ºªØ£¨◊÷∑˚¥Æ–Ë“‘'\0'Ω· ¯
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_SetLocalFilePath(char chLocalPath[256])
#else
JVCLIENT_API bool __stdcall	JVC_SetLocalFilePath(char chLocalPath[256])
#endif
{
    if(g_pCWorker != NULL)
    {
        if(g_pCWorker->SetLocalFilePath(chLocalPath))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_SetDomainName
 *π¶ƒ‹  : …Ë÷√–¬µƒ”Ú√˚£¨œµÕ≥Ω´¥”∆‰ªÒ»°∑˛ŒÒ∆˜¡–±Ì
 *≤Œ ˝  : [IN]  pchDomainName     ”Ú√˚
 [IN]  pchPathName       ”Ú√˚œ¬µƒŒƒº˛¬∑æ∂√˚ –Œ»Á£∫"/down/YSTOEM/yst0.txt"
 *∑µªÿ÷µ: TRUE  ≥…π¶
 FALSE  ß∞‹
 *∆‰À˚  : œµÕ≥≥ı ºªØ(JVC_InitSDK)ÕÍ∫Û…Ë÷√
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_SetDomainName(char *pchDomainName,char *pchPathName)
#else
JVCLIENT_API bool __stdcall	JVC_SetDomainName(char *pchDomainName,char *pchPathName)
#endif
{
    if(strlen(pchDomainName) <= 0 || strlen(pchPathName) <= 0 || strlen(pchDomainName) > MAX_PATH || strlen(pchPathName) > MAX_PATH)
    {
        return FALSE;
    }
    
    if(g_pCWorker!=NULL)
    {
        return g_pCWorker->SetWebName(pchDomainName,pchPathName);
    }
    return false;
}

/****************************************************************************
 *√˚≥∆  : JVC_WANGetChannelCount
 *π¶ƒ‹  : Õ®π˝Õ‚Õ¯ªÒ»°ƒ≥∏ˆ‘∆ ”Õ®∫≈¬ÎÀ˘æﬂ”–µƒÕ®µ¿◊‹ ˝
 *≤Œ ˝  : [IN]  chGroup   ±‡◊È∫≈
 [IN]  nYstNO    ‘∆ ”Õ®∫≈¬Î
 [IN]  nTimeOutS µ»¥˝≥¨ ± ±º‰(√Î)
 *∑µªÿ÷µ: >0  ≥…π¶,Õ®µ¿ ˝
 0   ß∞‹£¨≤Œ ˝”–ŒÛ
 -1  ß∞‹£¨∫≈¬ÎŒ¥…œœﬂ
 -2  ß∞‹£¨÷˜øÿ∞Ê±æΩœæ…£¨≤ª÷ß≥÷∏√≤È—Ø
 -3  ß∞‹£¨∆‰À˚‘≠“Ú
 *∆‰À˚  : œµÕ≥≥ı ºªØ(JVN_InitSDK)ÕÍ∫Û ø…∂¿¡¢µ˜”√
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS)
#else
JVCLIENT_API int __stdcall	JVC_WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS)
#endif
{//return -3;
    if(strlen(chGroup) <= 0 || nYSTNO <= 0 || nTimeOutS <= 0)
    {
        return 0;
    }
    
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->WANGetChannelCount(chGroup, nYSTNO, nTimeOutS);
    }
    
    return -3;
}

	/**
	 * IN: pChannelNum ���瑕���ヨ��涓�涓插�风��   姣�濡�锛�CHANNEL_NUM nnn[2] = {{"A",361,0},{"B",362,0}}
	 */
JVCLIENT_API int JVC_WANGetBatchChannelCount(char *pChannelNum, int nYSTNOCnt, int nTimeOutS){
	if(strlen(pChannelNum) <= 0 || nYSTNOCnt <= 0 || nTimeOutS <= 0)
		{
			return 0;
		}

		if(g_pCWorker != NULL)
		{
			return g_pCWorker->WANGetBatchChannelCount(pChannelNum, nYSTNOCnt, nTimeOutS);
		}

		return -3;
}
/****************************************************************************
 *√˚≥∆  : JVC_StartBroadcastServer
 *π¶ƒ‹  : ø™∆Ù◊‘∂®“Âπ„≤•∑˛ŒÒ
 *≤Œ ˝  : [IN] nLPort      ±æµÿ∑˛ŒÒ∂Àø⁄£¨<0 ±Œ™ƒ¨»œ9500
 [IN] nServerPort …Ë±∏∂À∑˛ŒÒ∂Àø⁄£¨<=0 ±Œ™ƒ¨»œ9106,Ω®“ÈÕ≥“ª”√ƒ¨»œ÷µ”Î∑˛ŒÒ∂À∆•≈‰
 [IN] BroadcastData  π„≤•Ω·π˚ªÿµ˜∫Ø ˝
 *∑µªÿ÷µ: TRUE/FALSE
 *∆‰À˚  : ø™∆Ù¡Àπ„≤•∑˛ŒÒ≤≈ø…“‘Ω” ’π„≤•Ω·π˚£¨π„≤•ƒ⁄»›Õ®π˝JVC_BroadcastOnceΩ”ø⁄÷∏∂®£ª
 ∂Àø⁄…Ë÷√«Î“ª∂®◊¢“‚∫Õ…Ë±∏À—À˜œ‡πÿ∂Àø⁄◊˜«¯±£¨∑Ò‘Ú ˝æ›Ω´“Ï≥££ª
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_StartBroadcastServer(int nLPort, int nServerPort, FUNC_CBCDATA_CALLBACK BCData)
#else
JVCLIENT_API bool __stdcall	JVC_StartBroadcastServer(int nLPort, int nServerPort, FUNC_CBCDATA_CALLBACK BCData)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(BCData != NULL)
        {
            g_pCWorker->m_pfBCData = BCData;
        }
        if(g_pCWorker->StartBCServer(nLPort, nServerPort))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_StopBroadcastServer
 *π¶ƒ‹  : Õ£÷π◊‘∂®“Âπ„≤•∑˛ŒÒ
 *≤Œ ˝  : Œﬁ
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_StopBroadcastServer()
#else
JVCLIENT_API void __stdcall	JVC_StopBroadcastServer()
#endif
{
    if(g_pCWorker != NULL)
    {
        g_pCWorker->StopBCServer();
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_BroadcastOnce
 *π¶ƒ‹  : ∑¢ÀÕ“ª¥Œπ„≤•œ˚œ¢
 *≤Œ ˝  : [IN] nBCID       π„≤•ID,”…µ˜”√’ﬂ∂®“Â,”√”⁄‘⁄ªÿµ˜∫Ø ˝÷–∆•≈‰«¯∑÷±æ¥Œπ„≤•
 [IN] pBuffer     π„≤•æª‘ÿ ˝æ›
 [IN] nSize       π„≤•æª‘ÿ ˝æ›≥§∂»
 [IN] nTimeOut    ±æ¥Œπ„≤•”––ß ±º‰£¨µ•Œª∫¡√Î°£≥¨π˝∏√ ±º‰µƒΩ·π˚Ω´±ª…·∆˙£¨
 ≥¨ ± ±º‰µΩ¥Ô∫Ûªÿµ˜∫Ø ˝÷–Ω´µ√µΩ≥¨ ±Ã· æ◊˜Œ™À—À˜Ω· ¯±Í÷æ°£
 »Áπ˚≤ªœÎ π”√SDK≥¨ ±¥¶¿Ìø…“‘÷√Œ™0£¨¥À ±Ω·π˚«ø»´≤ø∑µªÿ∏¯µ˜”√’ﬂ°£
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : µ±¡Ω≤Œ ˝Õ¨ ±Œ™0 ±£¨Ω´À—À˜æ÷”ÚÕ¯÷–À˘”–÷–Œ¨…Ë±∏
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API BOOL JVC_BroadcastOnce(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut )
#else
JVCLIENT_API BOOL __stdcall JVC_BroadcastOnce(int nBCID, BYTE *pBuffer, int nSize, int nTimeOut )
#endif
{
    if(g_pCWorker != NULL)
    {
        BOOL b=g_pCWorker->DoBroadcast(nBCID, pBuffer, nSize, nTimeOut);
        if(b)
        {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}


/****************************************************************************
 *√˚≥∆  : JVC_ClearBuffer
 *π¶ƒ‹  : «Âø’±æµÿª∫¥Ê
 *≤Œ ˝  : [IN] nLocalChannel ∑˛ŒÒÕ®µ¿∫≈ >=1
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : Ωˆ∂‘∆’Õ®ƒ£ Ω¡¥Ω””––ß£¨∂‡≤•¡¨Ω”≤ª‘ –Ì¥”±æµÿ«Âø’
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_ClearBuffer(int nLocalChannel)
#else
JVCLIENT_API void __stdcall	JVC_ClearBuffer(int nLocalChannel)
#endif
{
    if(g_pCWorker != NULL)
    {
        g_pCWorker->ClearBuffer(nLocalChannel);
    }
}

/****************************************************************************
 *√˚≥∆  : JVC_EnableHelp
 *π¶ƒ‹  : ∆Ù”√/Õ£”√øÏÀŸ¡¥Ω”∑˛ŒÒ
 *≤Œ ˝  : [IN] bEnable TRUEø™∆Ù/FALSEπÿ±’
 [IN] nType  1 µ±«∞ π”√’ﬂ «‘∆ ”Õ®–°÷˙ ÷(∂¿¡¢Ω¯≥Ã)
 2 µ±«∞ π”√’ﬂ «‘∆ ”Õ®øÕªß∂À£¨÷ß≥÷∂¿¡¢Ω¯≥Ãµƒ–°÷˙ ÷
 3 µ±«∞ π”√’ﬂ «‘∆ ”Õ®øÕªß∂À£¨≤ª÷ß≥÷∂¿¡¢Ω¯≥Ãµƒ–°÷˙ ÷
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : ∆Ù”√∏√π¶ƒ‹∫Û£¨Õ¯¬ÁSDKª·∂‘…Ë∂®µƒ∫≈¬ÎΩ¯––¡¨Ω”Ã·ÀŸµ»”≈ªØ£ª
 ∆Ù”√∏√π¶ƒ‹∫Û£¨Õ¯¬ÁSDKª·÷ß≥÷–°÷˙ ÷∫ÕøÕªß∂À÷Æº‰Ω¯––Ωªª•£ª
 »Áπ˚∑÷øÿ∂À÷ß≥÷–°÷˙ ÷Ω¯≥Ã£¨‘Ú”√–°÷˙ ÷∂À π”√nType=1£¨øÕªß∂À π”√nType=2º¥ø…£ª
 »Áπ˚øÕªß∂À≤ª÷ß≥÷–°÷˙ ÷Ω¯≥Ã£¨‘ÚøÕªß∂À π”√nType=3º¥ø…£¨±»»Á ÷ª˙øÕªß∂À£ª
 *****************************************************************************/
#ifndef WIN32
	JVCLIENT_API BOOL JVC_EnableHelp(BOOL bEnable, int nType,int nMaxLimit)
#else
JVCLIENT_API BOOL __stdcall JVC_EnableHelp(BOOL bEnable, int nType)
#endif
{//return TRUE;
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->EnableHelp(bEnable, nType);
    }
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_SetHelpYSTNO
 *π¶ƒ‹  : …Ë÷√∂‘ƒ≥–©‘∆ ”Õ®∫≈¬Îµƒ∏®÷˙÷ß≥÷
 *≤Œ ˝  : [IN] pBuffer ‘∆ ”Õ®∫≈¬ÎºØ∫œ£¨”…STBASEYSTNOΩ·ππ◊È≥…£ª±»»Á”–¡Ω∏ˆ∫≈¬Î
 STBASEYSTNO st1,STBASEYSTNO st1;
 pBufferµƒƒ⁄»›æÕ «:
 memcpy(bBuffer, &st1, sizeof(STBASEYSTNO));
 memcpy(&bBuffer[sizeof(STBASEYSTNO)], &st2, sizeof(STBASEYSTNO));
 [IN] nSize   pBufferµƒ µº ”––ß≥§∂»£ª»Áπ˚ «¡Ω∏ˆ∫≈¬Î‘ÚŒ™£∫
 2*sizeof(STBASEYSTNO);
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : ‘∆ ”Õ®–°÷˙ ÷∂À π”√£ª
 øÕªß∂À≤ª÷ß≥÷–°÷˙ ÷ ±øÕªß∂À π”√£ª
 
 ÃÌº”∫Û£¨Õ¯¬ÁSDKª·∂‘’‚–©‘∆ ”Õ®∫≈¬ÎΩ¯––¡¨Ω”Ã·ÀŸµ»”≈ªØ£ª
 ’‚ «≥ı º…Ë÷√£¨≥Ã–Ú‘À––÷–øÕªß∂À“≤ª·”––©–¬µƒ∫≈¬Î£¨
 ª·∂ØÃ¨ÃÌº”µΩƒ⁄≤ø≤¢Ã· æ∏¯–°÷˙ ÷∂À£ª
 STBASEYSTNOS ‘∆ ”Õ®∫≈¬Î,STYSTNO∂®“Â≤Œø¥JVNSDKDef.h
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API BOOL JVC_SetHelpYSTNO(BYTE *pBuffer, int nSize)
#else
JVCLIENT_API BOOL __stdcall JVC_SetHelpYSTNO(BYTE *pBuffer, int nSize)
#endif
{//return TRUE;
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->SetHelpYSTNO(pBuffer, nSize);
    }
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_GetHelpYSTNO
 *π¶ƒ‹  : ªÒ»°µ±«∞“—÷™µƒ‘∆ ”Õ®∫≈¬Î«Âµ•
 *≤Œ ˝  : [IN/OUT] pBuffer ”…µ˜”√’ﬂø™±Ÿƒ⁄¥Ê£ª
 ∑µªÿ‘∆ ”Õ®∫≈¬ÎºØ∫œ£¨”…STBASEYSTNOΩ·ππ◊È≥…£ª±»»Á”–¡Ω∏ˆ∫≈¬Î
 STBASEYSTNO st1,STBASEYSTNO st1;
 pBufferµƒƒ⁄»›æÕ «:
 memcpy(bBuffer, &st1, sizeof(STBASEYSTNO));
 memcpy(&bBuffer[sizeof(STBASEYSTNO)], &st2, sizeof(STBASEYSTNO));
 [IN/OUT] nSize   µ˜”√ ±¥´»Îµƒ «pBufferµƒ µº ø™±Ÿ≥§∂»£¨
 µ˜”√∫Û∑µªÿµƒ «pBufferµƒ µº ”––ß≥§∂»£ª»Áπ˚ «¡Ω∏ˆ∫≈¬Î‘ÚŒ™£∫
 2*sizeof(STBASEYSTNO);
 *∑µªÿ÷µ: -1  ¥ÌŒÛ£¨≤Œ ˝”–ŒÛ£¨pBufferŒ™ø’ªÚ «¥Û–°≤ª◊„“‘¥Ê¥¢Ω·π˚£ª
 0  ∑˛ŒÒŒ¥ø™∆Ù
 1  ≥…π¶
 *∆‰À˚  : ‘∆ ”Õ®–°÷˙ ÷∂À π”√£ª
 øÕªß∂À≤ª÷ß≥÷–°÷˙ ÷ ±øÕªß∂À π”√£ª
 
 ’‚ «≥Ã–Ú‘À––÷–“—÷™µƒÀ˘”–∫≈¬Î£¨º¥–°÷˙ ÷ª·∂‘’‚–©∫≈¬ÎΩ¯––¡¨Ω””≈ªØ÷ß≥÷£ª
 ∏√Ω”ø⁄Ωˆ”√”⁄≤È—Ø£¨”…”⁄ƒ⁄≤øª·◊‘∂ØÃÌº”£¨≤È—ØΩ·π˚≤ªª·≥§∆⁄”––ß£ª
 STBASEYSTNOS ‘∆ ”Õ®∫≈¬Î,STYSTNO∂®“Â≤Œø¥JVNSDKDef.h
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_GetHelpYSTNO(BYTE *pBuffer, int &nSize)
#else
JVCLIENT_API int __stdcall JVC_GetHelpYSTNO(BYTE *pBuffer, int &nSize)
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->GetHelpYSTNO(pBuffer, nSize);
    }
    return 0;
}

/****************************************************************************
 *√˚≥∆  : JVC_GetYSTStatus
 *π¶ƒ‹  : ªÒ»°ƒ≥∏ˆ‘∆ ”Õ®∫≈¬Îµƒ‘⁄œﬂ◊¥Ã¨
 *≤Œ ˝  : [IN] chGroup  ‘∆ ”Õ®∫≈¬Îµƒ±‡◊È∫≈£ª
 [IN] nYSTNO   ‘∆ ”Õ®∫≈¬Î
 [IN] nTimeOut ≥¨ ± ±º‰(√Î)£¨Ω®“È>=2√Î
 *∑µªÿ÷µ: -1  ¥ÌŒÛ£¨≤Œ ˝”–ŒÛ£¨chGroupŒ™ø’ªÚ «nYSTNO<=0£ª
 0  ±æµÿ≤È—ØÃ´∆µ∑±£¨…‘∫Û÷ÿ ‘
 1  ∫≈¬Î‘⁄œﬂ
 2  ∫≈¬Î≤ª‘⁄œﬂ
 3  ≤È—Ø ß∞‹£¨ªπ≤ªƒ‹≈–∂®∫≈¬Î «∑Ò‘⁄œﬂ
 *∆‰À˚  : 1.◊¢“‚£¨∏√∫Ø ˝ƒø«∞Ωˆœﬁ”√”⁄ ÷ª˙,pc∂À‘›≤ª‘ –Ì π”√£ª
 2.∏√∫Ø ˝∂‘Õ¨“ª∏ˆ∫≈¬Î≤ª‘ –Ì∆µ∑±µ˜”√£¨º‰∏Ù>=10s;
 3.∏√∫Ø ˝∂‘≤ªÕ¨∫≈¬Î≤ª‘ –Ì∆µ∑±µ˜”√£¨º‰∏Ù>=1s;
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut)
#else
JVCLIENT_API int __stdcall JVC_GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut)
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->GetYSTStatus(chGroup, nYSTNO, nTimeOut);
    }
    return 0;
}

/****************************************************************************
 *√˚≥∆  : JVC_EnableLANTool
 *π¶ƒ‹  : ø™∆Ù∑˛ŒÒø…“‘À—À˜≈‰÷√æ÷”ÚÕ¯÷–µƒ…Ë±∏
 *≤Œ ˝  : [IN] nEnable     1ø™∆Ù 0πÿ±’
 [IN] nLPort      ±æµÿ∑˛ŒÒ∂Àø⁄£¨<0 ±Œ™ƒ¨»œ9600
 [IN] nServerPort …Ë±∏∂À∑˛ŒÒ∂Àø⁄£¨<=0 ±Œ™ƒ¨»œ9104,Ω®“ÈÕ≥“ª”√ƒ¨»œ÷µ”Î∑˛ŒÒ∂À∆•≈‰
 [IN] LANTData    À—À˜Ω·π˚ªÿµ˜∫Ø ˝
 *∑µªÿ÷µ: 0 ß∞‹/1≥…π¶
 *∆‰À˚  : ø™∆Ù¡ÀÀ—À˜∑˛ŒÒ≤≈ø…“‘Ω” ’À—À˜Ω·π˚£¨À—À˜Ãıº˛Õ®π˝JVC_LANToolDeviceΩ”ø⁄÷∏∂®
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_EnableLANTool(int nEnable, int nLPort, int nServerPort, FUNC_CLANTDATA_CALLBACK LANTData)
#else
JVCLIENT_API int __stdcall	JVC_EnableLANTool(int nEnable, int nLPort, int nServerPort, FUNC_CLANTDATA_CALLBACK LANTData)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(nEnable == 1)
        {
            if(LANTData != NULL)
            {
                g_pCWorker->m_pfLANTData = LANTData;
            }
            else
            {
                return 0;
            }
        }
        return g_pCWorker->EnableLANTool(nEnable, nLPort, nServerPort);
    }
    
    return 0;
}

/****************************************************************************
 *√˚≥∆  : JVC_LANToolDevice
 *π¶ƒ‹  : À—À˜æ÷”ÚÕ¯÷–µƒø…≈‰÷√…Ë±∏
 *≤Œ ˝  : [IN] chPName     ”√ªß√˚
 [IN] chPWord     √‹¬Î£¨ π”√”√ªß√˚√‹¬Îø…Ã·∏ﬂIPCµƒ∞≤»´–‘£¨º¥≈‰÷√“≤ «–Ë“™»®œﬁµƒ
 [IN] nTimeOut    ±æµÿÀ—À˜”––ß ±º‰£¨µ•Œª∫¡√Î°£≥¨π˝∏√ ±º‰µƒΩ·π˚Ω´±ª…·∆˙£¨
 ≥¨ ± ±º‰µΩ¥Ô∫Ûªÿµ˜∫Ø ˝÷–Ω´µ√µΩ≥¨ ±Ã· æ◊˜Œ™À—À˜Ω· ¯±Í÷æ°£
 »Áπ˚≤ªœÎ π”√SDK≥¨ ±¥¶¿Ìø…“‘÷√Œ™0£¨¥À ±Ω·π˚«ø»´≤ø∑µªÿ∏¯µ˜”√’ﬂ°£
 
 Õ¨Õ¯∂Œµƒ…Ë±∏“ª∞„ƒ‹‘⁄<500µƒ ±º‰ƒ⁄À—À˜µΩ£ª
 …Ë±∏À—À˜Ω®“È÷¡…Ÿ…Ë÷√2000£¨»∑±£À—À˜µƒÕ¯∂Œ»´√Ê£ª
 º¥≤Âº¥”√À—À˜ø…∏˘æ›–Ë“™≥§ªÚ∂Ã£¨‘Ω∂Ã‘Ωø…ƒ‹“≈¬©≤ªÕ¨Õ¯∂Œµƒ…Ë±∏£ª
 *∑µªÿ÷µ: 0 ß∞‹ 1≥…π¶
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut)
#else
JVCLIENT_API int __stdcall	JVC_LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut)
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->LANToolDevice(chPName, chPWord, nTimeOut);
    }
    return FALSE;
}

/****************************************************************************
 *√˚≥∆  : JVC_SendCMD
 *π¶ƒ‹  : œÚ÷˜øÿ∂À∑¢ÀÕ“ª–©Ãÿ ‚√¸¡Ó
 *≤Œ ˝  : [IN] nLocalChannel   ±æµÿÕ®µ¿∫≈ >=1
 [IN] uchType          ˝æ›¿‡–Õ
 [IN] pBuffer         ¥˝∑¢ ˝æ›ƒ⁄»›
 [IN] nSize           ¥˝∑¢ ˝æ›≥§∂»
 *∑µªÿ÷µ: 0  ∑¢ÀÕ ß∞‹
 1  ∑¢ÀÕ≥…π¶
 2  ∂‘∑Ω≤ª÷ß≥÷∏√√¸¡Ó
 *∆‰À˚  : Ωˆ∂‘∆’Õ®ƒ£ Ω¡¥Ω””––ß£ª
 µ±«∞÷ß≥÷µƒ”–÷ª∑¢πÿº¸÷°∫Õªÿ∏¥¬˙÷°√¸¡Ó
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_SendCMD(int nLocalChannel, unsigned char uchType, BYTE *pBuffer, int nSize)
#else
JVCLIENT_API int __stdcall	JVC_SendCMD(int nLocalChannel, BYTE uchType, BYTE *pBuffer, int nSize)
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->SendCMD(nLocalChannel, uchType, pBuffer, nSize);
    }
    return 0;
}

/****************************************************************************
 *√˚≥∆  : JVC_AddFSIpSection
 *π¶ƒ‹  : ‘ˆº”◊‘∂®“ÂIP∂Œ£¨“‘π©”≈œ»À—À˜
 *≤Œ ˝  : [IN] pStartIp		  IPSECTION ˝◊Èµÿ÷∑
 [IN] nSize           IP∂Œ ˝*sizeof(IPSECTION)
 *∑µªÿ÷µ: 0£¨≥…π¶ °£-1£¨ ß∞‹
 *∆‰À˚  : Œﬁ
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API int JVC_AddFSIpSection( const IPSECTION * pStartIp, int nSize,BOOL bEnablePing  )
#else
JVCLIENT_API int __stdcall	JVC_AddFSIpSection( const IPSECTION * pStartIp, int nSize,BOOL bEnablePing  )
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->AddFSIpSection(pStartIp, nSize,bEnablePing);
    }
    
    return 0;
}


/****************************************************************************
 *√˚≥∆  : JVC_MOLANSerchDevice
 *π¶ƒ‹  :  ÷ª˙À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
 *≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
 [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
 [IN] nCardType   À—À˜ƒ≥–Õ∫≈µƒ…Ë±∏£¨>0”––ß,µ±nYSTNO>0 ±∏√≤Œ ˝Œﬁ–ß
 [IN] chDeviceNameÀ—À˜ƒ≥∏ˆ±√˚µƒ…Ë±∏£¨strlen>0”––ß£¨µ±nYSTNO>0 ±Œﬁ–ß
 [IN] nVariety    À—À˜ƒ≥∏ˆ¿‡±µƒ…Ë±∏£¨1∞Âø®;2DVR;3IPC;>0”––ß,µ±nYSTNO>0 ±∏√≤Œ ˝Œﬁ–ß
 [IN] nTimeOut    ±æµÿÀ—À˜”––ß ±º‰£¨µ•Œª∫¡√Î°£≥¨π˝∏√ ±º‰µƒΩ·π˚Ω´±ª…·∆˙£¨
 ≥¨ ± ±º‰µΩ¥Ô∫Ûªÿµ˜∫Ø ˝÷–Ω´µ√µΩ≥¨ ±Ã· æ◊˜Œ™À—À˜Ω· ¯±Í÷æ°£
 »Áπ˚≤ªœÎ π”√SDK≥¨ ±¥¶¿Ìø…“‘÷√Œ™0£¨¥À ±Ω·π˚«ø»´≤ø∑µªÿ∏¯µ˜”√’ﬂ°£
 [IN] unFrequence ÷¥––ping Õ¯πÿµƒ∆µ¬ £¨ƒ¨»œ30sÀ—À˜“ª¥Œ£¨>0&&<24*3600”––ß.
 
 *∑µªÿ÷µ: Œﬁ
 *∆‰À˚  : µ±¡Ω≤Œ ˝Õ¨ ±Œ™0 ±£¨Ω´À—À˜æ÷”ÚÕ¯÷–À˘”–÷–Œ¨…Ë±∏
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_MOLANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence)
#else
JVCLIENT_API bool __stdcall	JVC_MOLANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence)
#endif
{//return FALSE;
	if(g_pCWorker != NULL)
	{
		BOOL b=g_pCWorker->LANSerchDevice(chGroup, nYSTNO, nCardType, nVariety, chDeviceName, nTimeOut,TRUE,unFrequence);//标记手机搜索.
		if(b)
		{
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
/**
 * stop lansearch
 */
#ifndef WIN32
	JVCLIENT_API int JVC_MOStopLANSerchDevice()
#else
	JVCLIENT_API int __stdcall	JVC_MOStopLANSerchDevice()
#endif
{
	if(g_pCWorker != NULL)
	{
		int b=g_pCWorker->StopSearchThread();
		if(b==1)
		{
			return 1;
		}
		return 0;
	}
	return 0;
}
/****************************************************************************
 *√˚≥∆  : JVC_RegisterCommonCallBack
 *π¶ƒ‹  : ‘∆ ”Õ®ø‚”Î”¶”√≤„ ˝æ›Ωªª• ªÿµ˜◊¢≤·
 *≤Œ ˝  : ªÿµ˜∫Ø ˝
 
 *∑µªÿ÷µ: Œﬁ
 *****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_RegisterCommonCallBack(FUNC_COMM_DATA_CALLBACK pfWriteReadDataCallBack)
#else
JVCLIENT_API void __stdcall JVC_RegisterCommonCallBack(FUNC_COMM_DATA_CALLBACK pfWriteReadDataCallBack)
#endif

{
    if(g_pCWorker != NULL)
    {
        if(pfWriteReadDataCallBack != NULL)
        {
            g_pCWorker->m_pfWriteReadData = pfWriteReadDataCallBack;
        }
    }
}

void UDT_Recv(SOCKET sSocket,int nLocalPort,sockaddr* addrRemote,char *pMsg,int nLen,BOOL bNewUDP)//ªÿµ˜∫Ø ˝ ªÿµ˜UDTø‚ƒ⁄≤øΩ” ’µΩµƒ∑«UDT ˝æ›∞¸
{
    if(!g_pCWorker)
        return;
    
    if(!bNewUDP)
    {
        //	SOCKADDR_IN srv = {0};
        //	memcpy(&srv,addrRemote,sizeof(SOCKADDR_IN));
        //	int nType = 0;
        //	memcpy(&nType,pMsg,4);
        //	OutputDebug("UDP Data nLocalPort = %d type = 0x%X   nLen = %d",nLocalPort,nType,nLen);
        g_pCWorker->AddUdtRecv(sSocket,nLocalPort,addrRemote,pMsg,nLen,bNewUDP);
    }
    else//–¬ÃÌº”µƒ¥øudp–≠“È
    {
        //”–Œ Ã‚ ‘› ±»•µÙ£¨øº¬«∫√∫Û‘Ÿº”…œ
        
        /*		int nType = 0;
         memcpy(&nType,&pMsg[8],4);
         if(nType == JVN_KEEP_ACTIVE)//–¬÷˙ ÷–ƒÃ¯
         {
         char strGroup[4] = {0};
         int nYst = 0;
         
         char data[1024] = {0};
         memcpy(strGroup,&pMsg[12],4);
         memcpy(&nYst,&pMsg[16],4);
         
         //∏¸–¬Ω” ’ ±º‰
         g_pCWorker->m_Helper.UpdateTime(strGroup,nYst,sSocket,(SOCKADDR_IN* )addrRemote);
         
         //			OutputDebug("recv keepactive %s%d",strGroup,nYst);
         return;
         }*/
    }
}

char* GetPrivateString(char* lpAppName,char* lpKeyName,char* lpValue,char* lpFileName)
{
    char *iniData = NULL;
    FILE* file;
    file = fopen(lpFileName,"r");
    if(file)
    {
        fseek(file,0,SEEK_END);
        long l = ftell(file);
        iniData = new char [l + 1];
        memset(iniData,0,l + 1);
        fseek(file,0,SEEK_SET);
        fread(iniData,sizeof(char),l,file);
        fclose(file);
    }
    
    int i = 0;
    int sectionStart = 0;
    int keyStart = 0;
    int valueStart = 0;
    BOOL isFirst = TRUE;
    
    while (i< strlen(iniData))
    {
        //--≤È—Ø◊Û±ﬂøÚ
        if ('[' == iniData[i])
        {
            i++;
            //--≤È—Ø”“±ﬂøÚ
            while ((']' != iniData[i]) && (i< strlen(iniData)))
            {
                if (('\n' != iniData[i]) && ('\r' != iniData[i]) && isFirst)
                {
                    sectionStart = i;
                    isFirst = FALSE;
                }
                i++;
            }
            
            isFirst = TRUE;
            
            if (strncmp(iniData + sectionStart, lpAppName, strlen(lpAppName)) == 0)
            {
                i++;
                //--≤È—Ø◊Û±ﬂøÚ
                while ((iniData[i] != '[') && (i< strlen(iniData)))
                {
                    
                    //--≤È—Ø∏≥÷µ∫≈
                    while (iniData[i] != '=' && (i< strlen(iniData)) )
                    {
                        if (('\n' != iniData[i]) && ('\r' != iniData[i]) && isFirst)
                        {
                            keyStart = i;
                            isFirst = FALSE;
                        }
                        i++;
                    }
                    
                    isFirst = TRUE;
                    
                    //--øΩ±¥À˘“™µ√µΩµƒ ˝æ›
                    if (strncmp(iniData + keyStart, lpKeyName, strlen(lpKeyName)) == 0)
                    {
                        i++;
                        valueStart = i;
                        while (('\n' != iniData[i]) && ('\r' != iniData[i]) && (i< strlen(iniData)))
                        {
                            i++;
                        }
                        memcpy(lpValue, iniData+valueStart, i-valueStart);
                        lpValue[i-valueStart] = '\0';
                        if(iniData)
                            delete []iniData;
                        
                        return lpValue;
                    }
                    
                    //--Ã¯π˝∏√πÿº¸◊÷µƒ÷µ
                    while (('\n' != iniData[i]) && ('\r' != iniData[i]))
                    {
                        i++;
                    }
                    i++;
                }
            }
        }
        i++;
    }
    if(iniData)
        delete []iniData;
    
    return lpValue;
}

/****************************************************************************
 *√˚≥∆  : JVC_GetDemo
 *π¶ƒ‹  : ªÒ»°—› æµ„
 *≤Œ ˝  : [OUT] pBuff		  ¥Ê∑≈≤È—Øµƒ∫≈¬Îµƒ¡–±Ì,ƒ⁄¥Ê”…”¶”√≤„¥¥Ω®∑÷≈‰ ±‡◊È : 4 BYTE ∫≈¬Î : 4 BYTE Õ®µ¿ ˝ : 1 BYTE
 [IN] nBuffSize       ¥¥Ω®µƒƒ⁄¥Ê¥Û–°
 
 *∑µªÿ÷µ: ’˝ ˝ —› æµ„µƒ ˝¡ø£¨-2 œµÕ≥Œ¥≥ı ºªØ -1 Œ™ƒ⁄¥ÊÃ´–°£¨0 ø…ƒ‹ «Õ¯¬Áø‚ªπ√ª”–ªÒ»°µΩ
 *****************************************************************************/

#ifndef WIN32
JVCLIENT_API int JVC_GetDemo(BYTE* pBuff,int nBuffSize)
#else
JVCLIENT_API int __stdcall JVC_GetDemo(BYTE* pBuff,int nBuffSize)
#endif
{
    if(!g_pCWorker)
        return -2;
    
    return g_pCWorker->GetDemo(pBuff,nBuffSize);
}

/****************************************************************************
 *√˚≥∆  : JVC_HelperRemove
 *π¶ƒ‹  : …æ≥˝÷˙ ÷ƒ⁄µƒ∫≈¬Î 
 *≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
 [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
 
 *∑µªÿ÷µ: Œﬁ
 *****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_HelperRemove(char* pGroup,int nYST)
#else
JVCLIENT_API void __stdcall JVC_HelperRemove(char* pGroup,int nYST)
#endif
{
    if(!g_pCWorker)
        return ;
    
    g_pCWorker->HelpRemove(pGroup,nYST);
}


/****************************************************************************
 *√˚≥∆  : JVC_HelpQuery
 *π¶ƒ‹  : ≤È—Ø÷˙ ÷µƒ∫≈¬Î¡¨Ω”◊¥Ã¨ 
 *≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
 [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
 [OUT] nCount     ÷˙ ÷µƒ ˝¡ø
 
 *∑µªÿ÷µ: -1 Œ¥≥ı ºªØ 0 Œ¥¡¨Ω” 1 ¡¨Ω” ƒ⁄Õ¯ 2 ◊™∑¢¡¨Ω” 3 ¡¨Ω” Õ‚Õ¯
 *****************************************************************************/

#ifndef WIN32
JVCLIENT_API int JVC_HelpQuery(char* pGroup,int nYST,int *nCount)
#else
JVCLIENT_API int __stdcall JVC_HelpQuery(char* pGroup,int nYST,int *nCount)
#endif
{
    if(!g_pCWorker)
        return -1;
    
    nCount = 0;
    return g_pCWorker->GetHelper(pGroup,nYST,nCount);
}


/****************************************************************************
 *√˚≥∆  : JVC_QueryDevice
 *π¶ƒ‹  : ≤È—Ø∫≈¬Î «∑Ò“—æ≠À—À˜≥ˆ¿¥ 
 *≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
 [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
 [IN] nTimeOut    ≥¨ ± ±º‰µ•Œª∫¡√Î
 [IN] callBack    ªÿµ˜∫Ø ˝
 
 *∑µªÿ÷µ: 0 ¥ÌŒÛ 1 ≥…π¶ µ»¥˝ªÿµ˜
 *****************************************************************************/

#ifndef WIN32
JVCLIENT_API int JVC_QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack)
#else
JVCLIENT_API int __stdcall JVC_QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack)
#endif
{
    if(!g_pCWorker)
        return -1;
    //	if(strlen(pGroup) == 0 || nYST <= 0 || nTimeOut <= 0)
    if(nTimeOut <= 0)
    {
        return 0;
    }
    g_pCWorker->QueryDevice(pGroup,nYST,nTimeOut,callBack);
    return 1;
}

JVCLIENT_API void JVC_DeleteErrorLog(){
    deleteLog();
}
#ifndef WIN32
JVCLIENT_API char* JVC_GetVersion( )
#else
JVCLIENT_API char* __stdcall JVC_GetVersion( )
#endif
{
    return JVCLIENT_VERSION;
}

/****************************************************************************
*名称  : JVC_ConnectRTMP
*功能  : 分控连接流媒体服务器
*参数  :	nLocalChannel		本地通道号 唯一号 >=0
strURL				连接的地址
rtmpConnectChange	连接 断开回调函数
rtmpNormalData		视频音频数据回调函数

  *返回值: true 调用成功 等待连接状态返回 回调函数
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API bool JVC_ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout)
#else
JVCLIENT_API bool __stdcall JVC_ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout)
#endif
{
	if(g_pCWorker != NULL)
	{
//		LOGE("JVC_ConnectRTMP  url: %s, channel: %d",strURL,nLocalChannel);
		return g_pCWorker->ConnectRTMP(nLocalChannel,strURL,rtmpConnectChange,rtmpNormalData,nTimeout);
	}
	return false;
}



/****************************************************************************
*名称  : JVC_ShutdownRTMP
*功能  : 分控连接流媒体服务器
*参数  :	nLocalChannel		本地通道号 唯一号 >=0

  *返回值: 无
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_ShutdownRTMP(int nLocalChannel)
#else
JVCLIENT_API void __stdcall JVC_ShutdownRTMP(int nLocalChannel)
#endif
{
	if(g_pCWorker != NULL)
	{
//		LOGE("BENGIN SHUTDOWN");
		g_pCWorker->ShutdownRTMP(nLocalChannel);
	}
	
}


#ifndef WIN32
JVCLIENT_API void JVC_ClearHelpCache()
#else
JVCLIENT_API void __stdcall JVC_ClearHelpCache(void)
#endif
{
	if(g_pCWorker != NULL)
	{
		g_pCWorker->ClearHelpCache();
	}
}
/**
 * success:1
 * failed:0
 */
#ifndef WIN32
    JVCLIENT_API int JVC_SetMTU(int nMtu)
#else
    JVCLIENT_API int __stdcall  JVC_SetMTU(int nMtu)
#endif
{
	if(g_pCWorker != NULL)
	{
		return g_pCWorker->SetMTU(nMtu);
	}
	return 0;
}
/**
 * success:1
 * failed:0
 */
#ifndef WIN32
JVCLIENT_API int JVC_StopHelp()
#else
JVCLIENT_API int __stdcall  JVC_StopHelp()
#endif
{
    if(g_pCWorker != NULL)
    {
        return g_pCWorker->stopHelp();
    }
    return 0;
}



/*
 设置本地的服务器
 */
#ifndef WIN32
JVCLIENT_API int JVC_SetSelfServer(char* pGroup,char* pServer)
#else
JVCLIENT_API int __stdcall JVC_SetSelfServer(char* pGroup,char* pServer)
#endif
{
    if(g_pCWorker != NULL)
        return g_pCWorker->SetSelfServer(pGroup,pServer);
    return -1;
}

/*
 向主控发送设置服务器命令
 */

#ifndef WIN32
JVCLIENT_API int JVC_SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
#else
JVCLIENT_API int __stdcall JVC_SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
#endif
{
    if(g_pCWorker != NULL)
        return g_pCWorker->SendSetServer(pGroup,nYst,pServer,nLen,nTimeOut);
    return -1;
}


/*
 向主控发送删除服务器命令
 */

#ifndef WIN32
JVCLIENT_API int JVC_SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
#else
JVCLIENT_API int __stdcall JVC_SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut)
#endif
{
    if(g_pCWorker != NULL)
        return g_pCWorker->SendRemoveServer(pGroup,nYst,pServer,nLen,nTimeOut);
    return -1;
}

/****************************************************************************
 *名称  : JVC_StartBroadcastSelfServer
 *功能  : 开启自定义广播服务
 *参数  : [IN] nLPort      本地服务端口，<0时为默认9700
 [IN] nServerPort 设备端服务端口，<=0时为默认9108,建议统一用默认值与服务端匹配
 [IN] BCSelfData  自定义广播结果回调函数
 *返回值: TRUE/FALSE
 *其他  :
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API bool JVC_StartBroadcastSelfServer(int nLPort, int nServerPort, FUNC_CBCSELFDATA_CALLBACK BCSelfData)
#else
JVCLIENT_API bool __stdcall	JVC_StartBroadcastSelfServer(int nLPort, int nServerPort, FUNC_CBCSELFDATA_CALLBACK BCSelfData)
#endif
{
    if(g_pCWorker != NULL)
    {
        if(BCSelfData != NULL)
        {
            g_pCWorker->m_pfBCSelfData = BCSelfData;
        }
        if(g_pCWorker->StartBCSelfServer(nLPort, nServerPort))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************
 *名称  : JVC_StopBroadcastSelfServer
 *功能  : 停止自定义广播服务
 *参数  : 无
 *返回值: 无
 *其他  : 无
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API void JVC_StopBroadcastSelfServer()
#else
JVCLIENT_API void __stdcall	JVC_StopBroadcastSelfServer()
#endif
{
    if(g_pCWorker != NULL)
    {
        g_pCWorker->StopBCSelfServer();
    }
}

/****************************************************************************
 *名称  : JVC_BroadcastSelfOnce
 *功能  : 发送一次广播消息
 *参数  :
 [IN] pBuffer    自定义广播净载数据
 [IN] nSize       自定义广播净载数据长度
 [IN] nTimeOut 此参数目前可置为0
 *返回值: TRUE/FALSE
 *其他  :
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API BOOL JVC_BroadcastSelfOnce(BYTE *pBuffer, int nSize, int nTimeOut)
#else
JVCLIENT_API BOOL __stdcall JVC_BroadcastSelfOnce(BYTE *pBuffer, int nSize, int nTimeOut)
#endif
{
    if(g_pCWorker != NULL)
    {
        BOOL b=g_pCWorker->DoBroadcastSelf(pBuffer, nSize, nTimeOut);
        if(b)
        {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/****************************************************************************
 *名称  : JVC_SendSelfDataOnceFromBC
 *功能  : 从自定义广播套接字发送一次UDP消息
 *参数  :
 [IN] pBuffer     净载数据
 [IN] nSize       净载数据长度
 [IN] pchDeviceIP 目的IP地址
 [IN] nLocalPort	  目的端口
 *返回值: TRUE/FALSE
 *其他  :
 *****************************************************************************/
#ifndef WIN32
JVCLIENT_API BOOL JVC_SendSelfDataOnceFromBC(BYTE *pBuffer, int nSize, char *pchDeviceIP, int nDestPort)
#else
JVCLIENT_API BOOL __stdcall JVC_SendSelfDataOnceFromBC(BYTE *pBuffer, int nSize, char *pchDeviceIP, int nDestPort)
#endif
{
    if(g_pCWorker != NULL)
    {
        BOOL b=g_pCWorker->DoSendSelfDataFromBC(pBuffer, nSize, pchDeviceIP, nDestPort);
        if(b)
        {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}


