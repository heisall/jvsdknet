// SDNSCtrl.cpp: implementation of the CSDNSCtrl class.
//
//////////////////////////////////////////////////////////////////////

//注：该类目前仅限linux系统使用

#include "SDNSCtrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDNSCtrl::CSDNSCtrl()
{
	for(int i=0; i<5; i++)
	{
		memset(dns_servers[i], 0, 100);
	}
}

CSDNSCtrl::~CSDNSCtrl()
{

}

/****************************************************************************
*名称  : GetIPByDomain
*功能  : 域名解析
*参数  : [IN]  name       域名
         [OUT] chIP 解析出的IP地址
*返回值: TRUE  成功
         FALSE 失败
*其他  : 无
*****************************************************************************/
BOOL CSDNSCtrl::GetIPByDomain(const char* name, char chIP[16])
{
#ifndef WIN32
	printf("\ngethostbyname error:%d, try ngethostbyname..\n", errno);
#endif

	memset(chIP, 0, 16);

	if(!GetDNS())
	{
		return FALSE;
	}
	
	if(!ngethostbyname((unsigned char*)name, chIP))
	{
		return FALSE;
	}
	
	if(strlen(chIP) <= 0)
	{
		return FALSE;
	}

	return TRUE;
}

void CSDNSCtrl::ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host)
{
	memset(m_chHostTmp, 0, 100);
	if(strlen((char*)host) >= 100)
	{
		memcpy(m_chHostTmp, host, 100);
	}
	else
	{
		strcpy(m_chHostTmp, (char*)host);
	}
	
	int lock=0 , i;
	strcat((char*)m_chHostTmp,".");
	for(i=0;i<(int)strlen((char*)m_chHostTmp);i++)
	{
		if(m_chHostTmp[i]=='.')
		{
			*dns++=i-lock;
			for(;lock<i;lock++)
			{
				*dns++=m_chHostTmp[lock];
			}
			lock++; //or lock=i+1;
		}
	}
    *dns++='\0';
}

unsigned char* CSDNSCtrl::ReadName(unsigned char* reader, unsigned char* buffer, int* count)
{
	unsigned char *name;
	unsigned int p=0,jumped=0,offset=0;
	int i=0, j=0;
	
	*count = 1;
	name   = (unsigned char*)malloc(256);//分配了内存 注意释放
	name[0]='\0';
	
	//read the names in 3www6google3com format
	while(*reader!=0)
	{
		if(*reader>=192)
		{
			offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000  ;)
			reader = buffer + offset - 1;
			jumped = 1;  //we have jumped to another location so counting wont go up!
		}
		else 
		{
			name[p++]=*reader;
		}
		reader=reader+1;
		
		if(jumped==0)
		{
			*count = *count + 1; //if we havent jumped to another location then we can count up
		}
	}
	
	name[p]='\0';    //string complete
	if(jumped==1)
	{
		*count = *count + 1;  //number of steps we actually moved forward in the packet
	}
	//now convert 3www6google3com0 to www.google.com
	for(i=0;i<(int)strlen((const char*)name);i++)
	{
		p=name[i];
		for(j=0;j<(int)p;j++)
		{
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}
	name[i-1]='\0';	  //remove the last dot
	return name;
}

BOOL CSDNSCtrl::ngethostbyname(unsigned char *host, char chIP[16])
{
	unsigned char buf[65536]={0},*qname=NULL,*reader=NULL;
	int i=0, j=0, stop=0;
	
	SOCKET s=0;
	struct sockaddr_in a;

	struct RES_RECORD answers[20],auth[20],addit[20];  //the replies from the DNS server
	struct sockaddr_in dest;
	
	struct DNS_HEADER *dns = NULL;
	struct QUESTION   *qinfo = NULL;

	for(i=0; i<20; i++)
	{
		answers[i].name = NULL;
		answers[i].rdata = NULL;
		answers[i].resource = NULL;

		auth[i].name = NULL;
		auth[i].rdata = NULL;
		auth[i].resource = NULL;

		addit[i].name = NULL;
		addit[i].rdata = NULL;
		addit[i].resource = NULL;
	}

	s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);  //UDP packet for DNS queries

	dest.sin_family=AF_INET;
	dest.sin_port=htons(53);
	dest.sin_addr.s_addr=inet_addr(dns_servers[0]);  //dns servers 这里需要用到取出的DNS地址

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf;
#ifndef WIN32	
	dns->id = (unsigned short) htons(getpid());//(GetCurrentProcessId());
#else
	dns->id = (unsigned short) htons(GetCurrentProcessId());
#endif
	dns->qr = 0;      //This is a query
	dns->opcode = 0;  //This is a standard query
	dns->aa = 0;      //Not Authoritative
	dns->tc = 0;      //This message is not truncated
	dns->rd = 1;      //Recursion Desired
	dns->ra = 0;      //Recursion not available! hey we dont have it (lol)
	dns->z  = 0;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = 0;
	dns->q_count = htons(1);   //we have only 1 question
	dns->ans_count  = 0;
	dns->auth_count = 0;
	dns->add_count  = 0;
	
	//point to the query portion
	qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];
    
	ChangetoDnsNameFormat(qname,host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons(1);  //we are requesting the ipv4 address
	qinfo->qclass = htons(1); //its internet (lol)

	if(sendto(s,(char*)buf,sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),0,(struct sockaddr*)&dest,sizeof(dest)) <= 0)//==SOCKET_ERROR)
	{
	#ifndef WIN32
		printf("\nngethostbyname send error %d \n",errno);
	#endif	
		//printf("%d  error",WSAGetLastError());
		closesocket(s);
		return FALSE;
	}

	
	i=sizeof(dest);

	//if(recvfrom(s,(char*)buf,65536,0,(struct sockaddr*)&dest,&i) <= 0)//==SOCKET_ERROR)
	if(udpreceive(s,(char*)buf,65536,0,(struct sockaddr*)&dest,&i, 10) <= 0)
	{
	#ifndef WIN32
		printf("\nngethostbyname recv error %d \n",errno);
	#endif
		//printf("Failed. Error Code : %d",WSAGetLastError());

		closesocket(s);
		return FALSE;
	}
	
	dns=(struct DNS_HEADER*)buf;
	
	//move ahead of the dns header and the query field
	reader=&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

//    printf("\nThe response contains : ");
//	printf("\n %d Questions.",ntohs(dns->q_count));
//	printf("\n %d Answers.",ntohs(dns->ans_count));
//	printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
//	printf("\n %d Additional records.\n\n",ntohs(dns->add_count));

	//reading answers
	stop=0;
	
	for(i=0;i<ntohs(dns->ans_count);i++)
	{
		answers[i].name=ReadName(reader,buf,&stop);
		reader = reader + stop;
		
		answers[i].resource = (struct R_DATA*)(reader);
		reader = reader + 10;//sizeof(struct R_DATA);
	
		if(ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
		{
			answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));
			
			for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
			{
				answers[i].rdata[j]=reader[j];
			}

			answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
			
			reader = reader + ntohs(answers[i].resource->data_len);
		
		}
		else
		{
			answers[i].rdata = ReadName(reader,buf,&stop);
		    reader = reader + stop;
		}
		
		
	}
	
	//read authorities
	for(i=0;i<ntohs(dns->auth_count);i++)
	{
		auth[i].name=ReadName(reader,buf,&stop);
		reader+=stop;
		
		auth[i].resource=(struct R_DATA*)(reader);
		reader+=10;//sizeof(struct R_DATA);
	
		auth[i].rdata=ReadName(reader,buf,&stop);
		reader+=stop;
	}

	//read additional
	for(i=0;i<ntohs(dns->add_count);i++)
	{
		addit[i].name=ReadName(reader,buf,&stop);
		reader+=stop;
		
		addit[i].resource=(struct R_DATA*)(reader);
		reader+=10;//sizeof(struct R_DATA);
	
		if(ntohs(addit[i].resource->type)==1)
		{
			addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
			for(j=0;j<ntohs(addit[i].resource->data_len);j++)
			{
				addit[i].rdata[j]=reader[j];
			}
			addit[i].rdata[ntohs(addit[i].resource->data_len)]='\0';
			reader+=ntohs(addit[i].resource->data_len);
		
		}
		else
		{
			addit[i].rdata=ReadName(reader,buf,&stop);
		    reader+=stop;
		}
	}

	
	//print answers
	for(i=0;i<ntohs(dns->ans_count);i++)
	{
		//printf("\nAnswer : %d",i+1);
//		printf("answers Name  :  %s ",answers[i].name);
			
		if(ntohs(answers[i].resource->type)==1)   //IPv4 address
		{
			
			long *p;
			p=(long*)answers[i].rdata;
			a.sin_addr.s_addr=(*p);    //working without ntohl

			sprintf(chIP,"%s",inet_ntoa(a.sin_addr));
//			printf("has IPv4 address :  %s",chIP);
//			printf("has IPv4 address :  %s\n",inet_ntoa(a.sin_addr));
			
		}
//		if(ntohs(answers[i].resource->type)==5)   //Canonical name for an alias
//		{
//			printf("has alias name : %s",answers[i].rdata);
//		}		
//		printf("\n");
	}
/*
	//print authorities
	for(i=0;i<ntohs(dns->auth_count);i++)
	{
		//printf("\nAuthorities : %d",i+1);
		printf("auth Name  :  %s ",auth[i].name);
		if(ntohs(auth[i].resource->type)==2)
			printf("has authoritative nameserver : %s",auth[i].rdata);
		printf("\n");
	}

	//print additional resource records
	for(i=0;i<ntohs(dns->add_count);i++)
	{
		//printf("\nAdditional : %d",i+1);
		printf("addit Name  :  %s ",addit[i].name);
		if(ntohs(addit[i].resource->type)==1)
		{
			long *p;
			p=(long*)addit[i].rdata;
			a.sin_addr.s_addr=(*p);    //working without ntohl
			printf("has IPv4 address :  %s",inet_ntoa(a.sin_addr));
		}
		printf("\n");
	}
*/
	for(i=0; i<20; i++)
	{
		if(answers[i].name != NULL)
		{
			free(answers[i].name);
		}
		answers[i].name = NULL;
		if(answers[i].rdata != NULL)
		{
			free(answers[i].rdata);
		}
		answers[i].rdata = NULL;
		answers[i].resource = NULL;
		
		if(auth[i].name != NULL)
		{
			free(auth[i].name);
		}
		auth[i].name = NULL;
		if(auth[i].rdata != NULL)
		{
			free(auth[i].rdata);
		}
		auth[i].rdata = NULL;
		auth[i].resource = NULL;
		
		if(addit[i].name != NULL)
		{
			free(addit[i].name);
		}
		addit[i].name = NULL;
		if(addit[i].rdata != NULL)
		{
			free(addit[i].rdata);
		}
		addit[i].rdata = NULL;
		addit[i].resource = NULL;
	}

	closesocket(s);
	if(strlen(chIP) > 0)
	{
		return TRUE;
	}
    return FALSE;
}

//获取DNS地址
//参数    : char *ip_buf ：用来存放DNS的内存空间
//返回值  : 存放DNS的内存地址
BOOL CSDNSCtrl::GetDNS()
{
    FILE *fp;
    char buf[64]={0};
	char chIP[64] = {0};
 
    fp = fopen("/etc/resolv.conf", "r");
    if (fp == NULL)
	{
        return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp)) 
    {
        if(1 == sscanf(buf, "nameserver%s", chIP))
        {
            break;
        }
    }
    fclose(fp);

	if(strlen(chIP) <= 0)
	{
		return FALSE;
	}

	strcpy(dns_servers[0], chIP);
    return TRUE;
}

#ifdef WIN32
int CSDNSCtrl::udpreceive(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr FAR * from,int FAR * fromlen,int ntimeoverSec)
#else
int CSDNSCtrl::udpreceive(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr *from,int *fromlen,int ntimeoverSec)
#endif
{
	int   status,nbytesreceived;   
	if(s ==-1)     
	{
		return -1;   
	}
	
	struct   timeval   tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);     
	if(ntimeoverSec==0)   
	{
		status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);     
	}
	else
	{
		status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);     
	}
	switch(status)     
	{   
	case   -1:     
		{
			//"设置读取超时失败.");
		}
		return -1;   
	case   0:
		{
			//"读取注册返回信息超时(10s).";
		}
		return 0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
#ifndef WIN32
			if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,(socklen_t *)fromlen))==-1)   
			{
				return -1;   
			}
			else   
			{
				return nbytesreceived;   
			}
#else
			if((nbytesreceived=recvfrom(s,pchbuf,nlen,nflags,from,fromlen))==-1)   
			{
				return -1;   
			}
			else   
			{
				return nbytesreceived;   
			}
#endif
		}   
	}   
	return -1;   
}

#ifdef WIN32
int CSDNSCtrl::udpsend(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverSec)
#else
int CSDNSCtrl::udpsend(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverSec)
#endif
{
	int   status,nbytesreceived;   
	if(s ==-1)     
	{
		return -1;   
	}
	
	struct   timeval   tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);     
	if(ntimeoverSec==0)   
	{
		status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);     
	}
	else
	{
		status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);     
	}
	switch(status)     
	{   
	case   -1:     
		{
			//"设置读取超时失败.");
		}
		
		return -1;   
	case   0:               
		{
			//"读取注册返回信息超时(10s).";
		}
		
		return 0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
			if((nbytesreceived=sendto(s,pchbuf,nlen,nflags,to,ntolen))==-1)   
			{				
				return -1;   
			}
			else   
			{				
				return nbytesreceived;   
			}
		}   
	}   
	
	return -1;   
}

