/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "srs_lib_simple_socket.h"

#include "srs_kernel_error.h"
#include "Def.h"
#ifndef MOBILE_CLIENT
#include "unistd.h"
#else
#include <unistd.h>
#endif
#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#else
#include <winsock2.h>

#endif
#include <sys/uio.h>

#include "srs_kernel_utility.h"

#ifndef ST_UTIME_NO_TIMEOUT
    #define ST_UTIME_NO_TIMEOUT -1
#endif

SimpleSocketStream::SimpleSocketStream()
{
    m_ffffd = -1;
    send_timeout = recv_timeout = ST_UTIME_NO_TIMEOUT;
    recv_bytes = send_bytes = 0;
}

SimpleSocketStream::~SimpleSocketStream()
{
    if (m_ffffd != -1)
    {
#ifdef WIN32
		closesocket(m_ffffd);
#else
        ::close(m_ffffd);
#endif
        m_ffffd = -1;
    }
}

int SimpleSocketStream::tcp_receive(int s,char *pchbuf,int nsize,int ntimeoverMSec)
{
	int   status,nbytesreceived;   
	if(s==-1)     
	{
		return   0;   
	}
	struct   timeval   tv={ntimeoverMSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);     
	if(ntimeoverMSec==0)   
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
		//printf("read   select   error\n");       
		return   -1;   
	case   0:                                             
		//printf("receive   time   out\n");
		return   0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
			if((nbytesreceived=recv(s,pchbuf,nsize,0))==-1)   
			{
#ifdef WIN32
				int kkk = 0;
				kkk = WSAGetLastError();
				if(kkk == WSAECONNRESET)
				{
					return -1;
				}
#else
				if(errno == 104)
				{
					return -1;
				}
#endif

				return   0;   
			}
			else   
			{
				return   nbytesreceived;   
			}
		}   
	}   
	return 0;   
}

int SimpleSocketStream::create_socket()
{
    if((m_ffffd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return ERROR_SOCKET_CREATE;
    }
/*
	//修改套接字类型为非阻塞方式
#ifndef WIN32
	int flags=0;
	fcntl(m_ffffd, F_GETFL, 0);
	
	fcntl(m_ffffd, F_SETFL, flags | O_NONBLOCK);
#else
	unsigned long ulBlock = 1; 
	ioctlsocket(m_ffffd, FIONBIO, (unsigned long*)&ulBlock);   
#endif    
	
	//将套接字置为不等待未处理完的数据
	LINGER linger;
	linger.l_onoff = 0;
	linger.l_linger = 0;
    setsockopt(m_ffffd, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));     
*/
    return ERROR_SUCCESS;
}
int SimpleSocketStream::connect(const char* server_ip, int port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(server_ip);

    if(::connect(m_ffffd, (const struct sockaddr*)&addr, sizeof(sockaddr_in)) < 0)
	{
      return ERROR_SOCKET_CONNECT;
	 }
// 	if(0 != connectnb(m_ffffd,(SOCKADDR *)&addr, sizeof(SOCKADDR), 10))
// 	{
// 		return ERROR_SOCKET_CONNECT;
// 	OutputDebug(">>>>>>>>>>>>>>>>>>>>>>%s %d ok",server_ip,port);

    return ERROR_SUCCESS;
}

#include <sys/time.h>
int SimpleSocketStream::JVGetTime()
{
    int dwresult=0;

    struct timeval start;
    gettimeofday(&start,NULL);
    
    dwresult = (int)(start.tv_sec*1000 + start.tv_usec/1000);
  
        
        return dwresult;
}

// ISrsBufferReader
int SimpleSocketStream::read(void* buf, size_t size, ssize_t* nread,int nConnect)
{
    int ret = ERROR_SUCCESS;
	nConnect = 1;
	if(nConnect == 0)
	{
		*nread = ::recv(m_ffffd, (char*)buf, size, 0);
	}
 	else
 	{
//        int a = JVGetTime();
//         printf("tcp_receive spendtime begin\n");
 		*nread = tcp_receive(m_ffffd, (char*)buf, size, 3);
//        int b  = JVGetTime();
//        printf("tcp_receive spendtime end : %d\n",(b-a));
		if(*nread == 0)
		{
			*nread = 0;
			//recv_bytes += *nread;
			
			return -999;
		}
		if(*nread == -1)//断开
		{
			*nread = -1;
			//recv_bytes += *nread;
			
			return -9999;
		}
	} 
    // On success a non-negative integer indicating the number of bytes actually read is returned
    // (a value of 0 means the network connection is closed or end of file is reached).
    if (*nread <= 0)
	{
  /*      if (errno == ETIME)
		{
            return ERROR_SOCKET_TIMEOUT;
        }

        if (*nread == 0)
		{
            errno = ECONNRESET;
        }
        */
        return ERROR_SOCKET_READ;
    }

    recv_bytes += *nread;

    return ret;
}

// ISrsProtocolReader
void SimpleSocketStream::set_recv_timeout(int64_t timeout_us)
{
    recv_timeout = timeout_us;
}

int64_t SimpleSocketStream::get_recv_timeout()
{
    return recv_timeout;
}

int64_t SimpleSocketStream::get_recv_bytes()
{
    return recv_bytes;
}

// ISrsProtocolWriter
void SimpleSocketStream::set_send_timeout(int64_t timeout_us)
{
    send_timeout = timeout_us;
}

int64_t SimpleSocketStream::get_send_timeout()
{
    return send_timeout;
}

int64_t SimpleSocketStream::get_send_bytes()
{
    return send_bytes;
}

int SimpleSocketStream::writev(const iovec *iov, int iov_size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;

    *nwrite = ::writev(m_ffffd, iov, iov_size);

    if (*nwrite <= 0)
	{
    /*    if (errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        */
        return ERROR_SOCKET_WRITE;
    }

    send_bytes += *nwrite;

    return ret;
}

// ISrsProtocolReaderWriter
bool SimpleSocketStream::is_never_timeout(int64_t timeout_us)
{
    return timeout_us == (int64_t)ST_UTIME_NO_TIMEOUT;
}

int SimpleSocketStream::read_fully(void* buf, size_t size, ssize_t* nread)
{
    int ret = ERROR_SUCCESS;

    size_t left = size;
    *nread = 0;

    while (left > 0) {
        char* this_buf = (char*)buf + *nread;
        ssize_t this_nread;

        if ((ret = this->read(this_buf, left, &this_nread,0)) != ERROR_SUCCESS) {
            return ret;
        }

        *nread += this_nread;
        left -= this_nread;
    }

    recv_bytes += *nread;

    return ret;
}

int SimpleSocketStream::write(void* buf, size_t size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;

    *nwrite = ::send(m_ffffd, (char*)buf, size, 0);

    if (*nwrite <= 0)
	{
    /*    if (errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }*/

        return ERROR_SOCKET_WRITE;
    }

    send_bytes += *nwrite;

    return ret;
}


