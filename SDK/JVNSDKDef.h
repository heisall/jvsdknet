#ifndef _JVNSDKDEF_H
#define _JVNSDKDEF_H

//////////////////////////////////////////////////////////////////////////
//#include <afxinet.h>//CHttpFile
#include   <fstream>//
#include  "Def.h"
#ifdef WIN32
	#include <process.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
    #include "time.h" 
	#pragma comment(lib,"Ws2_32.lib")
	#include "JUDT\udt.h"
	#define jvs_min(a,b) min(a,b)
	#define jvs_max(a,b) max(a,b)
	#define JVSERVER_API extern "C" __declspec(dllexport)
    #define JVCLIENT_API extern "C" __declspec(dllexport)
typedef HANDLE pthread_mutex_t;
#else	
	#include <sys/stat.h>//
	#include <sys/wait.h>//

#ifndef MOBILE_CLIENT
	#include <sys/vfs.h>//
	#include <sys/sysinfo.h>//
#include "./JUDT/udt.h"
#else
#define MSG_NOSIGNAL 0
#include "udt.h"
#endif

	#define JVSERVER_API extern "C"
	#define JVCLIENT_API extern "C"
	#define jvs_min(a,b) (a<b?a:b)
	#define jvs_max(a,b) (a>b?a:b)
    #define MAX_PATH 256
	#define TRUE  1
	#define FALSE 0
	#define closesocket close
    #define SD_BOTH SHUT_RDWR
 
	typedef long  HRESULT;
	typedef struct _GUID 
	{ 
		unsigned long   Data1; 
		unsigned short  Data2; 
		unsigned short  Data3; 
		unsigned char   Data4[8]; 
	}GUID;
#endif

//////////////////////////////////////////////////////////////////////////
/* µ ±º‡øÿ ˝æ›¿‡–Õ*/
#define JVN_DATA_I           0x01// ”∆µI÷°
#define JVN_DATA_B           0x02// ”∆µB÷°
#define JVN_DATA_P           0x03// ”∆µP÷°
#define JVN_DATA_A           0x04//“Ù∆µ
#define JVN_DATA_S           0x05//÷°≥ﬂ¥Á
#define JVN_DATA_OK          0x06// ”∆µ÷° ’µΩ»∑»œ
#define JVN_DATA_DANDP       0x07//œ¬‘ÿªÚªÿ∑≈ ’µΩ»∑»œ
#define JVN_DATA_O           0x08//∆‰À˚◊‘∂®“Â ˝æ›
#define JVN_DATA_SKIP        0x09// ”∆µS÷°
#define JVN_DATA_SPEED		 0x64//÷˜øÿ¬Î¬ 
#define JVN_DATA_HEAD        0x66// ”∆µΩ‚¬ÎÕ∑£¨∏√ ˝æ›≥ˆœ÷µƒÕ¨ ±Ω´«Âø’ª∫¥Ê
/*«Î«Û¿‡–Õ*/
#define JVN_REQ_CHECK        0x10//«Î«Û¬ºœÒºÏÀ˜
#define JVN_REQ_DOWNLOAD     0x20//«Î«Û¬ºœÒœ¬‘ÿ
#define JVN_REQ_PLAY         0x30//«Î«Û‘∂≥Ãªÿ∑≈
#define JVN_REQ_CHAT         0x40//«Î«Û”Ô“Ù¡ƒÃÏ
#define JVN_REQ_TEXT         0x50//«Î«ÛŒƒ±æ¡ƒÃÏ
#define JVN_REQ_CHECKPASS    0x71//«Î«Û…Ì∑›—È÷§
#define JVN_REQ_RECHECK      0x13//‘§—È÷§
#define JVN_REQ_RATE		 0x63//∑÷øÿ«Î«Û¬Î¬ 
	
/*«Î«Û∑µªÿΩ·π˚¿‡–Õ*/
#define JVN_RSP_CHECKDATA    0x11//ºÏÀ˜Ω·π˚
#define JVN_RSP_CHECKOVER    0x12//ºÏÀ˜ÕÍ≥…
#define JVN_RSP_DOWNLOADDATA 0x21//œ¬‘ÿ ˝æ›
#define JVN_RSP_DOWNLOADOVER 0x22//œ¬‘ÿ ˝æ›ÕÍ≥…
#define JVN_RSP_DOWNLOADE    0x23//œ¬‘ÿ ˝æ› ß∞‹
#define JVN_RSP_PLAYDATA     0x31//ªÿ∑≈ ˝æ›
#define JVN_RSP_PLAYOVER     0x32//ªÿ∑≈ÕÍ≥…
#define JVN_RSP_PLAYE        0x39//ªÿ∑≈ ß∞‹
#define JVN_RSP_CHATDATA     0x41//”Ô“Ù ˝æ›
#define JVN_RSP_CHATACCEPT   0x42//Õ¨“‚”Ô“Ù«Î«Û
#define JVN_RSP_TEXTDATA     0x51//Œƒ±æ ˝æ›
#define JVN_RSP_TEXTACCEPT   0x52//Õ¨“‚Œƒ±æ«Î«Û
#define JVN_RSP_CHECKPASS    0x72//…Ì∑›—È÷§
#define JVN_RSP_CHECKPASST   0x72//…Ì∑›—È÷§≥…π¶ Œ™TCP±£¡Ù
#define JVN_RSP_CHECKPASSF   0x73//…Ì∑›—È÷§ ß∞‹ Œ™TCP±£¡Ù
#define JVN_RSP_NOSERVER     0x74//Œﬁ∏√Õ®µ¿∑˛ŒÒ
#define JVN_RSP_INVALIDTYPE  0x7A//¡¨Ω”¿‡–ÕŒﬁ–ß
#define JVN_RSP_OVERLIMIT    0x7B//¡¨Ω”≥¨π˝÷˜øÿ‘ –Ìµƒ◊Ó¥Û ˝ƒø
#define JVN_RSP_DLTIMEOUT    0x76//œ¬‘ÿ≥¨ ±
#define JVN_RSP_PLTIMEOUT    0x77//ªÿ∑≈≥¨ ±
#define JVN_RSP_RECHECK      0x14//‘§—È÷§
#define JVN_RSP_OLD          0x15//æ…∞Ê÷˜øÿªÿ∏¥

/*√¸¡Ó¿‡–Õ*/
#define JVN_CMD_DOWNLOADSTOP 0x24//Õ£÷πœ¬‘ÿ ˝æ›
#define JVN_CMD_PLAYUP       0x33//øÏΩ¯
#define JVN_CMD_PLAYDOWN     0x34//¬˝∑≈
#define JVN_CMD_PLAYDEF      0x35//‘≠ÀŸ≤•∑≈
#define JVN_CMD_PLAYSTOP     0x36//Õ£÷π≤•∑≈
#define JVN_CMD_PLAYPAUSE    0x37//‘›Õ£≤•∑≈
#define JVN_CMD_PLAYGOON     0x38//ºÃ–¯≤•∑≈
#define JVN_CMD_CHATSTOP     0x43//Õ£÷π”Ô“Ù¡ƒÃÏ
#define JVN_CMD_PLAYSEEK     0x44//≤•∑≈∂®Œª		∞¥÷°∂®Œª –Ë“™≤Œ ˝ ÷° ˝(1~◊Ó¥Û÷°)
#define JVN_CMD_TEXTSTOP     0x53//Õ£÷πŒƒ±æ¡ƒÃÏ
#define JVN_CMD_YTCTRL       0x60//‘∆Ã®øÿ÷∆
#define JVN_CMD_VIDEO        0x70// µ ±º‡øÿ
#define JVN_CMD_VIDEOPAUSE   0x75//‘›Õ£ µ ±º‡øÿ
#define JVN_CMD_TRYTOUCH     0x78//¥Ú∂¥∞¸
#define JVN_CMD_FRAMETIME    0x79//÷°∑¢ÀÕ ±º‰º‰∏Ù(µ•Œªms)
#define JVN_CMD_DISCONN      0x80//∂œø™¡¨Ω”
#define JVN_CMD_MOTYPE		 0x72//UDP ÷ª˙¿‡–Õ ◊¢£∫¥À÷µ”Îœ¬√Ê“ª¿‡–Õ∂®“Â÷µœ‡Õ¨£¨±æ”¶±‹√‚£¨‘›±£≥÷’‚—˘
#define JVN_CMD_ONLYI        0x61//∏√Õ®µ¿÷ª∑¢πÿº¸÷°
#define JVN_CMD_FULL         0x62//∏√Õ®µ¿ª÷∏¥¬˙÷°
#define JVN_CMD_ALLAUDIO	 0x65//“Ù∆µ»´◊™∑¢

/*”Î‘∆ ”Õ®∑˛ŒÒ∆˜µƒΩªª•œ˚œ¢*/
#define JVN_CMD_TOUCH        0x81//ÃΩ≤‚∞¸
#define JVN_REQ_ACTIVEYSTNO  0x82//÷˜øÿ«Î«Ûº§ªÓYST∫≈¬Î
#define JVN_RSP_YSTNO        0x82//∑˛ŒÒ∆˜∑µªÿYST∫≈¬Î
#define JVN_REQ_ONLINE       0x83//÷˜øÿ«Î«Û…œœﬂ
#define JVN_RSP_ONLINE       0x84//∑˛ŒÒ∆˜∑µªÿ…œœﬂ¡Ó≈∆
#define JVN_CMD_ONLINE       0x84//÷˜øÿµÿ÷∑∏¸–¬
#define JVN_CMD_OFFLINE      0x85//÷˜øÿœ¬œﬂ
#define JVN_CMD_KEEP         0x86//÷˜øÿ±£ªÓ
#define JVN_REQ_CONNA        0x87//∑÷øÿ«Î«Û÷˜øÿµÿ÷∑ udp ±Õ£”√
#define JVN_RSP_CONNA        0x87//∑˛ŒÒ∆˜œÚ∑÷øÿ∑µªÿ÷˜øÿµÿ÷∑
#define JVN_CMD_CONNB        0x87//∑˛ŒÒ∆˜√¸¡Ó÷˜øÿœÚ∑÷øÿ¥©Õ∏
#define JVN_RSP_CONNAF       0x88//∑˛ŒÒ∆˜œÚ∑÷øÿ∑µªÿ ÷˜øÿŒ¥…œœﬂ
#define JVN_CMD_RELOGIN		 0x89//Õ®÷™÷˜øÿ÷ÿ–¬µ«¬Ω
#define JVN_CMD_CLEAR		 0x8A//Õ®÷™÷˜øÿœ¬œﬂ≤¢«Â≥˝Õ¯¬Á–≈œ¢∞¸¿®‘∆ ”Õ®∫≈¬Î
#define JVN_CMD_REGCARD		 0x8B//÷˜øÿ◊¢≤·∞Âø®–≈œ¢µΩ∑˛ŒÒ∆˜


#define JVN_CMD_CONNB2				0xB0        //∑÷øÿ«Î«Û¡¨Ω”÷˜øÿ ¥¯≤Œ ˝


#define JVN_CMD_ONLINES2     0x8C//∑˛ŒÒ∆˜√¸¡Ó÷˜øÿœÚ◊™∑¢∑˛ŒÒ∆˜…œœﬂ/÷˜øÿœÚ◊™∑¢∑˛ŒÒ∆˜…œœﬂ(Õ£”√)
#define JVN_CMD_CONNS2       0x8D//∑˛ŒÒ∆˜√¸¡Ó∑÷øÿœÚ◊™∑¢∑˛ŒÒ∆˜∑¢∆¡¨Ω”
#define JVN_REQ_S2           0x8E//∑÷øÿœÚ∑˛ŒÒ∆˜«Î«Û◊™∑¢
#define JVN_TDATA_CONN       0x8F//∑÷øÿœÚ◊™∑¢∑˛ŒÒ∆˜∑¢∆¡¨Ω”(Õ£”√)
#define JVN_TDATA_NORMAL     0x90//∑÷øÿ/÷˜øÿœÚ◊™∑¢∑˛ŒÒ∆˜∑¢ÀÕ∆’Õ® ˝æ›
#define JVN_TDATA_AOL        0x8E//÷˜øÿœÚ◊™∑¢∑˛ŒÒ∆˜…œœﬂ(–¬)
#define JVN_TDATA_BCON       0x8D//∑÷øÿœÚ◊™∑¢∑˛ŒÒ∆˜∑¢∆¡¨Ω”(–¬)

#define JVN_CMD_CARDCHECK    0x91//∞Âø®—È÷§
#define JVN_CMD_ONLINEEX     0x92//÷˜øÿµÿ÷∑∏¸–¬¿©’π
#define JVN_CMD_TCPONLINES2  0x93//∑˛ŒÒ∆˜√¸¡Ó÷˜øÿTCPœÚ◊™∑¢∑˛ŒÒ∆˜…œœﬂ
#define JVN_CMD_CHANNELCOUNT 0x97//∑÷øÿ≤È—Ø÷˜øÿÀ˘æﬂ”–µƒÕ®µ¿ ˝ƒø
#define JVN_REQ_ONLINEEX     0x9C//÷˜øÿUDP1…œœﬂ¿©’π(–¬…œœﬂ)
#define JVN_REQ_MOS2		 0x9D//3G ÷ª˙œÚ∑˛ŒÒ∆˜«Î«Û◊™∑¢
#define JVN_REQ_ONLINEEX2	 0x9E//÷˜øÿUDP1…œœﬂ‘Ÿ¥Œ¿©’π(–¬…œœﬂ),¥¯ ’ºØ–≈œ¢
#define YST_A_NEW_ADDRESS    0x100//∑÷øÿ≤È—ØNAT π”√ ÷˜øÿ∑µªÿ∑˛ŒÒ∆˜–¬µƒNAT
#define JVN_CMD_ONLINEEX2	 0x102//÷˜øÿµÿ÷∑∏¸–¬‘Ÿ¥Œ¿©’π

//---------------------------------------v2.0.0.1
#define JVN_CMD_BM           0x94//BMπ„≤•œ˚œ¢ A->B
#define JVN_CMD_TCP          0x95//Ω⁄µ„º‰TCP¡¨Ω” B->B
#define JVN_CMD_KEEPLIVE     0x96//∑÷øÿ∫Õ÷˜øÿº‰µƒ–ƒÃ¯ ˝æ›
#define JVN_CMD_PLIST        0x98//◊È≥…‘±¡–±Ì       A->B B->A
#define JVN_RSP_BMDBUSY      0x99//ªÿ∏¥ƒ≥ ˝æ›∆¨œ÷‘⁄√¶¬µ B->B A->B
#define JVN_CMD_CMD          0x9A//÷˜øÿ“™«Û∑÷øÿ÷¥––Ãÿ ‚≤Ÿ◊˜ A->B
#define JVN_CMD_ADDR         0x9B//∑÷øÿΩ⁄µ„ƒ⁄Õ‚Õ¯µÿ÷∑ A->B

#define JVN_REQ_BMD          0x9D//«Î«Ûƒ≥ ˝æ›∆¨ B->A B->B
#define JVN_RSP_BMD          0x9E//ªÿ∏¥ƒ≥ ˝æ›∆¨ A->B B->B
#define JVN_CMD_LADDR        0x9F//∑÷øÿ…œ¥´◊‘º∫µƒƒ⁄Õ¯µÿ÷∑
#define JVN_RSP_BMDNULL      0xA0//ªÿ∏¥ƒ≥ ˝æ›∆¨ ß∞‹ A->B B->B
#define JVN_CMD_TRY          0xA1//A√¸¡ÓB œÚªÔ∞È¥Ú∂¥
#define JVN_DATA_RATE        0xA2//∂‡≤•æ≠–°÷˙ ÷ ±µƒª∫≥ÂΩ¯∂»
//---------------------------------------v2.0.0.1

/*∫Û–¯¿©’π*/
#define JVN_CMD_YSTCHECK     0xAC//≤È—Øº∞∑µªÿƒ≥∫≈¬Î «∑Ò‘⁄œﬂ“‘º∞∫≈¬Î÷˜øÿSDK∞Ê±æ
#define JVN_REQ_EXCONNA      0xAD//∑÷øÿ«Î«Û÷˜øÿµÿ÷∑
#define JVN_CMD_KEEPEX       0xAE//÷˜øÿ–ƒÃ¯¿©’π(¥¯±‡◊È+±‡∫≈+ ±º‰¥¡)
#define JVN_CMD_OLCOUNT      0xAF//ªÒ»°µ±«∞∑˛ŒÒ∆˜◊‹‘⁄œﬂ ˝ƒø


#define JVN_KEEP_ACTIVE			0xB7//–¬÷˙ ÷÷Æº‰–ƒÃ¯Œ¨≥÷

/*ºÏÀ˜∑˛ŒÒ∆˜œ‡πÿ*/
#define JVN_REQ_QUERYYSTNUM		0x41	//∫≈¬Î‘⁄œﬂ∑˛ŒÒ∆˜≤È—Ø«Î«Û

/*øÕªß∞Ê¡˜√ΩÃÂ∑˛ŒÒ∆˜œ‡πÿ*/
#define JVN_REQ_CONNSTREAM_SVR    0xD0//«Î«Û¡¨Ω”¡˜√ΩÃÂ∑˛ŒÒ∆˜÷˜øÿ
#define JVN_REQ_CONNSTREAM_CLT    0xD1//«Î«Û¡¨Ω”¡˜√ΩÃÂ∑˛ŒÒ∆˜∑÷øÿ
#define JVN_RSP_CONNSTREAM_SVR    0xD2//ªÿ∏¥
#define JVN_RSP_CONNSTREAM_CLT    0xD3
#define JVN_NOTIFY_ID			  0xD4
#define JVN_RSP_ID				  0xD5
#define JVN_CMD_CONNSSERVER		  0xD6
#define JVN_RSP_NEWCLIENT         0xD9

/* ’∑—π§æﬂœ‡πÿ*/
#define TOOL_USER_LOGIN		    0xD1// ’∑—π§æﬂµ«¬º
#define TOOL_USER_CHANGE		0xD2// ’∑—π§æﬂ–ﬁ∏ƒ√‹¬Î
#define TOOL_VIP_SERCH		    0xD3// ’∑—π§æﬂ≤È—ØVIP
#define TOOL_VIP_SET			0xD4// ’∑—π§æﬂ…Ë÷√VIP
#define TOOL_VIP_RESET		    0xD5// ’∑—π§æﬂ÷ÿ–¬…Ë÷√£¨∂‘”⁄…Ë÷√≤ª≥…π¶ ± π”√
#define A_VIP_README    	    0xE0//÷˜øÿ≤È—Ø»Á∫Œø™Õ®VIP
#define A_VIP_SET			    0xE1//÷˜øÿ…Í«Îø™Õ®vip ‘”√
#define A_VIP_SERCH  			0xE2//÷˜øÿ≤È—Øvip

/*æ÷”ÚÕ¯…Ë±∏À—À˜*/
#define JVN_REQ_LANSERCH  0x01//æ÷”ÚÕ¯…Ë±∏À—À˜√¸¡Ó
#define JVN_CMD_LANSALL   1//æ÷”ÚÕ¯À—À˜À˘”–÷–Œ¨…Ë±∏
#define JVN_CMD_LANSYST   2//æ÷”ÚÕ¯À—À˜÷∏∂®‘∆ ”Õ®∫≈¬Îµƒ…Ë±∏
#define JVN_CMD_LANSTYPE  3//æ÷”ÚÕ¯À—À˜÷∏∂®ø®œµµƒ…Ë±∏
#define JVN_CMD_LANSNAME  4//æ÷”ÚÕ¯À—À˜÷∏∂®±√˚µƒ…Ë±∏
#define JVN_RSP_LANSERCH  0x02//æ÷”ÚÕ¯…Ë±∏À—À˜œÏ”¶√¸¡Ó

#define JVN_DEVICENAMELEN  100//…Ë±∏±√˚≥§∂»œﬁ÷∆

/*æ÷”ÚÕ¯π„≤•*/
#define JVN_REQ_BC  0x03//æ÷”ÚÕ¯π„≤•√¸¡Ó
#define JVN_RSP_BC  0x04//æ÷”ÚÕ¯π„≤•œÏ”¶√¸¡Ó

/*æ÷”ÚÕ¯π‹¿Ìπ§æﬂ*/
#define JVN_REQ_TOOL 0x05//π§æﬂœ˚œ¢
#define JVN_RSP_TOOL 0x06//…Ë±∏œÏ”¶

/*÷˜∑÷øÿ∂À…Ë÷√*/
#define JVN_MAXREQ        500     //∑÷øÿ«Î«Û∂”¡–µƒ◊Ó¥Û»›¡ø
#define JVN_MAXREQRUN     20      //‘ –ÌÕ¨ ±¥¶¿Ìµƒ∑÷øÿ«Î«Û ˝
#define JVNC_DATABUFLEN   819200//800K//∑÷øÿΩ” ’ ˝æ›ª∫≥Â¥Û–°150*1024
#define JVNS_DATABUFLEN   150*1024//÷˜øÿΩ” ’ ˝æ›ª∫≥Â¥Û–°
#define JVN_RESENDFRAMEB  1       //ª÷∏¥∑¢ÀÕB÷°µƒÃıº˛ 
#define JVN_NOTSENDFRAMEB ((float)1/(float)2)  //≤ª∑¢ÀÕB÷°µƒÃıº˛
#define JVN_ASPACKDEFLEN  1024    //”Î∑˛ŒÒ∆˜º‰ ˝æ›∞¸ƒ¨»œ◊Ó¥Û≥§∂»
#define JVN_BAPACKDEFLEN  25*1024 //∑÷øÿœÚ÷˜øÿ∑¢ÀÕ ˝æ›∞¸ƒ¨»œ◊Ó¥Û≥§∂»
#define JVN_ABCHECKPLEN   14      //”Î∑÷øÿº‰Œƒº˛ºÏÀ˜≤Œ ˝ ˝æ›∞¸≥§∂»

#define JVNC_PTINFO_LEN   102400

#define JVN_ABFRAMERET    25      //÷°–Ú¡–÷–√ø∏ˆ∂‡…Ÿ÷°“ª∏ˆªÿ∏¥
#define JVNC_ABFRAMERET   15      //÷°–Ú¡–÷–√ø∏ˆ∂‡…Ÿ÷°“ª∏ˆªÿ∏¥
#define JVN_ABFRAMERET_MO 35      //÷°–Ú¡–÷–√ø∏ˆ∂‡…Ÿ÷°“ª∏ˆªÿ∏¥

#define JVN_RELOGINLIMIT  30   //µÙœﬂ∫Û÷ÿ–¬…œœﬂ ß∞‹¥Œ ˝…œœﬁ
#define JVN_RUNEVENTLEN   2048 //»’÷æŒƒ±æ≥§∂»
#define JVN_RUNFILELEN    4000*1024//»’÷æŒƒº˛¥Û–°…œœﬁ

#define LINUX_THREAD_STACK_SIZE 512*1024 //linuxª∑æ≥œ¬µƒœﬂ≥Ã∂—’ª…œœﬁ

#define JVN_WEBSITE1      "www.jovetech.com"//∑˛ŒÒÕ¯’æ1
#define JVN_WEBSITE2      "www.afdvr.com"//∑˛ŒÒÕ¯’æ2
#define JVN_WEBFOLDER     "/down/YST/"//"/down/ser703/oem800"//∑˛ŒÒ∆˜¡–±ÌŒƒº˛º–
//#define JVN_WEBFILE       "/yst.txt"//"/ipsecu.txt\n"//∑˛ŒÒ∆˜¡–±ÌŒƒº˛
//#define JVN_NEWWEBFILE    "/ystnew.txt"//∑˛ŒÒ∆˜¡–±ÌŒƒº˛
#define JVN_AGENTINFO     "User-Agent:Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)\r\n"

#define JVN_WEBSITE_POS       "www.afdvr.com"//µÿ«¯∂®Œª∑˛ŒÒÕ¯’æ
#define JVN_WEB_POSREQ         "/GetPos/"
#define JVN_POS_STR            "/yst_%s.txt"
#define JVN_YSTLIST_ALL        "/yst_all.txt"      //À˘”–∑˛ŒÒ∆˜«Âµ•
#define JVN_YSTLIST_HOME       "/yst_home.txt"      //ƒ¨»œ∑˛ŒÒ∆˜«Âµ•(π˙ƒ⁄À˘”–”√ªß+”––Ë“™µƒπ˙Õ‚”√ªß)
#define JVN_YSTLIST_INDEX      "/yst_index.txt"      //ºÏÀ˜∑˛ŒÒ∆˜¡–±Ì«Âµ•

//µÿ«¯«Âµ•£∫
#define JVN_YSTLIST_USA        "/yst_usa.txt"       //√¿π˙∑˛ŒÒ∆˜(√¿π˙”√ªß) ***********”––ß
#define JVN_YSTLIST_INDIA      "/yst_india.txt"     //”°∂»∑˛ŒÒ∆˜(”°∂»”√ªß)
#define JVN_YSTLIST_SINGAPORE  "/yst_singapore.txt" //–¬º”∆¬∑˛ŒÒ∆˜(–¬º”∆¬÷‹±ﬂ”√ªß)
#define JVN_YSTLIST_SOUTHCHINA "/yst_southchina.txt"//÷–π˙ƒœ≤ø∑˛ŒÒ∆˜(÷–π˙ƒœ∑Ω”√ªß)
#define JVN_YSTLIST_WESTCHINA  "/yst_westchina.txt" //÷–π˙Œ˜≤ø∑˛ŒÒ∆˜(÷–π˙Œ˜≤ø”√ªß)
#define JVN_YSTLIST_MIDDLEEAST "/yst_middleeast.txt"//÷–∂´∑˛ŒÒ∆˜(÷–∂´”√ªß)
#define JVN_YSTLIST_AP         "/yst_ap.txt"        //—«Ã´µÿ«¯∑˛ŒÒ∆˜(—«Ã´”√ªß)


#define JVN_ALLSERVER     0//À˘”–∑˛ŒÒ
#define JVN_ONLYNET       1//÷ªæ÷”ÚÕ¯∑˛ŒÒ
#define JVN_ONLYYST       2//÷ª‘∆ ”Õ®∫≈¬Î∑˛ŒÒ

#define JVN_NOTURN        0//‘∆ ”Õ®∑Ω Ω ±Ω˚”√◊™∑¢
#define JVN_TRYTURN       1//‘∆ ”Õ®∑Ω Ω ±∆Ù”√◊™∑¢
#define JVN_ONLYTURN      2//‘∆ ”Õ®∑Ω Ω ±Ωˆ”√◊™∑¢

#define JVN_CONNTYPE_LOCAL  1//æ÷”ÚÕ¯¡¨Ω”
#define JVN_CONNTYPE_P2P    2//P2P¥©Õ∏¡¨Ω”
#define JVN_CONNTYPE_TURN   3//◊™∑¢

#define JVN_LANGUAGE_ENGLISH  1
#define JVN_LANGUAGE_CHINESE  2

#define JVN_TRANS_ONLYI   1//πÿº¸÷°◊™∑¢
#define JVN_TRANS_ALL     2//ÕÍ’˚◊™∑¢/ÕÍ’˚¥´ ‰

#define TYPE_PC_UDP      1//¡¨Ω”¿‡–Õ UDP ÷ß≥÷UDP ’∑¢ÕÍ’˚ ˝æ›
#define TYPE_PC_TCP      2//¡¨Ω”¿‡–Õ TCP ÷ß≥÷TCP ’∑¢ÕÍ’˚ ˝æ›
#define TYPE_MO_TCP      3//¡¨Ω”¿‡–Õ TCP ÷ß≥÷TCP ’∑¢ºÚµ• ˝æ›,∆’Õ® ”∆µ÷°µ»≤ª‘Ÿ∑¢ÀÕ£¨÷ªƒ‹≤…”√◊®”√Ω”ø⁄ ’∑¢ ˝æ›(  ”√”⁄ ÷ª˙º‡øÿ)
#define TYPE_MO_UDP      4//¡¨Ω”¿‡–Õ UDP 
#define TYPE_3GMO_UDP    5//¡¨Ω”¿‡–Õ 3GUDP
#define TYPE_3GMOHOME_UDP 6//¡¨Ω”¿‡–Õ 3GHOME

#define PARTNER_ADDR     1//–Ë“™∂‘∑Ωµÿ÷∑
#define PARTNER_NATTRY   2//–Ë“™∂‘∑Ω¥Ú∂¥

#define OLD_RSP_IMOLD     1//∏Ê÷™∑÷øÿŒ“ «æ…–≠“È∞Ê±æ

/*Ãÿ ‚√¸¡Ó¿‡–Õ*/
#define CMD_TYPE_CLEARBUFFER    1//÷˜øÿ∫Õ∑÷øÿ«Âø’ª∫¥Ê£¨÷ÿ–¬Ω¯––ª∫¥Ê
//NAT¿‡–Õ∂®“Â
#define NAT_TYPE_UNKNOWN		0	//Œ¥÷™¿‡–Õ ªÚ Œ¥ÃΩ≤‚≥ˆ¿¥µƒ¿‡–Õ
#define NAT_TYPE_PUBLIC			1	//π´Õ¯ 
#define NAT_TYPE_FULL_CONE		2	//Õ∏√˜
#define NAT_TYPE_IP_CONE		3	//IP ‹œﬁ
#define NAT_TYPE_PORT_CONE		4	//∂Àø⁄ ‹œﬁ
#define NAT_TYPE_SYMMETRIC	 	5	//∂‘≥∆NAT

/*‘∆Ã®øÿ÷∆¿‡–Õ*/
#define JVN_YTCTRL_U      1//…œ
#define JVN_YTCTRL_D      2//œ¬
#define JVN_YTCTRL_L      3//◊Û
#define JVN_YTCTRL_R      4//”“
#define JVN_YTCTRL_A      5//◊‘∂Ø
#define JVN_YTCTRL_GQD    6//π‚»¶¥Û
#define JVN_YTCTRL_GQX    7//π‚»¶–°
#define JVN_YTCTRL_BJD    8//±‰Ωπ¥Û
#define JVN_YTCTRL_BJX    9//±‰Ωπ–°
#define JVN_YTCTRL_BBD    10//±‰±∂¥Û
#define JVN_YTCTRL_BBX    11//±‰±∂–°

#define JVN_YTCTRL_UT     21//…œÕ£÷π
#define JVN_YTCTRL_DT     22//œ¬Õ£÷π
#define JVN_YTCTRL_LT     23//◊ÛÕ£÷π
#define JVN_YTCTRL_RT     24//”“Õ£÷π
#define JVN_YTCTRL_AT     25//◊‘∂ØÕ£÷π
#define JVN_YTCTRL_GQDT   26//π‚»¶¥ÛÕ£÷π
#define JVN_YTCTRL_GQXT   27//π‚»¶–°Õ£÷π
#define JVN_YTCTRL_BJDT   28//±‰Ωπ¥ÛÕ£÷π
#define JVN_YTCTRL_BJXT   29//±‰Ωπ–°Õ£÷π
#define JVN_YTCTRL_BBDT   30//±‰±∂¥ÛÕ£÷π
#define JVN_YTCTRL_BBXT   31//±‰±∂–°Õ£÷π
#define JVN_YTCTRL_FZ1    32//∏®÷˙1
#define JVN_YTCTRL_FZ1T   33//∏®÷˙1Õ£÷π
#define JVN_YTCTRL_FZ2    34//∏®÷˙2
#define JVN_YTCTRL_FZ2T   35//∏®÷˙2Õ£÷π
#define JVN_YTCTRL_FZ3    36//∏®÷˙3
#define JVN_YTCTRL_FZ3T   37//∏®÷˙3Õ£÷π
#define JVN_YTCTRL_FZ4    38//∏®÷˙4
#define JVN_YTCTRL_FZ4T   39//∏®÷˙4Õ£÷π

#define JVN_YTCTRL_RECSTART  41//‘∂≥Ã¬ºœÒø™ º
#define JVN_YTCTRL_RECSTOP	 42//‘∂≥Ã¬ºœÒø™ º

//ABORT CAR
#define JVN_YTCTRL_FORWORD 43
#define JVN_YTCTRL_BACK    44
#define JVN_YTCTRL_STOP    45
#define JVN_YTCTRL_START   47
#define JVN_YTCTRL_TURNLEFT 48
#define JVN_YTCTRL_TURNRIGHT 49

/*‘∂≥Ãøÿ÷∆÷∏¡Ó(÷˜∑÷øÿ”¶”√≤„‘º∂®)*/
#define RC_DISCOVER		0x01 
#define RC_GETPARAM		0x02 
#define RC_SETPARAM		0x03 
#define RC_VERITY		0x04 
#define RC_SNAPSLIST	0x05 
#define RC_GETFILE		0x06 
#define RC_USERLIST		0x07 
#define RC_PRODUCTREG	0X08 
#define RC_GETSYSTIME	0x09 
#define RC_SETSYSTIME	0x0a 
#define RC_DEVRESTORE	0x0b 
#define RC_SETPARAMOK	0x0c 
#define RC_DVRBUSY		0X0d 
#define RC_GETDEVLOG	0x0e 
#define RC_DISCOVER2	0x0f	//zwq20111206,csst‘∆ ”Õ®∫≈¬Î÷±Ω”µ«¬º£¨æ÷”ÚÕ¯π„≤•À—À˜

#define JVN_VC_BrightUp			0x10 // ”∆µµ˜Ω⁄
#define JVN_VC_BrightDown		0x11
#define JVN_VC_ContrastUp		0x12
#define JVN_VC_ContrastDown		0x13
#define JVN_VC_SaturationUp		0x14
#define JVN_VC_SaturationDown	0x15
#define JVN_VC_HueUp			0x16
#define JVN_VC_HueDown			0x17
#define JVN_VC_SharpnessUp		0x18
#define JVN_VC_SharpnessDown	0x19
#define JVN_VC_PRESENT          0x20 //‘§÷√Œªµ˜”√

#define JVN_CMD_BATCH_CHANNELNUM		0x47	//¥”ºÏÀ˜∑˛ŒÒ∆˜≈˙¡øªÒ»°Õ®µ¿ ˝

typedef struct
{
	char chGroup[4];
	int nYSTNO;
	int wChannelNum;
}CHANNEL_NUM;
typedef struct _PLAY_INFO_
{
	unsigned char ucCommand;//√¸¡Ó◊÷
	int nClientID;//∂‘”¶◊≈ªÿ∑≈

	int nConnectionType;

	char strFileName[MAX_PATH];//Œƒº˛√˚

	int nSeekPos;//∂®Œª ±–Ë“™∂®ŒªµƒŒª÷√ ÷°

}PLAY_INFO;//≤•∑≈ªÿµ˜ π”√µƒΩ·ππ

typedef struct STLANTOOLINFO 
{
	BYTE uchType;      //œ˚œ¢¿‡–Õ£¨1¿¥◊‘π§æﬂµƒπ„≤•£ª2¿¥◊‘π§æﬂµƒ≈‰÷√£ª3…Ë±∏ªÿ”¶£ª

	/*π§æﬂ–≈œ¢*/
	char chPName[256]; //”√ªß√˚£¨”√”⁄Ã·∏ﬂIPC∞≤»´–‘£¨∑¿÷π∂Ò“‚≈‰÷√
	char chPWord[256]; //√‹¬Î£¨”√”⁄Ã·∏ﬂIPC∞≤»´–‘£¨∑¿÷π∂Ò“‚≈‰÷√
	int nYSTNUM;       //‘∆ ”Õ®∫≈¬Î£¨”√”⁄π§æﬂœÚ…Ë±∏∑¢ÀÕ≈‰÷√
	char chCurTime[20];//œµÕ≥ ±º‰£¨”√”⁄π§æﬂœÚ…Ë±∏∑¢ÀÕ≈‰÷√ xxxx-xx-xx xx:xx:xx
	char *pchData;     //≈‰÷√ƒ⁄»›£¨”√”⁄π§æﬂœÚ…Ë±∏∑¢ÀÕ≈‰÷√
	int nDLen;         //≈‰÷√ƒ⁄»›≥§∂»£¨”√”⁄π§æﬂœÚ…Ë±∏∑¢ÀÕ≈‰÷√

	/*”¶¥–≈œ¢*/
	int nCardType;     //…Ë±∏¿‡–Õ£¨”√”⁄…Ë±∏”¶¥ ±µƒ∏Ωº”–≈œ¢
	int	nDate;         //≥ˆ≥ß»’∆⁄ –Œ»Á 20091011
	int	nSerial;       //≥ˆ≥ß–Ú¡–∫≈
	GUID guid;         //Œ®“ªGUID
	char chGroup[4];   //…Ë±∏±„◊È∫≈£¨”√”⁄…Ë±∏”¶¥ ±µƒ∏Ωº”–≈œ¢

	char chIP[16];
	int nPort;
	
	STLANTOOLINFO()
	{
		uchType = 0;
		memset(chPName, 0, 256);
		memset(chPWord, 0, 256);
		nYSTNUM = 0;
		memset(chCurTime, 0, 20);
		pchData = NULL;
		nDLen = 0;
		
		nCardType = 0;
		memset(chGroup, 0, 4);
		nDate = 0;
		nSerial = 0;
		memset(&guid, 0, sizeof(GUID));

		memset(chIP, 0, 16);
		nPort = 0;
	}
}STLANTOOLINFO;//æ÷”ÚÕ¯…˙≤˙π§æﬂœ˚œ¢ƒ⁄»›

typedef struct STTOOLPACK
{
	int nCardType;//≤˙∆∑¿‡–Õ(4)
	int nPNLen;//”√ªß√˚≥§(4)
	int nPWLen;//√‹¬Î≥§(4)
	BYTE uchCType;// ˝æ›¿‡–Õ(1)
	char chGroup[4];
	int nYSTNUM;//‘∆ ”Õ®∫≈¬Î(4)
	char chCurTime[20];//œµÕ≥ ±º‰(4)
	int nDLen;//≈‰÷√≥§∂»(4)
	STTOOLPACK()
	{
		nCardType = 0;
		nPNLen = 0;
		nPWLen = 0;
		uchCType = 0;
		memset(chGroup, 0, 4);
		nYSTNUM = 0;
		memset(chCurTime, 0, 20);
		nDLen = 0;
	}
}STTOOLPACK;


#define  NET_MOD_UNKNOW 0 // Œ¥√˚
#define  NET_MOD_WIFI   1 //wifi ƒ£ Ω
#define  NET_MOD_WIRED  2 // ”–œﬂƒ£ Ω

#define  DEV_SET_ALL      0 // …Ë÷√»´≤ø
#define  DEV_SET_NET      1 //…Ë÷√…Ë±∏÷ß≥÷µƒÕ¯¬Áƒ£ Ω
#define  DEV_SET_CUR_NET  2 //…Ë÷√…Ë±∏µ±«∞Õ¯¬Áƒ£ Ω
#define  DEV_SET_NAME     3 // …Ë÷√±√˚

typedef struct  
{
	char chDeviceName[100];//…Ë±∏±√˚
	int nCurNetMod;// …Ë±∏µ±«∞Õ¯¬Áƒ£ Ω£¨”–œﬂ£¨wifiªÚ∆‰À¸
	int nNetMod; //…Ë±∏÷ß≥÷µƒÕ¯¬Áƒ£ Ω£¨Œ™ º∏÷÷ƒ£ ΩªÚ∆¿¥µƒ÷µ  nNetMod = NET_MOD_WIFI;
}STDEVINFO; //…Ë±∏≤Œ ˝

#ifndef WIN32
	typedef int (*FUNC_SCHECKPASS_CALLBACK)(int nLocalChannel, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SCONNECT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SONLINE_CALLBACK)(int nLocalChannel, unsigned char uchType);
	typedef void (*FUNC_SCHECKFILE_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort, char chStartTime[14], char chEndTime[14], unsigned char *pBuffer, int &nSize);
	typedef void (*FUNC_SCHAT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, BYTE *pBuffer, int nSize);
	typedef void (*FUNC_STEXT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, BYTE *pBuffer, int nSize);
	typedef void (*FUNC_SYTCTRL_CALLBACK)(int nLocalChannel, int nClientID, int nType, char chClientIP[16], int nClientPort);
	typedef void (*FUNC_SBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef int (*FUNC_SFPLAYCTRL_CALLBACK)(PLAY_INFO* pData);
	typedef void (*FUNC_DLFNAME_CALLBACK)(char chFilePathName[256]);

	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1 «–¥ 2 «∂¡

#else
	typedef bool (*FUNC_SCHECKPASS_CALLBACK)(int nLocalChannel, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SCONNECT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, char *pName, char *pWord);
	typedef void (*FUNC_SONLINE_CALLBACK)(int nLocalChannel, unsigned char uchType);
	typedef void (*FUNC_SCHECKFILE_CALLBACK)(int nLocalChannel, int nClientID, char chClientIP[16], int nClientPort, char chStartTime[14], char chEndTime[14], unsigned char *pBuffer, int &nSize);
	typedef void (*FUNC_SCHAT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, BYTE *pBuffer, int nSize);
	typedef void (*FUNC_STEXT_CALLBACK)(int nLocalChannel, int nClientID, unsigned char uchType, char chClientIP[16], int nClientPort, BYTE *pBuffer, int nSize);
	typedef void (*FUNC_SYTCTRL_CALLBACK)(int nLocalChannel, int nClientID, int nType, char chClientIP[16], int nClientPort);
	typedef void (*FUNC_SBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], int nPort);
	typedef bool (*FUNC_SFPLAYCTRL_CALLBACK)(PLAY_INFO* pData);
	typedef void (*FUNC_DLFNAME_CALLBACK)(char chFilePathName[256]);

	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1 «–¥ 2 «∂¡
#endif

	typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 ≥…π¶ 2  ß∞‹ 3 ∂œø™ 4 “Ï≥£∂œø™
	typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

typedef int (*FUNC_LANTOOL_CALLBACK)(STLANTOOLINFO* pData);//0≤ª–Ëœ‡”¶1œÏ”¶
/*∑÷øÿªÿµ˜∫Ø ˝*/
typedef struct STBASEYSTNO
{
	char chGroup[4];
	int nYSTNO;
	int nChannel;
	char chPName[MAX_PATH];
	char chPWord[MAX_PATH];
	int nConnectStatus;//¡¨Ω”◊¥Ã¨ …Ë÷√ ±=0£¨≤È—Ø ±±Ì æ◊¥Ã¨ 0 Œ¥¡¨Ω” 1 ƒ⁄Õ¯ 2 ◊™∑¢ 3Õ‚Õ¯	
}STBASEYSTNO;//‘∆ ”Õ®∫≈¬Îª˘±æ–≈œ¢£¨”√”⁄≥ı ºªØ–°÷˙ ÷µƒ–È¡¨Ω”

typedef struct 
{
	char chGroup[4];
	int nYSTNO;
	int nCardType;
	int nChannelCount;
	char chClientIP[16];
	int nClientPort;
	int nVariety;
	char chDeviceName[100];
	#ifndef WIN32
		int bTimoOut;
	#else
		BOOL bTimoOut;
	#endif

	int nNetMod;//¿˝»Á  «∑Òæﬂ”–wifiπ¶ƒ‹: nNetMod & NET_MOD_WIFI
	int nCurMod;//¿˝»Á µ±«∞ π”√µƒ(wifiªÚ”–œﬂ)£∫nCurMod(NET_MOD_WIFI ªÚ NET_MOD_WIRED)

	int nPrivateSize;//◊‘∂®“Â ˝æ› µº ≥§∂»
	char chPrivateInfo[500];//◊‘∂®“Â ˝æ›ƒ⁄»›
}STLANSRESULT;//æ÷”ÚÕ¯…Ë±∏À—À˜Ω·π˚
typedef void (*FUNC_CCONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);
typedef void (*FUNC_CNORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight);
typedef void (*FUNC_CCHECKRESULT_CALLBACK)(int nLocalChannel,unsigned char *pBuffer, int nSize);
typedef void (*FUNC_CCHATDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
typedef void (*FUNC_CTEXTDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize);
typedef void (*FUNC_CDOWNLOAD_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nFileLen);
typedef void (*FUNC_CPLAYDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nWidth, int nHeight, int nTotalFrame);
typedef void (*FUNC_CBUFRATE_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize, int nRate);
typedef void (*FUNC_CLANSDATA_CALLBACK)(STLANSRESULT stLSResult);
typedef void (*FUNC_CBCDATA_CALLBACK)(int nBCID, unsigned char *pBuffer, int nSize, char chIP[16], BOOL bTimeOut);
typedef int (*FUNC_CLANTDATA_CALLBACK)(STLANTOOLINFO *pLANTData);

typedef int (*FUNC_DEVICE_CALLBACK)(STLANSRESULT*);


#define JVN_COMMAND_AB		0x202	//分控通过云视通服务器与主控交互数据
#define JVN_COMMAND_BA		0x208	//主控通过云视通服务器与分控交互数据

/************÷˜øÿªÿµ˜∫Ø ˝≤Œ ˝¿‡–Õ***************/
/*FUNC_SCONNECT_CALLBACK*/
#define JVN_SCONNECTTYPE_PCCONNOK  0X01//”–PC∑÷øÿ¡¨Ω”≥…π¶
#define JVN_SCONNECTTYPE_DISCONNOK 0X02//”–∑÷øÿ∂œø™¡¨Ω”≥…π¶
#define JVN_SCONNECTTYPE_DISCONNE  0X03//¡¨Ω”“Ï≥£∂œø™
#define JVN_SCONNECTTYPE_MOCONNOK  0X04//”–“∆∂Ø…Ë±∏∑÷øÿ¡¨Ω”≥…π¶
/*FUNC_SONLINE_CALLBACK*/
#define JVN_SONLINETYPE_ONLINE     0x01//…œœﬂ
#define JVN_SONLINETYPE_OFFLINE    0x02//œ¬œﬂ
#define JVN_SONLINETYPE_CLEAR      0x03//YST∫≈¬ÎŒﬁ–ß£¨”¶«Âø’÷ÿ–¬…Í«Î

/************∑÷øÿªÿµ˜∫Ø ˝≤Œ ˝¿‡–Õ***************/
/*FUNC_CCONNECT_CALLBACK*/
#define JVN_CCONNECTTYPE_CONNOK    0X01//¡¨Ω”≥…π¶
#define JVN_CCONNECTTYPE_DISOK     0X02//∂œø™¡¨Ω”≥…π¶
#define JVN_CCONNECTTYPE_RECONN    0X03//≤ª±ÿ÷ÿ∏¥¡¨Ω”
#define JVN_CCONNECTTYPE_CONNERR   0X04//¡¨Ω” ß∞‹
#define JVN_CCONNECTTYPE_NOCONN    0X05//√ª¡¨Ω”
#define JVN_CCONNECTTYPE_DISCONNE  0X06//¡¨Ω”“Ï≥£∂œø™
#define JVN_CCONNECTTYPE_SSTOP     0X07//∑˛ŒÒÕ£÷π£¨¡¨Ω”∂œø™
#define JVN_CCONNECTTYPE_DISF      0x08//∂œø™¡¨Ω” ß∞‹


/*∑µªÿ÷µ*/
#define JVN_RETURNOK    0//≥…π¶
#define JVN_PARAERROR   1//≤Œ ˝¥ÌŒÛ
#define JVN_RETURNERROR 2// ß∞‹
#define JVN_NOMEMERROR  3//ƒ⁄¥ÊªÚ”≤≈Ãø’º‰≤ª◊„

//‘ˆº”IP∂Œ£¨À—À˜æ÷”ÚÕ¯…Ë±∏ ±∫Ú”√°£
typedef struct
{
	char startip[16];//∆ ºIP
	char endip[16];//÷’÷πIP
}IPSECTION;

typedef struct 
{
	char chIP[16];
	int nPort;
	unsigned char uchStatus;// 0 1
	unsigned char uchType;
	unsigned char uchProcType;
	int nDownSpeed;// KB/s
	float fDownTotal;// M
	float fUpTotal;// M
}STPTINFO;

/*≥ı ºªØSDK ±¥´»Î…Ë±∏œ‡πÿµƒ“ª–©–≈œ¢£¨”√”⁄ƒ⁄≤ø∏˘æ›≤ªÕ¨≤˙∆∑…Ë÷√≤ªÕ¨µƒ¥¶¿Ì*/
typedef struct 
{
	int nType;//≤˙∆∑¥Û¿‡ 0∆’Õ®ƒ¨»œ≤˙∆∑£ª1∞Âø®; 2DVR; 3IPC; 4JNVR; 5NVR; 6º“”√∞Ê–°IPC;
	int nMemLimit;//ƒ⁄¥Ê≥‰◊„∂» 0∆’Õ®ƒ¨»œ≤˙∆∑£ª1ƒ⁄¥Ê≥‰◊„(Õ¯¬Á÷¡…Ÿ”–20Mø…”√)£ª2ƒ⁄¥Êª˘±æπª”√(Õ¯¬Á‘º20Mø…”√)£ª3ƒ⁄¥ÊΩÙ’≈(Õ¯¬Á–°”⁄15Mø…”√)£ª
	              //ƒ⁄¥Ê‘ –Ìµƒ«∞Ã·œ¬£¨ƒ⁄¥Ê≥‰◊„∂»‘Ω∫√£¨Õ¯¥´¥¶¿Ì–ßπ˚ª·‘Ω∫√£ª»Áπ˚≤ª»∑∂®£¨«Î÷√Œ™0£ª
}STDeviceType;

/*º§ªÓ∫≈¬Î ±µ˜”√’ﬂ¥´»ÎΩ·ππ*/
typedef struct 
{
	char chGroup[4];//∑÷◊È∫≈£¨–Œ»Á"A" "AAAA"
	int nCardType;  //ø®œµ
	int	nDate;      //≥ˆ≥ß»’∆⁄ –Œ»Á 20091011
	int	nSerial;    //≥ˆ≥ß–Ú¡–∫≈
	GUID guid;      //Œ®“ªGUID MAPIGUID.H
}STGetNum;
/*≥ı ºªØ ±µ˜”√’ﬂ¥´»ÎµƒΩ·ππ*/
typedef struct
{
    int nCardType; /*≤˙∆∑¿‡–Õ£¨¿˝»Á£∫0xE71A, 0xD800,0xB800, 960*/
    int    nDate;     //≥ˆ≥ß»’∆⁄
    int    nSerial;   //≥ˆ≥ß–Ú¡–∫≈
    GUID gLoginKey;//∞Âø®GUID
    int  nYstNum;  //‘∆ ”Õ®∫≈¬Î
    char chGroup[4];//∑÷◊È∫≈£¨–Œ»Á"A" "AAAA"
    int  nChannelCount; //÷˜øÿÕ®µ¿◊‹ ˝ƒø  //old

	DWORD dwProductType;//≤˙∆∑ÕÍ’˚¿‡–Õ/*¿˝»Á£∫0xE71A4010,0xD8006001,0xB8007001,0xc896ffff*/
    DWORD dwEncryVer; //º”√‹∞Ê±æ, 0xB567709F£∫º”√‹–æ∆¨÷–∞¸∫¨‘∆ ”Õ®∫≈¬Î£¨”– ˝æ›ø‚;0xF097765B£∫≤ª∞¸∫¨∫≈¬Î£¨”– ˝æ›ø‚;0xB56881B0:∞¸∫¨∫≈¬Î£¨Œﬁ ˝æ›ø‚;
    DWORD dwDevVer; //º”√‹–æ∆¨”≤º˛∞Ê±æ
    int   nUIVer;//÷˜øÿ∞Ê±æ
    DWORD dwOemid;//≥ßº“id
    DWORD dwUser;//º”√‹»À‘±ID
    
   
    int nMaxConnNum;      //◊Ó¥Û¡¨Ω” ˝
    int nZone;//«¯”Ú-086÷–π˙
    int nSystemType;//œµÕ≥–Õ∫≈-∏ﬂ1◊÷Ω⁄œµÕ≥¿‡–Õ(0x1:windows 0x2:linux 0x3:MacOS 0x4:∞≤◊ø 0x5:∆‰À˚)£¨µÕ3◊÷Ω⁄œµÕ≥∞Ê±æ∫≈£¨∏˜œµÕ≥∞Ê±æ∫≈œÍº˚◊¢ Õ
    /*Windows:6.1; 6.0; 5.2; 5.1; 5.0; 4.9; 4.1; 4.0; 3.1; 3.0; 2.0; 1.0 µ»
    linux :2.6; 2.4; 2.2; 2.0; 1.2; 1.1; 1.0 µ»
    MAC OS:10.7; 10.6; 10.5; 10.4; 10.3; 10.2; 10.1; 10.0 µ»;
    Android 1.1; 1.5; 1.6; 2.0; 2.1; 2.2; 2.3; 2.4; 3.0; 3.1; 3.2; 4.0; 4.1; 4.2; 4.4µ»    */
    /*¿˝»Á£∫win7: 0x103d, win XP: 0x1033; linux2.6: 0x201a, linux2.5: 0x2019; MacOS10.7: 0x306B, MacOS10.6:0x306A; Android4.0: 0x4028; Android4.4: 0x402c*/
    
    char chProducType[64];//≤˙∆∑–Õ∫≈-◊÷∑˚¥Æ
    char chDevType[16];//”≤º˛–Õ∫≈-◊÷∑˚¥Æ
}STOnline;





/* ˝æ›∞¸:º§ªÓYST∫≈¬Î*/
typedef struct
{
	int nCardType;  //ø®œµ
	int	nDate;      //≥ˆ≥ß»’∆⁄ –Œ»Á 20091011
	int	nSerial;    //≥ˆ≥ß–Ú¡–∫≈
	GUID guid;      //Œ®“ªGUID MAPIGUID.H
}Pak_GetNum;

/* ˝æ›∞¸:…œœﬂ*/
typedef struct
{
	int nCardType; //ø®œµ
	int	nDate;     //≥ˆ≥ß»’∆⁄
	int	nSerial;   //≥ˆ≥ß–Ú¡–∫≈
	GUID gLoginKey;//∞Âø®GUID
	int  nYstNum;  //‘∆ ”Õ®∫≈¬Î
}Pak_Online1; 

typedef struct
{
	int		nCardType;//∞Âø®¿‡–Õ
	int		nDate; //≥ˆ≥ß»’∆⁄
	int		nSerial; //≥ˆ≥ß–Ú¡–∫≈
	GUID	gLoginKey; //µ«¬Ω¡Ó≈∆
	int     nYstNum;   //‘∆ ”Õ®∫≈¬Î
	char    chGroup[4];//±‡◊È∫≈ ‘›≤ª π”√
	int     nNSDKVer;//÷˜øÿÕ¯¬ÁSDK∞Ê±æ(–≠“È∞Ê±æ)
	int     nChannelCount; //÷˜øÿÕ®µ¿◊‹ ˝ƒø
}Pak_Online1Ex; //YST_AS_ONLINE1   ˝æ›Ω·ππ

typedef struct
{
	int nCardType; //ø®œµ
	int    nDate;     //≥ˆ≥ß»’∆⁄
	int    nSerial;   //≥ˆ≥ß–Ú¡–∫≈
	GUID gLoginKey;//∞Âø®GUID
	int  nYstNum;  //‘∆ ”Õ®∫≈¬Î
	char chGroup[4];//∑÷◊È∫≈£¨–Œ»Á"A" "AAAA"
	int     nNSDKVer;//÷˜øÿÕ¯¬ÁSDK∞Ê±æ(–≠“È∞Ê±æ)
	int  nChannelCount; //÷˜øÿÕ®µ¿◊‹ ˝ƒø  //old
	
	DWORD dwProductType;//≤˙∆∑¿‡–Õ
	DWORD dwEncryVer; //º”√‹∞Ê±æ, 0xB567709F£∫º”√‹–æ∆¨÷–∞¸∫¨‘∆ ”Õ®∫≈¬Î£¨”– ˝æ›ø‚;0xF097765B£∫≤ª∞¸∫¨∫≈¬Î£¨”– ˝æ›ø‚;0xB56881B0:∞¸∫¨∫≈¬Î£¨Œﬁ ˝æ›ø‚;
	int nProtocolEx;//∏√Ω·ππÃÂ¿©’π
	DWORD dwDevVer; //”≤º˛∞Ê±æ
	int   nUIVer;//÷˜øÿ∞Ê±æ
	DWORD dwOemid;//≥ßº“id
	DWORD dwUser;//º”√‹»À‘±ID
	
	int nMaxConnNum;      //◊Ó¥Û¡¨Ω” ˝
	int nZone;//«¯”Ú
	int nSystemType;//œµÕ≥–Õ∫≈
	char chProducType[64];//≤˙∆∑–Õ∫≈
	char chDevType[16];//”≤º˛–Õ∫≈
}Pak_Online1Ex2; //Pak_Online1Ex2   ˝æ›Ω·ππ

/* ˝æ›∞¸:…œœﬂ”¶¥*/
typedef struct
{
	int	nIndex;     //À≥–Ú∫≈
	GUID gLoginKey; //µ«¬Ω¡Ó≈∆
}Pak_LoginKey; 

/* ˝æ›∞¸: µÿ÷∑∏¸–¬*/
typedef struct
{
	int nYstNum;       //‘∆ ”Õ®∫≈¬Î
	int	nIndex;        //À≥–Ú∫≈
	GUID gLoginKey;    //µ«¬Ω¡Ó≈∆
#ifndef WIN32
	struct sockaddr InAddress;//ƒ⁄Õ¯µÿ÷∑
#else
	SOCKADDR InAddress;//ƒ⁄Õ¯µÿ÷∑
#endif
	int nVer;          //÷˜øÿµƒ∞Ê±æ
}Pak_Online2;

/* ˝æ›∞¸: µÿ÷∑∏¸–¬*/
typedef struct
{
	int nYstNum;       //‘∆ ”Õ®∫≈¬Î
	int	nIndex;        //À≥–Ú∫≈
	GUID gLoginKey;    //µ«¬Ω¡Ó≈∆
#ifndef WIN32
	struct sockaddr InAddress;//ƒ⁄Õ¯µÿ÷∑
#else
	SOCKADDR InAddress;//ƒ⁄Õ¯µÿ÷∑
#endif
	int nVer;          //÷˜øÿµƒ∞Ê±æ
	int nTCPSerPort;   //TCP∑˛ŒÒ∂Àø⁄
}Pak_Online2Ex;

typedef struct
{
	int nYstNum;       //‘∆ ”Õ®∫≈¬Î
	int	nIndex;        //À≥–Ú∫≈
	GUID gLoginKey;    //µ«¬Ω¡Ó≈∆
#ifndef WIN32
	struct sockaddr InAddress;//ƒ⁄Õ¯µÿ÷∑
#else
	SOCKADDR InAddress;//ƒ⁄Õ¯µÿ÷∑
#endif
	int nVer;          //÷˜øÿµƒ∞Ê±æ
	int nTCPSerPort;   //TCP∑˛ŒÒ∂Àø⁄
	int nNatType;		//Õ¯¬Á¿‡–Õ
}Pak_Online2Ex2;

/* ˝æ›∞¸: œ¬œﬂ*/
typedef struct
{
	int	nYstNum;
	int	nIndex;
	GUID gLoginKey;//µ«¬Ω¡Ó≈∆
}Pak_Logout;

typedef struct
{
	unsigned int nTypeLens; //YST_AS_REGCARD	
	//æª‘ÿ ˝æ›
	int nSize;
	char *pacData;
}SOCKET_DATA_TRAN;

typedef struct 
{
	SOCKET sSocket;//Ω” ’µƒÃ◊Ω”◊÷
	int nLocal;//±æµÿ∂Àø⁄
	
	SOCKADDR_IN addrRemote;//‘∂≥Ã∂Àø⁄
	char cData[2048];// ’µΩ ˝æ›
	int nLen;// ˝æ›≥§∂»
	unsigned long time;//Ω” ’µΩµƒ ±º‰ 
}UDP_PACKAGE;//UDT∑µªÿµƒ ˝æ›∞¸

typedef ::std::vector<UDP_PACKAGE >UDP_LIST;


typedef struct 
{
    char AdapterName[MAX_PATH + 4];
    char Description[MAX_PATH + 4];
	
	char IP[30];
	char SubMask[30];
	char GateWay[30];
	
	char IpHead[20];
	int nIP3;
	
}_Adapter;
typedef ::std::vector<_Adapter> AdapterList;

typedef struct
{
	char chGroup[4];//∑÷◊È∫≈£¨–Œ»Á"A" "AAAA"
	int  nChannelCount; //÷˜øÿÕ®µ¿◊‹ ˝ƒø
	int nCardType; //ø®œµ
	int	nDate;     //≥ˆ≥ß»’∆⁄
	int	nSerial;   //≥ˆ≥ß–Ú¡–∫≈
	GUID gLoginKey;//∞Âø®GUID
	int  nYstNum;  //‘∆ ”Õ®∫≈¬Î
}STOnlineold;//¡Ÿ ±

#endif
