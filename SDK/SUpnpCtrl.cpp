// SUpnpCtrl.cpp: implementation of the CSUpnpCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "SUpnpCtrl.h"
#include "CWorker.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSUpnpCtrl::CSUpnpCtrl()
{
	m_pWorker = NULL;
	m_stPortMaps.reserve(10);
	m_stPortMaps.clear();
}

CSUpnpCtrl::CSUpnpCtrl(CCWorker *pWorker)
{
	m_pWorker = pWorker;
	m_stPortMaps.reserve(10);
	m_stPortMaps.clear();

}

CSUpnpCtrl::~CSUpnpCtrl()
{
	DelPortMap();
	m_stPortMaps.clear();
}

/****************************************************************************
*??  : AddPortMap
*??  : ????
*??  : [IN] nPortE  ????
         [IN] nPortI  ????
		 [IN] chProto ?? UDP ? TCP
*???: ?
*??  : ?
*****************************************************************************/
int CSUpnpCtrl::AddPortMap(int nPortE, int nPortI, char chProto[10])
{
	int ret = -1;
	if(nPortI <= 0 || nPortE <= 0)
	{
		return -1;
	}

	char chPortE[20]={0};
	char chPortI[20]={0};

	sprintf(chPortI,"%d",nPortI);
	sprintf(chPortE,"%d",nPortE);

	struct UPNPDev * devlist = 0;
	char lanaddr[16];	/* my ip address on the LAN */
	const char * multicastif = 0;
	const char * minissdpdpath = 0;

	if((devlist = upnpDiscover(1000, multicastif, minissdpdpath, 0)))
	{
		struct UPNPUrls urls;
		struct IGDdatas data;

		int i = 1;
		if(i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))
		{
			ret = SetRedirectAndTest(&urls, &data,lanaddr, chPortI,chPortE, chProto);
			FreeUPNPUrls(&urls);
		}
	}

	freeUPNPDevlist(devlist);
	devlist = 0;
	return ret;
}
//??????
int CSUpnpCtrl::DelPortMap(int nPortE, char chProto[10])
{
	if(nPortE <= 0)
	{
		return 0;
	}

	char chPortE[20]={0};

	sprintf(chPortE,"%d",nPortE);

	struct UPNPDev * devlist = 0;
	char lanaddr[16];
	const char * multicastif = 0;
	const char * minissdpdpath = 0;

	int i=0;
	int nCount=m_stPortMaps.size();
	for(i=0; i<nCount; i++)
	{
		if(m_stPortMaps[i].nPortE == nPortE && strcmp(m_stPortMaps[i].chProto, chProto)==0)
		{//????
			if((devlist = upnpDiscover(1000, multicastif, minissdpdpath, 0)))
			{
				struct UPNPUrls urls;
				struct IGDdatas data;

				if(UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)) > 0)
				{
					RemoveRedirect(&urls, &data, chPortE, chProto);

					FreeUPNPUrls(&urls);
				}
			}

			freeUPNPDevlist(devlist);
			devlist = 0;

			m_stPortMaps.erase(m_stPortMaps.begin() + i);
		}
	}

	return 0;
}

//??????
int CSUpnpCtrl::DelPortMap()
{
	char chPortE[20]={0};

	struct UPNPDev * devlist = 0;
	char lanaddr[16];
	const char * multicastif = 0;
	const char * minissdpdpath = 0;

	if((devlist = upnpDiscover(1000, multicastif, minissdpdpath, 0)))
	{
		struct UPNPUrls urls;
		struct IGDdatas data;

		if(UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)) > 0)
		{
			int i = 0;
			int nCount=m_stPortMaps.size();
			for(i=0; i<nCount; i++)
			{
				if(m_stPortMaps[i].nPortE <= 0)
				{
					continue;
				}
				memset(chPortE, 0, 20);
				sprintf(chPortE,"%d",m_stPortMaps[i].nPortE);

				RemoveRedirect(&urls, &data, chPortE, m_stPortMaps[i].chProto);
			}

			FreeUPNPUrls(&urls);
		}
	}

	freeUPNPDevlist(devlist);
	devlist = 0;
	m_stPortMaps.clear();
	return 0;
}


void url_cpy_or_cat(char * dst, const char * src, int n)
{
	if(  (src[0] == 'h')
		&&(src[1] == 't')
		&&(src[2] == 't')
		&&(src[3] == 'p')
		&&(src[4] == ':')
		&&(src[5] == '/')
		&&(src[6] == '/'))
	{
		strncpy(dst, src, n);
	}
	else
	{
		int l = strlen(dst);
		if(src[0] != '/')
			dst[l++] = '/';
		if(l<=n)
			strncpy(dst + l, src, n - l);
	}
}

/* Start element handler :update nesting level counter and copy element name */
void IGDstartelt(void * d, const char * name, int l)
{
	struct IGDdatas * datas = (struct IGDdatas *)d;
	memcpy( datas->cureltname, name, l);
	datas->cureltname[l] = '\0';
	datas->level++;
	if((l==7) && !memcmp(name, "service", l))
	{
		datas->controlurl_tmp[0] = '\0';
		datas->eventsuburl_tmp[0] = '\0';
		datas->scpdurl_tmp[0] = '\0';
		datas->servicetype_tmp[0] = '\0';
	}
}

/* End element handler : update nesting level counter and update parser state if service element is parsed */
void IGDendelt(void * d, const char * name, int l)
{
	struct IGDdatas * datas = (struct IGDdatas *)d;
	datas->level--;
	/*printf("endelt %2d %.*s\n", datas->level, l, name);*/
	if( (l==7) && !memcmp(name, "service", l) )
	{
		if(0==strcmp(datas->servicetype_tmp,"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"))
		{
			memcpy(datas->controlurl_CIF, datas->controlurl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->eventsuburl_CIF, datas->eventsuburl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->scpdurl_CIF, datas->scpdurl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->servicetype_CIF, datas->servicetype_tmp, MINIUPNPC_URL_MAXSIZE);
		}
		else if(0==strcmp(datas->servicetype_tmp,"urn:schemas-upnp-org:service:WANIPConnection:1")
				|| 0==strcmp(datas->servicetype_tmp,"urn:schemas-upnp-org:service:WANPPPConnection:1") )
		{
			memcpy(datas->controlurl, datas->controlurl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->eventsuburl, datas->eventsuburl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->scpdurl, datas->scpdurl_tmp, MINIUPNPC_URL_MAXSIZE);
			memcpy(datas->servicetype, datas->servicetype_tmp, MINIUPNPC_URL_MAXSIZE);
		}
	}
}

/* Data handler :copy data depending on the current element name and state */
void IGDdata(void * d, const char * data, int l)
{
	struct IGDdatas * datas = (struct IGDdatas *)d;
	char * dstmember = 0;
	/*printf("%2d %s : %.*s\n",datas->level, datas->cureltname, l, data);	*/
	if( !strcmp(datas->cureltname, "URLBase"))
	{
		dstmember = datas->urlbase;
	}
	else if( !strcmp(datas->cureltname, "serviceType"))
	{
		dstmember = datas->servicetype_tmp;
	}
	else if( !strcmp(datas->cureltname, "controlURL"))
	{
		dstmember = datas->controlurl_tmp;
	}
	else if( !strcmp(datas->cureltname, "eventSubURL"))
	{
		dstmember = datas->eventsuburl_tmp;
	}
	else if( !strcmp(datas->cureltname, "SCPDURL"))
	{
		dstmember = datas->scpdurl_tmp;
	}
/*	else if( !strcmp(datas->cureltname, "deviceType") )
		dstmember = datas->devicetype_tmp;*/
	if(dstmember)
	{
		if(l>=MINIUPNPC_URL_MAXSIZE)
		{
			l = MINIUPNPC_URL_MAXSIZE-1;
		}
		memcpy(dstmember, data, l);
		dstmember[l] = '\0';
	}
}

void NameValueParserStartElt(void * d, const char * name, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
    if(l>63)
        l = 63;
    memcpy(data->curelt, name, l);
    data->curelt[l] = '\0';
}

void NameValueParserGetData(void * d, const char * datas, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
    struct NameValue * nv;
    nv = (struct NameValue *)malloc(sizeof(struct NameValue));
    if(l>63)
        l = 63;
    strncpy(nv->name, data->curelt, 64);
	nv->name[63] = '\0';
    memcpy(nv->value, datas, l);
    nv->value[l] = '\0';
    LIST_INSERT_HEAD( &(data->head), nv, entries);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
 /* upnpDiscover()
 * discover UPnP devices on the network.
 * The discovered devices are returned as a chained list.
 * It is up to the caller to free the list with freeUPNPDevlist().
 * delay (in millisecond) is the maximum time for waiting any device
 * response.
 * If available, device list will be obtained from MiniSSDPd.
 * Default path for minissdpd socket will be used if minissdpdsock argument
 * is NULL.
 * If multicastif is not NULL, it will be used instead of the default
 * multicast interface for sending SSDP discover packets.
 * If sameport is not null, SSDP packets will be sent from the source port
* 1900 (same as destination port) otherwise system assign a source port. */
struct UPNPDev * CSUpnpCtrl::upnpDiscover(int delay, const char * multicastif,const char * minissdpdsock, int sameport)
{
	struct UPNPDev * tmp;
	struct UPNPDev * devlist = 0;
	int opt = 1;
	static const char MSearchMsgFmt[] =
		"M-SEARCH * HTTP/1.1\r\n"
		"HOST: " UPNP_MCAST_ADDR ":" XSTR(PORT) "\r\n"
		"ST: %s\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"MX: %u\r\n"
		"\r\n";
	static const char * const deviceList[] = {
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
			"urn:schemas-upnp-org:service:WANIPConnection:1",
			"urn:schemas-upnp-org:service:WANPPPConnection:1",
			"upnp:rootdevice",
			0};
	int deviceIndex = 0;
	char bufr[1536];	/* reception and emission buffer */
	int sudp;
	int n;
	struct sockaddr_in sockudp_r, sockudp_w;
	unsigned int mx;

#ifndef WIN32
	/* first try to get infos from minissdpd ! */
	if(!minissdpdsock)
	{
		minissdpdsock = "/var/run/minissdpd.sock";
	}
	while(!devlist && deviceList[deviceIndex])
	{
		devlist = getDevicesFromMiniSSDPD(deviceList[deviceIndex],minissdpdsock);
		/* We return what we have found if it was not only a rootdevice */
		if(devlist && !strstr(deviceList[deviceIndex], "rootdevice"))
		{
			return devlist;
		}
		deviceIndex++;
	}
	deviceIndex = 0;
#endif
	/* fallback to direct discovery */
#ifdef WIN32
	sudp = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
	sudp = socket(PF_INET, SOCK_DGRAM, 0);
#endif
	if(sudp < 0)
	{
//		PRINT_SOCKET_ERROR("socket");
		return NULL;
	}
    /* reception */
    memset(&sockudp_r, 0, sizeof(struct sockaddr_in));
    sockudp_r.sin_family = AF_INET;
	if(sameport)
		sockudp_r.sin_port = htons(PORT);
    sockudp_r.sin_addr.s_addr = INADDR_ANY;
    /* emission */
    memset(&sockudp_w, 0, sizeof(struct sockaddr_in));
    sockudp_w.sin_family = AF_INET;
    sockudp_w.sin_port = htons(PORT);
    sockudp_w.sin_addr.s_addr = inet_addr(UPNP_MCAST_ADDR);

#ifdef WIN32
	if (setsockopt(sudp, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof (opt)) < 0)
#else
	if (setsockopt(sudp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) < 0)
#endif
	{
//		PRINT_SOCKET_ERROR("setsockopt");
		return NULL;
	}

	if(multicastif)
	{
		struct in_addr mc_if;
		mc_if.s_addr = inet_addr(multicastif);
		sockudp_r.sin_addr.s_addr = mc_if.s_addr;
		if(setsockopt(sudp, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&mc_if, sizeof(mc_if)) < 0)
		{
//			PRINT_SOCKET_ERROR("setsockopt");
		}
	}

	/* Avant d'envoyer le paquet on bind pour recevoir la reponse */
	if (bind(sudp, (struct sockaddr *)&sockudp_r, sizeof(struct sockaddr_in)) != 0)
	{
//		PRINT_SOCKET_ERROR("bind");
		closesocket(sudp);
		return NULL;
	}

	/* Calculating maximum response time in seconds */
	mx = ((unsigned int)delay) / 1000u;
	/* receiving SSDP response packet */
	for(n = 0;;)
	{
		if(n == 0)
		{
			/* sending the SSDP M-SEARCH packet */
			n = snprintf(bufr, sizeof(bufr),MSearchMsgFmt, deviceList[deviceIndex++], mx);
			/*printf("Sending %s", bufr);*/
			n = sendto(sudp, bufr, n, 0,(struct sockaddr *)&sockudp_w, sizeof(struct sockaddr_in));
			if (n < 0)
			{
//				PRINT_SOCKET_ERROR("sendto");
				closesocket(sudp);
				return devlist;
			}
		}
		/* Waiting for SSDP REPLY packet to M-SEARCH */
		n = ReceiveData(sudp, bufr, sizeof(bufr), delay);
		if (n < 0)
		{
			/* error */
			closesocket(sudp);
			return devlist;
		}
		else if (n == 0)
		{
			/* no data or Time Out */
			if (devlist || (deviceList[deviceIndex] == 0))
			{
				/* no more device type to look for... */
				closesocket(sudp);
				return devlist;
			}
		}
		else
		{
			const char * descURL=NULL;
			int urlsize=0;
			const char * st=NULL;
			int stsize=0;
			/*printf("%d byte(s) :\n%s\n", n, bufr);*/ /* affichage du message */
			parseMSEARCHReply(bufr, n, &descURL, &urlsize, &st, &stsize);
			if(st&&descURL)
			{
			/*printf("M-SEARCH Reply:\nST: %.*s\nLocation: %.*s\n",
				stsize, st, urlsize, descURL); */
				tmp = (struct UPNPDev *)malloc(sizeof(struct UPNPDev)+urlsize+stsize);
				tmp->pNext = devlist;
				tmp->descURL = tmp->buffer;
				tmp->st = tmp->buffer + 1 + urlsize;
				memcpy(tmp->buffer, descURL, urlsize);
				tmp->buffer[urlsize] = '\0';
				memcpy(tmp->buffer + urlsize + 1, st, stsize);
				tmp->buffer[urlsize+1+stsize] = '\0';
				devlist = tmp;
			}
		}
	}
}

/* freeUPNPDevlist() should be used to free the chained list returned by upnpDiscover() */
void CSUpnpCtrl::freeUPNPDevlist(struct UPNPDev * devlist)
{
	struct UPNPDev * next;
	while(devlist)
	{
		next = devlist->pNext;
		free(devlist);
		devlist = next;
	}
}


int CSUpnpCtrl::ReceiveData(int socket, char * data, int length, int timeout)
{
	if(length <= 0)
	{
		return 0;
	}

    int n;
#ifndef WIN32
    struct pollfd fds[1]; /* for the poll */
#ifdef MINIUPNPC_IGNORE_EINTR
    do {
#endif
        fds[0].fd = socket;
        fds[0].events = POLLIN;
        n = poll(fds, 1, timeout);
#ifdef MINIUPNPC_IGNORE_EINTR
    } while(n < 0 && errno == EINTR);
#endif
    if(n < 0)
    {
//        PRINT_SOCKET_ERROR("poll");
        return -1;
    }
    else if(n == 0)
    {
        return 0;
    }
#else
    fd_set socketSet;
    TIMEVAL timeval;
    FD_ZERO(&socketSet);
    FD_SET(socket, &socketSet);
    timeval.tv_sec = timeout / 1000;
    timeval.tv_usec = (timeout % 1000) * 1000;
    n = select(FD_SETSIZE, &socketSet, NULL, NULL, &timeval);
    if(n < 0)
    {
//        PRINT_SOCKET_ERROR("select");
        return -1;
    }
    else if(n == 0)
    {
        return 0;
    }
#endif

#ifdef WIN32
	n = recv(socket, data, length, 0);
#else
	n = recv(socket, data, length, MSG_NOSIGNAL);
#endif
	if(n<0)
	{
//		PRINT_SOCKET_ERROR("recv");
	}
	return n;
}


/* parseMSEARCHReply()
* the last 4 arguments are filled during the parsing :
*    - location/locationsize : "location:" field of the SSDP reply packet
*    - st/stsize : "st:" field of the SSDP reply packet.
* The strings are NOT null terminated */
void CSUpnpCtrl::parseMSEARCHReply(const char * reply,
									int size,
									const char * * location,
									int * locationsize,
									const char * * st,
									int * stsize)
{
	int a, b, i;
	i = 0;
	a = i;	/* start of the line */
	b = 0;
	while(i<size)
	{
		switch(reply[i])
		{
		case ':':
			if(b==0)
			{
				b = i; /* end of the "header" */
					   /*for(j=a; j<b; j++)
					   {
					   putchar(reply[j]);
					   }
				*/
			}
			break;
		case '\x0a':
		case '\x0d':
			if(b!=0)
			{
			/*for(j=b+1; j<i; j++)
			{
			putchar(reply[j]);
			}
				putchar('\n');*/
				do { b++; } while(reply[b]==' ');
				if(0==strncasecmp(reply+a, "location", 8))
				{
					*location = reply+b;
					*locationsize = i-b;
				}
				else if(0==strncasecmp(reply+a, "st", 2))
				{
					*st = reply+b;
					*stsize = i-b;
				}
				b = 0;
			}
			a = i+1;
			break;
		default:
			break;
		}
		i++;
	}
}

/* UPNP_GetIGDFromUrl()
* Used when skipping the discovery process.
* return value :
*   0 - Not ok
*   1 - OK */
int CSUpnpCtrl::UPNP_GetIGDFromUrl(const char * rootdescurl,
                                   struct UPNPUrls * urls,
                                   struct IGDdatas * data,
                                   char * lanaddr,
								   int lanaddrlen)
{
	char * descXML;
	int descXMLsize = 0;
	descXML = (char *)miniwget_getaddr(rootdescurl, &descXMLsize,lanaddr, lanaddrlen);
	if(descXML)
	{
		memset(data, 0, sizeof(struct IGDdatas));
		memset(urls, 0, sizeof(struct UPNPUrls));
		parserootdesc(descXML, descXMLsize, data);
		free(descXML);
		descXML = NULL;
		GetUPNPUrls(urls, data, rootdescurl);
		return 1;
	}
	else
	{
		return 0;
	}
}

/* UPNP_GetValidIGD() :
* return values :
*     0 = NO IGD found
*     1 = A valid connected IGD has been found
*     2 = A valid IGD has been found but it reported as
*         not connected
*     3 = an UPnP device has been found but was not recognized as an IGD
*
* In any non zero return case, the urls and data structures
* passed as parameters are set. Donc forget to call FreeUPNPUrls(urls) to
* free allocated memory.*/
int CSUpnpCtrl::UPNP_GetValidIGD(struct UPNPDev * devlist,
								 struct UPNPUrls * urls,
								 struct IGDdatas * data,
								 char * lanaddr, int lanaddrlen)
{
	char * descXML;
	int descXMLsize = 0;
	struct UPNPDev * dev;
	int ndev = 0;
	int state; /* state 1 : IGD connected. State 2 : IGD. State 3 : anything */
	if(!devlist)
	{
		return 0;
	}
	for(state = 1; state <= 3; state++)
	{
		for(dev = devlist; dev; dev = dev->pNext)
		{
		/* we should choose an internet gateway device.with st == urn:schemas-upnp-org:device:InternetGatewayDevice:1 */
			descXML = (char *)miniwget_getaddr(dev->descURL, &descXMLsize,lanaddr, lanaddrlen);
			if(descXML)
			{
				ndev++;
				memset(data, 0, sizeof(struct IGDdatas));
				memset(urls, 0, sizeof(struct UPNPUrls));
				parserootdesc(descXML, descXMLsize, data);
				free(descXML);
				descXML = NULL;
				if(0==strcmp(data->servicetype_CIF,"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1") || state >= 3 )
				{
					GetUPNPUrls(urls, data, dev->descURL);

					if((state >= 2) || UPNPIGD_IsConnected(urls, data))
					{
						return state;
					}
					FreeUPNPUrls(urls);
				}
				memset(data, 0, sizeof(struct IGDdatas));
			}
		}
	}
	return 0;
}

void CSUpnpCtrl::FreeUPNPUrls(struct UPNPUrls * urls)
{
	if(!urls)
	{
		return;
	}
	free(urls->controlURL);
	urls->controlURL = 0;
	free(urls->ipcondescURL);
	urls->ipcondescURL = 0;
	free(urls->controlURL_CIF);
	urls->controlURL_CIF = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void * CSUpnpCtrl::miniwget_getaddr(const char * url, int * size, char * addr, int addrlen)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(addr)
	{
		addr[0] = '\0';
	}
	if(!parseURL(url, hostname, &port, &path))
	{
		return NULL;
	}
	return miniwget2(url, hostname, port, path, size, addr, addrlen);
}

/* parserootdesc() : parse root XML description of a UPnP device and fill the IGDdatas structure. */
void CSUpnpCtrl::parserootdesc(const char * buffer, int bufsize, struct IGDdatas * data)
{
	struct xmlparser parser;
	/* xmlparser object */
	parser.xmlstart = buffer;
	parser.xmlsize = bufsize;
	parser.data = data;
	parser.starteltfunc = IGDstartelt;
	parser.endeltfunc = IGDendelt;
	parser.datafunc = IGDdata;
	parser.attfunc = 0;
	parsexml(&parser);
//#ifdef DEBUG
//	printIGD(data);
//#endif
}

/* Prepare the Urls for usage...*/
void CSUpnpCtrl::GetUPNPUrls(struct UPNPUrls * urls, struct IGDdatas * data,const char * descURL)
{
	char * p;
	int n1, n2, n3;
	n1 = strlen(data->urlbase);
	if(n1==0)
		n1 = strlen(descURL);
	n1 += 2;	/* 1 byte more for Null terminator, 1 byte for '/' if needed */
	n2 = n1; n3 = n1;
	n1 += strlen(data->scpdurl);
	n2 += strlen(data->controlurl);
	n3 += strlen(data->controlurl_CIF);

	urls->ipcondescURL = (char *)malloc(n1);
	urls->controlURL = (char *)malloc(n2);
	urls->controlURL_CIF = (char *)malloc(n3);
	/* maintenant on chope la desc du WANIPConnection */
	if(data->urlbase[0] != '\0')
	{
		strncpy(urls->ipcondescURL, data->urlbase, n1);
	}
	else
	{
		strncpy(urls->ipcondescURL, descURL, n1);
	}
	p = strchr(urls->ipcondescURL+7, '/');
	if(p)
	{
		p[0] = '\0';
	}
	strncpy(urls->controlURL, urls->ipcondescURL, n2);
	strncpy(urls->controlURL_CIF, urls->ipcondescURL, n3);

	url_cpy_or_cat(urls->ipcondescURL, data->scpdurl, n1);

	url_cpy_or_cat(urls->controlURL, data->controlurl, n2);

	url_cpy_or_cat(urls->controlURL_CIF, data->controlurl_CIF, n3);

#ifdef DEBUG
	printf("urls->ipcondescURL='%s' %d n1=%d\n", urls->ipcondescURL,strlen(urls->ipcondescURL), n1);
	printf("urls->controlURL='%s' %d n2=%d\n", urls->controlURL,strlen(urls->controlURL), n2);
	printf("urls->controlURL_CIF='%s' %d n3=%d\n", urls->controlURL_CIF,strlen(urls->controlURL_CIF), n3);
#endif
}

/* parseURL()
* arguments :
*   url :		source string not modified
*   hostname :	hostname destination string (size of MAXHOSTNAMELEN+1)
*   port :		port (destination)
*   path :		pointer to the path part of the URL
*
* Return values :
*    0 - Failure
*    1 - Success         */
int CSUpnpCtrl::parseURL(const char * url, char * hostname, unsigned short * port, char * * path)
{
	char * p1, *p2, *p3;
	p1 = strstr((char*)url, "://");
	if(!p1)
	{
		return 0;
	}
	p1 += 3;
	if((url[0]!='h') || (url[1]!='t') || (url[2]!='t') || (url[3]!='p'))
	{
		return 0;
	}
	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
	{
		return 0;
	}
	memset(hostname, 0, MAXHOSTNAMELEN + 1);
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		*port = 80;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	*path = p3;
	return 1;
}

/* miniwget2():* */
void * CSUpnpCtrl::miniwget2(const char * url, const char * host,
		                unsigned short port, const char * path,
		                int * size, char * addr_str, int addr_str_len)
{
	char buf[2048];
    int s;
	struct sockaddr_in dest;
	struct hostent *hp;
	int n;
	int len;
	int sent;
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	struct timeval timeout;
#endif
	*size = 0;
	hp = gethostbyname(host);
	if(hp==NULL)
	{
//		herror(host);
		return NULL;
	}
	/*  memcpy((char *)&dest.sin_addr, hp->h_addr, hp->h_length);  */
	memcpy(&dest.sin_addr, hp->h_addr, sizeof(dest.sin_addr));
	memset(dest.sin_zero, 0, sizeof(dest.sin_zero));
	s = socket(PF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
//		perror("socket");
		return NULL;
	}
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	/* setting a 3 seconds timeout for the connect() call */
	timeout.tv_sec = 1;//3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
//		perror("setsockopt");
	}
	timeout.tv_sec = 1;//3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
//		perror("setsockopt");
	}
#endif
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	n = connect(s, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
#ifdef MINIUPNPC_IGNORE_EINTR
	while(n < 0 && errno == EINTR)
	{
		socklen_t len;
		fd_set wset;
		int err;
		FD_ZERO(&wset);
		FD_SET(s, &wset);
		if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
		{
			continue;
		}
		/*len = 0;*/
		/*n = getpeername(s, NULL, &len);*/
		len = sizeof(err);
		if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
		{
//			perror("getsockopt");
			closesocket(s);
			return NULL;
		}
		if(err != 0) {
			errno = err;
			n = -1;
		}
	}
#endif
	if(n<0)
	{
//		perror("connect");
		closesocket(s);
		return NULL;
	}

	/* get address for caller ! */
	if(addr_str)
	{
		struct sockaddr_in saddr;
		socklen_t saddrlen;

		saddrlen = sizeof(saddr);
		if(getsockname(s, (struct sockaddr *)&saddr, &saddrlen) < 0)
		{
//			perror("getsockname");
		}
		else
		{
#ifndef WIN32
			inet_ntop(AF_INET, &saddr.sin_addr, addr_str, addr_str_len);
#else
	/* using INT WINAPI WSAAddressToStringA(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOA, LPSTR, LPDWORD);
     * But his function make a string with the port :  nn.nn.nn.nn:port */
/*		if(WSAAddressToStringA((SOCKADDR *)&saddr, sizeof(saddr),
                            NULL, addr_str, (DWORD *)&addr_str_len))
		{
		    printf("WSAAddressToStringA() failed : %d\n", WSAGetLastError());
		}*/
			strncpy(addr_str, inet_ntoa(saddr.sin_addr), addr_str_len);
#endif
		}
		//printf("address miniwget : %s\n", addr_str);
	}

	len = snprintf(buf, sizeof(buf),
                 "GET %s HTTP/1.1\r\n"
			     "Host: %s:%d\r\n"
				 "Connection: Close\r\n"
				 "User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

				 "\r\n",path, host, port);
	sent = 0;
	/* sending the HTTP request */
	while(sent < len)
	{
	#ifdef WIN32
		n = send(s, buf+sent, len-sent, 0);
	#else
		n = send(s, buf+sent, len-sent, MSG_NOSIGNAL);
	#endif
		if(n < 0)
		{
//			perror("send");
			closesocket(s);
			return NULL;
		}
		else
		{
			sent += n;
		}
	}
	{
		int headers=1;
		char * respbuffer = NULL;
		int allreadyread = 0;
		/*while((n = recv(s, buf, 2048, 0)) > 0)*/
		while((n = ReceiveData(s, buf, 2048, 10)) >0 )//5000)) > 0)
		{
			if(headers)
			{
				int i=0;
				while(i<n-3)
				{
					/* searching for the end of the HTTP headers */
					if(buf[i]=='\r' && buf[i+1]=='\n'
					   && buf[i+2]=='\r' && buf[i+3]=='\n')
					{
						headers = 0;	/* end */
						if(i<n-4)
						{
							/* Copy the content into respbuffet */
							respbuffer = (char *)realloc((void *)respbuffer,
														 allreadyread+(n-i-4));
							memcpy(respbuffer+allreadyread, buf + i + 4, n-i-4);
							allreadyread += (n-i-4);
						}
						break;
					}
					i++;
				}
			}
			else
			{
				respbuffer = (char *)realloc((void *)respbuffer,
								 allreadyread+n);
				memcpy(respbuffer+allreadyread, buf, n);
				allreadyread += n;
			}
		}
		*size = allreadyread;

		//printf("%d bytes read\n", *size);

		closesocket(s);
		return respbuffer;
	}
}

/* parsexml()
* the xmlparser structure must be initialized before the call
* the following structure members have to be initialized :
* xmlstart, xmlsize, data, *func
* xml is for internal usage, xmlend is computed automatically */
void CSUpnpCtrl::parsexml(struct xmlparser * parser)
{
	parser->xml = parser->xmlstart;
	parser->xmlend = parser->xmlstart + parser->xmlsize;
	parseelt(parser);
}

/* parseatt : used to parse the argument list return 0 (false) in case of success and -1 (true) if the end of the xmlbuffer is reached. */
int CSUpnpCtrl::parseatt(struct xmlparser * p)
{
	const char * attname;
	int attnamelen;
	const char * attvalue;
	int attvaluelen;
	while(p->xml < p->xmlend)
	{
		if(*p->xml=='/' || *p->xml=='>')
		{
			return 0;
		}
		if( !IS_WHITE_SPACE(*p->xml) )
		{
			char sep;
			attname = p->xml;
			attnamelen = 0;
			while(*p->xml!='=' && !IS_WHITE_SPACE(*p->xml))
			{
				attnamelen++; p->xml++;
				if(p->xml >= p->xmlend)
				{
					return -1;
				}
			}
			while(*(p->xml++) != '=')
			{
				if(p->xml >= p->xmlend)
				{
					return -1;
				}
			}
			while(IS_WHITE_SPACE(*p->xml))
			{
				p->xml++;
				if(p->xml >= p->xmlend)
				{
					return -1;
				}
			}
			sep = *p->xml;
			if(sep=='\'' || sep=='\"')
			{
				p->xml++;
				if(p->xml >= p->xmlend)
				{
					return -1;
				}
				attvalue = p->xml;
				attvaluelen = 0;
				while(*p->xml != sep)
				{
					attvaluelen++; p->xml++;
					if(p->xml >= p->xmlend)
					{
						return -1;
					}
				}
			}
			else
			{
				attvalue = p->xml;
				attvaluelen = 0;
				while(!IS_WHITE_SPACE(*p->xml) && *p->xml != '>' && *p->xml != '/')
				{
					attvaluelen++; p->xml++;
					if(p->xml >= p->xmlend)
					{
						return -1;
					}
				}
			}
			/*printf("%.*s='%.*s'\n",attnamelen, attname, attvaluelen, attvalue);*/
			if(p->attfunc)
			{
				p->attfunc(p->data, attname, attnamelen, attvalue, attvaluelen);
			}
		}
		p->xml++;
	}
	return -1;
}

/* parseelt parse the xml stream and call the callback functions when needed... */
void CSUpnpCtrl::parseelt(struct xmlparser * p)
{
	int i;
	const char * elementname;
	while(p->xml < (p->xmlend - 1))
	{
		if((p->xml)[0]=='<' && (p->xml)[1]!='?')
		{
			i = 0; elementname = ++p->xml;
			while( !IS_WHITE_SPACE(*p->xml) && (*p->xml!='>') && (*p->xml!='/'))
			{
				i++; p->xml++;
				if (p->xml >= p->xmlend)
				{
					return;
				}
				/* to ignore namespace : */
				if(*p->xml==':')
				{
					i = 0;
					elementname = ++p->xml;
				}
			}
			if(i>0)
			{
				if(p->starteltfunc)
				{
					p->starteltfunc(p->data, elementname, i);
				}
				if(parseatt(p))
				{
					return;
				}
				if(*p->xml!='/')
				{
					const char * data;
					i = 0; data = ++p->xml;
					if (p->xml >= p->xmlend)
					{
						return;
					}
					while( IS_WHITE_SPACE(*p->xml) )
					{
						p->xml++;
						if (p->xml >= p->xmlend)
						{
							return;
						}
					}
					while(*p->xml!='<')
					{
						i++; p->xml++;
						if (p->xml >= p->xmlend)
						{
							return;
						}
					}
					if(i>0 && p->datafunc)
					{
						p->datafunc(p->data, data, i);
					}
				}
			}
			else if(*p->xml == '/')
			{
				i = 0; elementname = ++p->xml;
				if (p->xml >= p->xmlend)
				{
					return;
				}
				while((*p->xml != '>'))
				{
					i++; p->xml++;
					if (p->xml >= p->xmlend)
					{
						return;
					}
				}
				if(p->endeltfunc)
				{
					p->endeltfunc(p->data, elementname, i);
				}
				p->xml++;
			}
		}
		else
		{
			p->xml++;
		}
	}
}

int CSUpnpCtrl::UPNPIGD_IsConnected(struct UPNPUrls * urls, struct IGDdatas * data)
{
	char status[64];
	unsigned int uptime;
	status[0] = '\0';
	UPNP_GetStatusInfo(urls->controlURL, data->servicetype,status, &uptime, NULL);
	if(0 == strcmp("Connected", status))
	{
		return 1;
	}
	else
		return 0;
}

/* UPNP_GetStatusInfo() call the corresponding UPNP method returns the current status and uptime */
int CSUpnpCtrl::UPNP_GetStatusInfo(const char * controlURL,
								   const char * servicetype,
								   char * status,
								   unsigned int * uptime,
								   char * lastconnerror)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	char * up;
	char * err;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!status && !uptime)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	simpleUPnPcommand(-1, controlURL, servicetype, "GetStatusInfo", 0, buffer, &bufsize);
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	up = GetValueFromNameValueList(&pdata, "NewUptime");
	p = GetValueFromNameValueList(&pdata, "NewConnectionStatus");
	err = GetValueFromNameValueList(&pdata, "NewLastConnectionError");
	if(p && up)
	{
		ret = UPNPCOMMAND_SUCCESS;
	}

	if(status)
	{
		if(p)
		{
			strncpy(status, p, 64 );
			status[63] = '\0';
		}
		else
		{
			status[0]= '\0';
		}
	}

	if(uptime)
	{
		if(up)
		{
			sscanf(up,"%u",uptime);
		}
		else
		{
			uptime = 0;
		}
	}

	if(lastconnerror)
	{
		if(err)
		{
			strncpy(lastconnerror, err, 64 );
			lastconnerror[63] = '\0';
		}
		else
		{
			lastconnerror[0] = '\0';
		}
	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}

/* simpleUPnPcommand :not so simple ! return values :   0 - OK  -1 - error */
int CSUpnpCtrl::simpleUPnPcommand(int s, const char * url, const char * service,
                                  const char * action, struct UPNParg * args,
                                  char * buffer, int * bufsize)
{
	struct sockaddr_in dest;
	char hostname[MAXHOSTNAMELEN+1];
	unsigned short port = 0;
	char * path;
	char soapact[128];
	char soapbody[2048];
	char * buf;
	int buffree;
    int n;
	int contentlen, headerlen;	/* for the response */
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	struct timeval timeout;
#endif
	snprintf(soapact, sizeof(soapact), "%s#%s", service, action);
	if(args==NULL)
	{
		/*soapbodylen = */snprintf(soapbody, sizeof(soapbody),
						"<?xml version=\"1.0\"?>\r\n"
	    	              "<" SOAPPREFIX ":Envelope "
						  "xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						  SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						  "<" SOAPPREFIX ":Body>"
						  "<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">"
						  "</" SERVICEPREFIX ":%s>"
						  "</" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>"
					 	  "\r\n", action, service, action);
	}
	else
	{
		char * p;
		const char * pe, * pv;
		int soapbodylen;
		soapbodylen = snprintf(soapbody, sizeof(soapbody),
						"<?xml version=\"1.0\"?>\r\n"
	    	            "<" SOAPPREFIX ":Envelope "
						"xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						"<" SOAPPREFIX ":Body>"
						"<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">",
						action, service);
		p = soapbody + soapbodylen;
		while(args->elt)
		{
			/* check that we are never overflowing the string... */
			if(soapbody + sizeof(soapbody) <= p + 100)
			{
				/* we keep a margin of at least 100 bytes */
				*bufsize = 0;
				return -1;
			}
			*(p++) = '<';
			pe = args->elt;
			while(*pe)
				*(p++) = *(pe++);
			*(p++) = '>';
			if((pv = args->val))
			{
				while(*pv)
					*(p++) = *(pv++);
			}
			*(p++) = '<';
			*(p++) = '/';
			pe = args->elt;
			while(*pe)
				*(p++) = *(pe++);
			*(p++) = '>';
			args++;
		}
		*(p++) = '<';
		*(p++) = '/';
		*(p++) = SERVICEPREFIX2;
		*(p++) = ':';
		pe = action;
		while(*pe)
			*(p++) = *(pe++);
		strncpy(p, "></" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>\r\n",soapbody + sizeof(soapbody) - p);
	}
	if(!parseURL(url, hostname, &port, &path)) return -1;
	if(s<0)
	{
		s = socket(PF_INET, SOCK_STREAM, 0);
		if(s<0)
		{
//			PRINT_SOCKET_ERROR("socket");
			*bufsize = 0;
			return -1;
		}
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
		/* setting a 3 seconds timeout for the connect() call */
		timeout.tv_sec = 5;//4;//3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
//			PRINT_SOCKET_ERROR("setsockopt");
		}
		timeout.tv_sec = 5;//4;//3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
//			PRINT_SOCKET_ERROR("setsockopt");
		}
#endif
		dest.sin_family = AF_INET;
		dest.sin_port = htons(port);
		dest.sin_addr.s_addr = inet_addr(hostname);
        n = connect(s, (struct sockaddr *)&dest, sizeof(struct sockaddr));
#ifdef MINIUPNPC_IGNORE_EINTR
        while(n < 0 && errno == EINTR)
		{
			socklen_t len;
			fd_set wset;
			int err;
			FD_ZERO(&wset);
			FD_SET(s, &wset);
			if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
				continue;
			/*len = 0;*/
			/*n = getpeername(s, NULL, &len);*/
			len = sizeof(err);
			if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
			{
//				PRINT_SOCKET_ERROR("getsockopt");
				closesocket(s);
				return -1;
			}
			if(err != 0)
			{
				errno = err;
				n = -1;
			}
			else
			{
				n = 0;
			}
		}
#endif
		if(n < 0)
        {
//			PRINT_SOCKET_ERROR("connect");
			closesocket(s);
			*bufsize = 0;
			return -1;
		}
	}

	n = soapPostSubmit(s, path, hostname, port, soapact, soapbody);
	if(n<=0)
	{
//		printf("Error sending SOAP request\n");
		closesocket(s);
		return -1;
	}

	contentlen = -1;
	headerlen = -1;
	buf = buffer;
	buffree = *bufsize;
	*bufsize = 0;
	while ((n = ReceiveData(s, buf, buffree, 10)) > 0)//5000)) > 0)
	{
		buffree -= n;
		buf += n;
		*bufsize += n;
		getContentLengthAndHeaderLength(buffer, *bufsize,&contentlen, &headerlen);

//		printf("received n=%dbytes bufsize=%d ContLen=%d HeadLen=%d\n",n, *bufsize, contentlen, headerlen);
		/* break if we received everything */
		if(contentlen > 0 && headerlen > 0 && *bufsize >= contentlen+headerlen)
			break;
	}

	closesocket(s);
	return 0;
}

void CSUpnpCtrl::ParseNameValue(const char * buffer, int bufsize,struct NameValueParserData * data)
{
    struct xmlparser parser;
    LIST_INIT(&(data->head));
    /* init xmlparser object */
    parser.xmlstart = buffer;
    parser.xmlsize = bufsize;
    parser.data = data;
    parser.starteltfunc = NameValueParserStartElt;
    parser.endeltfunc = 0;
    parser.datafunc = NameValueParserGetData;
	parser.attfunc = 0;
    parsexml(&parser);
}

void CSUpnpCtrl::ClearNameValueList(struct NameValueParserData * pdata)
{
    struct NameValue * nv;
    while((nv = pdata->head.lh_first) != NULL)
    {
        LIST_REMOVE(nv, entries);
        free(nv);
    }
}

char *CSUpnpCtrl::GetValueFromNameValueList(struct NameValueParserData * pdata,
                          const char * Name)
{
    struct NameValue * nv;
    char * p = NULL;
    for(nv = pdata->head.lh_first;
	(nv != NULL) && (p == NULL);
	nv = nv->entries.le_next)
    {
        if(strcmp(nv->name, Name) == 0)
            p = nv->value;
    }
    return p;
}

/* self explanatory  */
int CSUpnpCtrl::soapPostSubmit(int fd,const char * url,const char * host, unsigned short port,const char * action,const char * body)
{
	int bodysize;
	char headerbuf[512];
	int headerssize;
	char portstr[8];
	bodysize = (int)strlen(body);
	/* We are not using keep-alive HTTP connections.
	* HTTP/1.1 needs the header Connection: close to do that.
	* This is the default with HTTP/1.0 */
    /* Connection: Close is normally there only in HTTP/1.1 but who knows */
	portstr[0] = '\0';
	if(port != 80)
		snprintf(portstr, sizeof(portstr), ":%hu", port);
	headerssize = snprintf(headerbuf, sizeof(headerbuf),
		"POST %s HTTP/1.1\r\n"
		/*                       "POST %s HTTP/1.0\r\n"*/
		"Host: %s%s\r\n"
					   "User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"
					   "Content-Length: %d\r\n"
					   "Content-Type: text/xml\r\n"
					   "SOAPAction: \"%s\"\r\n"
					   "Connection: Close\r\n"
					   "Cache-Control: no-cache\r\n"	/* ??? */
					   "Pragma: no-cache\r\n"
					   "\r\n",
					   url, host, portstr, bodysize, action);

//	printf("SOAP request : headersize=%d bodysize=%d\n",headerssize, bodysize);

	return httpWrite(fd, body, bodysize, headerbuf, headerssize);
}

void CSUpnpCtrl::getContentLengthAndHeaderLength(char * p, int n,int * contentlen, int * headerlen)
{
	char * line;
	int linelen;
	int r;
	line = p;
	while(line < p + n)
	{
		linelen = 0;
		while(line[linelen] != '\r' && line[linelen] != '\r')
		{
			if(line+linelen >= p+n)
				return;
			linelen++;
		}
		r = getcontentlenfromline(line, linelen);
		if(r>0)
			*contentlen = r;
		line = line + linelen + 2;
		if(line[0] == '\r' && line[1] == '\n')
		{
			*headerlen = (line - p) + 2;
			return;
		}
	}
}

/* httpWrite sends the headers and the body to the socket and returns the number of bytes sent */
int CSUpnpCtrl::httpWrite(int fd, const char * body, int bodysize,const char * headers, int headerssize)
{
	int n = 0;
	/*n = write(fd, headers, headerssize);*/
	/*if(bodysize>0)
	n += write(fd, body, bodysize);*/
	/* Note : my old linksys router only took into account
	* soap request that are sent into only one packet */
	char * p;
	/* TODO: AVOID MALLOC */
	p = (char *)malloc(headerssize+bodysize);
	if(!p)
	{
		return 0;
	}
	memcpy(p, headers, headerssize);
	memcpy(p+headerssize, body, bodysize);
	/*n = write(fd, p, headerssize+bodysize);*/
#ifdef WIN32
	n = send(fd, p, headerssize+bodysize, 0);
#else
	n = send(fd, p, headerssize+bodysize, MSG_NOSIGNAL);
#endif
	if(n<0)
	{
//		PRINT_SOCKET_ERROR("send");
	}
	/* disable send on the socket */
	/* draytek routers dont seems to like that... */
#if 0
	#ifdef WIN32
		if(shutdown(fd, SD_SEND)<0)
	#else
		if(shutdown(fd, SHUT_WR)<0)
	#endif
		{ /*SD_SEND*/
			PRINT_SOCKET_ERROR("shutdown");
		}
#endif
		free(p);
		return n;
}

/* Content-length: nnn */
int CSUpnpCtrl::getcontentlenfromline(const char * p, int n)
{
	static const char contlenstr[] = "content-length";
	const char * p2 = contlenstr;
	int a = 0;
	while(*p2)
	{
		if(n==0)
			return -1;
		if(*p2 != *p && *p2 != (*p + 32))
			return -1;
		p++; p2++; n--;
	}
	if(n==0)
		return -1;
	if(*p != ':')
		return -1;
	p++; n--;
	while(*p == ' ')
	{
		if(n==0)
			return -1;
		p++; n--;
	}
	while(*p >= '0' && *p <= '9')
	{
		if(n==0)
			return -1;
		a = (a * 10) + (*p - '0');
		p++; n--;
	}
	return a;
}


void CSUpnpCtrl::DisplayInfos(struct UPNPUrls * urls, struct IGDdatas * data)
{
	char externalIPAddress[16];
	char connectionType[64];
	char status[64];
	char lastconnerr[64];
	unsigned int uptime;
	unsigned int brUp, brDown;
	int r;
	UPNP_GetConnectionTypeInfo(urls->controlURL,data->servicetype,connectionType);
	if(connectionType[0])
	{
		m_pWorker->m_Log.SetRunInfo(0,"Connection Type : ", __FILE__,__LINE__,connectionType);
	}
	else
	{
		m_pWorker->m_Log.SetRunInfo(0,"GetConnectionTypeInfo failed. ", __FILE__,__LINE__);
	}
	UPNP_GetStatusInfo(urls->controlURL, data->servicetype,status, &uptime, lastconnerr);

	char ch[2000]={0};
	sprintf(ch,"Status : %s, uptime=%u, LastConnectionError : %s",status, uptime, lastconnerr);
	m_pWorker->m_Log.SetRunInfo(0,"---", __FILE__,__LINE__,ch);

	UPNP_GetLinkLayerMaxBitRates(urls->controlURL_CIF, data->servicetype_CIF,&brDown, &brUp);

	memset(ch,0,2000);
	sprintf(ch,"MaxBitRateDown : %u bps   MaxBitRateUp %u bps", brDown, brUp);
	m_pWorker->m_Log.SetRunInfo(0,"---", __FILE__,__LINE__,ch);

	r = UPNP_GetExternalIPAddress(urls->controlURL,data->servicetype,externalIPAddress);
	if(r != UPNPCOMMAND_SUCCESS)
	{
		memset(ch,0,2000);
		sprintf(ch,"GetExternalIPAddress() returned %d", r);
		m_pWorker->m_Log.SetRunInfo(0,"---", __FILE__,__LINE__,ch);
	}
	if(externalIPAddress[0])
	{
		m_pWorker->m_Log.SetRunInfo(0,"ExternalIPAddress = ", __FILE__,__LINE__,externalIPAddress);
	}
	else
	{
		m_pWorker->m_Log.SetRunInfo(0,"GetExternalIPAddress failed.", __FILE__,__LINE__);
	}
}

/* UPNP_GetConnectionTypeInfo() call the corresponding UPNP method returns the connection type */
int CSUpnpCtrl::UPNP_GetConnectionTypeInfo(const char * controlURL,const char * servicetype,char * connectionType)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!connectionType)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	simpleUPnPcommand(-1, controlURL, servicetype,"GetConnectionTypeInfo", 0, buffer, &bufsize);
	ParseNameValue(buffer, bufsize, &pdata);
	p = GetValueFromNameValueList(&pdata, "NewConnectionType");
	/*p = GetValueFromNameValueList(&pdata, "NewPossibleConnectionTypes");*/
	/* PossibleConnectionTypes will have several values.... */
	if(p)
	{
		strncpy(connectionType, p, 64 );
		connectionType[63] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	}
	else
	{
		connectionType[0] = '\0';
	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}

/* UPNP_GetLinkLayerMaxBitRates()
* call WANCommonInterfaceConfig:1#GetCommonLinkProperties
*
* return values :
* UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error Code. */
int CSUpnpCtrl::UPNP_GetLinkLayerMaxBitRates(const char * controlURL,
											 const char * servicetype,
											 unsigned int * bitrateDown,
											 unsigned int* bitrateUp)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;
	char * down;
	char * up;
	char * p;

	if(!bitrateDown && !bitrateUp)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	/* shouldn't we use GetCommonLinkProperties ? */
	simpleUPnPcommand(-1, controlURL, servicetype,"GetCommonLinkProperties", 0, buffer, &bufsize);
	/*"GetLinkLayerMaxBitRates", 0, buffer, &bufsize);*/
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	/*down = GetValueFromNameValueList(&pdata, "NewDownstreamMaxBitRate");*/
	/*up = GetValueFromNameValueList(&pdata, "NewUpstreamMaxBitRate");*/
	down = GetValueFromNameValueList(&pdata, "NewLayer1DownstreamMaxBitRate");
	up = GetValueFromNameValueList(&pdata, "NewLayer1UpstreamMaxBitRate");
	/*GetValueFromNameValueList(&pdata, "NewWANAccessType");*/
	/*GetValueFromNameValueList(&pdata, "NewPhysicalLinkSatus");*/
	if(down && up)
	{
		ret = UPNPCOMMAND_SUCCESS;
	}

	if(bitrateDown)
	{
		if(down)
			sscanf(down,"%u",bitrateDown);
		else
			*bitrateDown = 0;
	}

	if(bitrateUp)
	{
		if(up)
			sscanf(up,"%u",bitrateUp);
		else
			*bitrateUp = 0;
	}
	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}

/* UPNP_GetExternalIPAddress() call the corresponding UPNP method.
* if the third arg is not null the value is copied to it.
* at least 16 bytes must be available
*
* Return values :
* 0 : SUCCESS
* NON ZERO : ERROR Either an UPnP error code or an unknown error.
*
* 402 Invalid Args - See UPnP Device Architecture section on Control.
* 501 Action Failed - See UPnP Device Architecture section on Control.*/
int CSUpnpCtrl::UPNP_GetExternalIPAddress(const char * controlURL, const char * servicetype, char * extIpAdd)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!extIpAdd || !controlURL || !servicetype)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	simpleUPnPcommand(-1, controlURL, servicetype, "GetExternalIPAddress", 0, buffer, &bufsize);
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	/*printf("external ip = %s\n", GetValueFromNameValueList(&pdata, "NewExternalIPAddress") );*/
	p = GetValueFromNameValueList(&pdata, "NewExternalIPAddress");
	if(p)
	{
		strncpy(extIpAdd, p, 16 );
		extIpAdd[15] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	}
	else
	{
		extIpAdd[0] = '\0';
	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}

	ClearNameValueList(&pdata);
	return ret;
}

void CSUpnpCtrl::ListRedirections(struct UPNPUrls * urls,struct IGDdatas * data)
{
	int r;
	int i = 0;
	char index[6];
	char intClient[16];
	char intPort[6];
	char extPort[6];
	char protocol[4];
	char desc[80];
	char enabled[6];
	char rHost[64];
	char duration[16];
	/*unsigned int num=0;
	UPNP_GetPortMappingNumberOfEntries(urls->controlURL, data->servicetype, &num);
	printf("PortMappingNumberOfEntries : %u\n", num);*/
	do {
		snprintf(index, 6, "%d", i);
		rHost[0] = '\0'; enabled[0] = '\0';
		duration[0] = '\0'; desc[0] = '\0';
		extPort[0] = '\0'; intPort[0] = '\0'; intClient[0] = '\0';
		r = UPNP_GetGenericPortMappingEntry(urls->controlURL, data->servicetype,
			                                index,extPort, intClient, intPort, protocol, desc, enabled,rHost, duration);
		if(r==0)
		{
			char ch[2000]={0};
			sprintf(ch,"%2d %s %5s->%s:%-5s '%s' '%s'",i, protocol, extPort, intClient, intPort,desc, rHost);
			m_pWorker->m_Log.SetRunInfo(0,"---", __FILE__,__LINE__,ch);
		}
		else
		{
			char ch[2000]={0};
			sprintf(ch,"GetGenericPortMappingEntry() returned %d (%s)",r, strupnperror(r));
			m_pWorker->m_Log.SetRunInfo(0,"---", __FILE__,__LINE__,ch);
		}
		i++;
	} while(r==0);
}

int CSUpnpCtrl::UPNP_GetGenericPortMappingEntry(const char * controlURL,
												const char * servicetype,
												const char * index,
												char * extPort,
												char * intClient,
												char * intPort,
												char * protocol,
												char * desc,
												char * enabled,
												char * rHost,
												char * duration)
{
	struct NameValueParserData pdata;
	struct UPNParg * GetPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int r = UPNPCOMMAND_UNKNOWN_ERROR;
	if(!index)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}
	intClient[0] = '\0';
	intPort[0] = '\0';
	GetPortMappingArgs = (struct UPNParg *)calloc(2, sizeof(struct UPNParg));
	GetPortMappingArgs[0].elt = "NewPortMappingIndex";
	GetPortMappingArgs[0].val = index;
	simpleUPnPcommand(-1, controlURL, servicetype,"GetGenericPortMappingEntry",GetPortMappingArgs, buffer, &bufsize);
	ParseNameValue(buffer, bufsize, &pdata);
	p = GetValueFromNameValueList(&pdata, "NewRemoteHost");
	if(p && rHost)
	{
		strncpy(rHost, p, 64);
		rHost[63] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewExternalPort");
	if(p && extPort)
	{
		strncpy(extPort, p, 6);
		extPort[5] = '\0';
		r = UPNPCOMMAND_SUCCESS;
	}
	p = GetValueFromNameValueList(&pdata, "NewProtocol");
	if(p && protocol)
	{
		strncpy(protocol, p, 4);
		protocol[3] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewInternalClient");
	if(p && intClient)
	{
		strncpy(intClient, p, 16);
		intClient[15] = '\0';
		r = 0;
	}
	p = GetValueFromNameValueList(&pdata, "NewInternalPort");
	if(p && intPort)
	{
		strncpy(intPort, p, 6);
		intPort[5] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewEnabled");
	if(p && enabled)
	{
		strncpy(enabled, p, 4);
		enabled[3] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewPortMappingDescription");
	if(p && desc)
	{
		strncpy(desc, p, 80);
		desc[79] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewLeaseDuration");
	if(p && duration)
	{
		strncpy(duration, p, 16);
		duration[15] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		r = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &r);
	}
	ClearNameValueList(&pdata);
	free(GetPortMappingArgs);
	return r;
}

const char * CSUpnpCtrl::strupnperror(int err)
{
	const char * s = NULL;
	switch(err)
	{
	case UPNPCOMMAND_SUCCESS:
		s = "Success";
		break;
	case UPNPCOMMAND_UNKNOWN_ERROR:
		s = "Miniupnpc Unknown Error";
		break;
	case UPNPCOMMAND_INVALID_ARGS:
		s = "Miniupnpc Invalid Arguments";
		break;
	case 401:
		s = "Invalid Action";
		break;
	case 402:
		s = "Invalid Args";
		break;
	case 501:
		s = "Action Failed";
		break;
	case 713:
		s = "SpecifiedArrayIndexInvalid";
		break;
	case 714:
		s = "NoSuchEntryInArray";
		break;
	case 715:
		s = "WildCardNotPermittedInSrcIP";
		break;
	case 716:
		s = "WildCardNotPermittedInExtPort";
		break;
	case 718:
		s = "ConflictInMappingEntry";
		break;
	case 724:
		s = "SamePortValuesRequired";
		break;
	case 725:
		s = "OnlyPermanentLeasesSupported";
		break;
	case 726:
		s = "RemoteHostOnlySupportsWildcard";
		break;
	case 727:
		s = "ExternalPortOnlySupportsWildcard";
		break;
	default:
		s = NULL;
	}
	return s;
}

/* Test function
* 1 - get connection type
* 2 - get extenal ip address
* 3 - Add port mapping
* 4 - get this port mapping from the IGD */
int CSUpnpCtrl::SetRedirectAndTest(struct UPNPUrls * urls,
									struct IGDdatas * data,
									const char * iaddr,
									const char * iport,
									const char * eport,
									const char * proto)
{
//	char externalIPAddress[16];
//	char intClient[16];
//	char intPort[6];
	int r;

	if(!iaddr || !iport || !eport || !proto)
	{
		fprintf(stderr, "Wrong arguments\n");
		return -1;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "invalid protocol\n");
		return -1;
	}

	r = UPNP_AddPortMapping(urls->controlURL, data->servicetype,eport, iport, iaddr, 0, proto, 0);

	return r;
}

/* protofix() checks if protocol is "UDP" or "TCP"  returns NULL if not */
const char * CSUpnpCtrl::protofix(const char * proto)
{
	static const char proto_tcp[4] = { 'T', 'C', 'P', 0};
	static const char proto_udp[4] = { 'U', 'D', 'P', 0};
	int i, b;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_tcp[i])
		|| (proto[i] == (proto_tcp[i] | 32)) );
	if(b)
		return proto_tcp;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_udp[i])
		|| (proto[i] == (proto_udp[i] | 32)) );
	if(b)
		return proto_udp;
	return 0;
}

int CSUpnpCtrl::UPNP_AddPortMapping(const char * controlURL,
									const char * servicetype,
									const char * extPort,
									const char * inPort,
									const char * inClient,
									const char * desc,
									const char * proto,
									const char * remoteHost)
{
	struct UPNParg * AddPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	struct NameValueParserData pdata;
	const char * resVal;
	int ret;

	if(!inPort || !inClient || !proto || !extPort)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	AddPortMappingArgs = (struct UPNParg *)calloc(9, sizeof(struct UPNParg));
	AddPortMappingArgs[0].elt = "NewRemoteHost";
	AddPortMappingArgs[0].val = remoteHost;
	AddPortMappingArgs[1].elt = "NewExternalPort";
	AddPortMappingArgs[1].val = extPort;
	AddPortMappingArgs[2].elt = "NewProtocol";
	AddPortMappingArgs[2].val = proto;
	AddPortMappingArgs[3].elt = "NewInternalPort";
	AddPortMappingArgs[3].val = inPort;
	AddPortMappingArgs[4].elt = "NewInternalClient";
	AddPortMappingArgs[4].val = inClient;
	AddPortMappingArgs[5].elt = "NewEnabled";
	AddPortMappingArgs[5].val = "1";
	AddPortMappingArgs[6].elt = "NewPortMappingDescription";
	AddPortMappingArgs[6].val = desc?desc:"jovetechupnp";
	AddPortMappingArgs[7].elt = "NewLeaseDuration";
	AddPortMappingArgs[7].val = "0";
	simpleUPnPcommand(-1, controlURL, servicetype, "AddPortMapping", AddPortMappingArgs, buffer, &bufsize);

	ParseNameValue(buffer, bufsize, &pdata);
	resVal = GetValueFromNameValueList(&pdata, "errorCode");
	if(resVal)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(resVal, "%d", &ret);
	}
	else
	{
		ret = UPNPCOMMAND_SUCCESS;
	}
	ClearNameValueList(&pdata);
	free(AddPortMappingArgs);
	return ret;
}

/* UPNP_GetSpecificPortMappingEntry retrieves an existing port mapping
* the result is returned in the intClient and intPort strings please provide 16 and 6 bytes of data */
int CSUpnpCtrl::UPNP_GetSpecificPortMappingEntry(const char * controlURL,
												 const char * servicetype,
												 const char * extPort,
												 const char * proto,
												 char * intClient,
												 char * intPort)
{
	struct NameValueParserData pdata;
	struct UPNParg * GetPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!intPort || !intClient || !extPort || !proto)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	GetPortMappingArgs = (struct UPNParg*)calloc(4, sizeof(struct UPNParg));
	GetPortMappingArgs[0].elt = "NewRemoteHost";
	GetPortMappingArgs[1].elt = "NewExternalPort";
	GetPortMappingArgs[1].val = extPort;
	GetPortMappingArgs[2].elt = "NewProtocol";
	GetPortMappingArgs[2].val = proto;
	simpleUPnPcommand(-1, controlURL, servicetype,"GetSpecificPortMappingEntry",GetPortMappingArgs, buffer, &bufsize);
	/*fd = simpleUPnPcommand(fd, controlURL, data.servicetype, "GetSpecificPortMappingEntry", AddPortMappingArgs, buffer, &bufsize); */
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);

	p = GetValueFromNameValueList(&pdata, "NewInternalClient");
	if(p)
	{
		strncpy(intClient, p, 16);
		intClient[15] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	}
	else
	{
		intClient[0] = '\0';
	}

	p = GetValueFromNameValueList(&pdata, "NewInternalPort");
	if(p)
	{
		strncpy(intPort, p, 6);
		intPort[5] = '\0';
	}
	else
	{
		intPort[0] = '\0';
	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}

	ClearNameValueList(&pdata);
	free(GetPortMappingArgs);
	return ret;
}

void CSUpnpCtrl::RemoveRedirect(struct UPNPUrls * urls,struct IGDdatas * data,const char * eport,const char * proto)
{
	int r;
	if(!proto || !eport)
	{
		fprintf(stderr, "invalid arguments\n");
		return;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "protocol invalid\n");
		return;
	}
	r = UPNP_DeletePortMapping(urls->controlURL, data->servicetype, eport, proto, 0);
//	printf("UPNP_DeletePortMapping() returned : %d\n", r);
}

int CSUpnpCtrl::UPNP_DeletePortMapping(const char * controlURL,
									   const char * servicetype,
                                       const char * extPort,
									   const char * proto,
                                       const char * remoteHost)
{
	/*struct NameValueParserData pdata;*/
	struct UPNParg * DeletePortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	struct NameValueParserData pdata;
	const char * resVal;
	int ret;

	if(!extPort || !proto)
	{
		return UPNPCOMMAND_INVALID_ARGS;
	}

	DeletePortMappingArgs = (struct UPNParg*)calloc(4, sizeof(struct UPNParg));
	DeletePortMappingArgs[0].elt = "NewRemoteHost";
	DeletePortMappingArgs[0].val = remoteHost;
	DeletePortMappingArgs[1].elt = "NewExternalPort";
	DeletePortMappingArgs[1].val = extPort;
	DeletePortMappingArgs[2].elt = "NewProtocol";
	DeletePortMappingArgs[2].val = proto;
	simpleUPnPcommand(-1, controlURL, servicetype,"DeletePortMapping",DeletePortMappingArgs, buffer, &bufsize);
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	resVal = GetValueFromNameValueList(&pdata, "errorCode");
	if(resVal)
	{
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(resVal, "%d", &ret);
	}
	else
	{
		ret = UPNPCOMMAND_SUCCESS;
	}
	ClearNameValueList(&pdata);
	free(DeletePortMappingArgs);
	return ret;
}

#ifndef WIN32
struct UPNPDev *CSUpnpCtrl::getDevicesFromMiniSSDPD(const char * devtype, const char * socketpath)
{
	struct UPNPDev * tmp;
	struct UPNPDev * devlist = NULL;
	unsigned char buffer[2048];
	ssize_t n;
	unsigned char * p;
	unsigned char * url;
	unsigned int i;
	unsigned int urlsize, stsize, usnsize, l;
	int s;
	struct sockaddr_un addr;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0)
	{
		/*syslog(LOG_ERR, "socket(unix): %m");*/
//		perror("socket(unix)");
		return NULL;
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socketpath, sizeof(addr.sun_path));
	/* TODO : check if we need to handle the EINTR */
	if(connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0)
	{
		/*syslog(LOG_WARNING, "connect(\"%s\"): %m", socketpath);*/

		close(s);
		return NULL;
	}
	stsize = strlen(devtype);
	buffer[0] = 1; /* request type 1 : request devices/services by type */
	p = buffer + 1;
	l = stsize;	CODELENGTH(l, p);
	if(p + stsize > buffer + sizeof(buffer))
	{
		/* devtype is too long ! */

		close(s);
		return NULL;
	}
	memcpy(p, devtype, stsize);
	p += stsize;
	if(write(s, buffer, p - buffer) < 0)
	{
		/*syslog(LOG_ERR, "write(): %m");*/
//		perror("minissdpc.c: write()");

		close(s);
		return NULL;
	}
	n = read(s, buffer, sizeof(buffer));
	if(n<=0)
	{
//		perror("minissdpc.c: read()");
		close(s);
		return NULL;
	}
	p = buffer + 1;
	for(i = 0; i < buffer[0]; i++)
	{
		if(p+2>=buffer+sizeof(buffer))
			break;
		DECODELENGTH(urlsize, p);
		if(p+urlsize+2>=buffer+sizeof(buffer))
			break;
		url = p;
		p += urlsize;
		DECODELENGTH(stsize, p);
		if(p+stsize+2>=buffer+sizeof(buffer))
			break;
		tmp = (struct UPNPDev *)malloc(sizeof(struct UPNPDev)+urlsize+stsize);
		tmp->pNext = devlist;
		tmp->descURL = tmp->buffer;
		tmp->st = tmp->buffer + 1 + urlsize;
		memcpy(tmp->buffer, url, urlsize);
		tmp->buffer[urlsize] = '\0';
		memcpy(tmp->buffer + urlsize + 1, p, stsize);
		p += stsize;
		tmp->buffer[urlsize+1+stsize] = '\0';
		devlist = tmp;
		/* added for compatibility with recent versions of MiniSSDPd
		* >= 2007/12/19 */
		DECODELENGTH(usnsize, p);
		p += usnsize;
		if(p>buffer + sizeof(buffer))
			break;
	}

	close(s);
	return devlist;
}
#endif

