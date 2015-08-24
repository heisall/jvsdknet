#ifndef __PAK_H__
#define __PAK_H__

#define PKT_HEAD_LEN 12
#define PKT_TAIL_LEN 4
#define MAX_PKT_LEN  1024*1024


#define HEADER_FLAG_ 0x38254b64			//头标志
#define ENDMARK 0x872b7881				//结束标志


#define  CS_REQ_FILE 0x01  // 请求服务器地址列表
#define  CS_ACK_FILE 0x02  // 回应服务器地址列表请求
#define  CS_ERROR_NOFILE_FOUND 0x03 // 未发现请求文件
#define  CS_ERROR_CMD 0x04  // 无法识别的指令


typedef struct pktheader{				//包头数据结构
	int headermos;						//头模数
	int ver;							//版本
	int datalen;							//长度
} PKTHEADER;


class CPakOp  
{
public:
	CPakOp();
	virtual ~CPakOp();
public:
	static int Decappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen);
	static int Encappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen);
};

extern CPakOp g_pakOp;
#endif