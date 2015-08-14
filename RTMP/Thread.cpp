//----------------------------------------------------------------------
// Thread.cpp
// 线程相关
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#include "JRCDef.h"
#include "Thread.h"
#include "RtmpStream.h"
#include  "Def.h"
namespace JMS
{
	Mutex::Mutex()
	{
#ifdef WIN32
		InitializeCriticalSection(&m_cs);
#else
		pthread_mutex_init(&m_cs, NULL);
#endif
	}

	Mutex::~Mutex()
	{
#ifdef WIN32
		DeleteCriticalSection(&m_cs);
#else
		pthread_mutex_destroy(&m_cs);
#endif
	}

	void Mutex::Lock()
	{
#ifdef WIN32
		EnterCriticalSection(&m_cs);
#else
		pthread_mutex_lock(&m_cs);
#endif
	}

	bool Mutex::TryLock()
	{

#ifdef WIN32
		EnterCriticalSection(&m_cs);
		return true;
#else
		return pthread_mutex_trylock(&m_cs) == 0;
#endif
	}

	void Mutex::Unlock()
	{
#ifdef WIN32
		LeaveCriticalSection(&m_cs);
#else
		pthread_mutex_unlock(&m_cs);
#endif
	}


	Event::Event(bool bManualReset, bool bInitState)
	{
#ifdef WIN32
		m_hHandle = CreateEvent(NULL, bManualReset ? TRUE : FALSE, bInitState ? TRUE : FALSE, NULL);
		assert(m_hHandle != NULL);
#else
		m_bState = bInitState;
		m_bManualReset = bManualReset;
		pthread_mutex_init(&m_hMutex, NULL);
		pthread_cond_init(&m_hCond, NULL);
#endif
	}

	Event::~Event()
	{
#ifdef WIN32
		if(m_hHandle != NULL)
		{
			CloseHandle(m_hHandle);
			m_hHandle = NULL;
		}
#else
		pthread_cond_destroy(&m_hCond);
		pthread_mutex_destroy(&m_hMutex);
#endif
	}

	bool Event::Set()
	{
#ifdef WIN32
		return ::SetEvent(m_hHandle) != FALSE;
#else
		if(pthread_mutex_lock(&m_hMutex) != 0)
		{
			return false;
		}

		m_bState = true;

		if(m_bManualReset)
		{
			if(pthread_cond_broadcast(&m_hCond))
			{
				return false;
			}
		}
		else
		{
			if(pthread_cond_signal(&m_hCond))
			{
				return false;
			}
		}

		if(pthread_mutex_unlock(&m_hMutex) != 0)
		{
			return false;
		}

		return true;
#endif
	}

	bool Event::Reset()
	{
#ifdef WIN32
		return ::ResetEvent(m_hHandle) != FALSE;
#else
		if(pthread_mutex_lock(&m_hMutex) != 0)
		{
			return false;
		}

		m_bState = false;

		if(pthread_mutex_unlock(&m_hMutex) != 0)
		{
			return false;
		}

		return true;
#endif
	}

	bool Event::Wait()
	{
#ifdef WIN32
		return WaitForSingleObject(m_hHandle, INFINITE) == WAIT_OBJECT_0;
#else
		if(pthread_mutex_lock(&m_hMutex))
		{
			return false;
		}

		while(!m_bState)
		{
			if(pthread_cond_wait(&m_hCond, &m_hMutex))
			{
				pthread_mutex_unlock(&m_hMutex);
				return false;
			}
		}

		if(!m_bManualReset)
		{
			m_bState = false;
		}

		if(pthread_mutex_unlock(&m_hMutex))
		{
			return false;
		}

		return true;
#endif
	}

	int Event::TimedWait(int nTimeout)
	{
#ifdef WIN32
		switch(WaitForSingleObject(m_hHandle, nTimeout))
		{
		case WAIT_OBJECT_0:
			return 1;
			break;

		case WAIT_TIMEOUT:
			return 0;
			break;
		}

		return -1;
#else
		int rc = 0;
		struct timespec abstime;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		abstime.tv_sec  = tv.tv_sec + nTimeout / 1000;
		abstime.tv_nsec = tv.tv_usec * 1000 + (nTimeout % 1000) * 1000000;
		if(abstime.tv_nsec >= 1000000000)
		{
			abstime.tv_nsec -= 1000000000;
			abstime.tv_sec++;
		}

		if(pthread_mutex_lock(&m_hMutex) != 0)
		{
			return -1;
		}

		while(!m_bState)
		{
			rc = pthread_cond_timedwait(&m_hCond, &m_hMutex, &abstime);
			if(rc)
			{
				if(rc == ETIMEDOUT)
				{
					break;
				}
				pthread_mutex_unlock(&m_hMutex);
				return -1;
			}
		}

		if(rc == 0 && !m_bManualReset)
		{
			m_bState = false;
		}
		if(pthread_mutex_unlock(&m_hMutex) != 0)
		{
			return -1;
		}
		if(rc == ETIMEDOUT)
		{
			return 0;
		}

		return 1;
#endif
	}


	RWLock::RWLock()
	{
#ifdef LINUX
		pthread_rwlock_init(&m_cs, NULL);
#else
		m_lCount = 0;
		m_lDirect = 0;
		m_hFinishEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(m_hFinishEvent != NULL);
		InitializeCriticalSection(&m_hStartLock);
#endif
	}

	RWLock::~RWLock()
	{
#ifdef LINUX
		pthread_rwlock_destroy(&m_cs);
#else
		assert(m_lCount == 0);
		CloseHandle(m_hFinishEvent);
		DeleteCriticalSection(&m_hStartLock);
#endif
	}

	void RWLock::ReadLock()
	{
#ifdef LINUX
		pthread_rwlock_rdlock(&m_cs);
#else
		EnterCriticalSection(&m_hStartLock);
		while(m_lCount > 0 && m_lDirect != 0)
		{
			WaitForSingleObject(m_hFinishEvent, INFINITE);
		}
		m_lDirect = 0;
		InterlockedIncrement((long* )&m_lCount);
		LeaveCriticalSection(&m_hStartLock);
#endif
	}

	bool RWLock::TryReadLock()
	{
#ifdef LINUX
		return pthread_rwlock_tryrdlock(&m_cs) == 0;
#else
		EnterCriticalSection(&m_hStartLock);
		if(m_lCount > 0 && m_lDirect != 0)
		{
			LeaveCriticalSection(&m_hStartLock);
			return false;
		}
		m_lDirect = 0;
		InterlockedIncrement((long* )&m_lCount);
		LeaveCriticalSection(&m_hStartLock);

		return true;
#endif
	}

	void RWLock::ReadUnlock()
	{
#ifdef LINUX
		pthread_rwlock_unlock(&m_cs);
#else
		assert(m_lCount > 0);
		assert(m_lDirect == 0);
		InterlockedDecrement((long* )&m_lCount);
		SetEvent(m_hFinishEvent);
#endif
	}

	void RWLock::WriteLock()
	{
#ifdef LINUX
		pthread_rwlock_wrlock(&m_cs);
#else
		EnterCriticalSection(&m_hStartLock);
		while(m_lCount > 0 && m_lDirect != 1)
		{
			WaitForSingleObject(m_hFinishEvent, INFINITE);
		}
		m_lDirect = 1;
		InterlockedIncrement((long* )&m_lCount);
		LeaveCriticalSection(&m_hStartLock);
#endif
	}

	bool RWLock::TryWriteLock()
	{
#ifdef LINUX
		return pthread_rwlock_trywrlock(&m_cs) == 0;
#else
		EnterCriticalSection(&m_hStartLock);
		if(m_lCount > 0 && m_lDirect != 1)
		{
			LeaveCriticalSection(&m_hStartLock);
			return false;
		}
		m_lDirect = 1;
		InterlockedIncrement((long* )&m_lCount);
		LeaveCriticalSection(&m_hStartLock);

		return true;
#endif
	}

	void RWLock::WriteUnlock()
	{
#ifdef LINUX
		pthread_rwlock_unlock(&m_cs);
#else
		assert(m_lCount > 0);
		assert(m_lDirect == 1);
		InterlockedDecrement((long* )&m_lCount);
		SetEvent(m_hFinishEvent);
#endif
	}


	Semaphore::Semaphore(int nInitCount)
	{
#ifdef WIN32
		m_hHandle = CreateSemaphore(NULL, nInitCount, 0x7FFFFFFF, NULL);
		assert(m_hHandle != INVALID_HANDLE_VALUE);
#else
#  ifdef DEBUG
		int nRet =
#  endif
		sem_init(&m_sem, 0, nInitCount);
		assert(nRet == 0);
#endif
	}

	Semaphore::~Semaphore()
	{
#ifdef WIN32
		CloseHandle(m_hHandle);
#else
		::sem_destroy(&m_sem);
#endif
	}

	bool Semaphore::Wait()
	{
#ifdef WIN32
		return WaitForSingleObject(m_hHandle, -1) == 0;
#else
		return sem_wait(&m_sem) == 0;
#endif
	}

	bool Semaphore::Post()
	{
#ifdef WIN32
		return ReleaseSemaphore(m_hHandle, 1, NULL) != FALSE;
#else
		return sem_post(&m_sem) == 0;
#endif
	}


	Thread::Thread()
	{
#ifdef WIN32
		m_hHandle = INVALID_HANDLE_VALUE;
#else
#endif
		m_pThreadProc = NULL;
	}

	Thread::~Thread()
	{
#ifdef WIN32
#else
#endif
	}

	bool Thread::Run(ThreadProc_t proc, void *pParam)
	{
		assert(proc != NULL);

		m_pThreadProc = proc;
		m_pParam = pParam;
		bool bRet;

#ifdef WIN32
		m_hHandle = (HANDLE)_beginthread(AriesThreadProc, 0, this);
		bRet = m_hHandle != INVALID_HANDLE_VALUE;
#else
		bRet = pthread_create(&m_hHandle, NULL, AriesThreadProc, this) == 0;
#endif
		if(!bRet)
		{
			m_pThreadProc = NULL;
		}
		return bRet;
	}

	bool Thread::WaitForEnd(uint32_t dwTimeOut)
	{
		if(m_pThreadProc == NULL)
		{
			return true;
		}
		else
		{
#ifdef WIN32
			DWORD dwRet = WaitForSingleObject(m_hHandle, dwTimeOut);
			return dwRet == WAIT_OBJECT_0;
#else
			void *pRet = NULL;
#ifdef MOBILE_CLIENT
			return false;
#else
			return pthread_join(m_hHandle, &pRet) == 0;
#endif
#endif
		}
	}

	bool Thread::KillThread()
	{
		RtmpStreamBase* pBase = (RtmpStreamBase* )m_pParam;
#ifdef LINUX
#ifdef MOBILE_CLIENT
		if(pBase)
		{
            pBase->m_bExit = true;
			while(!pBase->m_bExitSignal)
			{
				Sleep(10);
//                printf("kill thread still\n");
			}
		}
        
        return true;

#else
		if(pthread_cancel(m_hHandle) != 0)
		{
			return false;
		}
		void *pRet = NULL;
		return pthread_join(m_hHandle, &pRet) == 0;
#endif
#else
		return TerminateThread(m_hHandle, -1) != FALSE;
#endif
}

	bool Thread::IsRunning()
	{
		return m_pThreadProc != NULL;
	}

	Thread::ThreadProcRetType Thread::AriesThreadProc(void *pParam)
	{
		Thread *pThread = (Thread*)pParam;
		pThread->m_pThreadProc(pThread->m_pParam);
		pThread->m_pThreadProc = NULL;

#ifdef LINUX
		pthread_exit(0);
		return NULL;
#else
		_endthread();
#endif
	}

	void Thread::Sleep(uint32_t dwMiliSec)
	{
#ifdef WIN32
		::Sleep(dwMiliSec);
#else
		::usleep(1000 * dwMiliSec);
#endif
	}

	int Thread::GetThreadID()
	{
#ifdef LINUX
		return (long)pthread_self();
#else
		return GetCurrentThreadId();
#endif
	}
}
