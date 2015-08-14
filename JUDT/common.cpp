/*****************************************************************************
written by
   Yunhong Gu, last updated 05/21/2009
*****************************************************************************/


#ifndef WIN32
   #include <cstring>
   #include <cerrno>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #ifdef LEGACY_WIN32
      #include <wspiapi.h>
   #endif
#endif
#include <cmath>
#include "md5.h"
#include "common.h"

uint64_t CTimer::s_ullCPUFrequency = CTimer::readCPUFrequency();
#ifndef WIN32
   pthread_mutex_t CTimer::m_EventLock = PTHREAD_MUTEX_INITIALIZER;
   pthread_cond_t CTimer::m_EventCond = PTHREAD_COND_INITIALIZER;
#else
   pthread_mutex_t CTimer::m_EventLock = CreateMutex(NULL, false, NULL);
   pthread_cond_t CTimer::m_EventCond = CreateEvent(NULL, false, false, NULL);
#endif

CTimer::CTimer():
m_ullSchedTime(),
m_TickCond(),
m_TickLock()
{
   #ifndef WIN32
      pthread_mutex_init(&m_TickLock, NULL);
      pthread_cond_init(&m_TickCond, NULL);
	  //////////////////////////////////////////////////////////////////////////
//	  pthread_condattr_t condattr;
//	  int ret = pthread_condattr_init(&condattr);
//	  if (ret == 0) 
//	  {
//		  pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
//	  }
//	  pthread_cond_init(&m_TickCond, &condattr);	  
	  //////////////////////////////////////////////////////////////////////////
   #else
      m_TickLock = CreateMutex(NULL, false, NULL);
      m_TickCond = CreateEvent(NULL, false, false, NULL);
   #endif
}

CTimer::~CTimer()
{
   #ifndef WIN32
      pthread_mutex_destroy(&m_TickLock);
      pthread_cond_destroy(&m_TickCond);
   #else
      CloseHandle(m_TickLock);
      CloseHandle(m_TickCond);
   #endif
}

void CTimer::rdtsc(uint64_t &x)
{
   #ifdef WIN32
      //HANDLE hCurThread = ::GetCurrentThread(); 
      //DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1); 
      BOOL ret = QueryPerformanceCounter((LARGE_INTEGER *)&x);
      //SetThreadAffinityMask(hCurThread, dwOldMask);

      if (!ret)
         x = getTime() * s_ullCPUFrequency;

   #elif IA32
      uint32_t lval, hval;
      //asm volatile ("push %eax; push %ebx; push %ecx; push %edx");
      //asm volatile ("xor %eax, %eax; cpuid");
      asm volatile ("rdtsc" : "=a" (lval), "=d" (hval));
      //asm volatile ("pop %edx; pop %ecx; pop %ebx; pop %eax");
      x = hval;
      x = (x << 32) | lval;
   #elif IA64
      asm ("mov %0=ar.itc" : "=r"(x) :: "memory");
   #elif AMD64
      uint32_t lval, hval;
      asm ("rdtsc" : "=a" (lval), "=d" (hval));
      x = hval;
      x = (x << 32) | lval;
   #else
      // use system call to read time clock for other archs
      timeval t;
      gettimeofday(&t, 0);
      x = (uint64_t)t.tv_sec * (uint64_t)1000000 + (uint64_t)t.tv_usec;
	  //////////////////////////////////////////////////////////////////////////
//	  struct timespec ts;
//	  clock_gettime(CLOCK_MONOTONIC, &ts);	
//	  x = (uint64_t)(ts.tv_sec*1000000 + ts.tv_nsec/1000);
	  //////////////////////////////////////////////////////////////////////////
   #endif
}

uint64_t CTimer::readCPUFrequency()
{
   #ifdef WIN32
      int64_t ccf;
      if (QueryPerformanceFrequency((LARGE_INTEGER *)&ccf))
         return ccf / 1000000;
      else
         return 1;
   #elif IA32 || IA64 || AMD64
      uint64_t t1, t2;

      rdtsc(t1);
      timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 100000000;
      nanosleep(&ts, NULL);
      rdtsc(t2);

      // CPU clocks per microsecond
      return (t2 - t1) / 100000;
   #else
      return 1;
   #endif
}

uint64_t CTimer::getCPUFrequency()
{
   return s_ullCPUFrequency;
}

void CTimer::sleep(const uint64_t& interval)
{
   uint64_t t;
   rdtsc(t);

   // sleep next "interval" time
   sleepto(t + interval);
}

void CTimer::sleepto(const uint64_t& nexttime)
{
   // Use class member such that the method can be interrupted by others
   m_ullSchedTime = nexttime;

   uint64_t t;
   rdtsc(t);

   while (t < m_ullSchedTime)
   {
      #ifndef NO_BUSY_WAITING
         #ifdef IA32
            __asm__ volatile ("pause; rep; nop; nop; nop; nop; nop;");
         #elif IA64
            __asm__ volatile ("nop 0; nop 0; nop 0; nop 0; nop 0;");
         #elif AMD64
            __asm__ volatile ("nop; nop; nop; nop; nop;");
         #endif
      #else
         #ifndef WIN32
            timeval now;
            timespec timeout;
            gettimeofday(&now, 0);
            if (now.tv_usec < 990000)
            {
               timeout.tv_sec = now.tv_sec;
               timeout.tv_nsec = (now.tv_usec + 10000) * 1000;
            }
            else
            {
               timeout.tv_sec = now.tv_sec + 1;
               timeout.tv_nsec = (now.tv_usec + 10000 - 1000000) * 1000;
            }
            pthread_mutex_lock(&m_TickLock);
            pthread_cond_timedwait(&m_TickCond, &m_TickLock, &timeout);
            pthread_mutex_unlock(&m_TickLock);

			//////////////////////////////////////////////////////////////////////////
/*			struct timespec tv;
			clock_gettime(CLOCK_MONOTONIC, &tv);
			if(tv.tv_nsec < 990000000)//1000000000-10000000)
			{
				tv.tv_nsec += 10000000;//
			}
			else
			{
				tv.tv_sec += 1;//
				tv.tv_nsec = (tv.tv_nsec + 10000000 - 1000000000);
			}			
			pthread_mutex_lock(&m_TickLock);
			pthread_cond_timedwait(&m_TickCond, &m_TickLock, &tv);
			pthread_mutex_unlock(&m_TickLock);
*/			//////////////////////////////////////////////////////////////////////////
         #else
            WaitForSingleObject(m_TickCond, 1);
         #endif
      #endif

      rdtsc(t);
   }
}

void CTimer::interrupt()
{
   // schedule the sleepto time to the current CCs, so that it will stop
   rdtsc(m_ullSchedTime);

   tick();
}

void CTimer::tick()
{
   #ifndef WIN32
      pthread_cond_signal(&m_TickCond);
   #else
      SetEvent(m_TickCond);
   #endif
}

uint64_t CTimer::getTime()
{
   #ifndef WIN32
      timeval t;
      gettimeofday(&t, 0);
      return t.tv_sec * 1000000ULL + t.tv_usec;
	  //////////////////////////////////////////////////////////////////////////
//	  struct timespec ts;
//      clock_gettime(CLOCK_MONOTONIC, &ts);	
//      return (uint64_t)(ts.tv_sec*1000000 + ts.tv_nsec/1000);
	  //////////////////////////////////////////////////////////////////////////
   #else
/*wm     LARGE_INTEGER ccf;
      HANDLE hCurThread = ::GetCurrentThread(); 
      DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1);
      if (QueryPerformanceFrequency(&ccf))
      {
         LARGE_INTEGER cc;
         if (QueryPerformanceCounter(&cc))
         {
            SetThreadAffinityMask(hCurThread, dwOldMask); 
            return (cc.QuadPart * 1000000UL / ccf.QuadPart);//(cc.QuadPart * 1000000ULL / ccf.QuadPart);
         }
      }

      SetThreadAffinityMask(hCurThread, dwOldMask); 
*/      return GetTickCount() * 1000UL;//GetTickCount() * 1000ULL;
   #endif
}

void CTimer::triggerEvent()
{
   #ifndef WIN32
      pthread_cond_signal(&m_EventCond);
   #else
      SetEvent(m_EventCond);
   #endif
}

void CTimer::waitForEvent()
{
   #ifndef WIN32
      timeval now;
      timespec timeout;
      gettimeofday(&now, 0);
      if (now.tv_usec < 990000)
      {
         timeout.tv_sec = now.tv_sec;
         timeout.tv_nsec = (now.tv_usec + 10000) * 1000;
      }
      else
      {
         timeout.tv_sec = now.tv_sec + 1;
         timeout.tv_nsec = (now.tv_usec + 10000 - 1000000) * 1000;
      }
      pthread_mutex_lock(&m_EventLock);
      pthread_cond_timedwait(&m_EventCond, &m_EventLock, &timeout);
      pthread_mutex_unlock(&m_EventLock);
   #else
      WaitForSingleObject(m_EventCond, 1);
   #endif
}

DWORD CTimer::GetTime()
{
	DWORD dwresult=0;
#ifndef WIN32
#ifdef MOBILE_CLIENT
	struct timeval start;
	gettimeofday(&start,NULL);	
	dwresult = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
#else
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);	
    dwresult = (DWORD)(ts.tv_sec*1000 + ts.tv_nsec/1000000);
#endif
#else
	dwresult = GetTickCount();
#endif
	
	return dwresult;
}


//
// Automatically lock in constructor
CGuard::CGuard(pthread_mutex_t& lock):
m_Mutex(lock),
m_iLocked()
{
   #ifndef WIN32
      m_iLocked = pthread_mutex_lock(&m_Mutex);
   #else
      m_iLocked = WaitForSingleObject(m_Mutex, INFINITE);
   #endif
}

// Automatically unlock in destructor
CGuard::~CGuard()
{
   #ifndef WIN32
      if (0 == m_iLocked)
         pthread_mutex_unlock(&m_Mutex);
   #else
      if (WAIT_FAILED != m_iLocked)
         ReleaseMutex(m_Mutex);
   #endif
}

void CGuard::enterCS(pthread_mutex_t& lock)
{
   #ifndef WIN32
      pthread_mutex_lock(&lock);
   #else
      WaitForSingleObject(lock, INFINITE);
   #endif
}

void CGuard::leaveCS(pthread_mutex_t& lock)
{
   #ifndef WIN32
      pthread_mutex_unlock(&lock);
   #else
      ReleaseMutex(lock);
   #endif
}


//
CUDTException::CUDTException(int major, int minor, int err):
m_iMajor(major),
m_iMinor(minor)
{
   if (-1 == err)
      #ifndef WIN32
         m_iErrno = errno;
      #else
         m_iErrno = GetLastError();
      #endif
   else
   {
	   m_iErrno = err;
	   m_strMsg[0] = '\0';
   }
}

CUDTException::CUDTException(const CUDTException& e):
m_iMajor(e.m_iMajor),
m_iMinor(e.m_iMinor),
m_iErrno(e.m_iErrno),
m_strMsg()
{
	m_strMsg[0] = '\0';
}

CUDTException::~CUDTException()
{
}

const char* CUDTException::getErrorMessage()
{
   // translate "Major:Minor" code into text message.

   switch (m_iMajor)
   {
      case 0:
        //m_strMsg = "Success";
		  strcpy(m_strMsg, "Success");
        break;

      case 1:
        //m_strMsg = "Connection setup failure";
		  strcpy(m_strMsg, "Connection setup failure");

        switch (m_iMinor)
        {
        case 1:
           //m_strMsg += ": connection time out";
			strcat(m_strMsg, ": connection time out");
           break;

        case 2:
           //m_strMsg += ": connection rejected";
			strcat(m_strMsg, ": connection rejected");
           break;

        case 3:
           //m_strMsg += ": unable to create/configure UDP socket";
		    strcat(m_strMsg, ": unable to create/configure UDP socket");
           break;

        case 4:
           //m_strMsg += ": abort for security reasons";
			strcat(m_strMsg, ": abort for security reasons");
           break;

        default:
           break;
        }

        break;

      case 2:
        switch (m_iMinor)
        {
        case 1:
           //m_strMsg = "Connection was broken";
			strcpy(m_strMsg, "Connection was broken");
           break;

        case 2:
           //m_strMsg = "Connection does not exist";
			strcpy(m_strMsg, "Connection does not exist");
           break;

        default:
           break;
        }

        break;

      case 3:
        //m_strMsg = "System resource failure";
		  strcpy(m_strMsg, "System resource failure");

        switch (m_iMinor)
        {
        case 1:
           //m_strMsg += ": unable to create new threads";
			strcat(m_strMsg, ": unable to create new threads");
           break;

        case 2:
           //m_strMsg += ": unable to allocate buffers";
		   strcat(m_strMsg, ": unable to allocate buffers");
           break;

        default:
           break;
        }

        break;

      case 4:
        //m_strMsg = "File system failure";
		  strcpy(m_strMsg, "File system failure");

        switch (m_iMinor)
        {
        case 1:
           //m_strMsg += ": cannot seek read position";
			strcat(m_strMsg, ": cannot seek read position");
           break;

        case 2:
           //m_strMsg += ": failure in read";
			strcat(m_strMsg, ": failure in read");
           break;

        case 3:
           //m_strMsg += ": cannot seek write position";
			strcat(m_strMsg, ": cannot seek write position");
           break;

        case 4:
           //m_strMsg += ": failure in write";
			strcat(m_strMsg, ": failure in write");
           break;

        default:
           break;
        }

        break;

      case 5:
        //m_strMsg = "Operation not supported";
		  strcpy(m_strMsg, "Operation not supported");
 
        switch (m_iMinor)
        {
        case 1:
           //m_strMsg += ": Cannot do this operation on a BOUND socket";
			strcat(m_strMsg, ": Cannot do this operation on a BOUND socket");
           break;

        case 2:
           //m_strMsg += ": Cannot do this operation on a CONNECTED socket";
			strcat(m_strMsg, ": Cannot do this operation on a CONNECTED socket");
           break;

        case 3:
           //m_strMsg += ": Bad parameters";
			strcat(m_strMsg, ": Bad parameters");
           break;

        case 4:
           //m_strMsg += ": Invalid socket ID";
			strcat(m_strMsg, ": Invalid socket ID");
           break;

        case 5:
           //m_strMsg += ": Cannot do this operation on an UNBOUND socket";
			strcat(m_strMsg, ": Cannot do this operation on an UNBOUND socket");
           break;

        case 6:
           //m_strMsg += ": Socket is not in listening state";
			strcat(m_strMsg, ": Socket is not in listening state");
           break;

        case 7:
           //m_strMsg += ": Listen/accept is not supported in rendezous connection setup";
			strcat(m_strMsg, ": Listen/accept is not supported in rendezous connection setup");
           break;

        case 8:
           //m_strMsg += ": Cannot call connect on UNBOUND socket in rendezvous connection setup";
			strcat(m_strMsg, ": Cannot call connect on UNBOUND socket in rendezvous connection setup");
           break;

        case 9:
           //m_strMsg += ": This operation is not supported in SOCK_STREAM mode";
			strcat(m_strMsg, ": This operation is not supported in SOCK_STREAM mode");
           break;

        case 10:
           //m_strMsg += ": This operation is not supported in SOCK_DGRAM mode";
			strcat(m_strMsg, ": This operation is not supported in SOCK_DGRAM mode");
           break;

        case 11:
           //m_strMsg += ": Another socket is already listening on the same port";
			strcat(m_strMsg, ": Another socket is already listening on the same port");
           break;

        case 12:
           //m_strMsg += ": Message is too large to send (it must be less than the UDT send buffer size)";
			strcat(m_strMsg, ": Message is too large to send (it must be less than the UDT send buffer size)");
           break;

        default:
           break;
        }

        break;

     case 6:
        //m_strMsg = "Non-blocking call failure";
		 strcpy(m_strMsg, "Non-blocking call failure");

        switch (m_iMinor)
        {
        case 1:
           //m_strMsg += ": no buffer available for sending";
			strcat(m_strMsg, ": no buffer available for sending");
           break;

        case 2:
           //m_strMsg += ": no data available for reading";
			strcat(m_strMsg, ": no data available for reading");
           break;

        default:
           break;
        }

        break;

      default:
        //m_strMsg = "Unknown error";
		  strcpy(m_strMsg, "Unknown error");
   }

   // Adding "errno" information
   if ((0 != m_iMajor) && (0 < m_iErrno))
   {
      //m_strMsg += ": ";
	   strcat(m_strMsg, ": ");
      #ifndef WIN32
	      char errmsg[1024]={0};
	      if (strerror_r(m_iErrno, errmsg, 1024) == 0)
		      strcat(m_strMsg, errmsg);//m_strMsg += errmsg;
      #else
         LPVOID lpMsgBuf;
         FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, m_iErrno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
         strcat(m_strMsg, (char*)lpMsgBuf);//m_strMsg += (char*)lpMsgBuf;
         LocalFree(lpMsgBuf);
      #endif
   }

   // period
   #ifndef WIN32
      strcat(m_strMsg, ".");//m_strMsg += ".";
   #endif

   return m_strMsg;//m_strMsg.c_str();
}

const int CUDTException::getErrorCode() const
{
   return m_iMajor * 1000 + m_iMinor;
}

void CUDTException::clear()
{
   m_iMajor = 0;
   m_iMinor = 0;
   m_iErrno = 0;
   m_strMsg[0] = '\0';
}
void CUDTException::reset(const CUDTException& e)
{
	m_iMajor = e.m_iMajor;
	m_iMinor = e.m_iMinor;
	m_iErrno = e.m_iErrno;
	m_strMsg[0] = '\0';
}
void CUDTException::reset(int major, int minor, int err)
{
	m_iMajor = major;
	m_iMinor = minor;
	m_iErrno = err;
	m_strMsg[0] = '\0';
}

const int CUDTException::SUCCESS = 0;
const int CUDTException::ECONNSETUP = 1000;
const int CUDTException::ENOSERVER = 1001;
const int CUDTException::ECONNREJ = 1002;
const int CUDTException::ESOCKFAIL = 1003;
const int CUDTException::ESECFAIL = 1004;
const int CUDTException::ECONNFAIL = 2000;
const int CUDTException::ECONNLOST = 2001;
const int CUDTException::ENOCONN = 2002;
const int CUDTException::ERESOURCE = 3000;
const int CUDTException::ETHREAD = 3001;
const int CUDTException::ENOBUF = 3002;
const int CUDTException::EFILE = 4000;
const int CUDTException::EINVRDOFF = 4001;
const int CUDTException::ERDPERM = 4002;
const int CUDTException::EINVWROFF = 4003;
const int CUDTException::EWRPERM = 4004;
const int CUDTException::EINVOP = 5000;
const int CUDTException::EBOUNDSOCK = 5001;
const int CUDTException::ECONNSOCK = 5002;
const int CUDTException::EINVPARAM = 5003;
const int CUDTException::EINVSOCK = 5004;
const int CUDTException::EUNBOUNDSOCK = 5005;
const int CUDTException::ENOLISTEN = 5006;
const int CUDTException::ERDVNOSERV = 5007;
const int CUDTException::ERDVUNBOUND = 5008;
const int CUDTException::ESTREAMILL = 5009;
const int CUDTException::EDGRAMILL = 5010;
const int CUDTException::EDUPLISTEN = 5011;
const int CUDTException::ELARGEMSG = 5012;
const int CUDTException::EASYNCFAIL = 6000;
const int CUDTException::EASYNCSND = 6001;
const int CUDTException::EASYNCRCV = 6002;
const int CUDTException::EUNKNOWN = -1;


//
bool CIPAddress::ipcmp(const sockaddr* addr1, const sockaddr* addr2, const int& ver)
{
   if (AF_INET == ver)
   {
      sockaddr_in* a1 = (sockaddr_in*)addr1;
      sockaddr_in* a2 = (sockaddr_in*)addr2;

      if ((a1->sin_port == a2->sin_port) && (a1->sin_addr.s_addr == a2->sin_addr.s_addr))
         return true;
   }
   else
   {
      sockaddr_in6* a1 = (sockaddr_in6*)addr1;
      sockaddr_in6* a2 = (sockaddr_in6*)addr2;

      if (a1->sin6_port == a2->sin6_port)
      {
         for (int i = 0; i < 16; ++ i)
            if (*((char*)&(a1->sin6_addr) + i) != *((char*)&(a2->sin6_addr) + i))
               return false;

         return true;
      }
   }

   return false;
}

void CIPAddress::ntop(const sockaddr* addr, uint32_t ip[4], const int& ver)
{
   if (AF_INET == ver)
   {
      sockaddr_in* a = (sockaddr_in*)addr;
      ip[0] = a->sin_addr.s_addr;
   }
   else
   {
      sockaddr_in6* a = (sockaddr_in6*)addr;
      ip[3] = (a->sin6_addr.s6_addr[15] << 24) + (a->sin6_addr.s6_addr[14] << 16) + (a->sin6_addr.s6_addr[13] << 8) + a->sin6_addr.s6_addr[12];
      ip[2] = (a->sin6_addr.s6_addr[11] << 24) + (a->sin6_addr.s6_addr[10] << 16) + (a->sin6_addr.s6_addr[9] << 8) + a->sin6_addr.s6_addr[8];
      ip[1] = (a->sin6_addr.s6_addr[7] << 24) + (a->sin6_addr.s6_addr[6] << 16) + (a->sin6_addr.s6_addr[5] << 8) + a->sin6_addr.s6_addr[4];
      ip[0] = (a->sin6_addr.s6_addr[3] << 24) + (a->sin6_addr.s6_addr[2] << 16) + (a->sin6_addr.s6_addr[1] << 8) + a->sin6_addr.s6_addr[0];
   }
}

void CIPAddress::pton(sockaddr* addr, const uint32_t ip[4], const int& ver)
{
   if (AF_INET == ver)
   {
      sockaddr_in* a = (sockaddr_in*)addr;
      a->sin_addr.s_addr = ip[0];
   }
   else
   {
      sockaddr_in6* a = (sockaddr_in6*)addr;
      for (int i = 0; i < 4; ++ i)
      {
         a->sin6_addr.s6_addr[i * 4] = ip[i] & 0xFF;
         a->sin6_addr.s6_addr[i * 4 + 1] = (unsigned char)((ip[i] & 0xFF00) >> 8);
         a->sin6_addr.s6_addr[i * 4 + 2] = (unsigned char)((ip[i] & 0xFF0000) >> 16);
         a->sin6_addr.s6_addr[i * 4 + 3] = (unsigned char)((ip[i] & 0xFF000000) >> 24);
      }
   }
}

//
void CMD5::compute(const char* input, unsigned char result[16])
{
   md5_state_t state;

   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)input, strlen(input));
   md5_finish(&state, result);
}
