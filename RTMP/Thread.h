//?//----------------------------------------------------------------------
// Thread.h
// 线程相关
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#include "JRCDef.h"

namespace JMS
{
	/**
	* @class Mutex
	* @brief 互斥锁
	*/
	class Mutex
	{
	public:
		Mutex();
		~Mutex();

	public:
		/**
		* @brief 锁定同步锁
		*/
		void Lock();

		/**
		* @brief 尝试锁定同步锁
		* @return 返回操作结果
		*/
		bool TryLock();

		/**
		* @brief 解锁同步锁
		*/
		void Unlock();

	private:
#ifdef WIN32
		CRITICAL_SECTION m_cs;		//临界区句柄
#else 
		pthread_mutex_t m_cs;		//临界区句柄
#endif
	};

	/**
	* @class Event
	* @brief 同步事件
	*/
	class Event
	{
	public:
		Event(bool bManualReset, bool bInitState);
		~Event();

	public:
		/**
		* @brief 设置事件
		* @return 返回操作结果
		*/
		bool Set();

		/**
		* @brief 复位事件
		* @return 返回操作结果
		*/
		bool Reset();

		/**
		* @brief 等待事件
		* @return 返回操作结果
		*/
		bool Wait();

		/**
		* @brief 超时等待
		* @param nTimeout 超时时间(毫秒)
		* @return 成功返回正数，失败返回负数，超时返回0
		*/
		int TimedWait(int nTimeout);

	private:
#ifdef WIN32
		HANDLE m_hHandle;			//同步事件句柄句柄
#else
		bool m_bState;			//当前状态
		bool m_bManualReset;		//自动复位
		pthread_mutex_t m_hMutex;	//临界区句柄
		pthread_cond_t m_hCond;		//信号量句柄
#endif
	};

	/**
	* @class RWLock
	* @brief 读写锁
	*/
	class RWLock
	{
	public:
		RWLock();
		~RWLock();

	public:
		/**
		* @brief 读锁定
		*/
		void ReadLock();

		/**
		* @brief 尝试读锁定
		* @return 返回操作结果
		*/
		bool TryReadLock();

		/**
		* @brief 读解锁
		*/
		void ReadUnlock();

		/**
		* @brief 写锁定
		*/
		void WriteLock();

		/**
		* @brief 尝试写锁定
		* @return 返回操作结果
		*/
		bool TryWriteLock();

		/**
		* @brief 写解锁
		*/
		void WriteUnlock();

	private:
#ifdef LINUX
		pthread_rwlock_t m_cs;			//锁句柄
#else
		volatile LONG m_lCount;			//锁定计数
		volatile LONG m_lDirect;		//锁定状态,1表示写,0表示读
		HANDLE m_hFinishEvent;			//事件句柄
		CRITICAL_SECTION m_hStartLock;	//同步锁
#endif
	};

	/**
	* @class Semaphore
	* @brief 信号量
	*/
	class Semaphore
	{
	public:
		Semaphore(int nInitCount);
		~Semaphore();

	public:
		/**
		* @brief 等待信号量
		* @return 返回操作结果
		*/
		bool Wait();

		/**
		* @brief 释放信号量
		* @return 返回操作结果
		*/
		bool Post();

	private:
#ifdef WIN32
		HANDLE m_hHandle;		//信号量句柄
#else
		sem_t m_sem;			//信号量句柄
#endif
	};

	/**
	* @class Thread
	* @brief 线程类
	*/
	class Thread
	{
	public:
		typedef void (*ThreadProc_t)(void *pParam);

#ifdef WIN32
		typedef void ThreadProcRetType;
#else
		typedef void* ThreadProcRetType;
#endif

		Thread();
		~Thread();

	public:
		/**
		* @brief 创建并运行线程
		* @param proc 线程处理函数
		* @param pParam 用户数据指针
		* @return 返回执行结果
		*/
		bool Run(ThreadProc_t proc, void *pParam = NULL);

		/**
		* @brief 等待结束
		* @param dwTimeOut 超时时间(ms)
		* @param bKillWhenTimeout 等待超时是是否杀死线程
		* @return 线程正常退出返回true,等待超时返回false
		*/
		bool WaitForEnd(uint32_t dwTimeOut = -1);

		/**
		* @brief 杀死线程
		* @return 返回执行结果
		*/
		bool KillThread();

		/**
		* @brief 线程是否正在运行
		* @return 返回线程是否正在运行
		*/
		bool IsRunning();

		/**
		* @brief 休眠本线程
		* @param dwMiliSec 休眠时间(ms)
		*/
		static void Sleep(uint32_t dwMiliSec);

		/**
		* @brief 获取线程ID
		* @return 返回当前线程ID
		*/
		static int GetThreadID();

	private:
		static ThreadProcRetType AriesThreadProc(void *pParam);

	protected:
		ThreadProc_t m_pThreadProc;		//线程函数指针
		void *m_pParam;					//用户数据指针

#ifdef WIN32
		HANDLE m_hHandle;				//线程句柄
#else
		pthread_t m_hHandle;			//线程句柄
#endif
	};
}
