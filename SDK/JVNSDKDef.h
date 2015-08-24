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
/*¬†¬µ¬†¬±¬∫‚Ä°√∏√ø¬†Àù√¶‚Ä∫¬ø‚Ä°‚Äì√ï*/
#define JVN_DATA_I           0x01//¬†‚Äù‚àÜ¬µI√∑¬∞
#define JVN_DATA_B           0x02//¬†‚Äù‚àÜ¬µB√∑¬∞
#define JVN_DATA_P           0x03//¬†‚Äù‚àÜ¬µP√∑¬∞
#define JVN_DATA_A           0x04//‚Äú√ô‚àÜ¬µ
#define JVN_DATA_S           0x05//√∑¬∞‚â•Ô¨Ç¬•√Å
#define JVN_DATA_OK          0x06//¬†‚Äù‚àÜ¬µ√∑¬∞¬†‚Äô¬µŒ©¬ª‚àë¬ª≈ì
#define JVN_DATA_DANDP       0x07//≈ì¬¨‚Äò√ø¬™√ö¬™√ø‚àë‚âà¬†‚Äô¬µŒ©¬ª‚àë¬ª≈ì
#define JVN_DATA_O           0x08//‚àÜ‚Ä∞√ÄÀö‚óä‚Äò‚àÇ¬Æ‚Äú√Ç¬†Àù√¶‚Ä∫
#define JVN_DATA_SKIP        0x09//¬†‚Äù‚àÜ¬µS√∑¬∞
#define JVN_DATA_SPEED		 0x64//√∑Àú√∏√ø¬¨√é¬¨¬†
#define JVN_DATA_HEAD        0x66//¬†‚Äù‚àÜ¬µŒ©‚Äö¬¨√é√ï‚àë¬£¬®‚àè‚àö¬†Àù√¶‚Ä∫‚â•ÀÜ≈ì√∑¬µ∆í√ï¬®¬†¬±Œ©¬¥¬´√Ç√∏‚Äô¬™‚à´¬•√ä
/*¬´√é¬´√õ¬ø‚Ä°‚Äì√ï*/
#define JVN_REQ_CHECK        0x10//¬´√é¬´√õ¬¨¬∫≈ì√í¬∫√è√ÄÀú
#define JVN_REQ_DOWNLOAD     0x20//¬´√é¬´√õ¬¨¬∫≈ì√í≈ì¬¨‚Äò√ø
#define JVN_REQ_PLAY         0x30//¬´√é¬´√õ‚Äò‚àÇ‚â•√É¬™√ø‚àë‚âà
#define JVN_REQ_CHAT         0x40//¬´√é¬´√õ‚Äù√î‚Äú√ô¬°∆í√É√è
#define JVN_REQ_TEXT         0x50//¬´√é¬´√õ≈í∆í¬±√¶¬°∆í√É√è
#define JVN_REQ_CHECKPASS    0x71//¬´√é¬´√õ‚Ä¶√å‚àë‚Ä∫‚Äî√à√∑¬ß
#define JVN_REQ_RECHECK      0x13//‚Äò¬ß‚Äî√à√∑¬ß
#define JVN_REQ_RATE		 0x63//‚àë√∑√∏√ø¬´√é¬´√õ¬¨√é¬¨¬†
	
/*¬´√é¬´√õ‚àë¬µ¬™√øŒ©¬∑œÄÀö¬ø‚Ä°‚Äì√ï*/
#define JVN_RSP_CHECKDATA    0x11//¬∫√è√ÄÀúŒ©¬∑œÄÀö
#define JVN_RSP_CHECKOVER    0x12//¬∫√è√ÄÀú√ï√ç‚â•‚Ä¶
#define JVN_RSP_DOWNLOADDATA 0x21//≈ì¬¨‚Äò√ø¬†Àù√¶‚Ä∫
#define JVN_RSP_DOWNLOADOVER 0x22//≈ì¬¨‚Äò√ø¬†Àù√¶‚Ä∫√ï√ç‚â•‚Ä¶
#define JVN_RSP_DOWNLOADE    0x23//≈ì¬¨‚Äò√ø¬†Àù√¶‚Ä∫¬†√ü‚àû‚Äπ
#define JVN_RSP_PLAYDATA     0x31//¬™√ø‚àë‚âà¬†Àù√¶‚Ä∫
#define JVN_RSP_PLAYOVER     0x32//¬™√ø‚àë‚âà√ï√ç‚â•‚Ä¶
#define JVN_RSP_PLAYE        0x39//¬™√ø‚àë‚âà¬†√ü‚àû‚Äπ
#define JVN_RSP_CHATDATA     0x41//‚Äù√î‚Äú√ô¬†Àù√¶‚Ä∫
#define JVN_RSP_CHATACCEPT   0x42//√ï¬®‚Äú‚Äö‚Äù√î‚Äú√ô¬´√é¬´√õ
#define JVN_RSP_TEXTDATA     0x51//≈í∆í¬±√¶¬†Àù√¶‚Ä∫
#define JVN_RSP_TEXTACCEPT   0x52//√ï¬®‚Äú‚Äö≈í∆í¬±√¶¬´√é¬´√õ
#define JVN_RSP_CHECKPASS    0x72//‚Ä¶√å‚àë‚Ä∫‚Äî√à√∑¬ß
#define JVN_RSP_CHECKPASST   0x72//‚Ä¶√å‚àë‚Ä∫‚Äî√à√∑¬ß‚â•‚Ä¶œÄ¬∂ ≈í‚Ñ¢TCP¬±¬£¬°√ô
#define JVN_RSP_CHECKPASSF   0x73//‚Ä¶√å‚àë‚Ä∫‚Äî√à√∑¬ß¬†√ü‚àû‚Äπ ≈í‚Ñ¢TCP¬±¬£¬°√ô
#define JVN_RSP_NOSERVER     0x74//≈íÔ¨Å‚àè‚àö√ï¬Æ¬µ¬ø‚àëÀõ≈í√í
#define JVN_RSP_INVALIDTYPE  0x7A//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï≈íÔ¨Å‚Äì√ü
#define JVN_RSP_OVERLIMIT    0x7B//¬°¬®Œ©‚Äù‚â•¬®œÄÀù√∑Àú√∏√ø‚Äò¬†‚Äì√å¬µ∆í‚óä√ì¬•√õ¬†Àù∆í√∏
#define JVN_RSP_DLTIMEOUT    0x76//≈ì¬¨‚Äò√ø‚â•¬®¬†¬±
#define JVN_RSP_PLTIMEOUT    0x77//¬™√ø‚àë‚âà‚â•¬®¬†¬±
#define JVN_RSP_RECHECK      0x14//‚Äò¬ß‚Äî√à√∑¬ß
#define JVN_RSP_OLD          0x15//√¶‚Ä¶‚àû√ä√∑Àú√∏√ø¬™√ø‚àè¬•

/*‚àö¬∏¬°√ì¬ø‚Ä°‚Äì√ï*/
#define JVN_CMD_DOWNLOADSTOP 0x24//√ï¬£√∑œÄ≈ì¬¨‚Äò√ø¬†Àù√¶‚Ä∫
#define JVN_CMD_PLAYUP       0x33//√∏√èŒ©¬Ø
#define JVN_CMD_PLAYDOWN     0x34//¬¨Àù‚àë‚âà
#define JVN_CMD_PLAYDEF      0x35//‚Äò‚â†√Ä≈∏‚â§‚Ä¢‚àë‚âà
#define JVN_CMD_PLAYSTOP     0x36//√ï¬£√∑œÄ‚â§‚Ä¢‚àë‚âà
#define JVN_CMD_PLAYPAUSE    0x37//‚Äò‚Ä∫√ï¬£‚â§‚Ä¢‚àë‚âà
#define JVN_CMD_PLAYGOON     0x38//¬∫√É‚Äì¬Ø‚â§‚Ä¢‚àë‚âà
#define JVN_CMD_CHATSTOP     0x43//√ï¬£√∑œÄ‚Äù√î‚Äú√ô¬°∆í√É√è
#define JVN_CMD_PLAYSEEK     0x44//‚â§‚Ä¢‚àë‚âà‚àÇ¬Æ≈í¬™		‚àû¬•√∑¬∞‚àÇ¬Æ≈í¬™ ‚Äì√ã‚Äú‚Ñ¢‚â§≈í¬†Àù √∑¬∞¬†Àù(1~‚óä√ì¬•√õ√∑¬∞)
#define JVN_CMD_TEXTSTOP     0x53//√ï¬£√∑œÄ≈í∆í¬±√¶¬°∆í√É√è
#define JVN_CMD_YTCTRL       0x60//‚Äò‚àÜ√É¬Æ√∏√ø√∑‚àÜ
#define JVN_CMD_VIDEO        0x70//¬†¬µ¬†¬±¬∫‚Ä°√∏√ø
#define JVN_CMD_VIDEOPAUSE   0x75//‚Äò‚Ä∫√ï¬£¬†¬µ¬†¬±¬∫‚Ä°√∏√ø
#define JVN_CMD_TRYTOUCH     0x78//¬•√ö‚àÇ¬•‚àû¬∏
#define JVN_CMD_FRAMETIME    0x79//√∑¬∞‚àë¬¢√Ä√ï¬†¬±¬∫‚Ä∞¬∫‚Ä∞‚àè√ô(¬µ‚Ä¢≈í¬™ms)
#define JVN_CMD_DISCONN      0x80//‚àÇ≈ì√∏‚Ñ¢¬°¬®Œ©‚Äù
#define JVN_CMD_MOTYPE		 0x72//UDP¬†√∑¬™Àô¬ø‚Ä°‚Äì√ï ‚óä¬¢¬£‚à´¬•√Ä√∑¬µ‚Äù√é≈ì¬¨‚àö√ä‚Äú¬™¬ø‚Ä°‚Äì√ï‚àÇ¬Æ‚Äú√Ç√∑¬µ≈ì‚Ä°√ï¬®¬£¬®¬±√¶‚Äù¬∂¬±‚Äπ‚àö‚Äö¬£¬®‚Äò‚Ä∫¬±¬£‚â•√∑‚Äô‚Äö‚ÄîÀò
#define JVN_CMD_ONLYI        0x61//‚àè‚àö√ï¬Æ¬µ¬ø√∑¬™‚àë¬¢œÄ√ø¬∫¬∏√∑¬∞
#define JVN_CMD_FULL         0x62//‚àè‚àö√ï¬Æ¬µ¬ø¬™√∑‚àè¬•¬¨Àô√∑¬∞
#define JVN_CMD_ALLAUDIO	 0x65//‚Äú√ô‚àÜ¬µ¬ª¬¥‚óä‚Ñ¢‚àë¬¢

/*‚Äù√é‚Äò‚àÜ¬†‚Äù√ï¬Æ‚àëÀõ≈í√í‚àÜÀú¬µ∆íŒ©¬™¬™‚Ä¢≈ìÀö≈ì¬¢*/
#define JVN_CMD_TOUCH        0x81//√ÉŒ©‚â§‚Äö‚àû¬∏
#define JVN_REQ_ACTIVEYSTNO  0x82//√∑Àú√∏√ø¬´√é¬´√õ¬∫¬ß¬™√ìYST‚à´‚âà¬¨√é
#define JVN_RSP_YSTNO        0x82//‚àëÀõ≈í√í‚àÜÀú‚àë¬µ¬™√øYST‚à´‚âà¬¨√é
#define JVN_REQ_ONLINE       0x83//√∑Àú√∏√ø¬´√é¬´√õ‚Ä¶≈ì≈ìÔ¨Ç
#define JVN_RSP_ONLINE       0x84//‚àëÀõ≈í√í‚àÜÀú‚àë¬µ¬™√ø‚Ä¶≈ì≈ìÔ¨Ç¬°√ì‚âà‚àÜ
#define JVN_CMD_ONLINE       0x84//√∑Àú√∏√ø¬µ√ø√∑‚àë‚àè¬∏‚Äì¬¨
#define JVN_CMD_OFFLINE      0x85//√∑Àú√∏√ø≈ì¬¨≈ìÔ¨Ç
#define JVN_CMD_KEEP         0x86//√∑Àú√∏√ø¬±¬£¬™√ì
#define JVN_REQ_CONNA        0x87//‚àë√∑√∏√ø¬´√é¬´√õ√∑Àú√∏√ø¬µ√ø√∑‚àë udp¬†¬±√ï¬£‚Äù‚àö
#define JVN_RSP_CONNA        0x87//‚àëÀõ≈í√í‚àÜÀú≈ì√ö‚àë√∑√∏√ø‚àë¬µ¬™√ø√∑Àú√∏√ø¬µ√ø√∑‚àë
#define JVN_CMD_CONNB        0x87//‚àëÀõ≈í√í‚àÜÀú‚àö¬∏¬°√ì√∑Àú√∏√ø≈ì√ö‚àë√∑√∏√ø¬•¬©√ï‚àè
#define JVN_RSP_CONNAF       0x88//‚àëÀõ≈í√í‚àÜÀú≈ì√ö‚àë√∑√∏√ø‚àë¬µ¬™√ø √∑Àú√∏√ø≈í¬•‚Ä¶≈ì≈ìÔ¨Ç
#define JVN_CMD_RELOGIN		 0x89//√ï¬Æ√∑‚Ñ¢√∑Àú√∏√ø√∑√ø‚Äì¬¨¬µ¬´¬¨Œ©
#define JVN_CMD_CLEAR		 0x8A//√ï¬Æ√∑‚Ñ¢√∑Àú√∏√ø≈ì¬¨≈ìÔ¨Ç‚â§¬¢¬´√Ç‚â•Àù√ï¬Ø¬¨√Å‚Äì‚âà≈ì¬¢‚àû¬∏¬ø¬Æ‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
#define JVN_CMD_REGCARD		 0x8B//√∑Àú√∏√ø‚óä¬¢‚â§¬∑‚àû√Ç√∏¬Æ‚Äì‚âà≈ì¬¢¬µŒ©‚àëÀõ≈í√í‚àÜÀú


#define JVN_CMD_CONNB2				0xB0        //‚àë√∑√∏√ø¬´√é¬´√õ¬°¬®Œ©‚Äù√∑Àú√∏√ø ¬•¬Ø‚â§≈í¬†Àù


#define JVN_CMD_ONLINES2     0x8C//‚àëÀõ≈í√í‚àÜÀú‚àö¬∏¬°√ì√∑Àú√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚Ä¶≈ì≈ìÔ¨Ç/√∑Àú√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚Ä¶≈ì≈ìÔ¨Ç(√ï¬£‚Äù‚àö)
#define JVN_CMD_CONNS2       0x8D//‚àëÀõ≈í√í‚àÜÀú‚àö¬∏¬°√ì‚àë√∑√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚àë¬¢‚àÜÔ£ø¬°¬®Œ©‚Äù
#define JVN_REQ_S2           0x8E//‚àë√∑√∏√ø≈ì√ö‚àëÀõ≈í√í‚àÜÀú¬´√é¬´√õ‚óä‚Ñ¢‚àë¬¢
#define JVN_TDATA_CONN       0x8F//‚àë√∑√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚àë¬¢‚àÜÔ£ø¬°¬®Œ©‚Äù(√ï¬£‚Äù‚àö)
#define JVN_TDATA_NORMAL     0x90//‚àë√∑√∏√ø/√∑Àú√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚àë¬¢√Ä√ï‚àÜ‚Äô√ï¬Æ¬†Àù√¶‚Ä∫
#define JVN_TDATA_AOL        0x8E//√∑Àú√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚Ä¶≈ì≈ìÔ¨Ç(‚Äì¬¨)
#define JVN_TDATA_BCON       0x8D//‚àë√∑√∏√ø≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚àë¬¢‚àÜÔ£ø¬°¬®Œ©‚Äù(‚Äì¬¨)

#define JVN_CMD_CARDCHECK    0x91//‚àû√Ç√∏¬Æ‚Äî√à√∑¬ß
#define JVN_CMD_ONLINEEX     0x92//√∑Àú√∏√ø¬µ√ø√∑‚àë‚àè¬∏‚Äì¬¨¬ø¬©‚ÄôœÄ
#define JVN_CMD_TCPONLINES2  0x93//‚àëÀõ≈í√í‚àÜÀú‚àö¬∏¬°√ì√∑Àú√∏√øTCP≈ì√ö‚óä‚Ñ¢‚àë¬¢‚àëÀõ≈í√í‚àÜÀú‚Ä¶≈ì≈ìÔ¨Ç
#define JVN_CMD_CHANNELCOUNT 0x97//‚àë√∑√∏√ø‚â§√à‚Äî√ò√∑Àú√∏√ø√ÄÀò√¶Ô¨Ç‚Äù‚Äì¬µ∆í√ï¬Æ¬µ¬ø¬†Àù∆í√∏
#define JVN_REQ_ONLINEEX     0x9C//√∑Àú√∏√øUDP1‚Ä¶≈ì≈ìÔ¨Ç¬ø¬©‚ÄôœÄ(‚Äì¬¨‚Ä¶≈ì≈ìÔ¨Ç)
#define JVN_REQ_MOS2		 0x9D//3G¬†√∑¬™Àô≈ì√ö‚àëÀõ≈í√í‚àÜÀú¬´√é¬´√õ‚óä‚Ñ¢‚àë¬¢
#define JVN_REQ_ONLINEEX2	 0x9E//√∑Àú√∏√øUDP1‚Ä¶≈ì≈ìÔ¨Ç‚Äò≈∏¬•≈í¬ø¬©‚ÄôœÄ(‚Äì¬¨‚Ä¶≈ì≈ìÔ¨Ç),¬•¬Ø¬†‚Äô¬∫√ò‚Äì‚âà≈ì¬¢
#define YST_A_NEW_ADDRESS    0x100//‚àë√∑√∏√ø‚â§√à‚Äî√òNAT¬†œÄ‚Äù‚àö √∑Àú√∏√ø‚àë¬µ¬™√ø‚àëÀõ≈í√í‚àÜÀú‚Äì¬¨¬µ∆íNAT
#define JVN_CMD_ONLINEEX2	 0x102//√∑Àú√∏√ø¬µ√ø√∑‚àë‚àè¬∏‚Äì¬¨‚Äò≈∏¬•≈í¬ø¬©‚ÄôœÄ

//---------------------------------------v2.0.0.1
#define JVN_CMD_BM           0x94//BMœÄ‚Äû‚â§‚Ä¢≈ìÀö≈ì¬¢ A->B
#define JVN_CMD_TCP          0x95//Œ©‚ÅÑ¬µ‚Äû¬∫‚Ä∞TCP¬°¬®Œ©‚Äù B->B
#define JVN_CMD_KEEPLIVE     0x96//‚àë√∑√∏√ø‚à´√ï√∑Àú√∏√ø¬∫‚Ä∞¬µ∆í‚Äì∆í√É¬Ø¬†Àù√¶‚Ä∫
#define JVN_CMD_PLIST        0x98//‚óä√à‚â•‚Ä¶‚Äò¬±¬°‚Äì¬±√å       A->B B->A
#define JVN_RSP_BMDBUSY      0x99//¬™√ø‚àè¬•∆í‚â•¬†Àù√¶‚Ä∫‚àÜ¬®≈ì√∑‚Äò‚ÅÑ‚àö¬∂¬¨¬µ B->B A->B
#define JVN_CMD_CMD          0x9A//√∑Àú√∏√ø‚Äú‚Ñ¢¬´√õ‚àë√∑√∏√ø√∑¬•‚Äì‚Äì√É√ø¬†‚Äö‚â§≈∏‚óäÀú A->B
#define JVN_CMD_ADDR         0x9B//‚àë√∑√∏√øŒ©‚ÅÑ¬µ‚Äû∆í‚ÅÑ√ï‚Äö√ï¬Ø¬µ√ø√∑‚àë A->B

#define JVN_REQ_BMD          0x9D//¬´√é¬´√õ∆í‚â•¬†Àù√¶‚Ä∫‚àÜ¬® B->A B->B
#define JVN_RSP_BMD          0x9E//¬™√ø‚àè¬•∆í‚â•¬†Àù√¶‚Ä∫‚àÜ¬® A->B B->B
#define JVN_CMD_LADDR        0x9F//‚àë√∑√∏√ø‚Ä¶≈ì¬•¬¥‚óä‚Äò¬∫‚à´¬µ∆í∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#define JVN_RSP_BMDNULL      0xA0//¬™√ø‚àè¬•∆í‚â•¬†Àù√¶‚Ä∫‚àÜ¬®¬†√ü‚àû‚Äπ A->B B->B
#define JVN_CMD_TRY          0xA1//A‚àö¬∏¬°√ìB ≈ì√ö¬™√î‚àû√à¬•√ö‚àÇ¬•
#define JVN_DATA_RATE        0xA2//‚àÇ‚Ä°‚â§‚Ä¢√¶‚â†‚Äì¬∞√∑Àô¬†√∑¬†¬±¬µ∆í¬™‚à´‚â•√ÇŒ©¬Ø‚àÇ¬ª
//---------------------------------------v2.0.0.1

/*‚à´√õ‚Äì¬Ø¬ø¬©‚ÄôœÄ*/
#define JVN_CMD_YSTCHECK     0xAC//‚â§√à‚Äî√ò¬∫‚àû‚àë¬µ¬™√ø∆í‚â•‚à´‚âà¬¨√é¬†¬´‚àë√í‚Äò‚ÅÑ≈ìÔ¨Ç‚Äú‚Äò¬∫‚àû‚à´‚âà¬¨√é√∑Àú√∏√øSDK‚àû√ä¬±√¶
#define JVN_REQ_EXCONNA      0xAD//‚àë√∑√∏√ø¬´√é¬´√õ√∑Àú√∏√ø¬µ√ø√∑‚àë
#define JVN_CMD_KEEPEX       0xAE//√∑Àú√∏√ø‚Äì∆í√É¬Ø¬ø¬©‚ÄôœÄ(¬•¬Ø¬±‚Ä°‚óä√à+¬±‚Ä°‚à´‚âà+¬†¬±¬∫‚Ä∞¬•¬°)
#define JVN_CMD_OLCOUNT      0xAF//¬™√í¬ª¬∞¬µ¬±¬´‚àû‚àëÀõ≈í√í‚àÜÀú‚óä‚Äπ‚Äò‚ÅÑ≈ìÔ¨Ç¬†Àù∆í√∏


#define JVN_KEEP_ACTIVE			0xB7//‚Äì¬¨√∑Àô¬†√∑√∑√Ü¬∫‚Ä∞‚Äì∆í√É¬Ø≈í¬®‚â•√∑

/*¬∫√è√ÄÀú‚àëÀõ≈í√í‚àÜÀú≈ì‚Ä°œÄ√ø*/
#define JVN_REQ_QUERYYSTNUM		0x41	//‚à´‚âà¬¨√é‚Äò‚ÅÑ≈ìÔ¨Ç‚àëÀõ≈í√í‚àÜÀú‚â§√à‚Äî√ò¬´√é¬´√õ

/*√∏√ï¬™√ü‚àû√ä¬°Àú‚àöŒ©√É√Ç‚àëÀõ≈í√í‚àÜÀú≈ì‚Ä°œÄ√ø*/
#define JVN_REQ_CONNSTREAM_SVR    0xD0//¬´√é¬´√õ¬°¬®Œ©‚Äù¬°Àú‚àöŒ©√É√Ç‚àëÀõ≈í√í‚àÜÀú√∑Àú√∏√ø
#define JVN_REQ_CONNSTREAM_CLT    0xD1//¬´√é¬´√õ¬°¬®Œ©‚Äù¬°Àú‚àöŒ©√É√Ç‚àëÀõ≈í√í‚àÜÀú‚àë√∑√∏√ø
#define JVN_RSP_CONNSTREAM_SVR    0xD2//¬™√ø‚àè¬•
#define JVN_RSP_CONNSTREAM_CLT    0xD3
#define JVN_NOTIFY_ID			  0xD4
#define JVN_RSP_ID				  0xD5
#define JVN_CMD_CONNSSERVER		  0xD6
#define JVN_RSP_NEWCLIENT         0xD9

/*¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç≈ì‚Ä°œÄ√ø*/
#define TOOL_USER_LOGIN		    0xD1//¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç¬µ¬´¬¨¬∫
#define TOOL_USER_CHANGE		0xD2//¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç‚ÄìÔ¨Å‚àè∆í‚àö‚Äπ¬¨√é
#define TOOL_VIP_SERCH		    0xD3//¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç‚â§√à‚Äî√òVIP
#define TOOL_VIP_SET			0xD4//¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç‚Ä¶√ã√∑‚àöVIP
#define TOOL_VIP_RESET		    0xD5//¬†‚Äô‚àë‚ÄîœÄ¬ß√¶Ô¨Ç√∑√ø‚Äì¬¨‚Ä¶√ã√∑‚àö¬£¬®‚àÇ‚Äò‚Äù‚ÅÑ‚Ä¶√ã√∑‚àö‚â§¬™‚â•‚Ä¶œÄ¬∂¬†¬±¬†œÄ‚Äù‚àö
#define A_VIP_README    	    0xE0//√∑Àú√∏√ø‚â§√à‚Äî√ò¬ª√Å‚à´≈í√∏‚Ñ¢√ï¬ÆVIP
#define A_VIP_SET			    0xE1//√∑Àú√∏√ø‚Ä¶√ç¬´√é√∏‚Ñ¢√ï¬Ævip¬†‚Äò‚Äù‚àö
#define A_VIP_SERCH  			0xE2//√∑Àú√∏√ø‚â§√à‚Äî√òvip

/*√¶√∑‚Äù√ö√ï¬Ø‚Ä¶√ã¬±‚àè√Ä‚Äî√ÄÀú*/
#define JVN_REQ_LANSERCH  0x01//√¶√∑‚Äù√ö√ï¬Ø‚Ä¶√ã¬±‚àè√Ä‚Äî√ÄÀú‚àö¬∏¬°√ì
#define JVN_CMD_LANSALL   1//√¶√∑‚Äù√ö√ï¬Ø√Ä‚Äî√ÄÀú√ÄÀò‚Äù‚Äì√∑‚Äì≈í¬®‚Ä¶√ã¬±‚àè
#define JVN_CMD_LANSYST   2//√¶√∑‚Äù√ö√ï¬Ø√Ä‚Äî√ÄÀú√∑‚àè‚àÇ¬Æ‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é¬µ∆í‚Ä¶√ã¬±‚àè
#define JVN_CMD_LANSTYPE  3//√¶√∑‚Äù√ö√ï¬Ø√Ä‚Äî√ÄÀú√∑‚àè‚àÇ¬Æ√∏¬Æ≈ì¬µ¬µ∆í‚Ä¶√ã¬±‚àè
#define JVN_CMD_LANSNAME  4//√¶√∑‚Äù√ö√ï¬Ø√Ä‚Äî√ÄÀú√∑‚àè‚àÇ¬Æ¬±Ô£ø‚àöÀö¬µ∆í‚Ä¶√ã¬±‚àè
#define JVN_RSP_LANSERCH  0x02//√¶√∑‚Äù√ö√ï¬Ø‚Ä¶√ã¬±‚àè√Ä‚Äî√ÄÀú≈ì√è‚Äù¬∂‚àö¬∏¬°√ì

#define JVN_DEVICENAMELEN  100//‚Ä¶√ã¬±‚àè¬±Ô£ø‚àöÀö‚â•¬ß‚àÇ¬ª≈ìÔ¨Å√∑‚àÜ

/*√¶√∑‚Äù√ö√ï¬ØœÄ‚Äû‚â§‚Ä¢*/
#define JVN_REQ_BC  0x03//√¶√∑‚Äù√ö√ï¬ØœÄ‚Äû‚â§‚Ä¢‚àö¬∏¬°√ì
#define JVN_RSP_BC  0x04//√¶√∑‚Äù√ö√ï¬ØœÄ‚Äû‚â§‚Ä¢≈ì√è‚Äù¬∂‚àö¬∏¬°√ì

/*√¶√∑‚Äù√ö√ï¬ØœÄ‚Äπ¬ø√åœÄ¬ß√¶Ô¨Ç*/
#define JVN_REQ_TOOL 0x05//œÄ¬ß√¶Ô¨Ç≈ìÀö≈ì¬¢
#define JVN_RSP_TOOL 0x06//‚Ä¶√ã¬±‚àè≈ì√è‚Äù¬∂

/*√∑Àú‚àë√∑√∏√ø‚àÇ√Ä‚Ä¶√ã√∑‚àö*/
#define JVN_MAXREQ        500     //‚àë√∑√∏√ø¬´√é¬´√õ‚àÇ‚Äù¬°‚Äì¬µ∆í‚óä√ì¬•√õ¬ª‚Ä∫¬°√∏
#define JVN_MAXREQRUN     20      //‚Äò¬†‚Äì√å√ï¬®¬†¬±¬•¬∂¬ø√å¬µ∆í‚àë√∑√∏√ø¬´√é¬´√õ¬†Àù
#define JVNC_DATABUFLEN   819200//800K//‚àë√∑√∏√øŒ©‚Äù¬†‚Äô¬†Àù√¶‚Ä∫¬™‚à´‚â•√Ç¬•√õ‚Äì¬∞150*1024
#define JVNS_DATABUFLEN   150*1024//√∑Àú√∏√øŒ©‚Äù¬†‚Äô¬†Àù√¶‚Ä∫¬™‚à´‚â•√Ç¬•√õ‚Äì¬∞
#define JVN_RESENDFRAMEB  1       //¬™√∑‚àè¬•‚àë¬¢√Ä√ïB√∑¬∞¬µ∆í√Éƒ±¬∫Àõ 
#define JVN_NOTSENDFRAMEB ((float)1/(float)2)  //‚â§¬™‚àë¬¢√Ä√ïB√∑¬∞¬µ∆í√Éƒ±¬∫Àõ
#define JVN_ASPACKDEFLEN  1024    //‚Äù√é‚àëÀõ≈í√í‚àÜÀú¬∫‚Ä∞¬†Àù√¶‚Ä∫‚àû¬∏∆í¬®¬ª≈ì‚óä√ì¬•√õ‚â•¬ß‚àÇ¬ª
#define JVN_BAPACKDEFLEN  25*1024 //‚àë√∑√∏√ø≈ì√ö√∑Àú√∏√ø‚àë¬¢√Ä√ï¬†Àù√¶‚Ä∫‚àû¬∏∆í¬®¬ª≈ì‚óä√ì¬•√õ‚â•¬ß‚àÇ¬ª
#define JVN_ABCHECKPLEN   14      //‚Äù√é‚àë√∑√∏√ø¬∫‚Ä∞≈í∆í¬∫Àõ¬∫√è√ÄÀú‚â§≈í¬†Àù¬†Àù√¶‚Ä∫‚àû¬∏‚â•¬ß‚àÇ¬ª

#define JVNC_PTINFO_LEN   102400

#define JVN_ABFRAMERET    25      //√∑¬∞‚Äì√ö¬°‚Äì√∑‚Äì‚àö√∏‚àèÀÜ‚àÇ‚Ä°‚Ä¶≈∏√∑¬∞‚Äú¬™‚àèÀÜ¬™√ø‚àè¬•
#define JVNC_ABFRAMERET   15      //√∑¬∞‚Äì√ö¬°‚Äì√∑‚Äì‚àö√∏‚àèÀÜ‚àÇ‚Ä°‚Ä¶≈∏√∑¬∞‚Äú¬™‚àèÀÜ¬™√ø‚àè¬•
#define JVN_ABFRAMERET_MO 35      //√∑¬∞‚Äì√ö¬°‚Äì√∑‚Äì‚àö√∏‚àèÀÜ‚àÇ‚Ä°‚Ä¶≈∏√∑¬∞‚Äú¬™‚àèÀÜ¬™√ø‚àè¬•

#define JVN_RELOGINLIMIT  30   //¬µ√ô≈ìÔ¨Ç‚à´√õ√∑√ø‚Äì¬¨‚Ä¶≈ì≈ìÔ¨Ç¬†√ü‚àû‚Äπ¬•≈í¬†Àù‚Ä¶≈ì≈ìÔ¨Å
#define JVN_RUNEVENTLEN   2048 //¬ª‚Äô√∑√¶≈í∆í¬±√¶‚â•¬ß‚àÇ¬ª
#define JVN_RUNFILELEN    4000*1024//¬ª‚Äô√∑√¶≈í∆í¬∫Àõ¬•√õ‚Äì¬∞‚Ä¶≈ì≈ìÔ¨Å

#define LINUX_THREAD_STACK_SIZE 512*1024 //linux¬™‚àë√¶‚â•≈ì¬¨¬µ∆í≈ìÔ¨Ç‚â•√É‚àÇ‚Äî‚Äô¬™‚Ä¶≈ì≈ìÔ¨Å

#define JVN_WEBSITE1      "www.jovetech.com"//‚àëÀõ≈í√í√ï¬Ø‚Äô√¶1
#define JVN_WEBSITE2      "www.afdvr.com"//‚àëÀõ≈í√í√ï¬Ø‚Äô√¶2
#define JVN_WEBFOLDER     "/down/YST/"//"/down/ser703/oem800"//‚àëÀõ≈í√í‚àÜÀú¬°‚Äì¬±√å≈í∆í¬∫Àõ¬∫‚Äì
//#define JVN_WEBFILE       "/yst.txt"//"/ipsecu.txt\n"//‚àëÀõ≈í√í‚àÜÀú¬°‚Äì¬±√å≈í∆í¬∫Àõ
//#define JVN_NEWWEBFILE    "/ystnew.txt"//‚àëÀõ≈í√í‚àÜÀú¬°‚Äì¬±√å≈í∆í¬∫Àõ
#define JVN_AGENTINFO     "User-Agent:Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)\r\n"

#define JVN_WEBSITE_POS       "www.afdvr.com"//¬µ√ø¬´¬Ø‚àÇ¬Æ≈í¬™‚àëÀõ≈í√í√ï¬Ø‚Äô√¶
#define JVN_WEB_POSREQ         "/GetPos/"
#define JVN_POS_STR            "/yst_%s.txt"
#define JVN_YSTLIST_ALL        "/yst_all.txt"      //√ÄÀò‚Äù‚Äì‚àëÀõ≈í√í‚àÜÀú¬´√Ç¬µ‚Ä¢
#define JVN_YSTLIST_HOME       "/yst_home.txt"      //∆í¬®¬ª≈ì‚àëÀõ≈í√í‚àÜÀú¬´√Ç¬µ‚Ä¢(œÄÀô∆í‚ÅÑ√ÄÀò‚Äù‚Äì‚Äù‚àö¬™√ü+‚Äù‚Äì‚Äì√ã‚Äú‚Ñ¢¬µ∆íœÄÀô√ï‚Äö‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_INDEX      "/yst_index.txt"      //¬∫√è√ÄÀú‚àëÀõ≈í√í‚àÜÀú¬°‚Äì¬±√å¬´√Ç¬µ‚Ä¢

//¬µ√ø¬´¬Ø¬´√Ç¬µ‚Ä¢¬£‚à´
#define JVN_YSTLIST_USA        "/yst_usa.txt"       //‚àö¬øœÄÀô‚àëÀõ≈í√í‚àÜÀú(‚àö¬øœÄÀô‚Äù‚àö¬™√ü) ***********‚Äù‚Äì‚Äì√ü
#define JVN_YSTLIST_INDIA      "/yst_india.txt"     //‚Äù¬∞‚àÇ¬ª‚àëÀõ≈í√í‚àÜÀú(‚Äù¬∞‚àÇ¬ª‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_SINGAPORE  "/yst_singapore.txt" //‚Äì¬¨¬∫‚Äù‚àÜ¬¨‚àëÀõ≈í√í‚àÜÀú(‚Äì¬¨¬∫‚Äù‚àÜ¬¨√∑‚Äπ¬±Ô¨Ç‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_SOUTHCHINA "/yst_southchina.txt"//√∑‚ÄìœÄÀô∆í≈ì‚â§√∏‚àëÀõ≈í√í‚àÜÀú(√∑‚ÄìœÄÀô∆í≈ì‚àëŒ©‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_WESTCHINA  "/yst_westchina.txt" //√∑‚ÄìœÄÀô≈íÀú‚â§√∏‚àëÀõ≈í√í‚àÜÀú(√∑‚ÄìœÄÀô≈íÀú‚â§√∏‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_MIDDLEEAST "/yst_middleeast.txt"//√∑‚Äì‚àÇ¬¥‚àëÀõ≈í√í‚àÜÀú(√∑‚Äì‚àÇ¬¥‚Äù‚àö¬™√ü)
#define JVN_YSTLIST_AP         "/yst_ap.txt"        //‚Äî¬´√É¬¥¬µ√ø¬´¬Ø‚àëÀõ≈í√í‚àÜÀú(‚Äî¬´√É¬¥‚Äù‚àö¬™√ü)


#define JVN_ALLSERVER     0//√ÄÀò‚Äù‚Äì‚àëÀõ≈í√í
#define JVN_ONLYNET       1//√∑¬™√¶√∑‚Äù√ö√ï¬Ø‚àëÀõ≈í√í
#define JVN_ONLYYST       2//√∑¬™‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é‚àëÀõ≈í√í

#define JVN_NOTURN        0//‚Äò‚àÜ¬†‚Äù√ï¬Æ‚àëŒ©¬†Œ©¬†¬±Œ©Àö‚Äù‚àö‚óä‚Ñ¢‚àë¬¢
#define JVN_TRYTURN       1//‚Äò‚àÜ¬†‚Äù√ï¬Æ‚àëŒ©¬†Œ©¬†¬±‚àÜ√ô‚Äù‚àö‚óä‚Ñ¢‚àë¬¢
#define JVN_ONLYTURN      2//‚Äò‚àÜ¬†‚Äù√ï¬Æ‚àëŒ©¬†Œ©¬†¬±Œ©ÀÜ‚Äù‚àö‚óä‚Ñ¢‚àë¬¢

#define JVN_CONNTYPE_LOCAL  1//√¶√∑‚Äù√ö√ï¬Ø¬°¬®Œ©‚Äù
#define JVN_CONNTYPE_P2P    2//P2P¬•¬©√ï‚àè¬°¬®Œ©‚Äù
#define JVN_CONNTYPE_TURN   3//‚óä‚Ñ¢‚àë¬¢

#define JVN_LANGUAGE_ENGLISH  1
#define JVN_LANGUAGE_CHINESE  2

#define JVN_TRANS_ONLYI   1//œÄ√ø¬∫¬∏√∑¬∞‚óä‚Ñ¢‚àë¬¢
#define JVN_TRANS_ALL     2//√ï√ç‚ÄôÀö‚óä‚Ñ¢‚àë¬¢/√ï√ç‚ÄôÀö¬•¬¥¬†‚Ä∞

#define TYPE_PC_UDP      1//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï UDP √∑√ü‚â•√∑UDP¬†‚Äô‚àë¬¢√ï√ç‚ÄôÀö¬†Àù√¶‚Ä∫
#define TYPE_PC_TCP      2//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï TCP √∑√ü‚â•√∑TCP¬†‚Äô‚àë¬¢√ï√ç‚ÄôÀö¬†Àù√¶‚Ä∫
#define TYPE_MO_TCP      3//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï TCP √∑√ü‚â•√∑TCP¬†‚Äô‚àë¬¢¬∫√ö¬µ‚Ä¢¬†Àù√¶‚Ä∫,‚àÜ‚Äô√ï¬Æ¬†‚Äù‚àÜ¬µ√∑¬∞¬µ¬ª‚â§¬™‚Äò≈∏‚àë¬¢√Ä√ï¬£¬®√∑¬™∆í‚Äπ‚â§‚Ä¶‚Äù‚àö‚óä¬Æ‚Äù‚àöŒ©‚Äù√∏‚ÅÑ¬†‚Äô‚àë¬¢¬†Àù√¶‚Ä∫(¬†¬†‚Äù‚àö‚Äù‚ÅÑ¬†√∑¬™Àô¬∫‚Ä°√∏√ø)
#define TYPE_MO_UDP      4//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï UDP 
#define TYPE_3GMO_UDP    5//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï 3GUDP
#define TYPE_3GMOHOME_UDP 6//¬°¬®Œ©‚Äù¬ø‚Ä°‚Äì√ï 3GHOME

#define PARTNER_ADDR     1//‚Äì√ã‚Äú‚Ñ¢‚àÇ‚Äò‚àëŒ©¬µ√ø√∑‚àë
#define PARTNER_NATTRY   2//‚Äì√ã‚Äú‚Ñ¢‚àÇ‚Äò‚àëŒ©¬•√ö‚àÇ¬•

#define OLD_RSP_IMOLD     1//‚àè√ä√∑‚Ñ¢‚àë√∑√∏√ø≈í‚Äú¬†¬´√¶‚Ä¶‚Äì‚â†‚Äú√à‚àû√ä¬±√¶

/*√É√ø¬†‚Äö‚àö¬∏¬°√ì¬ø‚Ä°‚Äì√ï*/
#define CMD_TYPE_CLEARBUFFER    1//√∑Àú√∏√ø‚à´√ï‚àë√∑√∏√ø¬´√Ç√∏‚Äô¬™‚à´¬•√ä¬£¬®√∑√ø‚Äì¬¨Œ©¬Ø‚Äì‚Äì¬™‚à´¬•√ä
//NAT¬ø‚Ä°‚Äì√ï‚àÇ¬Æ‚Äú√Ç
#define NAT_TYPE_UNKNOWN		0	//≈í¬•√∑‚Ñ¢¬ø‚Ä°‚Äì√ï ¬™√ö ≈í¬•√ÉŒ©‚â§‚Äö‚â•ÀÜ¬ø¬•¬µ∆í¬ø‚Ä°‚Äì√ï
#define NAT_TYPE_PUBLIC			1	//œÄ¬¥√ï¬Ø 
#define NAT_TYPE_FULL_CONE		2	//√ï‚àè‚àöÀú
#define NAT_TYPE_IP_CONE		3	//IP¬†‚Äπ≈ìÔ¨Å
#define NAT_TYPE_PORT_CONE		4	//‚àÇ√Ä√∏‚ÅÑ¬†‚Äπ≈ìÔ¨Å
#define NAT_TYPE_SYMMETRIC	 	5	//‚àÇ‚Äò‚â•‚àÜNAT

/*‚Äò‚àÜ√É¬Æ√∏√ø√∑‚àÜ¬ø‚Ä°‚Äì√ï*/
#define JVN_YTCTRL_U      1//‚Ä¶≈ì
#define JVN_YTCTRL_D      2//≈ì¬¨
#define JVN_YTCTRL_L      3//‚óä√õ
#define JVN_YTCTRL_R      4//‚Äù‚Äú
#define JVN_YTCTRL_A      5//‚óä‚Äò‚àÇ√ò
#define JVN_YTCTRL_GQD    6//œÄ‚Äö¬ª¬∂¬•√õ
#define JVN_YTCTRL_GQX    7//œÄ‚Äö¬ª¬∂‚Äì¬∞
#define JVN_YTCTRL_BJD    8//¬±‚Ä∞Œ©œÄ¬•√õ
#define JVN_YTCTRL_BJX    9//¬±‚Ä∞Œ©œÄ‚Äì¬∞
#define JVN_YTCTRL_BBD    10//¬±‚Ä∞¬±‚àÇ¬•√õ
#define JVN_YTCTRL_BBX    11//¬±‚Ä∞¬±‚àÇ‚Äì¬∞

#define JVN_YTCTRL_UT     21//‚Ä¶≈ì√ï¬£√∑œÄ
#define JVN_YTCTRL_DT     22//≈ì¬¨√ï¬£√∑œÄ
#define JVN_YTCTRL_LT     23//‚óä√õ√ï¬£√∑œÄ
#define JVN_YTCTRL_RT     24//‚Äù‚Äú√ï¬£√∑œÄ
#define JVN_YTCTRL_AT     25//‚óä‚Äò‚àÇ√ò√ï¬£√∑œÄ
#define JVN_YTCTRL_GQDT   26//œÄ‚Äö¬ª¬∂¬•√õ√ï¬£√∑œÄ
#define JVN_YTCTRL_GQXT   27//œÄ‚Äö¬ª¬∂‚Äì¬∞√ï¬£√∑œÄ
#define JVN_YTCTRL_BJDT   28//¬±‚Ä∞Œ©œÄ¬•√õ√ï¬£√∑œÄ
#define JVN_YTCTRL_BJXT   29//¬±‚Ä∞Œ©œÄ‚Äì¬∞√ï¬£√∑œÄ
#define JVN_YTCTRL_BBDT   30//¬±‚Ä∞¬±‚àÇ¬•√õ√ï¬£√∑œÄ
#define JVN_YTCTRL_BBXT   31//¬±‚Ä∞¬±‚àÇ‚Äì¬∞√ï¬£√∑œÄ
#define JVN_YTCTRL_FZ1    32//‚àè¬Æ√∑Àô1
#define JVN_YTCTRL_FZ1T   33//‚àè¬Æ√∑Àô1√ï¬£√∑œÄ
#define JVN_YTCTRL_FZ2    34//‚àè¬Æ√∑Àô2
#define JVN_YTCTRL_FZ2T   35//‚àè¬Æ√∑Àô2√ï¬£√∑œÄ
#define JVN_YTCTRL_FZ3    36//‚àè¬Æ√∑Àô3
#define JVN_YTCTRL_FZ3T   37//‚àè¬Æ√∑Àô3√ï¬£√∑œÄ
#define JVN_YTCTRL_FZ4    38//‚àè¬Æ√∑Àô4
#define JVN_YTCTRL_FZ4T   39//‚àè¬Æ√∑Àô4√ï¬£√∑œÄ

#define JVN_YTCTRL_RECSTART  41//‚Äò‚àÇ‚â•√É¬¨¬∫≈ì√í√∏‚Ñ¢¬†¬∫
#define JVN_YTCTRL_RECSTOP	 42//‚Äò‚àÇ‚â•√É¬¨¬∫≈ì√í√∏‚Ñ¢¬†¬∫

//ABORT CAR
#define JVN_YTCTRL_FORWORD 43
#define JVN_YTCTRL_BACK    44
#define JVN_YTCTRL_STOP    45
#define JVN_YTCTRL_START   47
#define JVN_YTCTRL_TURNLEFT 48
#define JVN_YTCTRL_TURNRIGHT 49

/*‚Äò‚àÇ‚â•√É√∏√ø√∑‚àÜ√∑‚àè¬°√ì(√∑Àú‚àë√∑√∏√ø‚Äù¬∂‚Äù‚àö‚â§‚Äû‚Äò¬∫‚àÇ¬Æ)*/
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
#define RC_DISCOVER2	0x0f	//zwq20111206,csst‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é√∑¬±Œ©‚Äù¬µ¬´¬¨¬∫¬£¬®√¶√∑‚Äù√ö√ï¬ØœÄ‚Äû‚â§‚Ä¢√Ä‚Äî√ÄÀú

#define JVN_VC_BrightUp			0x10 //¬†‚Äù‚àÜ¬µ¬µÀúŒ©‚ÅÑ
#define JVN_VC_BrightDown		0x11
#define JVN_VC_ContrastUp		0x12
#define JVN_VC_ContrastDown		0x13
#define JVN_VC_SaturationUp		0x14
#define JVN_VC_SaturationDown	0x15
#define JVN_VC_HueUp			0x16
#define JVN_VC_HueDown			0x17
#define JVN_VC_SharpnessUp		0x18
#define JVN_VC_SharpnessDown	0x19
#define JVN_VC_PRESENT          0x20 //‚Äò¬ß√∑‚àö≈í¬™¬µÀú‚Äù‚àö

#define JVN_CMD_BATCH_CHANNELNUM		0x47	//¬•‚Äù¬∫√è√ÄÀú‚àëÀõ≈í√í‚àÜÀú‚âàÀô¬°√∏¬™√í¬ª¬∞√ï¬Æ¬µ¬ø¬†Àù
#define JVN_INDIRRECT_CONN   0x205 //云视通服务器转发连接请求
typedef struct
{
    char chGroup[4];
    int nYSTNO;
	int wChannelNum;
}CHANNEL_NUM;
typedef struct _PLAY_INFO_
{
	unsigned char ucCommand;//‚àö¬∏¬°√ì‚óä√∑
	int nClientID;//‚àÇ‚Äò‚Äù¬∂‚óä‚âà¬™√ø‚àë‚âà

	int nConnectionType;

	char strFileName[MAX_PATH];//≈í∆í¬∫Àõ‚àöÀö

	int nSeekPos;//‚àÇ¬Æ≈í¬™¬†¬±‚Äì√ã‚Äú‚Ñ¢‚àÇ¬Æ≈í¬™¬µ∆í≈í¬™√∑‚àö √∑¬∞

}PLAY_INFO;//‚â§‚Ä¢‚àë‚âà¬™√ø¬µÀú¬†œÄ‚Äù‚àö¬µ∆íŒ©¬∑œÄœÄ

typedef struct STLANTOOLINFO 
{
	BYTE uchType;      //≈ìÀö≈ì¬¢¬ø‚Ä°‚Äì√ï¬£¬®1¬ø¬•‚óä‚ÄòœÄ¬ß√¶Ô¨Ç¬µ∆íœÄ‚Äû‚â§‚Ä¢¬£¬™2¬ø¬•‚óä‚ÄòœÄ¬ß√¶Ô¨Ç¬µ∆í‚âà‚Ä∞√∑‚àö¬£¬™3‚Ä¶√ã¬±‚àè¬™√ø‚Äù¬∂¬£¬™

	/*œÄ¬ß√¶Ô¨Ç‚Äì‚âà≈ì¬¢*/
	char chPName[256]; //‚Äù‚àö¬™√ü‚àöÀö¬£¬®‚Äù‚àö‚Äù‚ÅÑ√É¬∑‚àèÔ¨ÇIPC‚àû‚â§¬ª¬¥‚Äì‚Äò¬£¬®‚àë¬ø√∑œÄ‚àÇ√í‚Äú‚Äö‚âà‚Ä∞√∑‚àö
	char chPWord[256]; //‚àö‚Äπ¬¨√é¬£¬®‚Äù‚àö‚Äù‚ÅÑ√É¬∑‚àèÔ¨ÇIPC‚àû‚â§¬ª¬¥‚Äì‚Äò¬£¬®‚àë¬ø√∑œÄ‚àÇ√í‚Äú‚Äö‚âà‚Ä∞√∑‚àö
	int nYSTNUM;       //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é¬£¬®‚Äù‚àö‚Äù‚ÅÑœÄ¬ß√¶Ô¨Ç≈ì√ö‚Ä¶√ã¬±‚àè‚àë¬¢√Ä√ï‚âà‚Ä∞√∑‚àö
	char chCurTime[20];//≈ì¬µ√ï‚â•¬†¬±¬∫‚Ä∞¬£¬®‚Äù‚àö‚Äù‚ÅÑœÄ¬ß√¶Ô¨Ç≈ì√ö‚Ä¶√ã¬±‚àè‚àë¬¢√Ä√ï‚âà‚Ä∞√∑‚àö xxxx-xx-xx xx:xx:xx
	char *pchData;     //‚âà‚Ä∞√∑‚àö∆í‚ÅÑ¬ª‚Ä∫¬£¬®‚Äù‚àö‚Äù‚ÅÑœÄ¬ß√¶Ô¨Ç≈ì√ö‚Ä¶√ã¬±‚àè‚àë¬¢√Ä√ï‚âà‚Ä∞√∑‚àö
	int nDLen;         //‚âà‚Ä∞√∑‚àö∆í‚ÅÑ¬ª‚Ä∫‚â•¬ß‚àÇ¬ª¬£¬®‚Äù‚àö‚Äù‚ÅÑœÄ¬ß√¶Ô¨Ç≈ì√ö‚Ä¶√ã¬±‚àè‚àë¬¢√Ä√ï‚âà‚Ä∞√∑‚àö

	/*‚Äù¬∂¬•Ô£ø‚Äì‚âà≈ì¬¢*/
	int nCardType;     //‚Ä¶√ã¬±‚àè¬ø‚Ä°‚Äì√ï¬£¬®‚Äù‚àö‚Äù‚ÅÑ‚Ä¶√ã¬±‚àè‚Äù¬∂¬•Ô£ø¬†¬±¬µ∆í‚àèŒ©¬∫‚Äù‚Äì‚âà≈ì¬¢
	int	nDate;         //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ ‚Äì≈í¬ª√Å 20091011
	int	nSerial;       //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID guid;         //≈í¬Æ‚Äú¬™GUID
	char chGroup[4];   //‚Ä¶√ã¬±‚àè¬±‚Äû‚óä√à‚à´‚âà¬£¬®‚Äù‚àö‚Äù‚ÅÑ‚Ä¶√ã¬±‚àè‚Äù¬∂¬•Ô£ø¬†¬±¬µ∆í‚àèŒ©¬∫‚Äù‚Äì‚âà≈ì¬¢

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
}STLANTOOLINFO;//√¶√∑‚Äù√ö√ï¬Ø‚Ä¶Àô‚â§ÀôœÄ¬ß√¶Ô¨Ç≈ìÀö≈ì¬¢∆í‚ÅÑ¬ª‚Ä∫

typedef struct STTOOLPACK
{
	int nCardType;//‚â§Àô‚àÜ‚àë¬ø‚Ä°‚Äì√ï(4)
	int nPNLen;//‚Äù‚àö¬™√ü‚àöÀö‚â•¬ß(4)
	int nPWLen;//‚àö‚Äπ¬¨√é‚â•¬ß(4)
	BYTE uchCType;//¬†Àù√¶‚Ä∫¬ø‚Ä°‚Äì√ï(1)
	char chGroup[4];
	int nYSTNUM;//‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é(4)
	char chCurTime[20];//≈ì¬µ√ï‚â•¬†¬±¬∫‚Ä∞(4)
	int nDLen;//‚âà‚Ä∞√∑‚àö‚â•¬ß‚àÇ¬ª(4)
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


#define  NET_MOD_UNKNOW 0 // ≈í¬•‚àöÀö
#define  NET_MOD_WIFI   1 //wifi ∆í¬£¬†Œ©
#define  NET_MOD_WIRED  2 // ‚Äù‚Äì≈ìÔ¨Ç∆í¬£¬†Œ©

#define  DEV_SET_ALL      0 // ‚Ä¶√ã√∑‚àö¬ª¬¥‚â§√∏
#define  DEV_SET_NET      1 //‚Ä¶√ã√∑‚àö‚Ä¶√ã¬±‚àè√∑√ü‚â•√∑¬µ∆í√ï¬Ø¬¨√Å∆í¬£¬†Œ©
#define  DEV_SET_CUR_NET  2 //‚Ä¶√ã√∑‚àö‚Ä¶√ã¬±‚àè¬µ¬±¬´‚àû√ï¬Ø¬¨√Å∆í¬£¬†Œ©
#define  DEV_SET_NAME     3 // ‚Ä¶√ã√∑‚àö¬±Ô£ø‚àöÀö

typedef struct  
{
	char chDeviceName[100];//‚Ä¶√ã¬±‚àè¬±Ô£ø‚àöÀö
	int nCurNetMod;// ‚Ä¶√ã¬±‚àè¬µ¬±¬´‚àû√ï¬Ø¬¨√Å∆í¬£¬†Œ©¬£¬®‚Äù‚Äì≈ìÔ¨Ç¬£¬®wifi¬™√ö‚àÜ‚Ä∞√Ä¬∏
	int nNetMod; //‚Ä¶√ã¬±‚àè√∑√ü‚â•√∑¬µ∆í√ï¬Ø¬¨√Å∆í¬£¬†Œ©¬£¬®≈í‚Ñ¢ ¬∫‚àè√∑√∑∆í¬£¬†Œ©¬™√ö‚àÜÔ£ø¬ø¬•¬µ∆í√∑¬µ  nNetMod = NET_MOD_WIFI;
}STDEVINFO; //‚Ä¶√ã¬±‚àè‚â§≈í¬†Àù

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

	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1¬†¬´‚Äì¬• 2¬†¬´‚àÇ¬°

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

	typedef void (*FUNC_COMM_DATA_CALLBACK)(int nType,unsigned char *chGroup,char* chFileName,unsigned char *pBuffer, int *nSize);//nType = 1¬†¬´‚Äì¬• 2¬†¬´‚àÇ¬°
#endif

	typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 ‚â•‚Ä¶œÄ¬∂ 2 ¬†√ü‚àû‚Äπ 3 ‚àÇ≈ì√∏‚Ñ¢ 4 ‚Äú√è‚â•¬£‚àÇ≈ì√∏‚Ñ¢
	typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

typedef int (*FUNC_LANTOOL_CALLBACK)(STLANTOOLINFO* pData);//0‚â§¬™‚Äì√ã≈ì‚Ä°‚Äù¬∂1≈ì√è‚Äù¬∂
/*‚àë√∑√∏√ø¬™√ø¬µÀú‚à´√ò¬†Àù*/
typedef struct STBASEYSTNO
{
	char chGroup[4];
	int nYSTNO;
	int nChannel;
	char chPName[MAX_PATH];
	char chPWord[MAX_PATH];
	int nConnectStatus;//¬°¬®Œ©‚Äù‚óä¬•√É¬® ‚Ä¶√ã√∑‚àö¬†¬±=0¬£¬®‚â§√à‚Äî√ò¬†¬±¬±√å¬†√¶‚óä¬•√É¬® 0 ≈í¬•¬°¬®Œ©‚Äù 1 ∆í‚ÅÑ√ï¬Ø 2 ‚óä‚Ñ¢‚àë¬¢ 3√ï‚Äö√ï¬Ø	
}STBASEYSTNO;//‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é¬™Àò¬±√¶‚Äì‚âà≈ì¬¢¬£¬®‚Äù‚àö‚Äù‚ÅÑ‚â•ƒ±¬†¬∫¬™√ò‚Äì¬∞√∑Àô¬†√∑¬µ∆í‚Äì√à¬°¬®Œ©‚Äù

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

	int nNetMod;//¬øÀù¬ª√Å ¬†¬´‚àë√í√¶Ô¨Ç‚Äù‚ÄìwifiœÄ¬∂∆í‚Äπ: nNetMod & NET_MOD_WIFI
	int nCurMod;//¬øÀù¬ª√Å ¬µ¬±¬´‚àû¬†œÄ‚Äù‚àö¬µ∆í(wifi¬™√ö‚Äù‚Äì≈ìÔ¨Ç)¬£‚à´nCurMod(NET_MOD_WIFI ¬™√ö NET_MOD_WIRED)

	int nPrivateSize;//‚óä‚Äò‚àÇ¬Æ‚Äú√Ç¬†Àù√¶‚Ä∫¬†¬µ¬∫¬†‚â•¬ß‚àÇ¬ª
	char chPrivateInfo[500];//‚óä‚Äò‚àÇ¬Æ‚Äú√Ç¬†Àù√¶‚Ä∫∆í‚ÅÑ¬ª‚Ä∫
}STLANSRESULT;//√¶√∑‚Äù√ö√ï¬Ø‚Ä¶√ã¬±‚àè√Ä‚Äî√ÄÀúŒ©¬∑œÄÀö
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


#define JVN_COMMAND_AB		0x202	//ÂàÜÊéßÈÄöËøá‰∫ëËßÜÈÄöÊúçÂä°Âô®‰∏é‰∏ªÊéß‰∫§‰∫íÊï∞ÊçÆ
#define JVN_COMMAND_BA		0x208	//‰∏ªÊéßÈÄöËøá‰∫ëËßÜÈÄöÊúçÂä°Âô®‰∏éÂàÜÊéß‰∫§‰∫íÊï∞ÊçÆ

/************√∑Àú√∏√ø¬™√ø¬µÀú‚à´√ò¬†Àù‚â§≈í¬†Àù¬ø‚Ä°‚Äì√ï***************/
/*FUNC_SCONNECT_CALLBACK*/
#define JVN_SCONNECTTYPE_PCCONNOK  0X01//‚Äù‚ÄìPC‚àë√∑√∏√ø¬°¬®Œ©‚Äù‚â•‚Ä¶œÄ¬∂
#define JVN_SCONNECTTYPE_DISCONNOK 0X02//‚Äù‚Äì‚àë√∑√∏√ø‚àÇ≈ì√∏‚Ñ¢¬°¬®Œ©‚Äù‚â•‚Ä¶œÄ¬∂
#define JVN_SCONNECTTYPE_DISCONNE  0X03//¬°¬®Œ©‚Äù‚Äú√è‚â•¬£‚àÇ≈ì√∏‚Ñ¢
#define JVN_SCONNECTTYPE_MOCONNOK  0X04//‚Äù‚Äì‚Äú‚àÜ‚àÇ√ò‚Ä¶√ã¬±‚àè‚àë√∑√∏√ø¬°¬®Œ©‚Äù‚â•‚Ä¶œÄ¬∂
/*FUNC_SONLINE_CALLBACK*/
#define JVN_SONLINETYPE_ONLINE     0x01//‚Ä¶≈ì≈ìÔ¨Ç
#define JVN_SONLINETYPE_OFFLINE    0x02//≈ì¬¨≈ìÔ¨Ç
#define JVN_SONLINETYPE_CLEAR      0x03//YST‚à´‚âà¬¨√é≈íÔ¨Å‚Äì√ü¬£¬®‚Äù¬∂¬´√Ç√∏‚Äô√∑√ø‚Äì¬¨‚Ä¶√ç¬´√é

/************‚àë√∑√∏√ø¬™√ø¬µÀú‚à´√ò¬†Àù‚â§≈í¬†Àù¬ø‚Ä°‚Äì√ï***************/
/*FUNC_CCONNECT_CALLBACK*/
#define JVN_CCONNECTTYPE_CONNOK    0X01//¬°¬®Œ©‚Äù‚â•‚Ä¶œÄ¬∂
#define JVN_CCONNECTTYPE_DISOK     0X02//‚àÇ≈ì√∏‚Ñ¢¬°¬®Œ©‚Äù‚â•‚Ä¶œÄ¬∂
#define JVN_CCONNECTTYPE_RECONN    0X03//‚â§¬™¬±√ø√∑√ø‚àè¬•¬°¬®Œ©‚Äù
#define JVN_CCONNECTTYPE_CONNERR   0X04//¬°¬®Œ©‚Äù¬†√ü‚àû‚Äπ
#define JVN_CCONNECTTYPE_NOCONN    0X05//‚àö¬™¬°¬®Œ©‚Äù
#define JVN_CCONNECTTYPE_DISCONNE  0X06//¬°¬®Œ©‚Äù‚Äú√è‚â•¬£‚àÇ≈ì√∏‚Ñ¢
#define JVN_CCONNECTTYPE_SSTOP     0X07//‚àëÀõ≈í√í√ï¬£√∑œÄ¬£¬®¬°¬®Œ©‚Äù‚àÇ≈ì√∏‚Ñ¢
#define JVN_CCONNECTTYPE_DISF      0x08//‚àÇ≈ì√∏‚Ñ¢¬°¬®Œ©‚Äù¬†√ü‚àû‚Äπ


/*‚àë¬µ¬™√ø√∑¬µ*/
#define JVN_RETURNOK    0//‚â•‚Ä¶œÄ¬∂
#define JVN_PARAERROR   1//‚â§≈í¬†Àù¬•√å≈í√õ
#define JVN_RETURNERROR 2//¬†√ü‚àû‚Äπ
#define JVN_NOMEMERROR  3//∆í‚ÅÑ¬•√ä¬™√ö‚Äù‚â§‚âà√É√∏‚Äô¬∫‚Ä∞‚â§¬™‚óä‚Äû

//‚ÄòÀÜ¬∫‚ÄùIP‚àÇ≈í¬£¬®√Ä‚Äî√ÄÀú√¶√∑‚Äù√ö√ï¬Ø‚Ä¶√ã¬±‚àè¬†¬±‚à´√ö‚Äù‚àö¬∞¬£
typedef struct
{
	char startip[16];//‚àÜÔ£ø¬†¬∫IP
	char endip[16];//√∑‚Äô√∑œÄIP
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

/*‚â•ƒ±¬†¬∫¬™√òSDK¬†¬±¬•¬¥¬ª√é‚Ä¶√ã¬±‚àè≈ì‚Ä°œÄ√ø¬µ∆í‚Äú¬™‚Äì¬©‚Äì‚âà≈ì¬¢¬£¬®‚Äù‚àö‚Äù‚ÅÑ∆í‚ÅÑ‚â§√∏‚àèÀò√¶‚Ä∫‚â§¬™√ï¬®‚â§Àô‚àÜ‚àë‚Ä¶√ã√∑‚àö‚â§¬™√ï¬®¬µ∆í¬•¬∂¬ø√å*/
typedef struct 
{
	int nType;//‚â§Àô‚àÜ‚àë¬•√õ¬ø‚Ä° 0‚àÜ‚Äô√ï¬Æ∆í¬®¬ª≈ì‚â§Àô‚àÜ‚àë¬£¬™1‚àû√Ç√∏¬Æ; 2DVR; 3IPC; 4JNVR; 5NVR; 6¬∫‚Äú‚Äù‚àö‚àû√ä‚Äì¬∞IPC;
	int nMemLimit;//∆í‚ÅÑ¬•√ä‚â•‚Ä∞‚óä‚Äû‚àÇ¬ª 0‚àÜ‚Äô√ï¬Æ∆í¬®¬ª≈ì‚â§Àô‚àÜ‚àë¬£¬™1∆í‚ÅÑ¬•√ä‚â•‚Ä∞‚óä‚Äû(√ï¬Ø¬¨√Å√∑¬°‚Ä¶≈∏‚Äù‚Äì20M√∏‚Ä¶‚Äù‚àö)¬£¬™2∆í‚ÅÑ¬•√ä¬™Àò¬±√¶œÄ¬™‚Äù‚àö(√ï¬Ø¬¨√Å‚Äò¬∫20M√∏‚Ä¶‚Äù‚àö)¬£¬™3∆í‚ÅÑ¬•√äŒ©√ô‚Äô‚âà(√ï¬Ø¬¨√Å‚Äì¬∞‚Äù‚ÅÑ15M√∏‚Ä¶‚Äù‚àö)¬£¬™
	              //∆í‚ÅÑ¬•√ä‚Äò¬†‚Äì√å¬µ∆í¬´‚àû√É¬∑≈ì¬¨¬£¬®∆í‚ÅÑ¬•√ä‚â•‚Ä∞‚óä‚Äû‚àÇ¬ª‚ÄòŒ©‚à´‚àö¬£¬®√ï¬Ø¬•¬¥¬•¬∂¬ø√å‚Äì√üœÄÀö¬™¬∑‚ÄòŒ©‚à´‚àö¬£¬™¬ª√ÅœÄÀö‚â§¬™¬ª‚àë‚àÇ¬Æ¬£¬®¬´√é√∑‚àö≈í‚Ñ¢0¬£¬™
}STDeviceType;

/*¬∫¬ß¬™√ì‚à´‚âà¬¨√é¬†¬±¬µÀú‚Äù‚àö‚ÄôÔ¨Ç¬•¬¥¬ª√éŒ©¬∑œÄœÄ*/
typedef struct 
{
	char chGroup[4];//‚àë√∑‚óä√à‚à´‚âà¬£¬®‚Äì≈í¬ª√Å"A" "AAAA"
	int nCardType;  //√∏¬Æ≈ì¬µ
	int	nDate;      //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ ‚Äì≈í¬ª√Å 20091011
	int	nSerial;    //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID guid;      //≈í¬Æ‚Äú¬™GUID MAPIGUID.H
}STGetNum;
/*‚â•ƒ±¬†¬∫¬™√ò¬†¬±¬µÀú‚Äù‚àö‚ÄôÔ¨Ç¬•¬¥¬ª√é¬µ∆íŒ©¬∑œÄœÄ*/
typedef struct
{
    int nCardType; /*‚â§Àô‚àÜ‚àë¬ø‚Ä°‚Äì√ï¬£¬®¬øÀù¬ª√Å¬£‚à´0xE71A, 0xD800,0xB800, 960*/
    int    nDate;     //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ
    int    nSerial;   //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
    GUID gLoginKey;//‚àû√Ç√∏¬ÆGUID
    int  nYstNum;  //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
    char chGroup[4];//‚àë√∑‚óä√à‚à´‚âà¬£¬®‚Äì≈í¬ª√Å"A" "AAAA"
    int  nChannelCount; //√∑Àú√∏√ø√ï¬Æ¬µ¬ø‚óä‚Äπ¬†Àù∆í√∏  //old

	DWORD dwProductType;//‚â§Àô‚àÜ‚àë√ï√ç‚ÄôÀö¬ø‚Ä°‚Äì√ï/*¬øÀù¬ª√Å¬£‚à´0xE71A4010,0xD8006001,0xB8007001,0xc896ffff*/
    DWORD dwEncryVer; //¬∫‚Äù‚àö‚Äπ‚àû√ä¬±√¶, 0xB567709F¬£‚à´¬∫‚Äù‚àö‚Äπ‚Äì√¶‚àÜ¬®√∑‚Äì‚àû¬∏‚à´¬®‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é¬£¬®‚Äù‚Äì¬†Àù√¶‚Ä∫√∏‚Äö;0xF097765B¬£‚à´‚â§¬™‚àû¬∏‚à´¬®‚à´‚âà¬¨√é¬£¬®‚Äù‚Äì¬†Àù√¶‚Ä∫√∏‚Äö;0xB56881B0:‚àû¬∏‚à´¬®‚à´‚âà¬¨√é¬£¬®≈íÔ¨Å¬†Àù√¶‚Ä∫√∏‚Äö;
    DWORD dwDevVer; //¬∫‚Äù‚àö‚Äπ‚Äì√¶‚àÜ¬®‚Äù‚â§¬∫Àõ‚àû√ä¬±√¶
    int   nUIVer;//√∑Àú√∏√ø‚àû√ä¬±√¶
    DWORD dwOemid;//‚â•√ü¬∫‚Äúid
    DWORD dwUser;//¬∫‚Äù‚àö‚Äπ¬ª√Ä‚Äò¬±ID
    
   
    int nMaxConnNum;      //‚óä√ì¬•√õ¬°¬®Œ©‚Äù¬†Àù
    int nZone;//¬´¬Ø‚Äù√ö-086√∑‚ÄìœÄÀô
    int nSystemType;//≈ì¬µ√ï‚â•‚Äì√ï‚à´‚âà-‚àèÔ¨Ç1‚óä√∑Œ©‚ÅÑ≈ì¬µ√ï‚â•¬ø‚Ä°‚Äì√ï(0x1:windows 0x2:linux 0x3:MacOS 0x4:‚àû‚â§‚óä√∏ 0x5:‚àÜ‚Ä∞√ÄÀö)¬£¬®¬µ√ï3‚óä√∑Œ©‚ÅÑ≈ì¬µ√ï‚â•‚àû√ä¬±√¶‚à´‚âà¬£¬®‚àèÀú≈ì¬µ√ï‚â•‚àû√ä¬±√¶‚à´‚âà≈ì√ç¬∫Àö‚óä¬¢¬†√ï
    /*Windows:6.1; 6.0; 5.2; 5.1; 5.0; 4.9; 4.1; 4.0; 3.1; 3.0; 2.0; 1.0 ¬µ¬ª
    linux :2.6; 2.4; 2.2; 2.0; 1.2; 1.1; 1.0 ¬µ¬ª
    MAC OS:10.7; 10.6; 10.5; 10.4; 10.3; 10.2; 10.1; 10.0 ¬µ¬ª;
    Android 1.1; 1.5; 1.6; 2.0; 2.1; 2.2; 2.3; 2.4; 3.0; 3.1; 3.2; 4.0; 4.1; 4.2; 4.4¬µ¬ª    */
    /*¬øÀù¬ª√Å¬£‚à´win7: 0x103d, win XP: 0x1033; linux2.6: 0x201a, linux2.5: 0x2019; MacOS10.7: 0x306B, MacOS10.6:0x306A; Android4.0: 0x4028; Android4.4: 0x402c*/
    
    char chProducType[64];//‚â§Àô‚àÜ‚àë‚Äì√ï‚à´‚âà-‚óä√∑‚àëÀö¬•√Ü
    char chDevType[16];//‚Äù‚â§¬∫Àõ‚Äì√ï‚à´‚âà-‚óä√∑‚àëÀö¬•√Ü
}STOnline;





/*¬†Àù√¶‚Ä∫‚àû¬∏:¬∫¬ß¬™√ìYST‚à´‚âà¬¨√é*/
typedef struct
{
	int nCardType;  //√∏¬Æ≈ì¬µ
	int	nDate;      //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ ‚Äì≈í¬ª√Å 20091011
	int	nSerial;    //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID guid;      //≈í¬Æ‚Äú¬™GUID MAPIGUID.H
}Pak_GetNum;

/*¬†Àù√¶‚Ä∫‚àû¬∏:‚Ä¶≈ì≈ìÔ¨Ç*/
typedef struct
{
	int nCardType; //√∏¬Æ≈ì¬µ
	int	nDate;     //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ
	int	nSerial;   //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID gLoginKey;//‚àû√Ç√∏¬ÆGUID
	int  nYstNum;  //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
}Pak_Online1; 

typedef struct
{
	int		nCardType;//‚àû√Ç√∏¬Æ¬ø‚Ä°‚Äì√ï
	int		nDate; //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ
	int		nSerial; //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID	gLoginKey; //¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
	int     nYstNum;   //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
	char    chGroup[4];//¬±‚Ä°‚óä√à‚à´‚âà ‚Äò‚Ä∫‚â§¬™¬†œÄ‚Äù‚àö
	int     nNSDKVer;//√∑Àú√∏√ø√ï¬Ø¬¨√ÅSDK‚àû√ä¬±√¶(‚Äì‚â†‚Äú√à‚àû√ä¬±√¶)
	int     nChannelCount; //√∑Àú√∏√ø√ï¬Æ¬µ¬ø‚óä‚Äπ¬†Àù∆í√∏
}Pak_Online1Ex; //YST_AS_ONLINE1  ¬†Àù√¶‚Ä∫Œ©¬∑œÄœÄ

typedef struct
{
	int nCardType; //√∏¬Æ≈ì¬µ
	int    nDate;     //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ
	int    nSerial;   //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID gLoginKey;//‚àû√Ç√∏¬ÆGUID
	int  nYstNum;  //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
	char chGroup[4];//‚àë√∑‚óä√à‚à´‚âà¬£¬®‚Äì≈í¬ª√Å"A" "AAAA"
	int     nNSDKVer;//√∑Àú√∏√ø√ï¬Ø¬¨√ÅSDK‚àû√ä¬±√¶(‚Äì‚â†‚Äú√à‚àû√ä¬±√¶)
	int  nChannelCount; //√∑Àú√∏√ø√ï¬Æ¬µ¬ø‚óä‚Äπ¬†Àù∆í√∏  //old
	
	DWORD dwProductType;//‚â§Àô‚àÜ‚àë¬ø‚Ä°‚Äì√ï
	DWORD dwEncryVer; //¬∫‚Äù‚àö‚Äπ‚àû√ä¬±√¶, 0xB567709F¬£‚à´¬∫‚Äù‚àö‚Äπ‚Äì√¶‚àÜ¬®√∑‚Äì‚àû¬∏‚à´¬®‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é¬£¬®‚Äù‚Äì¬†Àù√¶‚Ä∫√∏‚Äö;0xF097765B¬£‚à´‚â§¬™‚àû¬∏‚à´¬®‚à´‚âà¬¨√é¬£¬®‚Äù‚Äì¬†Àù√¶‚Ä∫√∏‚Äö;0xB56881B0:‚àû¬∏‚à´¬®‚à´‚âà¬¨√é¬£¬®≈íÔ¨Å¬†Àù√¶‚Ä∫√∏‚Äö;
	int nProtocolEx;//‚àè‚àöŒ©¬∑œÄœÄ√É√Ç¬ø¬©‚ÄôœÄ
	DWORD dwDevVer; //‚Äù‚â§¬∫Àõ‚àû√ä¬±√¶
	int   nUIVer;//√∑Àú√∏√ø‚àû√ä¬±√¶
	DWORD dwOemid;//‚â•√ü¬∫‚Äúid
	DWORD dwUser;//¬∫‚Äù‚àö‚Äπ¬ª√Ä‚Äò¬±ID
	
	int nMaxConnNum;      //‚óä√ì¬•√õ¬°¬®Œ©‚Äù¬†Àù
	int nZone;//¬´¬Ø‚Äù√ö
	int nSystemType;//≈ì¬µ√ï‚â•‚Äì√ï‚à´‚âà
	char chProducType[64];//‚â§Àô‚àÜ‚àë‚Äì√ï‚à´‚âà
	char chDevType[16];//‚Äù‚â§¬∫Àõ‚Äì√ï‚à´‚âà
}Pak_Online1Ex2; //Pak_Online1Ex2  ¬†Àù√¶‚Ä∫Œ©¬∑œÄœÄ

/*¬†Àù√¶‚Ä∫‚àû¬∏:‚Ä¶≈ì≈ìÔ¨Ç‚Äù¬∂¬•Ô£ø*/
typedef struct
{
	int	nIndex;     //√Ä‚â•‚Äì√ö‚à´‚âà
	GUID gLoginKey; //¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
}Pak_LoginKey; 

/*¬†Àù√¶‚Ä∫‚àû¬∏: ¬µ√ø√∑‚àë‚àè¬∏‚Äì¬¨*/
typedef struct
{
	int nYstNum;       //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
	int	nIndex;        //√Ä‚â•‚Äì√ö‚à´‚âà
	GUID gLoginKey;    //¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
#ifndef WIN32
	struct sockaddr InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#else
	SOCKADDR InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#endif
	int nVer;          //√∑Àú√∏√ø¬µ∆í‚àû√ä¬±√¶
}Pak_Online2;

/*¬†Àù√¶‚Ä∫‚àû¬∏: ¬µ√ø√∑‚àë‚àè¬∏‚Äì¬¨*/
typedef struct
{
	int nYstNum;       //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
	int	nIndex;        //√Ä‚â•‚Äì√ö‚à´‚âà
	GUID gLoginKey;    //¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
#ifndef WIN32
	struct sockaddr InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#else
	SOCKADDR InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#endif
	int nVer;          //√∑Àú√∏√ø¬µ∆í‚àû√ä¬±√¶
	int nTCPSerPort;   //TCP‚àëÀõ≈í√í‚àÇ√Ä√∏‚ÅÑ
}Pak_Online2Ex;

typedef struct
{
	int nYstNum;       //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
	int	nIndex;        //√Ä‚â•‚Äì√ö‚à´‚âà
	GUID gLoginKey;    //¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
#ifndef WIN32
	struct sockaddr InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#else
	SOCKADDR InAddress;//∆í‚ÅÑ√ï¬Ø¬µ√ø√∑‚àë
#endif
	int nVer;          //√∑Àú√∏√ø¬µ∆í‚àû√ä¬±√¶
	int nTCPSerPort;   //TCP‚àëÀõ≈í√í‚àÇ√Ä√∏‚ÅÑ
	int nNatType;		//√ï¬Ø¬¨√Å¬ø‚Ä°‚Äì√ï
}Pak_Online2Ex2;

/*¬†Àù√¶‚Ä∫‚àû¬∏: ≈ì¬¨≈ìÔ¨Ç*/
typedef struct
{
	int	nYstNum;
	int	nIndex;
	GUID gLoginKey;//¬µ¬´¬¨Œ©¬°√ì‚âà‚àÜ
}Pak_Logout;

typedef struct
{
	unsigned int nTypeLens; //YST_AS_REGCARD	
	//√¶¬™‚Äò√ø¬†Àù√¶‚Ä∫
	int nSize;
	char *pacData;
}SOCKET_DATA_TRAN;

typedef struct 
{
	SOCKET sSocket;//Œ©‚Äù¬†‚Äô¬µ∆í√É‚óäŒ©‚Äù‚óä√∑
	int nLocal;//¬±√¶¬µ√ø‚àÇ√Ä√∏‚ÅÑ
	
	SOCKADDR_IN addrRemote;//‚Äò‚àÇ‚â•√É‚àÇ√Ä√∏‚ÅÑ
	char cData[2048];//¬†‚Äô¬µŒ©¬†Àù√¶‚Ä∫
	int nLen;//¬†Àù√¶‚Ä∫‚â•¬ß‚àÇ¬ª
	unsigned long time;//Œ©‚Äù¬†‚Äô¬µŒ©¬µ∆í¬†¬±¬∫‚Ä∞ 
}UDP_PACKAGE;//UDT‚àë¬µ¬™√ø¬µ∆í¬†Àù√¶‚Ä∫‚àû¬∏

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
	char chGroup[4];//‚àë√∑‚óä√à‚à´‚âà¬£¬®‚Äì≈í¬ª√Å"A" "AAAA"
	int  nChannelCount; //√∑Àú√∏√ø√ï¬Æ¬µ¬ø‚óä‚Äπ¬†Àù∆í√∏
	int nCardType; //√∏¬Æ≈ì¬µ
	int	nDate;     //‚â•ÀÜ‚â•√ü¬ª‚Äô‚àÜ‚ÅÑ
	int	nSerial;   //‚â•ÀÜ‚â•√ü‚Äì√ö¬°‚Äì‚à´‚âà
	GUID gLoginKey;//‚àû√Ç√∏¬ÆGUID
	int  nYstNum;  //‚Äò‚àÜ¬†‚Äù√ï¬Æ‚à´‚âà¬¨√é
}STOnlineold;//¬°≈∏¬†¬±

#endif
