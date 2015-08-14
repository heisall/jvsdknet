//#include <sys/time.h>
#include "Def.h"
#ifndef MOBILE_CLIENT
#include "uio.h"
#include "unistd.h"
#else
#include <unistd.h>
#include <sys/uio.h>
#endif



#include <errno.h>

#if !defined(ECURDIR)
#define ECURDIR EACCES
#endif
#if !defined(ENOSYS)
#define ENOSYS EPERM
#endif

#ifdef WIN32
#include <Winsock.h>
#else

#endif

#ifndef WIN32
#include <sys/uio.h>
#endif

#define NUM_ELEMENTS(ar) (sizeof(ar) / sizeof(ar[0]))

#ifdef WIN32

int Errno_From_Win32(unsigned long w32Err)
{
    /*	size_t i;
     for(i = 0; i < NUM_ELEMENTS(errmap); ++i)
     {
     if(w32Err == errmap[i].w32Err)
     {
     return errmap[i].eerrno;
     }
     }*/
    return EINVAL;
}

// void* Win32_Handle_From_File(int fd)
// {
// 	union
// 	{
// 		HANDLE h;
// 		long i;
// 	} u;
//
// 	if(fd < 0)
// 	{
// 		return INVALID_HANDLE_VALUE;
// 	}
// 	else
// 	{
// 		u.h = NULL;
// 		u.i = _get_osfhandle(fd);
//
// 		return u.h;
// 	}
// }

long FILETIMEToUNIXTime(FILETIME const* ft,long* microseconds)
{
    LONGLONG    i;
    
    i = ft->dwHighDateTime;
    i <<= 32;
    i |= ft->dwLowDateTime;
    
    i -= 116444736000000000L;
    if(NULL != microseconds)
    {
        *microseconds = (long)((i % 10000000) / 10);
    }
    i /= 10000000;
    return (long)i;
}

int gettimeofday(struct timeval*  tv,void*dummy)
{
    SYSTEMTIME  st;
    FILETIME    ft;
    
    ((void)dummy);
    GetSystemTime(&st);
    (void)SystemTimeToFileTime(&st, &ft);
    tv->tv_sec = FILETIMEToUNIXTime(&ft, &tv->tv_usec);
    
    return 0;
}

void usleep(long usec)
{
    LARGE_INTEGER lFrequency;
    LARGE_INTEGER lEndTime;
    LARGE_INTEGER lCurTime;
    
    QueryPerformanceFrequency (&lFrequency);
    if (lFrequency.QuadPart) {
        QueryPerformanceCounter (&lEndTime);
        lEndTime.QuadPart += (LONGLONG) usec *
        lFrequency.QuadPart / 1000000;
        do {
            QueryPerformanceCounter (&lCurTime);
            Sleep(0);
        } while (lCurTime.QuadPart < lEndTime.QuadPart);
    }
}

//ssize_t readv(int fd,struct iovec const*  vector, int count)
//{
//	int             i;
//	size_t          total;
//	void*           pv;
//	ssize_t  ret;
//	HANDLE  h = NULL;
//	DWORD   dw;
//
//	for(i = 0, total = 0; i < count; ++i)
//	{
//		total += vector[i].iov_len;
//	}
//
//	pv = HeapAlloc(GetProcessHeap(), 0, total);
//
//	if(NULL == pv)
//	{
//		errno = Errno_From_Win32(GetLastError());
//
//		ret = -1;
//	}
//	else
//	{
//		h = (HANDLE)Win32_Handle_From_File(fd);
//
//		if(!ReadFile(h, pv, (DWORD)total, &dw, NULL))
//		{
//			errno = Errno_From_Win32(GetLastError());
//
//			ret = -1;
//		}
//		else
//		{
//			for(i = 0, ret = 0; i < count && 0 != dw; ++i)
//			{
//				size_t n = (dw < vector[i].iov_len) ? dw : vector[i].iov_len;
//
//				(void)memcpy(vector[i].iov_base, (char const*)pv + ret, n);
//
//				ret +=  (ssize_t)n;
//				dw  -=  (DWORD)n;
//			}
//		}
//
//		(void)HeapFree(GetProcessHeap(), 0, pv);
//	}
//
//	return ret;
//}

//cxt ◊‘  ”√”⁄Ã◊Ω”◊÷
ssize_t writev(int fd, struct iovec const*  vector,int count)
{
    int             i;
    size_t          total;
    void*           pv;
    ssize_t  ret;
    HANDLE  h = NULL;
    //	DWORD   dw;
    
    /* Determine the total size. */
    for(i = 0, total = 0; i < count; ++i)
    {
        total += vector[i].iov_len;
    }
    
    pv = HeapAlloc(GetProcessHeap(), 0, total);
    
    if(NULL == pv)
    {
        errno = Errno_From_Win32(GetLastError());
        
        ret = -1;
    }
    else
    {
        //h = (HANDLE)Win32_Handle_From_File(fd);
        
        for(i = 0, ret = 0; i < count; ++i)
        {
            (void)memcpy((char*)pv + ret, vector[i].iov_base, vector[i].iov_len);
            
            ret += (ssize_t)vector[i].iov_len;
        }
        
        //if(!WriteFile(h, pv, (DWORD)total, &dw, NULL))
        //{
        //	errno = Errno_From_Win32(GetLastError());
        
        //	ret = -1;
        //}
        //else
        //{
        //	ret = (int)dw;
        //}
        ret = ::send(fd, (char*)pv, total, 0);
        
        (void)HeapFree(GetProcessHeap(), 0, pv);
    }
    
    return ret;
}
#endif


