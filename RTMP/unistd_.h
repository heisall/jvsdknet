#ifndef _UNISTD_H
#define _UNISTD_H

#ifdef WIN32
#include <io.h>
#include <process.h>
#include <time.h>

#endif

typedef unsigned int u_int32_t;

#define mkdir _mkdir
#define getpid _getpid
#define getcwd _getcwd

#define snprintf _snprintf

#ifdef __cplusplus
extern "C" {
#endif

void usleep(long usec);

#ifdef __cplusplus
}
#endif

#endif /* _UNISTD_H */