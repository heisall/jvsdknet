#ifndef JVCLIENT_H
#define JVCLIENT_H
#include "JVNSDKDef.h"
#ifndef WIN32//∑«windowsœµÕ≥

#ifdef MOBILE_CLIENT
extern "C" {
#else
	#ifdef __cplusplus
		#define JVCLIENT_API extern "C"
	#else
		#define JVCLIENT_API
	#endif
#endif
#else
#define JVCLIENT_API extern "C" __declspec(dllexport)
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                     //
//                                      ∑÷øÿ∂ÀΩ”ø⁄                                                     //
//                                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/***************************************Ω”ø⁄«Âµ•********************************************************
JVC_InitSDK -----------------01 ≥ı ºªØSDK◊ ‘¥
JVC_ReleaseSDK --------------02  Õ∑≈SDK◊ ‘¥£¨±ÿ–Î◊Ó∫Û±ªµ˜”√
JVC_RegisterCallBack --------03 …Ë÷√∑÷øÿ∂Àªÿµ˜∫Ø ˝
JVC_Connect -----------------04 ¡¨Ω”ƒ≥Õ®µ¿Õ¯¬Á∑˛ŒÒ
JVC_DisConnect --------------05 ∂œø™ƒ≥Õ®µ¿∑˛ŒÒ¡¨Ω”
JVC_SendData ----------------06 ∑¢ÀÕ ˝æ›
JVC_EnableLog ---------------07 …Ë÷√–¥≥ˆ¥Ì»’÷æ «∑Ò”––ß
JVC_SetLanguage -------------08 …Ë÷√»’÷æ/Ã· æ–≈œ¢”Ô—‘(”¢Œƒ/÷–Œƒ)
JVC_TCPConnect --------------09 ∑Ω Ω¡¨Ω”ƒ≥Õ®µ¿Õ¯¬Á∑˛ŒÒ
JVC_GetPartnerInfo ----------10 ªÒ»°ªÔ∞ÈΩ⁄µ„–≈œ¢
JVC_RegisterRateCallBack ----11 ◊¢≤·ª∫≥ÂΩ¯∂»ªÿµ˜∫Ø ˝
JVC_StartLANSerchServer -----12 ø™∆Ù∑˛ŒÒø…“‘À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
JVC_StopLANSerchServer ------13 Õ£÷πÀ—À˜∑˛ŒÒ
JVC_LANSerchDevice ----------14 À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
JVC_SetLocalFilePath --------15 ◊‘∂®“Â±æµÿŒƒº˛¥Ê¥¢¬∑æ∂£¨∞¸¿®»’÷æ£¨…˙≥…µƒ∆‰À˚πÿº¸Œƒº˛µ»
JVC_SetDomainName -----------16 …Ë÷√–¬µƒ”Ú√˚£¨œµÕ≥Ω´¥”∆‰ªÒ»°∑˛ŒÒ∆˜¡–±Ì
JVC_WANGetChannelCount ------17 Õ®π˝Õ‚Õ¯ªÒ»°ƒ≥∏ˆ‘∆ ”Õ®∫≈¬ÎÀ˘æﬂ”–µƒÕ®µ¿◊‹ ˝
JVC_StartBroadcastServer ----18 ø™∆Ù◊‘∂®“Âπ„≤•∑˛ŒÒ
JVC_StopBroadcastServer -----19 Õ£÷π◊‘∂®“Âπ„≤•∑˛ŒÒ
JVC_BroadcastOnce -----------20 ∑¢ÀÕπ„≤•œ˚œ¢
JVC_ClearBuffer -------------21 «Â¿Ì±æµÿª∫¥Ê

JVC_EnableHelp---------------22 ∆Ù”√/Õ£”√øÏÀŸ¡¥Ω”∑˛ŒÒ(‘∆ ”Õ®–°÷˙ ÷ π”√)
JVC_SetHelpYSTNO-------------23 …Ë÷√∂‘ƒ≥–©‘∆ ”Õ®∫≈¬Îµƒ∏®÷˙÷ß≥÷
JVC_GetHelpYSTNO-------------24 ªÒ»°∂‘ƒ≥–©‘∆ ”Õ®∫≈¬Îµƒ∏®÷˙÷ß≥÷
JVC_EnableLANTool------------25 ø™∆Ù∑˛ŒÒø…“‘À—À˜≈‰÷√æ÷”ÚÕ¯÷–µƒ…Ë±∏
JVC_LANToolDevice------------26 À—À˜æ÷”ÚÕ¯÷–µƒø…≈‰÷√…Ë±∏
JVC_SendCMD------------------27 œÚ÷˜øÿ∂À∑¢ÀÕ“ª–©Ãÿ ‚√¸¡Ó
JVC_AddFSIpSection-----------28 ‘ˆº””≈œ»∑¢ÀÕπ„≤•µƒIP◊È
JVC_MOLANSerchDevice --------29  ÷ª˙À—À˜æ÷”ÚÕ¯÷–Œ¨…Ë±∏
JVC_RegisterCommonCallBack---30  ÷ª˙◊®”√ªÿµ˜◊¢≤·
*******************************************************************************************************/


/****************************************************************************
*√˚≥∆  : JVC_InitSDK
*π¶ƒ‹  : ≥ı ºªØSDK◊ ‘¥£¨±ÿ–Î±ªµ⁄“ª∏ˆµ˜”√
*≤Œ ˝  : [IN] nLocalStartPort ±æµÿ¡¨Ω” π”√µƒ∆ º∂Àø⁄ <0 ±ƒ¨»œ9200
*∑µªÿ÷µ: TRUE     ≥…π¶
         FALSE     ß∞‹
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
#ifdef MOBILE_CLIENT 
	JVCLIENT_API int JVC_InitSDK(int nLocStartPort,char* pfWriteReadData);
#else
	JVCLIENT_API int JVC_InitSDK(int nLocStartPort);
#endif
#else
	JVCLIENT_API bool __stdcall	JVC_InitSDK(int nLocStartPort);
#endif

/****************************************************************************
*√˚≥∆  : JVC_ReleaseSDK
*π¶ƒ‹  :  Õ∑≈SDK◊ ‘¥£¨±ÿ–Î◊Ó∫Û±ªµ˜”√
*≤Œ ˝  : Œﬁ
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_ReleaseSDK();
#else
	JVCLIENT_API void __stdcall	JVC_ReleaseSDK();
#endif

/****************************************************************************
*√˚≥∆  : JVC_RegisterSCallBack
*π¶ƒ‹  : …Ë÷√∑÷øÿ∂Àªÿµ˜∫Ø ˝
*≤Œ ˝  : [IN] ConnectCallBack   ”Î÷˜øÿ¡¨Ω”◊¥øˆªÿµ˜∫Ø ˝
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : ∑÷øÿ∂Àªÿµ˜∫Ø ˝∞¸¿®£∫
             ”Î÷˜øÿ∂ÀÕ®–≈◊¥Ã¨∫Ø ˝£ª      (¡¨Ω”◊¥Ã¨)
			  µ ±º‡øÿ¥¶¿Ì                ( ’µΩ µ ±º‡øÿ ˝æ›)
		     ¬ºœÒºÏÀ˜Ω·π˚¥¶¿Ì∫Ø ˝£ª      ( ’µΩ¬ºœÒºÏÀ˜Ω·π˚)
			 ”Ô“Ù¡ƒÃÏ/Œƒ±æ¡ƒÃÏ∫Ø ˝       (‘∂≥Ã”Ô“Ù∫ÕŒƒ±æ¡ƒÃÏ)
			 ‘∂≥Ãœ¬‘ÿ∫Ø ˝£ª              (‘∂≥Ãœ¬‘ÿ ˝æ›)
			 ‘∂≥Ãªÿ∑≈∫Ø ˝£ª              (‘∂≥Ãªÿ∑≈ ˝æ›)
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void  JVC_RegisterCallBack(FUNC_CCONNECT_CALLBACK ConnectCallBack,
													FUNC_CNORMALDATA_CALLBACK NormalData,
													FUNC_CCHECKRESULT_CALLBACK CheckResult,
													FUNC_CCHATDATA_CALLBACK ChatData,
													FUNC_CTEXTDATA_CALLBACK TextData,
													FUNC_CDOWNLOAD_CALLBACK DownLoad,
													FUNC_CPLAYDATA_CALLBACK PlayData);
#else
	JVCLIENT_API void __stdcall	JVC_RegisterCallBack(FUNC_CCONNECT_CALLBACK ConnectCallBack,
													FUNC_CNORMALDATA_CALLBACK NormalData,
													FUNC_CCHECKRESULT_CALLBACK CheckResult,
													FUNC_CCHATDATA_CALLBACK ChatData,
													FUNC_CTEXTDATA_CALLBACK TextData,
													FUNC_CDOWNLOAD_CALLBACK DownLoad,
													FUNC_CPLAYDATA_CALLBACK PlayData);
#endif

    
    /**
     nEnableOem: 1:enable; 0:unenable  default:0
     
     return 1 ok -1 uninitsdk
     **/
    JVCLIENT_API int JVC_EnableOEM(int nEnableOem);
    
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
		 [IN] nTURNType     ◊™∑¢π¶ƒ‹¿‡–Õ(Ω˚”√◊™∑¢\∆Ù”√◊™∑¢\Ωˆ”√◊™∑¢)
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : nLocalChannel <= -2 «“ nChannel = -1 ø…¡¨Ω”∑˛ŒÒ∂ÀµƒÃÿ ‚Õ®µ¿£¨
         ø…±‹ø™ ”∆µ ˝æ›£¨”√”⁄ ’∑¢∆’Õ® ˝æ›
*****************************************************************************/
#ifndef WIN32
	#ifdef MOBILE_CLIENT
	JVCLIENT_API void JVC_Connect(int nLocalChannel,int nChannel,
		                          char *pchServerIP,int nServerPort,
		                          char *pchPassName,char *pchPassWord,
		                          int nYSTNO,char chGroup[4],
		                          int bLocalTry,
		                          int nTURNType,
		                          int bCache,
								  int nConnectType,BOOL isBeRequestVedio,int nVIP,int nOnlyTCP);
    #else
	JVCLIENT_API void JVC_Connect(int nLocalChannel,int nChannel,
		                          char *pchServerIP,int nServerPort,
		                          char *pchPassName,char *pchPassWord,
		                          int nYSTNO,char chGroup[4],
		                          int bLocalTry,
		                          int nTURNType,
		                          int bCache);
    #endif
#else
	JVCLIENT_API void __stdcall	JVC_Connect(int nLocalChannel,int nChannel,
											char *pchServerIP,int nServerPort,
											char *pchPassName,char *pchPassWord,
											int nYSTNO,char chGroup[4],
											BOOL bLocalTry,
											int nTURNType,
											BOOL bCache);
#endif

/****************************************************************************
*√˚≥∆  : JVC_DisConnect
*π¶ƒ‹  : ∂œø™ƒ≥Õ®µ¿∑˛ŒÒ¡¨Ω”
*≤Œ ˝  : [IN] nLocalChannel ∑˛ŒÒÕ®µ¿∫≈ >=1
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  :
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_DisConnect(int nLocalChannel);
#else
	JVCLIENT_API void __stdcall	JVC_DisConnect(int nLocalChannel);
#endif

/****************************************************************************
*√˚≥∆  : JVC_SendData
*π¶ƒ‹  : ∑¢ÀÕ ˝æ›
*≤Œ ˝  : [IN] nLocalChannel   ±æµÿÕ®µ¿∫≈ >=1
         [IN] uchType           ˝æ›¿‡–Õ£∫∏˜÷÷«Î«Û£ª∏˜÷÷øÿ÷∆£ª∏˜÷÷”¶¥
         [IN] pBuffer         ¥˝∑¢ ˝æ›ƒ⁄»›
		 [IN] nSize           ¥˝∑¢ ˝æ›≥§∂»
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : œÚÕ®µ¿¡¨Ω”µƒ÷˜øÿ∑¢ÀÕ ˝æ›
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API BOOL JVC_SendData(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer,int nSize);
#else
	JVCLIENT_API void __stdcall	JVC_SendData(int nLocalChannel, unsigned char uchType, unsigned char  *pBuffer,int nSize);
#endif

/****************************************************************************
*√˚≥∆  : JVN_EnableLog
*π¶ƒ‹  : …Ë÷√–¥≥ˆ¥Ì»’÷æ «∑Ò”––ß
*≤Œ ˝  : [IN] bEnable  TRUE:≥ˆ¥Ì ±–¥»’÷æ£ªFALSE:≤ª–¥»Œ∫Œ»’÷æ
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_EnableLog(int bEnable);
#else
	JVCLIENT_API void __stdcall	JVC_EnableLog(bool bEnable);
#endif

/****************************************************************************
*√˚≥∆  : JVC_SetLanguage
*π¶ƒ‹  : …Ë÷√»’÷æ/Ã· æ–≈œ¢”Ô—‘(”¢Œƒ/÷–Œƒ)
*≤Œ ˝  : [IN] nLgType  JVN_LANGUAGE_ENGLISH/JVN_LANGUAGE_CHINESE
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_SetLanguage(int nLgType);
#else
	JVCLIENT_API void __stdcall	JVC_SetLanguage(int nLgType);
#endif

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
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_TCPConnect(int nLocalChannel,int nChannel,
									char *pchServerIP,int nServerPort,
									char *pchPassName,char *pchPassWord,
									int nYSTNO,char chGroup[4],
									int bLocalTry,
									int nConnectType,
									int nTURNType);
#else
	JVCLIENT_API void __stdcall	JVC_TCPConnect(int nLocalChannel,int nChannel,
												char *pchServerIP,int nServerPort,
												char *pchPassName,char *pchPassWord,
												int nYSTNO,char chGroup[4],
												BOOL bLocalTry,
												int nConnectType,
												int nTURNType);
#endif


/****************************************************************************
*√˚≥∆  : JVC_GetPartnerInfo
*π¶ƒ‹  : ªÒ»°ªÔ∞ÈΩ⁄µ„–≈œ¢
*≤Œ ˝  : [IN] nLocalChannel   ±æµÿÕ®µ¿∫≈ >=1
         [OUT] pMsg   –≈œ¢ƒ⁄»›
		              ( «∑Ò∂‡≤•(1)+‘⁄œﬂ◊‹∏ˆ ˝(4)+“—¡¨Ω”◊‹ ˝(4)+[IP(16) + port(4)+¡¨Ω”◊¥Ã¨(1)+œ¬‘ÿÀŸ∂»(4)+œ¬‘ÿ◊‹¡ø(4)+…œ¥´◊‹¡ø(4)]
					                +[...]...)
		 [OUT] nSize  –≈œ¢◊‹≥§∂»
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : µ˜”√∆µ¬ —œΩ˚Ã´∏ﬂ£¨∑Ò‘Úª·”∞œÏ ”∆µ¥¶¿ÌÀŸ∂»£ª
         ∆µ∑±≥Ã∂»∂»≤ªƒ‹µÕ”⁄1√Î£¨◊Ó∫√‘⁄2√Î“‘…œªÚ∏¸≥§ ±º‰£¨ ±º‰‘Ω≥§”∞œÏ‘Ω–°°£
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_GetPartnerInfo(int nLocalChannel, char *pMsg, int *nSize);
#else
	JVCLIENT_API void __stdcall	JVC_GetPartnerInfo(int nLocalChannel, char *pMsg, int &nSize);
#endif

#ifndef WIN32
	JVCLIENT_API void JVC_RegisterRateCallBack(FUNC_CBUFRATE_CALLBACK BufRate);
#else
	JVCLIENT_API void __stdcall	JVC_RegisterRateCallBack(FUNC_CBUFRATE_CALLBACK BufRate);
#endif

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
	JVCLIENT_API int JVC_StartLANSerchServer(int nLPort, int nServerPort, FUNC_CLANSDATA_CALLBACK LANSData);
#else
	JVCLIENT_API bool __stdcall	JVC_StartLANSerchServer(int nLPort, int nServerPort, FUNC_CLANSDATA_CALLBACK LANSData);
#endif

/****************************************************************************
*√˚≥∆  : JVC_StopLANSerchServer
*π¶ƒ‹  : Õ£÷πÀ—À˜∑˛ŒÒ
*≤Œ ˝  : Œﬁ
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_StopLANSerchServer();
#else
	JVCLIENT_API void __stdcall	JVC_StopLANSerchServer();
#endif


#ifndef WIN32
    JVCLIENT_API int JVC_MOStopLANSerchServer();
#else
    JVCLIENT_API int __stdcall	JVC_MOStopLANSerchServer();
#endif
    
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

						  Õ¨Õ¯∂Œµƒ…Ë±∏“ª∞„ƒ‹‘⁄<500µƒ ±º‰ƒ⁄À—À˜µΩ£ª
						  …Ë±∏À—À˜Ω®“È÷¡…Ÿ…Ë÷√2000£¨»∑±£À—À˜µƒÕ¯∂Œ»´√Ê£ª
						  º¥≤Âº¥”√À—À˜ø…∏˘æ›–Ë“™≥§ªÚ∂Ã£¨‘Ω∂Ã‘Ωø…ƒ‹“≈¬©≤ªÕ¨Õ¯∂Œµƒ…Ë±∏£ª
		 [IN] unFrequence (µ•ŒªŒ™s )÷¥––ping Õ¯πÿµƒ∆µ¬ £¨ƒ¨»œ«ÎÃÓ»Î30sÀ—À˜“ª¥Œ£¨«∂»Î Ω…Ë±∏ø…∏˘æ›–Ë“™∏¸∏ƒ>20&&<24*3600”––ß.

*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : µ±¡Ω≤Œ ˝Õ¨ ±Œ™0 ±£¨Ω´À—À˜æ÷”ÚÕ¯÷–À˘”–÷–Œ¨…Ë±∏
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int  JVC_LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence );
#else
	JVCLIENT_API bool __stdcall	JVC_LANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence );
#endif

/****************************************************************************
*√˚≥∆  : JVC_SetLocalFilePath
*π¶ƒ‹  : ◊‘∂®“Â±æµÿŒƒº˛¥Ê¥¢¬∑æ∂£¨∞¸¿®»’÷æ£¨…˙≥…µƒ∆‰À˚πÿº¸Œƒº˛µ»
*≤Œ ˝  : [IN] chLocalPath  ¬∑æ∂ –Œ»Á£∫"C:\\jovision"  ∆‰÷–jovision «Œƒº˛º–
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : ≤Œ ˝ π”√ƒ⁄¥ÊøΩ±¥ ±«Î◊¢“‚≥ı ºªØ£¨◊÷∑˚¥Æ–Ë“‘'\0'Ω· ¯
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_SetLocalFilePath(char chLocalPath[256]);
#else
	JVCLIENT_API bool __stdcall	JVC_SetLocalFilePath(char chLocalPath[256]);
#endif

/****************************************************************************
*√˚≥∆  : JVC_SetDomainName
*π¶ƒ‹  : …Ë÷√–¬µƒ”Ú√˚£¨œµÕ≥Ω´¥”∆‰ªÒ»°∑˛ŒÒ∆˜¡–±Ì
*≤Œ ˝  : [IN]  pchDomainName     ”Ú√˚
[IN]  pchPathName       ”Ú√˚œ¬µƒŒƒº˛¬∑æ∂√˚ –Œ»Á£∫"/down/YSTOEM/yst0.txt"
*∑µªÿ÷µ: TRUE  ≥…π¶
FALSE  ß∞‹
*∆‰À˚  : œµÕ≥≥ı ºªØ(JVN_InitSDK)ÕÍ∫Û…Ë÷√
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_SetDomainName(char *pchDomainName,char *pchPathName);
#else
	JVCLIENT_API bool __stdcall	JVC_SetDomainName(char *pchDomainName,char *pchPathName);
#endif

/****************************************************************************
*√˚≥∆  : JVC_WANGetChannelCount
*π¶ƒ‹  : Õ®π˝Õ‚Õ¯ªÒ»°ƒ≥∏ˆ‘∆ ”Õ®∫≈¬ÎÀ˘æﬂ”–µƒÕ®µ¿◊‹ ˝
*≤Œ ˝  : [IN]  chGroup   ±‡◊È∫≈
         [IN]  nYstNO    ‘∆ ”Õ®∫≈¬Î
		 [IN]  nTimeOutS µ»¥˝≥¨ ± ±º‰(√Î)
*∑µªÿ÷µ: >0  ≥…π¶,Õ®µ¿ ˝
         -1  ß∞‹£¨‘≠“ÚŒ¥÷™
		 -2  ß∞‹£¨∫≈¬ÎŒ¥…œœﬂ
		 -3  ß∞‹£¨÷˜øÿ∞Ê±æΩœæ…£¨≤ª÷ß≥÷∏√≤È—Ø
*∆‰À˚  : œµÕ≥≥ı ºªØ(JVN_InitSDK)ÕÍ∫Û ø…∂¿¡¢µ˜”√
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS);
#else
	JVCLIENT_API int __stdcall	JVC_WANGetChannelCount(char chGroup[4], int nYSTNO, int nTimeOutS);
#endif

JVCLIENT_API int JVC_WANGetBatchChannelCount(char *pChannelNum, int nYSTNOCnt, int nTimeOutS);
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
	JVCLIENT_API int JVC_StartBroadcastServer(int nLPort, int nServerPort, FUNC_CBCDATA_CALLBACK BCData);
#else
	JVCLIENT_API bool __stdcall	JVC_StartBroadcastServer(int nLPort, int nServerPort, FUNC_CBCDATA_CALLBACK BCData);
#endif

/****************************************************************************
*√˚≥∆  : JVC_StopBroadcastServer
*π¶ƒ‹  : Õ£÷π◊‘∂®“Âπ„≤•∑˛ŒÒ
*≤Œ ˝  : Œﬁ
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_StopBroadcastServer();
#else
	JVCLIENT_API void __stdcall	JVC_StopBroadcastServer();
#endif

/****************************************************************************
*√˚≥∆  : JVC_BroadcastOnce
*π¶ƒ‹  : ∑¢ÀÕπ„≤•œ˚œ¢
*≤Œ ˝  : [IN] nBCID       π„≤•ID,”…µ˜”√’ﬂ∂®“Â,”√”⁄‘⁄ªÿµ˜∫Ø ˝÷–∆•≈‰«¯∑÷±æ¥Œπ„≤•
         [IN] pBuffer     π„≤•æª‘ÿ ˝æ›, ˝æ›≤ª‘ –Ì≥¨≥ˆ10k,∑Ò‘ÚΩ´…·∆˙∂‡”‡≤ø∑÷
         [IN] nSize       π„≤•æª‘ÿ ˝æ›≥§∂»
		 [IN] nTimeOut    ±æ¥Œπ„≤•”––ß ±º‰£¨µ•Œª∫¡√Î°£≥¨π˝∏√ ±º‰µƒΩ·π˚Ω´±ª…·∆˙£¨
		                  ≥¨ ± ±º‰µΩ¥Ô∫Ûªÿµ˜∫Ø ˝÷–Ω´µ√µΩ≥¨ ±Ã· æ◊˜Œ™À—À˜Ω· ¯±Í÷æ°£
						  »Áπ˚≤ªœÎ π”√SDK≥¨ ±¥¶¿Ìø…“‘÷√Œ™0£¨¥À ±Ω·π˚«ø»´≤ø∑µªÿ∏¯µ˜”√’ﬂ°£
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : ƒø«∞∏√π¶ƒ‹‘›≤ª÷ß≥÷≤¢∑¢π„≤•£¨º¥≤¢∑¢µ˜”√ ±◊Ó∫Û“ª¥Œπ„≤•Ω´∏≤∏«÷Æ«∞µƒπ„≤•
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_BroadcastOnce(int nBCID, unsigned char *pBuffer, int nSize, int nTimeOut);
#else
	JVCLIENT_API BOOL __stdcall JVC_BroadcastOnce(int nBCID, unsigned char  *pBuffer, int nSize, int nTimeOut);
#endif

/****************************************************************************
*√˚≥∆  : JVC_ClearBuffer
*π¶ƒ‹  : «Âø’±æµÿª∫¥Ê
*≤Œ ˝  : [IN] nLocalChannel ∑˛ŒÒÕ®µ¿∫≈ >=1
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Ωˆ∂‘∆’Õ®ƒ£ Ω¡¥Ω””––ß£¨∂‡≤•¡¨Ω”≤ª‘ –Ì¥”±æµÿ«Âø’
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API void JVC_ClearBuffer(int nLocalChannel);
#else
	JVCLIENT_API void __stdcall	JVC_ClearBuffer(int nLocalChannel);
#endif

/****************************************************************************
*√˚≥∆  : JVC_EnableHelp
*π¶ƒ‹  : ∆Ù”√/Õ£”√øÏÀŸ¡¥Ω”∑˛ŒÒ
*≤Œ ˝  : [IN] bEnable TRUEø™∆Ù/FALSEπÿ±’
         [IN] nType  µ±«∞ π”√’ﬂ «À≠£¨µ±bEnableŒ™TRUE ±”––ß
		             1 µ±«∞ π”√’ﬂ «‘∆ ”Õ®–°÷˙ ÷(∂¿¡¢Ω¯≥Ã)
		             2 µ±«∞ π”√’ﬂ «‘∆ ”Õ®øÕªß∂À£¨÷ß≥÷∂¿¡¢Ω¯≥Ãµƒ–°÷˙ ÷
		             3 µ±«∞ π”√’ﬂ «‘∆ ”Õ®øÕªß∂À£¨≤ª÷ß≥÷∂¿¡¢Ω¯≥Ãµƒ–°÷˙ ÷
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : ∆Ù”√∏√π¶ƒ‹∫Û£¨Õ¯¬ÁSDKª·∂‘…Ë∂®µƒ∫≈¬ÎΩ¯––¡¨Ω”Ã·ÀŸµ»”≈ªØ£ª
		 ∆Ù”√∏√π¶ƒ‹∫Û£¨Õ¯¬ÁSDKª·÷ß≥÷–°÷˙ ÷∫ÕøÕªß∂À÷Æº‰Ω¯––Ωªª•£ª
		 »Áπ˚∑÷øÿ∂À÷ß≥÷–°÷˙ ÷Ω¯≥Ã£¨‘Ú”√–°÷˙ ÷∂À π”√nType=1£¨øÕªß∂À π”√nType=2º¥ø…£ª
		 »Áπ˚øÕªß∂À≤ª÷ß≥÷–°÷˙ ÷Ω¯≥Ã£¨‘ÚøÕªß∂À π”√nType=3º¥ø…£¨±»»Á ÷ª˙øÕªß∂À£ª
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_EnableHelp(int bEnable, int nType);
#else
	JVCLIENT_API BOOL __stdcall JVC_EnableHelp(BOOL bEnable, int nType);
#endif

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
		 ª·∂ØÃ¨ÃÌº”µΩƒ⁄≤ø£ª
		 STBASEYSTNOS ‘∆ ”Õ®∫≈¬Î,STYSTNO∂®“Â≤Œø¥JVNSDKDef.h
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_SetHelpYSTNO(unsigned char *pBuffer, int nSize);
#else
	JVCLIENT_API BOOL __stdcall JVC_SetHelpYSTNO(unsigned char  *pBuffer, int nSize);
#endif

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
	JVCLIENT_API int JVC_GetHelpYSTNO(unsigned char *pBuffer, int *nSize);
#else
	JVCLIENT_API int __stdcall JVC_GetHelpYSTNO(unsigned char  *pBuffer, int &nSize);
#endif

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
	JVCLIENT_API int JVC_GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut);
#else
	JVCLIENT_API int __stdcall JVC_GetYSTStatus(char chGroup[4], int nYSTNO, int nTimeOut);
#endif

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
	JVCLIENT_API int JVC_EnableLANTool(int nEnable, int nLPort, int nServerPort, FUNC_CLANTDATA_CALLBACK LANTData);
#else
	JVCLIENT_API int __stdcall	JVC_EnableLANTool(int nEnable, int nLPort, int nServerPort, FUNC_CLANTDATA_CALLBACK LANTData);
#endif

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
*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut);
#else
	JVCLIENT_API int __stdcall	JVC_LANToolDevice(char chPName[256], char chPWord[256], int nTimeOut);
#endif

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
         µ±«∞÷ß≥÷µƒ”–:÷ª∑¢πÿº¸÷°√¸¡ÓJVN_CMD_ONLYI
		              ∫Õª÷∏¥¬˙÷°√¸¡ÓJVN_CMD_FULL
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int JVC_SendCMD(int nLocalChannel, unsigned char uchType, unsigned char  *pBuffer, int nSize);
#else
	JVCLIENT_API int __stdcall	JVC_SendCMD(int nLocalChannel, unsigned char  uchType, unsigned char  *pBuffer, int nSize);
#endif

/****************************************************************************
*√˚≥∆  : JVC_AddFSIpSection
*π¶ƒ‹  : ‘ˆº”◊‘∂®“ÂIP∂Œ£¨“‘π©”≈œ»À—À˜ ,”≈œ»À—À˜÷∏∂®µƒIP∂Œ first search
*≤Œ ˝  : [IN] pStartIp		  IPSECTION ˝◊Èµÿ÷∑
		 [IN] nSize           IP∂Œ ˝*sizeof(IPSECTION)
		 [IN] bEnablePing     ‘›Õ£°¢ºÃ–¯ pingœﬂ≥Ã 
*∑µªÿ÷µ: 0£¨≥…π¶ -1  ß∞‹
*∆‰À˚  : Œﬁ
*****************************************************************************/
#ifndef WIN32
 	JVCLIENT_API int JVC_AddFSIpSection( const IPSECTION * pStartIp, int nSize ,int bEnablePing );
#else
 	JVCLIENT_API int __stdcall	JVC_AddFSIpSection( const IPSECTION * pStartIp, int nSize ,BOOL bEnablePing );
 
#endif

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

						  Õ¨Õ¯∂Œµƒ…Ë±∏“ª∞„ƒ‹‘⁄<500µƒ ±º‰ƒ⁄À—À˜µΩ£ª
						  …Ë±∏À—À˜Ω®“È÷¡…Ÿ…Ë÷√2000£¨»∑±£À—À˜µƒÕ¯∂Œ»´√Ê£ª
						  º¥≤Âº¥”√À—À˜ø…∏˘æ›–Ë“™≥§ªÚ∂Ã£¨‘Ω∂Ã‘Ωø…ƒ‹“≈¬©≤ªÕ¨Õ¯∂Œµƒ…Ë±∏£ª
		 [IN] unFrequence (µ•ŒªŒ™s )÷¥––ping Õ¯πÿµƒ∆µ¬ £¨ƒ¨»œ«ÎÃÓ»Î30sÀ—À˜“ª¥Œ£¨«∂»Î Ω…Ë±∏ø…∏˘æ›–Ë“™∏¸∏ƒ>20&&<24*3600”––ß.

*∑µªÿ÷µ: Œﬁ
*∆‰À˚  : µ±¡Ω≤Œ ˝Õ¨ ±Œ™0 ±£¨Ω´À—À˜æ÷”ÚÕ¯÷–À˘”–÷–Œ¨…Ë±∏
*****************************************************************************/
#ifndef WIN32
	JVCLIENT_API int  JVC_MOLANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence);
#else
	JVCLIENT_API bool __stdcall	JVC_MOLANSerchDevice(char chGroup[4], int nYSTNO, int nCardType, int nVariety, char chDeviceName[100], int nTimeOut,unsigned int unFrequence);
#endif

/**
 * stop lansearch
 */
#ifndef WIN32
	JVCLIENT_API int JVC_MOStopLANSerchDevice();
#else
	JVCLIENT_API int __stdcall	JVC_MOStopLANSerchDevice();
#endif
/****************************************************************************
*√˚≥∆  : JVC_RegisterCommonCallBack
*π¶ƒ‹  : ‘∆ ”Õ®ø‚”Î”¶”√≤„ ˝æ›Ωªª• ªÿµ˜◊¢≤·
*≤Œ ˝  : ªÿµ˜∫Ø ˝

*∑µªÿ÷µ: Œﬁ
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_RegisterCommonCallBack(FUNC_COMM_DATA_CALLBACK pfWriteReadDataCallBack);
#else
JVCLIENT_API void __stdcall JVC_RegisterCommonCallBack(FUNC_COMM_DATA_CALLBACK pfWriteReadDataCallBack);
#endif


/****************************************************************************
*√˚≥∆  : JVC_GetDemo
*π¶ƒ‹  : ªÒ»°—› æµ„
*≤Œ ˝  : [OUT] pBuff		  ¥Ê∑≈≤È—Øµƒ∫≈¬Îµƒ¡–±Ì,ƒ⁄¥Ê”…”¶”√≤„¥¥Ω®∑÷≈‰ ±‡◊È : 4 BYTE ∫≈¬Î : 4 BYTE Õ®µ¿ ˝ : 1 BYTE
         [IN] nBuffSize       ¥¥Ω®µƒƒ⁄¥Ê¥Û–°

*∑µªÿ÷µ: ’˝ ˝ —› æµ„µƒ ˝¡ø£¨-2 œµÕ≥Œ¥≥ı ºªØ -1 Œ™ƒ⁄¥ÊÃ´–°£¨0 ø…ƒ‹ «Õ¯¬Áø‚ªπ√ª”–ªÒ»°µΩ
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API int JVC_GetDemo(BYTE* pBuff,int nBuffSize);
#else
JVCLIENT_API int __stdcall JVC_GetDemo(BYTE* pBuff,int nBuffSize);
#endif

/****************************************************************************
*√˚≥∆  : JVC_HelperRemove
*π¶ƒ‹  : …æ≥˝÷˙ ÷ƒ⁄µƒ∫≈¬Î 
*≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
         [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß

*∑µªÿ÷µ: Œﬁ
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_HelperRemove(char* pGroup,int nYST);
#else
JVCLIENT_API void __stdcall JVC_HelperRemove(char* pGroup,int nYST);
#endif


/****************************************************************************
*√˚≥∆  : JVC_HelpQuery
*π¶ƒ‹  : ≤È—Ø÷˙ ÷µƒ∫≈¬Î¡¨Ω”◊¥Ã¨ 
*≤Œ ˝  : [IN] chGroup     ±‡◊È∫≈£¨±‡◊È∫≈+nYSTNOø…»∑∂®Œ®“ª…Ë±∏
         [IN] nYSTNO      À—À˜æﬂ”–ƒ≥‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏£¨>0”––ß
		 [OUT] nCount     ÷˙ ÷µƒ ˝¡ø

*∑µªÿ÷µ: -1 Œ¥≥ı ºªØ 0 Œ¥¡¨Ω” 1 ¡¨Ω” ƒ⁄Õ¯ 2 ◊™∑¢¡¨Ω” 3 ¡¨Ω” Õ‚Õ¯
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API int JVC_HelpQuery(char* pGroup,int nYST,int &nCount);
#else
JVCLIENT_API int __stdcall JVC_HelpQuery(char* pGroup,int nYST,int &nCount);
#endif


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
JVCLIENT_API int JVC_QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack);
#else
JVCLIENT_API int __stdcall JVC_QueryDevice(char* pGroup,int nYST,int nTimeOut,FUNC_DEVICE_CALLBACK callBack);
#endif

    

    JVCLIENT_API void JVC_DeleteErrorLog();

    

#ifndef WIN32
JVCLIENT_API char* JVC_GetVersion( );
#else
JVCLIENT_API char* __stdcall JVC_GetVersion( );
#endif






/****************************************************************************
*√˚≥∆  : JVC_ConnectRTMP
*π¶ƒ‹  : ∑÷øÿ¡¨Ω”¡˜√ΩÃÂ∑˛ŒÒ∆˜
*≤Œ ˝  :	nLocalChannel		±æµÿÕ®µ¿∫≈ Œ®“ª∫≈ >=0
strURL				¡¨Ω”µƒµÿ÷∑
rtmpConnectChange	¡¨Ω” ∂œø™ªÿµ˜∫Ø ˝
rtmpNormalData		 ”∆µ“Ù∆µ ˝æ›ªÿµ˜∫Ø ˝

  *∑µªÿ÷µ: true µ˜”√≥…π¶ µ»¥˝¡¨Ω”◊¥Ã¨∑µªÿ ªÿµ˜∫Ø ˝
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API bool JVC_ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout);
#else
JVCLIENT_API bool __stdcall JVC_ConnectRTMP(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout);
#endif

/****************************************************************************
*√˚≥∆  : JVC_ShutdownRTMP
*π¶ƒ‹  : ∑÷øÿ¡¨Ω”¡˜√ΩÃÂ∑˛ŒÒ∆˜
*≤Œ ˝  :	nLocalChannel		±æµÿÕ®µ¿∫≈ Œ®“ª∫≈ >=0

  *∑µªÿ÷µ: Œﬁ
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_ShutdownRTMP(int nLocalChannel);
#else
JVCLIENT_API void __stdcall JVC_ShutdownRTMP(int nLocalChannel);
#endif


/****************************************************************************
*Ñ1§7Ñ1§7Ñ1§7ÁßÑ1§7  : JVC_ClearHelpCache
*Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7  : Ê∏Ñ1§7Á©∫ËÑ1§7Ñ1§7Ñ1§7Ñ1§7©ËÑ1§7Ñ1§7Ñ1§7Ñ1§7•ÁÑ1§7Ñ1§7Â≠Ñ1§7
*Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7  : Ñ1§7Ñ1§7Ñ1§7

  *ËøÑ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7Ñ1§7: Ñ1§7Ñ1§7Ñ1§7
*****************************************************************************/

#ifndef WIN32
JVCLIENT_API void JVC_ClearHelpCache();
#else
JVCLIENT_API void __stdcall JVC_ClearHelpCache(void);
#endif

    
    /**
     * success:1
     * failed:0
     */
#ifndef WIN32
    JVCLIENT_API int JVC_SetMTU(int nMtu);
#else
    JVCLIENT_API int __stdcall  JVC_SetMTU(int nMtu);
#endif
    
/**
 * success:1
 * failed:0
*/
#ifndef WIN32
    JVCLIENT_API int JVC_StopHelp();
#else
    JVCLIENT_API int __stdcall  JVC_StopHelp();
#endif
    
/*
 设置本地的服务器
*/
#ifndef WIN32
    JVCLIENT_API int JVC_SetSelfServer(char* pGroup,char* pServer);
#else
    JVCLIENT_API int __stdcall JVC_SetSelfServer(char* pGroup,char* pServer);
#endif
  
    
/*
 向主控发送设置服务器命令
*/
    
#ifndef WIN32
    JVCLIENT_API int JVC_SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
#else
    JVCLIENT_API int __stdcall JVC_SendSetServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
#endif
   
    
/*
 向主控发送删除服务器命令
*/
    
#ifndef WIN32
    JVCLIENT_API int JVC_SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
#else
    JVCLIENT_API int __stdcall JVC_SendRemoveServer(char* pGroup,int nYst,char* pServer,int *nLen,int nTimeOut);
#endif
   
    
  
    
#endif
