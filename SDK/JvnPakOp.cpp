
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JvnPakOp.h"

//CPakOp g_pakOp;

CPakOp::CPakOp()
{
}
CPakOp::~CPakOp()
{

}



int CPakOp::Decappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen)
{
	if ((!targetbuf) || (targetbuflen <= 0) || (!srcbuf) || (srclen <= PKT_HEAD_LEN + PKT_TAIL_LEN))
	{
		printf("%s:%d...................Decappkt error here,srclen:%d !\n",__FILE__,__LINE__,srclen);
		return -1;
	}

	PKTHEADER pktheader_struct;
	memcpy(&pktheader_struct, srcbuf, PKT_HEAD_LEN);

	//ÅÐ¶ÏÍ·Ä£Êý
	if (pktheader_struct.headermos != HEADER_FLAG_)
	{
		printf("%s:%d...................error here ,headermos : %x,HEADER_FLAG_:%x !\n",__FILE__,__LINE__,pktheader_struct.headermos,HEADER_FLAG_);
		return -1;
	}
	
	int decaplen = pktheader_struct.datalen - PKT_HEAD_LEN - PKT_TAIL_LEN;
	if ((decaplen <= 0) || (decaplen > MAX_PKT_LEN) || (decaplen > targetbuflen) 
		|| (decaplen + PKT_HEAD_LEN + PKT_TAIL_LEN > srclen))
	{
		printf("%s:%d...................error here !\n",__FILE__,__LINE__);
		return -1;
	}

	//*sendtype = pktheader_struct.sendtype;
	memcpy(targetbuf, srcbuf + PKT_HEAD_LEN, decaplen);
	int i = 0;
	for(i = 0; i < decaplen; i++)
	{
		targetbuf[i] ^= i;
		i++;
	}
	return decaplen;
}
int CPakOp::Encappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen)
{
	if ((!targetbuf) || (targetbuflen <= 0) || (!srcbuf) || (srclen <= 0))
	{
		//OutputDebugString("ggc----------g_encappkt failed 1\n");
		return -1;
	}

	int i = 0;
	for(i = 0; i < srclen; i++)
	{
		srcbuf[i] ^= i;
		i++;
	}

	if (targetbuflen < srclen + PKT_HEAD_LEN + PKT_TAIL_LEN)
	{
		//OutputDebugString("ggc----------g_encappkt failed 4\n");
		return -1;
	}

	memcpy(targetbuf + PKT_HEAD_LEN, srcbuf, srclen);
	PKTHEADER pktheader_struct;
	pktheader_struct.headermos = HEADER_FLAG_;
	pktheader_struct.datalen = PKT_HEAD_LEN + PKT_TAIL_LEN + srclen;
	pktheader_struct.ver = 0;


	int endmark = ENDMARK;
	memcpy(targetbuf, (char *)&pktheader_struct, PKT_HEAD_LEN);
	memcpy(targetbuf + PKT_HEAD_LEN + srclen, (char *)&endmark, PKT_TAIL_LEN);

	return PKT_HEAD_LEN + PKT_TAIL_LEN + srclen;
}