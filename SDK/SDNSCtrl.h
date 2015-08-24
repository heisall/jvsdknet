// SDNSCtrl.h: interface for the CSDNSCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SDNSCTRL_H__7F822D9C_C3A9_4284_B272_9A0044892A1A__INCLUDED_)
#define AFX_SDNSCTRL_H__7F822D9C_C3A9_4284_B272_9A0044892A1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#ifndef WIN32
#include <stdlib.h>
#endif

//DNS header structure
struct DNS_HEADER
{
	unsigned	short id;		    // identification number
	
	unsigned	char rd     :1;		// recursion desired
	unsigned	char tc     :1;		// truncated message
	unsigned	char aa     :1;		// authoritive answer
	unsigned	char opcode :4;	    // purpose of message
	unsigned	char qr     :1;		// query/response flag
	
	unsigned	char rcode  :4;	    // response code
	unsigned	char cd     :1;	    // checking disabled
	unsigned	char ad     :1;	    // authenticated data
	unsigned	char z      :1;		// its z! reserved
	unsigned	char ra     :1;		// recursion available
	
	unsigned    short q_count;	    // number of question entries
	unsigned	short ans_count;	// number of answer entries
	unsigned	short auth_count;	// number of authority entries
	unsigned	short add_count;	// number of resource entries
};


//Constant sized fields of query structure
struct QUESTION
{
	unsigned short qtype;
	unsigned short qclass;
};


//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct  R_DATA
{
	unsigned short type;
	unsigned short _class;
	unsigned int   ttl;
	unsigned short data_len;
};
#pragma pack(pop)


//Pointers to resource record contents
struct RES_RECORD
{
	unsigned char  *name;
	struct R_DATA  *resource;
	unsigned char  *rdata;
};

//Structure of a Query
typedef struct
{
	unsigned char *name;
	struct QUESTION      *ques;
} QUERY;

class CSDNSCtrl  
{
public:
	CSDNSCtrl();
	virtual ~CSDNSCtrl();

	BOOL GetIPByDomain(const char* name, char chIP[16]);//”Ú√˚Ω‚Œˆ
private:
	void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host);
	unsigned char* ReadName(unsigned char* reader, unsigned char* buffer, int* count);
	BOOL ngethostbyname(unsigned char *host, char chIP[16]);
	BOOL GetDNS();
	#ifndef WIN32
		static int udpreceive(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr * from,int * fromlen,int ntimeoverSec);
		static int udpsend(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr * to,int ntolen,int ntimeoverSec);
	#else
		static int udpreceive(SOCKET s,char *pchbuf,int nlen,int nflags,struct sockaddr FAR * from,int FAR * fromlen,int ntimeoverSec);
		static int udpsend(SOCKET s,char * pchbuf,int nlen,int nflags,const struct sockaddr FAR * to,int ntolen,int ntimeoverSec);
	#endif
private:
	char dns_servers[5][100];
	char m_chHostTmp[100];
};

#endif // !defined(AFX_SDNSCTRL_H__7F822D9C_C3A9_4284_B272_9A0044892A1A__INCLUDED_)
