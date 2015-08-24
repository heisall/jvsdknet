#ifndef __JVN_DBG_H__
#define __JVN_DBG_H__

#ifndef WIN32
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <process.h>
#endif
#include <vector>


#define ENABLE_DBG_MODE //使能

#define RUN_MODE_PUB 0 //发布版运行模式
#define RUN_MODE_DBG 1 // 调试版运行模式

#define OUT_TO_CONSOLE 2 //输出到标准输出
#define OUT_TO_FILE 3   // 输出到文件
#define OUT_TO_FILE_ON 4 // 有条件输出到文件
#define OUT_TO_NET 5    // 输出到网络

#define OUT_NORM_MSG 0 //普通信息输出
#define OUT_JVC_MSG 6 // 分控信息输出
#define OUT_JVS_MSG 7 // 主控信息输出
#define OUT_LOG_MSG 8 // 输出日志信息
#define OUT_JVS_ERR 9 // 主控错误信息输出
#define OUT_JVC_ERR 14 // 分控错误信息输出
#define OUT_JVS_WARN 16 // 主控警告信息输出
#define OUT_JVC_WARN 17 //分控警告信息输出
#define OUT_ON_EVENT 18 // 事件触发输出
#define OUT_YST_ONLINE 19 // 上线信息输出
#define OUT_ALL_MSG 20 //所有信息输出
#define OUT_JVS_VIDEO_IN 21 //主控视频发送入口信息
#define OUT_JVS_VIDEO_SND 22// 主控视频发送出口信息
#define OUT_JVS_VIDEO_RCV 24 //主控接受信息
#define OUT_SOCKET_CHECK 29 // 套接字错误检测
#define OUT_JVS_CONNECT  32// // 连接信息输出
#define OUT_JVS_CHAT_SDN 33  // chat 信息输出
#define OUT_JVS_LANSERCH 34 //设备搜索相关输出
#define OUT_JVS_BCSERCH 35 // 广播




#define DBG_CTRL_OUTTO  10 // 控制输出终端指令
#define DBG_CTRL_RUN     11 // 控制运行模式
#define DBG_CTRL_MSG     12 // 控制输出信息类别，如分控信息输出，日志信息输出，错误信息输出等
#define DBG_CTRL_COND     15 // 设置调试变量值
#define DBG_CTRL_VIDEO    23 // 控制视频的输出信息 cmd(1):type(1):chnum(4):clientIndex(4):enable(1):other(4)
#define DBG_CTRL_THREAD_CPUTIME 25 // 显示线程指定时间内cpu使用时间 cmd(1):threadid(4):time(4）


#define SHOW_JVS_CLIENT_MSG 13 // 显示连接到主控的分控信息
#define SHOW_JVS_THREAD_MSG 27 // 显示主控线程ID
#define SHOW_SYS_MEM 28 // 显示系统内存剩余
#define SHOW_JVS_CH_CONFIG 31 // 显示主控配置信息
#define SHOW_JVS_CH_FRAMEBUFFER 30 // 显示主控通道缓冲信息
#define CHECK_JVS_LOCK_STAT 36 //检测主控死锁情况 

#define HEART_BEAT 37   //心跳包
#ifdef WIN32
#define JVN_DBG_LOG_DIR "JVNDBG"
#else
#define JVN_DBG_LOG_DIR "/etc/conf.d/JVNDBG"
#endif
#ifndef WIN32
	#define closesocket(s)   g_dbg.closesocketdbg(s, __FILE__, __LINE__)
	#define outdbg(...)      g_dbg.out((__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__)  // 普通信息输出
	//#define outdbgto(A,...)  g_dbg.outto((__FILE__),(__LINE__),(__FUNCTION__),A,##__VA_ARGS__) //自定义输出终端
	#define outdbgc(A,...)   g_dbg.jvcout(A,(__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__) //分控信息输出
	#define outdbgs(A,...)   g_dbg.jvsout(A,(__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__) // 主控信息输出
#else
	#define __FUNCTION__ ""
	#define outdbg      //
	#define outdbgc    //
	#define outdbgs   //
	#define  pid_t unsigned int
#endif


#define DBG_COND_VALUE   g_dbg.m_nCondValue  //调试变量
#define RUN_MODE         g_dbg.m_nRunMode  // 运行模式变量

#define  THREAD_TYPE_CH 1  //通道绑定线程
typedef struct _ThreadStat
{
	unsigned long threadId;
	char threadName[64];
	int nCH;
	unsigned long lastThreadTime;
}ThreadStat;



class CDbgInfo
{
public:
	CDbgInfo();
	~CDbgInfo();
	void out(const char* file,const int line,const char* func,char* format, ...); //
	void outto(const char* file,const int line,const char* func,int terminal,char* format, ...);//输出到？终端
	void jvsout(int type,const char* file,const int line,const char* func,char* format, ...); //主控信息输出
	void jvcout(int type,const char* file,const int line,const char* func,char* format, ...);//分控信息输出
	void logout(const char* file,const int line,const char* func,char* format,...); // 日志信息输出
	int SendToNet(char* buffer,int len);

	int closesocketdbg(int &nSocket, char *szFile, int nLine);

	int SetRunMode(int type); //设置运行模式，pub模式或dbg模式
	int SetOutTerminal(int terminal); //设置输出终端
	int SetCondValue(int cond); // 设置调试变量值
	int SetVideoMsg(char type,int nCh,int nClientID,char run,int nOther);// 设置视频显示相关信息
	int SetCpuInterval(pid_t tid,unsigned int time); // 设置线程cpu 使用时间采样间隔

	int EnableOut(int type,bool run);// 使能输出
	
	//void jvs_sleep(unsigned long time);

	void AddThreadID(const char* func,int type,void* data);
	void DeleteThreadID(const char* func);
	void ListThreadMsg(void);

	void GetProcCpuTime(void); //保留
	void GetThreadCpuTime(void);
	unsigned long GetSysMemSnap(void);
	void GetThreadInProcRate(void);//保留

#ifndef WIN32
	void SetClientAddr(sockaddr_in addr);
	void SetServerSocket(int sock);
	int try_get_lock(pthread_mutex_t *mutex,const char* pfile,const int line,const char* pfun);
#endif

public:
	int m_nJvsCH;// 选定通道
	int m_nJvsTypeIn; //主控视频输入类型选择
	int m_nJvsTypeSnd; // 主控发送类型选择
	int m_nJvsTypeRcv; // 主控接受类型选择
	int m_nClientIndex; // 分控ID
	unsigned int m_nTimeInterval;// 线程耗用时间采样间隔，单位miao
	
private:
	unsigned long GetProcValue(char* src,int start,int num);
	unsigned long GetProcSnap(void);
	unsigned long GetThreadSnap(pid_t tid);
	unsigned long GetSysSnap(void);
	float GetRate(unsigned long rator,unsigned long total);
	void SetDbgByFile(char* src);
	void GetCurModuePath(char *strDir);
private:
	int m_nOutTerminal;// 调试信息输出终端
	int m_nSelectOut;// 调试信息选择性输出
	bool m_bRun; // 
	bool m_bJvs;// 是否启动主控输出
	bool m_bJvc;// 是否启动分控输出
	bool m_bLog; //是否启动日志输出
	bool m_bJvsErr;//是否启动主控错误输出
	bool m_bJvcErr;//是否启动分控错误输出
	bool m_bJvsWarn;//是否启动主控错误输出
	bool m_bJvcWarn;//是否启动分控错误输出
	bool m_bJvsOnline; //是否启动上线信息输出
	bool m_bJvsVideoIn;// 主控视频入口信息
	bool m_bJvsVideoOut;// 主控视频出口信息
	bool m_bJvsRcv; // 主控接受信息
	bool m_bSocketCheck; // 套接字检测
	bool m_bJvsCon; //是否启动主控连接相关信息
	bool m_bJvsChat; //
	bool m_bJvsLanSerch; //
	bool m_bJvsBCSearch;
	::std::vector<ThreadStat> m_threads;
	

#ifndef WIN32
	sockaddr_in m_clientAddr;
	int m_nSock;
#endif
	
	pid_t m_pid;
	unsigned long m_lastCpuTime;
	unsigned long m_lastProcTime;
	pid_t m_tid;

#ifndef WIN32
	pthread_t m_hDbgHander;
	pthread_mutex_t m_ctThread;
#else
	
#endif
public:
	volatile int m_nCondValue;// 内部调试用
	volatile int m_nRunMode; // 运行模式，调试或发布
};

extern CDbgInfo g_dbg;

#endif                                                                                                                                                                   
  
