// SUpnpCtrl.h: interface for the CSUpnpCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SUPNPCTRL_H__659AEDB6_CA61_46CB_917F_73C517F94863__INCLUDED_)
#define AFX_SUPNPCTRL_H__659AEDB6_CA61_46CB_917F_73C517F94863__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

/* port upnp discover : SSDP protocol */
#define PORT 1900
#define XSTR(s) STR(s)
#define STR(s) #s
#define UPNP_MCAST_ADDR "239.255.255.250"
#define MAXHOSTNAMELEN 64
#define OS_STRING "Window$"
#define MINIUPNPC_VERSION_STRING "1.4"
#define IS_WHITE_SPACE(c) ((c==' ') || (c=='\t') || (c=='\r') || (c=='\n'))
/* MiniUPnPc return codes : */
#define UPNPCOMMAND_SUCCESS (0)
#define UPNPCOMMAND_UNKNOWN_ERROR (-1)
#define UPNPCOMMAND_INVALID_ARGS (-2)

#ifdef QUEUE_MACRO_DEBUG
#define _Q_INVALIDATE(a) (a) = ((void *)-1)
#else
#define _Q_INVALIDATE(a)
#endif

#define SOAPPREFIX "s"
#define SERVICEPREFIX "u"
#define SERVICEPREFIX2 'u'

#define MIN(x,y) (((x)<(y))?(x):(y))

/* List access methods*/
#define	LIST_FIRST(head)		((head)->lh_first)
#define	LIST_END(head)			NULL
#define	LIST_EMPTY(head)		(LIST_FIRST(head) == LIST_END(head))
#define	LIST_NEXT(elm, field)		((elm)->field.le_next)

/* List definitions.*/
#define LIST_HEAD(name, type)						\
	struct name {								\
	struct type *lh_first;	/* first element */			\
}

/*List functions.*/
#define	LIST_INIT(head) do {						\
	LIST_FIRST(head) = LIST_END(head);				\
} while (0)

#define LIST_ENTRY(type)						\
	struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

#define LIST_INSERT_HEAD(head, elm, field) do {				\
	if (((elm)->field.le_next = (head)->lh_first) != NULL)		\
	(head)->lh_first->field.le_prev = &(elm)->field.le_next;\
	(head)->lh_first = (elm);					\
	(elm)->field.le_prev = &(head)->lh_first;			\
} while (0)

#define LIST_REMOVE(elm, field) do {					\
	if ((elm)->field.le_next != NULL)				\
	(elm)->field.le_next->field.le_prev =			\
	(elm)->field.le_prev;				\
	*(elm)->field.le_prev = (elm)->field.le_next;			\
	_Q_INVALIDATE((elm)->field.le_prev);				\
	_Q_INVALIDATE((elm)->field.le_next);				\
} while (0)

/* Encode length by using 7bit per Byte :
* Most significant bit of each byte specifies that the
* following byte is part of the code */
#define DECODELENGTH(n, p) n = 0; \
	                       do { n = (n << 7) | (*p & 0x7f); } \
                           while(*(p++)&0x80);

#define CODELENGTH(n, p) if(n>=268435456) *(p++) = (n >> 28) | 0x80; \
						 if(n>=2097152) *(p++) = (n >> 21) | 0x80; \
						 if(n>=16384) *(p++) = (n >> 14) | 0x80; \
						 if(n>=128) *(p++) = (n >> 7) | 0x80; \
						 *(p++) = n & 0x7f;


#ifdef WIN32
	#define socklen_t int
	#define snprintf _snprintf
    typedef SSIZE_T ssize_t;
	typedef unsigned short uint16_t;

	#if defined(_MSC_VER) && (_MSC_VER >= 1400)
		#define strncasecmp _memicmp
	#else
		#define strncasecmp memicmp
	#endif
#else
	#include <poll.h>
#endif

#define UNIX_PATH_LEN   108
struct sockaddr_un
{
	uint16_t sun_family;
	char sun_path[UNIX_PATH_LEN];
};

class CCWorker;
struct UPNPDev
{
	struct UPNPDev * pNext;
	char * descURL;
	char * st;
	char buffer[2];
};

struct UPNPUrls
{
	char * controlURL;
	char * ipcondescURL;
	char * controlURL_CIF;
};

/* Structure to store the result of the parsing of UPnP
* descriptions of Internet Gateway Devices */
#define MINIUPNPC_URL_MAXSIZE (128)
struct IGDdatas
{
	char cureltname[MINIUPNPC_URL_MAXSIZE];
	char urlbase[MINIUPNPC_URL_MAXSIZE];
	int level;
	/*int state;*/
	/* "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1" */
	char controlurl_CIF[MINIUPNPC_URL_MAXSIZE];
	char eventsuburl_CIF[MINIUPNPC_URL_MAXSIZE];
	char scpdurl_CIF[MINIUPNPC_URL_MAXSIZE];
	char servicetype_CIF[MINIUPNPC_URL_MAXSIZE];
	/*char devicetype_CIF[MINIUPNPC_URL_MAXSIZE];*/
	/* "urn:schemas-upnp-org:service:WANIPConnection:1"
	* "urn:schemas-upnp-org:service:WANPPPConnection:1" */
	char controlurl[MINIUPNPC_URL_MAXSIZE];
	char eventsuburl[MINIUPNPC_URL_MAXSIZE];
	char scpdurl[MINIUPNPC_URL_MAXSIZE];
	char servicetype[MINIUPNPC_URL_MAXSIZE];
	/*char devicetype[MINIUPNPC_URL_MAXSIZE];*/
	/* tmp */
	char controlurl_tmp[MINIUPNPC_URL_MAXSIZE];
	char eventsuburl_tmp[MINIUPNPC_URL_MAXSIZE];
	char scpdurl_tmp[MINIUPNPC_URL_MAXSIZE];
	char servicetype_tmp[MINIUPNPC_URL_MAXSIZE];
	/*char devicetype_tmp[MINIUPNPC_URL_MAXSIZE];*/
};

/* if a callback function pointer is set to NULL,
* the function is not called */
struct xmlparser
{
	const char *xmlstart;
	const char *xmlend;
	const char *xml;	/* pointer to current character */
	int xmlsize;
	void * data;
	void (*starteltfunc) (void *, const char *, int);
	void (*endeltfunc) (void *, const char *, int);
	void (*datafunc) (void *, const char *, int);
	void (*attfunc) (void *, const char *, int, const char *, int);
};

struct NameValueParserData
{
    LIST_HEAD(listhead, NameValue) head;
    char curelt[64];
};

/* Structures definitions : */
struct UPNParg
{
	const char * elt;
	const char * val;
};

struct NameValue
{
    LIST_ENTRY(NameValue) entries;
    char name[64];
    char value[64];
};

struct STPORTMAP
{
	int nPortE;//????
	int nPortI;//????
	char chProto[10];//??
};

typedef ::std::vector<STPORTMAP> STPortMapList;//????

class CSUpnpCtrl
{
public:
	CSUpnpCtrl();
	CSUpnpCtrl(CCWorker *pWorker);

	virtual ~CSUpnpCtrl();//????

	int AddPortMap(int nPortE, int nPortI, char chProto[10]);//????

	int DelPortMap(int nPortE, char chProto[10]);//??????
	int DelPortMap();//??????

public:
	CCWorker *m_pWorker;

private:
	struct UPNPDev * upnpDiscover(int delay, const char * multicastif,const char * minissdpdsock, int sameport);
	void freeUPNPDevlist(struct UPNPDev * devlist);
	int UPNP_GetIGDFromUrl(const char * rootdescurl,struct UPNPUrls * urls,struct IGDdatas * data, char * lanaddr, int lanaddrlen);
	int	UPNP_GetValidIGD(struct UPNPDev * devlist,struct UPNPUrls * urls,struct IGDdatas * data,char * lanaddr, int lanaddrlen);
	void FreeUPNPUrls(struct UPNPUrls *);

private:
	int ReceiveData(int socket, char * data, int length, int timeout);
	static void parseMSEARCHReply(const char * reply,
								int size,
								const char * * location,
								int * locationsize,
								const char * * st,
								int * stsize);
	void * miniwget_getaddr(const char *, int *, char *, int);
	void parserootdesc(const char *, int, struct IGDdatas *);
	void GetUPNPUrls(struct UPNPUrls *, struct IGDdatas *, const char *);
	int parseURL(const char *, char *, unsigned short *, char * *);
	void * miniwget2(const char * url, const char * host,
					 unsigned short port, const char * path,
					 int * size, char * addr_str, int addr_str_len);

	void parsexml(struct xmlparser *);
	int parseatt(struct xmlparser * p);
	void parseelt(struct xmlparser * p);


	int UPNPIGD_IsConnected(struct UPNPUrls *, struct IGDdatas *);// return 0 or 1
	int UPNP_GetStatusInfo(const char * controlURL,
		                   const char * servicetype,
						   char * status,
						   unsigned int * uptime,
						   char * lastconnerror);
	int UPNP_GetConnectionTypeInfo(const char * controlURL,const char * servicetype,char * connectionType);
	int UPNP_GetLinkLayerMaxBitRates(const char* controlURL,const char* servicetype,unsigned int * bitrateDown,unsigned int * bitrateUp);
	int UPNP_GetExternalIPAddress(const char * controlURL,const char * servicetype,char * extIpAdd);
	int UPNP_GetGenericPortMappingEntry(const char * controlURL,
		                                const char * servicetype,
										const char * index,
										char * extPort,
										char * intClient,
										char * intPort,
										char * protocol,
										char * desc,
										char * enabled,
										char * rHost,
										char * duration);
	int UPNP_AddPortMapping(const char * controlURL,
		                    const char * servicetype,
							const char * extPort,
							const char * inPort,
							const char * inClient,
							const char * desc,
							const char * proto,
							const char * remoteHost);
	int UPNP_GetSpecificPortMappingEntry(const char * controlURL,
										const char * servicetype,
										const char * extPort,
										const char * proto,
										char * intClient,
										char * intPort);
	int UPNP_DeletePortMapping(const char * controlURL, const char * servicetype,const char * extPort, const char * proto,const char * remoteHost);

	int simpleUPnPcommand(int, const char *, const char *,const char *, struct UPNParg *,char *, int *);
	void ParseNameValue(const char * buffer, int bufsize,struct NameValueParserData * data);
	char *GetValueFromNameValueList(struct NameValueParserData * pdata,const char * Name);
	void ClearNameValueList(struct NameValueParserData * pdata);
	int soapPostSubmit(int, const char *, const char *, unsigned short,const char *, const char *);
	void getContentLengthAndHeaderLength(char * p, int n,int * contentlen, int * headerlen);
	int httpWrite(int fd, const char * body, int bodysize,const char * headers, int headerssize);
	int getcontentlenfromline(const char * p, int n);
	void DisplayInfos(struct UPNPUrls * urls, struct IGDdatas * data);
	void ListRedirections(struct UPNPUrls * urls,struct IGDdatas * data);
	int SetRedirectAndTest(struct UPNPUrls * urls,struct IGDdatas * data,const char * iaddr,const char * iport,const char * eport,const char * proto);
	const char * protofix(const char * proto);
	void RemoveRedirect(struct UPNPUrls * urls,struct IGDdatas * data,const char * eport,const char * proto);

	#ifndef WIN32
		struct UPNPDev *getDevicesFromMiniSSDPD(const char * devtype, const char * socketpath);
	#endif

	const char * strupnperror(int err);
private:
	::std::vector<STPORTMAP> m_stPortMaps;
};

#endif // !defined(AFX_SUPNPCTRL_H__659AEDB6_CA61_46CB_917F_73C517F94863__INCLUDED_)
