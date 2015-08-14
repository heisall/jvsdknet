#ifndef _UIO_H
#define _UIO_H

#include <inttypes.h>

#include "srs_protocol_rtmp.h"
#ifdef WIN32

struct iovec
{
	void*   iov_base;  /*!< Base address */
	size_t  iov_len;    /*!< Number of bytes referenced */
};

#endif
#ifndef WIN32
#include <sys/uio.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif


ssize_t readv(int fd,struct iovec const*  vector, int count);
ssize_t writev(int fd, struct iovec const*  vector,int count);

#ifdef __cplusplus
}
#endif

#endif
