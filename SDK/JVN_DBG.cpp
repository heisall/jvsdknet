#include "JVN_DBG.h"

#ifndef WIN32
	#include <unistd.h>
	#include <errno.h>
	#include <stdarg.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/types.h>
	
	#include <sys/syscall.h> 
	#include <string.h>
	#include <stdlib.h>
#else
	#include <process.h>
	//#include <sys/stat.h>
#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
//	#include <cstring>
#endif

#include <fstream>
#include <iostream>

#include "JvnPakOp.h"
#include "./RunLog.h"

//#define JOV_SVR_LIB 

#ifdef JOV_SVR_LIB
CRunLog g_log;
#endif
CDbgInfo g_dbg;
using namespace std;

void CDbgInfo::SetDbgByFile(char* src)
{
#ifndef WIN32

	char* pBuf = src;
	char* pThis = NULL;
	char* pObj = NULL;

	int index = 0;
	unsigned long key = 0;
	
	while(1)
	{
		pObj = strtok_r(pBuf," ",&pThis);
		index++;
		if (NULL == pObj)
		{
			break;
		}
		key = atoi(pObj);
		if (index == 1)  // 主控输出
		{
			m_bJvs = key;
		}else if (index == 2) // log
		{
			m_bLog = key;
		}else if (index == 3) // 分控输出
		{
			m_bJvc = key;
		}else if (index == 4) //主控error
		{
			m_bJvsErr = key;
		}else if (index == 5) //分控error
		{
			m_bJvcErr = key;
		}else if (index == 6) // 主控warn
		{
			m_bJvsWarn = key;
		}else if (index ==7)// 分控warn
		{
			m_bJvcWarn = key;
		}else if (index == 8) // yst online
		{
			m_bJvsOnline = key;
		}else if (index == 9)// socket check
		{
			m_bSocketCheck = key;;
		}else if (index == 10) //主控连接
		{
			m_bJvsCon = key;
		}else if (index == 11) // chat 
		{
			m_bJvsChat = key;
		}else if (index == 12) //主控lanserch
		{
			m_bJvsLanSerch = key;
		}else if (index == 13) // 主控broadcast
		{
			m_bJvsBCSearch = key;
		}else if (index == 14) // 输出终端
		{
			m_nOutTerminal = key;
		}else if (index == 15) //运行模式
		{
			m_nRunMode = key;
		}
		
		pBuf = NULL;


	}
#else
	char strobj[128] = {0};
	char strdir[260] = {0};
	GetCurModuePath(strdir);
	GetPrivateProfileString("JOV","jvs","0",strobj,sizeof(strobj),strdir);
	m_bJvs = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","log","0",strobj,sizeof(strobj),strdir);
	m_bLog = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvc","0",strobj,sizeof(strobj),strdir);
	m_bJvc = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvserr","0",strobj,sizeof(strobj),strdir);
	m_bJvsErr = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvcerr","0",strobj,sizeof(strobj),strdir);
	m_bJvcErr = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvswarn","0",strobj,sizeof(strobj),strdir);
	m_bJvsWarn = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvcwarn","0",strobj,sizeof(strobj),strdir);
	m_bJvcWarn = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvsonline","0",strobj,sizeof(strobj),strdir);
	m_bJvsOnline = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","socketcheck","0",strobj,sizeof(strobj),strdir);
	m_bSocketCheck = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvscon","0",strobj,sizeof(strobj),strdir);
	m_bJvsCon = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvschat","0",strobj,sizeof(strobj),strdir);
	m_bJvsChat = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvslanserch","0",strobj,sizeof(strobj),strdir);
	m_bJvsLanSerch = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","jvsbcserch","0",strobj,sizeof(strobj),strdir);
	m_bJvsBCSearch = (atoi(strobj) == 1?true:false);
	GetPrivateProfileString("JOV","outterminal","0",strobj,sizeof(strobj),strdir);
	m_nOutTerminal = atoi(strobj);
	GetPrivateProfileString("JOV","runmode","0",strobj,sizeof(strobj),strdir);
	m_nRunMode = atoi(strobj);	
#endif
	return;
}
CDbgInfo::CDbgInfo()
{
	m_bRun = true;
	m_bJvs = false;
	m_bJvc = true;
	m_bJvsErr = false;
	m_bJvcErr = false;
	m_bJvsWarn = false;
	m_bJvcWarn = false;
	m_bJvsOnline = false;
	m_bLog = true;
	m_bJvsVideoIn = false;
	m_bJvsVideoOut = false;
	m_bSocketCheck = false;
	m_bJvsCon = false;
	m_bJvsChat = false;
	m_bJvsLanSerch = false;
	m_bJvsBCSearch = false;
	m_nOutTerminal = OUT_TO_CONSOLE;
	m_nRunMode = RUN_MODE_DBG;
	m_nCondValue = 0;

	m_nJvsCH = -1;
	m_nJvsTypeIn = -1;
	m_nJvsTypeRcv = -1;
	m_nJvsTypeSnd = -1;
	m_nClientIndex = -1;

	m_tid = 0;
	m_nTimeInterval = 0; //时间间隔为0，不采样
#ifdef JOV_SVR_LIB
	g_log.SetLogDir(JVN_DBG_LOG_DIR);
	g_log.EnableLog(true);
#endif

#ifndef WIN32

	char *szFilename = "/etc/conf.d/jvndbg";
	bool bOutputDebugInfo = access(szFilename, 0) == 0;
	char text[512] = {0};
	if(bOutputDebugInfo)
	{
		FILE* fd = NULL;
		fd = fopen(szFilename,"r");
		if (NULL == fd)
		{
			perror("fopen");
			return;
		}
		fgets(text,sizeof(text),fd);
		SetDbgByFile(text);
		fclose(fd);
	}
	pthread_mutex_init(&m_ctThread, NULL);
	m_threads.clear();
	m_pid = getpid();

	m_lastCpuTime = GetSysSnap();
	m_lastProcTime = GetProcSnap();
#else
	//SetDbgByFile(NULL);
#endif
}

CDbgInfo::~CDbgInfo()
{
	m_bRun = false;
#ifndef WIN32
	pthread_mutex_destroy(&m_ctThread);
#else
#endif
}
#ifdef WIN32
void CDbgInfo::GetCurModuePath(char *strDir)
{
	char exeFullPath[MAX_PATH],strPath[MAX_PATH];
	char c6[MAX_PATH];
	char c8[MAX_PATH];
	char ext[MAX_PATH];
	GetModuleFileName(NULL,exeFullPath,MAX_PATH);

	_splitpath(exeFullPath,c6,strPath,c8,ext);
	strcat(strDir,strPath);
	strcat(strDir,"jvndbg.ini");
}
#endif
void CDbgInfo::AddThreadID(const char* func,int type,void* data)
{
#ifndef WIN32
	#ifndef MOBILE_CLIENT
		pid_t id =  syscall(SYS_gettid);
		ThreadStat stat;
		stat.threadId = (unsigned long)id;
		memcpy(stat.threadName,func,strlen(func)+1);
		if (type == THREAD_TYPE_CH)
		{
			stat.nCH = (int)data;
		}else
		{
			stat.nCH = -1;
		}
		pthread_mutex_lock(&m_ctThread);
		stat.lastThreadTime = GetThreadSnap(id);
		m_threads.push_back(stat);
		pthread_mutex_unlock(&m_ctThread);
	#endif
#else

#endif
}

void CDbgInfo::DeleteThreadID(const char* func)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctThread);
	vector<ThreadStat>::iterator it = m_threads.begin();
	while (it != m_threads.end())
	{
		if (strcmp(it->threadName,func) == 0)
		{
			m_threads.erase(it);
			pthread_mutex_unlock(&m_ctThread);
			return;
		}
	}
	pthread_mutex_unlock(&m_ctThread);
#else

#endif
}

void CDbgInfo::ListThreadMsg(void)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctThread);
	vector<ThreadStat>::iterator it = m_threads.begin();
	while (it != m_threads.end())
	{
		out(__FILE__,__LINE__,__FUNCTION__,".......channalID:%d,threadID:%d,threadHander:%s",it->nCH,it->threadId,it->threadName);
		it++;
	}
	pthread_mutex_unlock(&m_ctThread);
#else

#endif
}


int CDbgInfo::closesocketdbg(int &nSocket, char *szFile, int nLine)
{	
#ifndef WIN32
	if (m_nRunMode == RUN_MODE_PUB)
	{
		int ret = close(nSocket);
		nSocket = 0;
		return ret;
	}


	int ret = close(nSocket);
	if (m_bSocketCheck)
	{
		out(szFile,nLine,"","...closesockdbg ,closesocket_%d, closereturn_%d",nSocket,ret);
	}

	nSocket = 0;
	return ret;
#endif
	return 0;
}

int CDbgInfo::SendToNet(char* buffer,int len)
{
#ifndef WIN32
	char target[2048] = {0};
	int ret = CPakOp::Encappkt(target,sizeof(target),buffer,len);

	return sendto(m_nSock,target,ret,0,(struct sockaddr*)&m_clientAddr,sizeof(m_clientAddr));
#endif
	return 0;
}

/****************************************************************************
*名称  : out
*功能  : 输出调试信息 无主控区分
*参数  : 
*返回值: 无
*其他  : 无
*****************************************************************************/
void CDbgInfo::out(const char* file,const int line,const char* func,char* format, ...)
{
	if (m_nRunMode == RUN_MODE_PUB)
	{
		return;
	}


	va_list arglist;
	int index = 0;
	char buffer[2048] = {0};
	char target[2048] = {0};
	char* p = NULL;
#ifndef WIN32
	index = sprintf(buffer,"%s:%d:%s...",file,line,func);
#else
	index = sprintf(buffer,"JOV %s:%d:%s...",file,line,func);
#endif
	
	p = (char*)&buffer[index];

	va_start (arglist,format);
	vsprintf(p, format, arglist);
	va_end (arglist);

	if (m_nOutTerminal == OUT_TO_FILE)
	{		
#ifdef JOV_SVR_LIB
	g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
	}else if(m_nOutTerminal == OUT_TO_FILE_ON)
	{
#ifdef JOV_SVR_LIB
	g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
		return;
	}else if (m_nOutTerminal == OUT_TO_CONSOLE)
	{
#ifndef WIN32
		printf("%s\n",buffer);
#else
		OutputDebugString(buffer);
#endif
	}else if (m_nOutTerminal == OUT_TO_NET)
	{
#ifndef WIN32
		SendToNet(buffer,strlen(buffer)+1);
#endif	
	}
}

/****************************************************************************
*名称  : jvsout
*功能  : 控制主控调试信息的输出
*参数  : type: 输出类别	
*返回值: 无
*其他  : 无
*****************************************************************************/
void CDbgInfo::jvsout(int type,const char* file,const int line,const char* func,char* format,...)
{  
	char buffer[2048] = {0};
	char target[2048] = {0};
	if (type == HEART_BEAT)
	{
#ifndef WIN32
		sprintf(buffer,"Heat repley");
		SendToNet(buffer,strlen(buffer)+1);
#endif
		return;
	}
	if (!m_bJvs || (m_nRunMode == RUN_MODE_PUB) )
	{   
		return;
	}
	if (type != OUT_ON_EVENT)
	{
		

		if (type == OUT_JVS_VIDEO_IN )
		{
			if (!m_bJvsVideoIn)
			{
				return;
			}
		}
		if (type == OUT_JVS_VIDEO_SND &&(!m_bJvsVideoOut))
		{
			return;
		}
		if (type == OUT_JVS_VIDEO_RCV &&(!m_bJvsRcv))
		{  
			return;
		}
		if (type == OUT_JVS_ERR && (!m_bJvsErr) )
		{
			return;
		}
		if (type == OUT_JVS_CONNECT && (!m_bJvsCon) )
		{
			return;
		}
		if (type == OUT_JVS_LANSERCH &&(!m_bJvsLanSerch))
		{
			return;
		}
		if (type == OUT_JVS_BCSERCH && (!m_bJvsBCSearch))
		{
			return;
		}
		if (type == OUT_JVS_CHAT_SDN && (!m_bJvsChat) )
		{
			return;
		}
		if (type == OUT_JVS_WARN && (!m_bJvsWarn))
		{
			return;
		}
		if (type == OUT_YST_ONLINE &&(!m_bJvsOnline))
		{
			return;
		}

	}
	
	va_list arglist;
	int index = 0;
	
	char* p = NULL;
#ifndef WIN32
	index = sprintf(buffer,"%s:%d:%s...",file,line,func);
#else
	index = sprintf(buffer,"JOV %s:%d:%s...",file,line,func);
#endif
	p = (char*)&buffer[index];

	va_start (arglist,format);
	vsprintf(p, format, arglist);
	va_end (arglist);


	if (m_nOutTerminal == OUT_TO_FILE)
	{
#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
	}else if(m_nOutTerminal == OUT_TO_FILE_ON)
	{
	#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
		return;
	}else if (m_nOutTerminal == OUT_TO_CONSOLE)
	{
#ifndef WIN32
		printf("%s\n",buffer);
#else
		OutputDebugString(buffer);
#endif
		return;
	}else if (m_nOutTerminal == OUT_TO_NET)
	{
#ifndef WIN32
		int ret = SendToNet(buffer,strlen(buffer)+1);
#endif	
	}
}

/****************************************************************************
*名称  : jvcout
*功能  : 控制分控调试信息的输出
*参数  : type: 输出类别
*返回值: 无
*其他  : 无
*****************************************************************************/
void CDbgInfo::jvcout(int type,const char* file,const int line,const char* func,char* format,...)
{
	if (!m_bJvc || (m_nRunMode == RUN_MODE_PUB))
	{
		return;
	}
	if (type !=  OUT_ON_EVENT)
	{
		if (type == OUT_JVC_ERR && (!m_bJvcErr))
		{
			return;
		}else if (type == OUT_JVC_WARN && (!m_bJvcWarn))
		{
			return;
		}
	}
	

	va_list arglist;
	int index = 0;
	char buffer[2048] = {0};
	char* p = NULL;
#ifndef WIN32
	index = sprintf(buffer,"%s:%d:%s...",file,line,func);
#else
	index = sprintf(buffer,"JOV %s:%d:%s...",file,line,func);
#endif
	p = (char*)&buffer[index];

	va_start (arglist,format);
	vsprintf(p, format, arglist);
	va_end (arglist);


	if (m_nOutTerminal == OUT_TO_FILE)
	{
	#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
	}else if(m_nOutTerminal == OUT_TO_FILE_ON)
	{
		
	#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
		return;
	}else if (m_nOutTerminal == OUT_TO_CONSOLE)
	{
#ifndef WIN32
		printf("%s\n",buffer);
#else
		OutputDebugString(buffer);
#endif
	}else if (m_nOutTerminal == OUT_TO_NET)
	{
#ifndef WIN32
		SendToNet(buffer,strlen(buffer)+1);
#endif	
	}
}

void CDbgInfo::logout(const char* file,const int line,const char* func,char* format,...)
{
	if ((m_nRunMode == RUN_MODE_PUB) || (!m_bLog))
	{
		return;
	}

	va_list arglist;
	int index = 0;
	char buffer[2048] = {0};
	char* p = NULL;

#ifndef WIN32
	index = sprintf(buffer,"%s:%d:%s...",file,line,func);
#else
	index = sprintf(buffer,"JOV %s:%d:%s...",file,line,func);
#endif

	p = (char*)&buffer[index];

	va_start (arglist,format);
	vsprintf(p, format, arglist);
	va_end (arglist);

	if (m_nOutTerminal == OUT_TO_FILE)
	{
#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
	}else if(m_nOutTerminal == OUT_TO_FILE_ON)
	{
#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,p, (char*)file,(int)line,NULL);
#endif
		return;
	}else if (m_nOutTerminal == OUT_TO_CONSOLE)
	{
#ifndef WIN32
		printf("%s\n",buffer);
#else
		OutputDebugString(buffer);
#endif
	}else if (m_nOutTerminal == OUT_TO_NET)
	{
#ifndef WIN32
		SendToNet(buffer,strlen(buffer)+1);
#endif	
	}
}

/****************************************************************************
*名称  : out
*功能  : 输出调试信息 无主控区分
*参数  : terminal: 输出终端
*返回值: 无
*其他  : 无
*****************************************************************************/
void CDbgInfo::outto(const char* file,const int line,const char* func,int terminal,char* format, ...)
{
	if (m_nRunMode == RUN_MODE_PUB)
	{
		return;
	}
#ifndef ENABLE_DBG_MODE
	return;
#else
#ifndef WIN32
	char *szKeyFilename = "/etc/conf.d/zh";
	va_list arglist;
	char buffer[2048];

	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);

	if (terminal == OUT_TO_FILE)
	{
	}else if(terminal == OUT_TO_FILE_ON)
	{
		bool bOutputDebugInfo = access(szKeyFilename, 0) == 0;
		if(bOutputDebugInfo)
		{
#ifdef JOV_SVR_LIB
		g_log.SetRunInfo(0,buffer, (char*)file,(int)line,NULL);
#endif
		}
		return;
	}else if (terminal == OUT_TO_CONSOLE)
	{
		printf("%s:%d:%s.........%s\n",file,line,func,buffer);
	}else if (m_nOutTerminal == OUT_TO_NET)
	{
#ifndef WIN32
		SendToNet(buffer,strlen(buffer)+1);
#endif	
	}

#endif
#endif

}

int CDbgInfo::SetRunMode(int type)
{
	if (type == RUN_MODE_PUB)
	{
		m_nRunMode = type;
	}else if (type == RUN_MODE_DBG)
	{
		m_nRunMode = type;
	}else
	{
		return -1;
	}
	return 0;
}

int CDbgInfo::SetOutTerminal(int terminal)
{
	if ((terminal == OUT_TO_FILE) || (terminal == OUT_TO_CONSOLE) || (terminal == OUT_TO_FILE_ON) || (terminal == OUT_TO_NET))
	{
		m_nOutTerminal = terminal;
		return 0;
	}
	return -1;
}

int CDbgInfo::SetCondValue(int cond)
{
	m_nCondValue = cond;
	return 0;
}

int CDbgInfo::EnableOut(int type,bool run)
{
	switch (type)
	{
	case OUT_JVC_MSG:
			m_bJvc = run;
		break;
	case OUT_JVS_MSG:
			m_bJvs = run;
		break;
	case OUT_LOG_MSG:
			m_bLog = run;
		break;
	case OUT_JVS_ERR:
			m_bJvsErr = run;
		break;
	case OUT_JVS_LANSERCH:
			m_bJvsLanSerch = run;
		break;
	case OUT_JVS_BCSERCH:
			m_bJvsBCSearch = run;
		break;
	case OUT_JVC_ERR:
			m_bJvcErr = run;
		break;
	case OUT_JVS_WARN:
			m_bJvsWarn = run;
		break;
	case OUT_JVS_CHAT_SDN:
			m_bJvsChat = run;
		break;
	case OUT_JVS_CONNECT:
			m_bJvsCon = run;
		break;
	case OUT_JVC_WARN:
			m_bJvcWarn = run;
		break;
	case OUT_YST_ONLINE:
			m_bJvsOnline = run;
		break;
	case OUT_ALL_MSG:
		{
			m_bJvs = run;
			m_bJvc = run;
			m_bJvsErr = run;
			m_bJvcErr = run;
			m_bJvsWarn = run;
			m_bJvcWarn = run;
			m_bJvsOnline = run;
			m_bLog = run;
			m_bSocketCheck = run;
			m_bJvsCon = run;
			m_bJvsChat = run;
			m_bJvsLanSerch = run;
			m_bJvsBCSearch = run;

			m_nRunMode = RUN_MODE_DBG;
		}
		break;
	case OUT_JVS_VIDEO_IN:
		{
			m_bJvsVideoIn = run;
		}
		break;
	case OUT_JVS_VIDEO_SND:
		{
			m_bJvsVideoOut = run;
		}
		break;
	case OUT_SOCKET_CHECK:
		{
			m_bSocketCheck = run;
		}
		break;

	default:
		break;
	}
	return 0;
}
/****************************************************************************
*名称  : SetVideoMsg
*功能  : 设置通信相关信息
*参数  : type: 通信点，包括主分控通信入口，通信出口
		 ch： 通道号
		 clientID: 分控ID
		 run: 是否启用信息输出，1 输出，0 不输出
		 other:过滤类型
*其他  : 无
*****************************************************************************/
int CDbgInfo::SetVideoMsg(char type,int nCh,int nClientID,char run,int nOther)
{
	if (type == OUT_JVS_VIDEO_IN)
	{
		m_nJvsCH = nCh;
		m_nJvsTypeIn = nOther;
		if (run == 0)
		{
			m_bJvsVideoIn = false;
		}else
		{
			m_bJvsVideoIn = true;
		}
		
	}else if (type == OUT_JVS_VIDEO_SND)
	{
		m_nJvsTypeSnd = nOther;
		m_nClientIndex = nClientID;
		if (run == 0)
		{
			m_bJvsVideoOut = false;
		}else
		{
			m_bJvsVideoOut = true;
		}
	}else if (type == OUT_JVS_VIDEO_RCV)
	{
		m_nJvsTypeRcv = nOther;
		m_nJvsCH = nCh;
		m_nClientIndex = nClientID;
		if (run == 0)
		{
			m_bJvsRcv = false;
		}else
		{
			m_bJvsRcv = true;
		}
	}
	return 0;
}

int CDbgInfo::SetCpuInterval(pid_t tid,unsigned int time)
{
	m_nTimeInterval = time;
	m_tid = tid;
	return 0;
}

/****************************************************************************
*名称  : GetProcValue
*功能  : 计算stat文件读到的cpu占用时间和
*参数  : src, /proc//stat 配置文件中读到的cpu时间字符串，形如 12 34 45 ...
		 start, 有效数据起始值
		 num:   有效数据的个数
*返回值: 时间和
*其他  : 无
*****************************************************************************/
unsigned long CDbgInfo::GetProcValue(char* src,int start,int num)
{
#ifndef WIN32

	char* pBuf = src;
	char* pThis = NULL;
	char* pObj = NULL;
	
	int index = 0;
	unsigned long add = 0;
	while(1)
	{
		pObj = strtok_r(pBuf," ",&pThis);
		index++;
		if ((index > (start-1)) && (index < (start+num)) && pObj)
		{
			add += atoi(pObj);
		}
		if (NULL == pObj || index > (start+num - 1))
		{
			break;
		}
		pBuf = NULL;


	}
	return add;
#endif
	return 0;
}
/****************************************************************************
*名称  : GetProcSnap
*功能  : 获取进程cpu时间使用快照，即读取 /proc/pid/stat 文件
*参数  : 无
*返回值: 当前进程使用的cpu时间
*其他  : 无
*****************************************************************************/
unsigned long CDbgInfo::GetProcSnap(void)
{
#ifndef WIN32
	char path[260] = {0};
	sprintf(path,"/proc/%d/stat",m_pid);

	char text[1024] = {0};
	FILE* fd = NULL;
	fd = fopen(path,"r");
	if (NULL == fd)
	{
		perror("fopen");
		printf("%s:%d............open error, path:%s,pid:%d\n",__FILE__,__LINE__,path,m_pid);
		return 0;
	}
	fgets(text,sizeof(text),fd);
	fclose(fd);
	return GetProcValue(text,14,4);
#endif
	return 0;
}

/****************************************************************************
*名称  : GetSysMemSnap
*功能  : 获取进程内存使用快照，即读取 /proc/pid/stat 文件
*参数  : 无
*返回值: 当前进程使用的cpu时间
*其他  : 无
*****************************************************************************/
unsigned long CDbgInfo::GetSysMemSnap(void)
{
#ifndef WIN32
	char name[128] = {0};
	unsigned long freeSize = 0;
	unsigned long cacheSize = 0;
	char name2[128] = {0};
	char buf[256] = {0};
	FILE* fd = NULL;
	fd = fopen("/proc/meminfo","r");
	fgets(buf,sizeof(buf),fd);//toal
	fgets(buf,sizeof(buf),fd);//free
	sscanf(buf,"%s %u %s",name,&freeSize,name2);
	//printf("name:%s, size:%d, name2:%s\n",name,freeSize,name2);

	fgets(buf,sizeof(buf),fd);//buff
	fgets(buf,sizeof(buf),fd);//cache
	sscanf(buf,"%s %u %s",name,&cacheSize,name2);
	fclose(fd);
	out(__FILE__,__LINE__,__FUNCTION__,"...freeMem:%d K,cacheMem:%d K",freeSize,cacheSize);
	return (freeSize+cacheSize);
#else
	return 0;
#endif
}
/****************************************************************************
*名称  : GetThreadSnap
*功能  : 获取线程cpu时间使用快照，即读取 /proc/pid/tast/tid/stat 文件
*参数  : tid, 线程id
*返回值: 当前线程使用的cpu时间
*其他  : 无
*****************************************************************************/
unsigned long CDbgInfo::GetThreadSnap(pid_t tid)
{
#ifndef WIN32
	char path[260] = {0};
	sprintf(path,"/proc/%d/task/%d/stat",m_pid,tid);

	char text[1024] = {0};
	FILE* fd = NULL;
	fd = fopen(path,"r");
	if (NULL == fd)
	{
		perror("fopen");
		printf("%s:%d............open error, path:%s,tid:%d\n",__FILE__,__LINE__,path,tid);
		return 0;
	}
	fgets(text,sizeof(text),fd);
	fclose(fd);

	return GetProcValue(text,14,4);
#endif
	return 0;
}
/****************************************************************************
*名称  : GetThreadSnap
*功能  : 获取cpu时间使用快照，即读取 /proc/stat 文件
*参数  : 无
*返回值: 使用的cpu时间
*其他  : 无
*****************************************************************************/
unsigned long CDbgInfo::GetSysSnap(void)
{
#ifndef WIN32
	char text[1024] = {0};
	FILE* fd = NULL;
	fd = fopen("/proc/stat","r");
	if (NULL == fd)
	{
		perror("fopen");
		printf("%s:%d............open error !\n",__FILE__,__LINE__);
		return 0;
	}
	fgets(text,sizeof(text),fd);
	fclose(fd);
#endif
	return 0;
}

float CDbgInfo::GetRate(unsigned long rator,unsigned long total)
{
	float a = (float)rator;
	float b = (float)total;
	if (total < 1 || rator < 0)
	{
		printf("............rator:%d,total:%d\n",rator,total);
		return 0;
	}
	
	float e = (a/b)*100;
	return e;
}
/****************************************************************************
*名称  : GetProcCpuTime
*功能  : 当前进程cpu占用时间
*参数  : 采样时间，单位秒
*返回值: 使用的cpu时间
*其他  : 无
*****************************************************************************/
void CDbgInfo::GetProcCpuTime(void)
{
#ifndef WIN32
	unsigned long curCpuTime = 0;
	unsigned long curProcTime = 0;
	
	curProcTime = GetProcSnap();
	m_lastProcTime = curProcTime;

#endif
	return;
}

/****************************************************************************
*名称  : GetThreadCpuTime
*功能  : 当前线程cpu占用时间
*参数  : 采样时间，单位秒
*返回值: 使用的cpu时间
*其他  : 无
*****************************************************************************/
void CDbgInfo::GetThreadCpuTime(void)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctThread);
	unsigned long curCpuTime = 0;
	//curCpuTime = GetSysSnap();
	vector<ThreadStat>::iterator it = m_threads.begin();
	float e = 0;
	while (it != m_threads.end())
	{
		if (m_tid > 0 && m_tid != it->threadId)
		{
			it++;
			continue;;
		}
		
		unsigned long curThreadTime = 0;
		curThreadTime = GetThreadSnap(it->threadId);
		out(__FILE__,__LINE__,__FUNCTION__,"...threadName:%s,channalID:%d,curThreadTime:%d,lastThreadTime:%d, cur - last: %d\n",it->threadName,it->nCH,curThreadTime,it->lastThreadTime,curThreadTime - it->lastThreadTime);
		it->lastThreadTime = curThreadTime;
		it++;
	}
	printf("\n");
	pthread_mutex_unlock(&m_ctThread);
#endif
	return;
}
/****************************************************************************
*名称  : GetThreadCpuTime
*功能  : 当前线程在进程中cpu占用时间
*参数  : 采样时间，单位秒
*返回值: 使用的cpu时间
*其他  : 无
*****************************************************************************/
void CDbgInfo::GetThreadInProcRate(void)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctThread);
	vector<ThreadStat>::iterator it = m_threads.begin();
	while (it != m_threads.end())
	{
		unsigned long curProcTime = 0;
		unsigned long curThreadTime = 0;

		curProcTime = GetProcSnap();
		curThreadTime = GetThreadSnap(it->threadId);
		float e = GetRate(curThreadTime - it->lastThreadTime,curProcTime - m_lastProcTime);
		printf("%s..............%5.2f\n",it->threadName,e);
		m_lastProcTime = curProcTime;
		it->lastThreadTime = curThreadTime;
		it++;
	}
	pthread_mutex_unlock(&m_ctThread);
#endif
	return;
}

#ifndef WIN32
void CDbgInfo::SetClientAddr(sockaddr_in addr)
{
	m_clientAddr = addr;
	return;
}
#endif

#ifndef WIN32
void CDbgInfo::SetServerSocket(int sock)
{
	m_nSock = sock;
	return;
}
#endif


#ifndef WIN32
int CDbgInfo::try_get_lock(pthread_mutex_t *mutex,const char* pfile,const int line,const char* pfun)
{
	if (NULL == mutex)
	{
		return -1;
	}

	int ret = 0;
	int cnt = 5;

	while(cnt--)
	{
		ret = pthread_mutex_trylock(mutex);
		if (ret == 0)
		{
			return ret;
		}else if (ret > 0)
		{
		}
		usleep(200);
	}
	
	return ret;
}
#endif
