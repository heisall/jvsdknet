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

#include "srs_kernel_log.h"

#include "srs_kernel_error.h"

#ifdef WIN32
#include <stdarg.h>
#include <Windows.h>

#else
#include <sys/time.h>
#include <stdarg.h>


#endif


ISrsLog::ISrsLog()
{
}

ISrsLog::~ISrsLog() 
{
}

int ISrsLog::initialize()
{
    return ERROR_SUCCESS;
}

void ISrsLog::verbose(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::info(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::trace(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::warn(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::error(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

ISrsThreadContext::ISrsThreadContext()
{
}

ISrsThreadContext::~ISrsThreadContext()
{
}

void ISrsThreadContext::generate_id()
{
}

int ISrsThreadContext::get_id()
{
    return 0;
}


void srs_verbose(char* format, ...) //_srs_log->verbose(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
{/*
#ifdef WIN32
#ifdef _DEBUG
	va_list arglist;
	char buffer[1024];
	
	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);
	
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	
	char data[1024] = {0};
	sprintf(data,"%02d:%02d:%02d %3d srs_verbose %s\n",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds,buffer);
	OutputDebugString(data);
#endif
#else
    char data[1024] = {0};
    char szData[1024] = {0};
    va_list va_args;
    va_start(va_args, format);
	
    size_t length = vsnprintf(NULL, 0,format, va_args);
    int result = vsnprintf(szData, length + 1, format, va_args);
	
	time_t now = time(0);
	tm *tnow = localtime(&now);
	
	sprintf(data,"%02d:%02d:%02d srs_verbose %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
	
    printf(data);
	
    va_end(va_args);
	
#endif*/
}

void srs_info(char* format, ...)    //_srs_log->info(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
{
	/*
#ifdef WIN32
#ifdef _DEBUG
	va_list arglist;
	char buffer[1024];
	
	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);
	
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	
	char data[1024] = {0};
	sprintf(data,"%02d:%02d:%02d %3d srs_info %s\n",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds,buffer);
	OutputDebugString(data);
#endif
#else
    char data[1024] = {0};
    char szData[1024] = {0};
    va_list va_args;
    va_start(va_args, format);
	
    size_t length = vsnprintf(NULL, 0,format, va_args);
    int result = vsnprintf(szData, length + 1, format, va_args);
	
	time_t now = time(0);
	tm *tnow = localtime(&now);
	
	sprintf(data,"%02d:%02d:%02d srs_info %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
	
    printf(data);
	
    va_end(va_args);
	
#endif*/
}

void srs_trace(char* format, ...)   //_srs_log->trace(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
{
	/*
#ifdef WIN32
#ifdef _DEBUG
	va_list arglist;
	char buffer[1024];
	
	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);
	
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	
	char data[1024] = {0};
	sprintf(data,"%02d:%02d:%02d %3d srs_trace %s\n",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds,buffer);
	OutputDebugString(data);
#endif
#else
    char data[1024] = {0};
    char szData[1024] = {0};
    va_list va_args;
    va_start(va_args, format);
	
    size_t length = vsnprintf(NULL, 0,format, va_args);
    int result = vsnprintf(szData, length + 1, format, va_args);
	
	time_t now = time(0);
	tm *tnow = localtime(&now);
	
	sprintf(data,"%02d:%02d:%02d srs_trace %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
	
    printf(data);
	
    va_end(va_args);
	
#endif
	*/
}

void srs_warn(char* format, ...)    //_srs_log->warn(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
{
	/*
#ifdef WIN32
#ifdef _DEBUG
	va_list arglist;
	char buffer[1024];
	
	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);
	
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	
	char data[1024] = {0};
	sprintf(data,"%02d:%02d:%02d %3d srs_warn %s\n",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds,buffer);
	OutputDebugString(data);
#endif
#else
    char data[1024] = {0};
    char szData[1024] = {0};
    va_list va_args;
    va_start(va_args, format);
	
    size_t length = vsnprintf(NULL, 0,format, va_args);
    int result = vsnprintf(szData, length + 1, format, va_args);
	
	time_t now = time(0);
	tm *tnow = localtime(&now);
	
	sprintf(data,"%02d:%02d:%02d srs_warn %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
	
    printf(data);
	
    va_end(va_args);
	
#endif
	*/
}

void srs_error(char* format, ...)   //_srs_log->error(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
{
#ifdef WIN32
#ifdef _DEBUG
	va_list arglist;
	char buffer[1024];
	
	va_start (arglist,format);
	vsprintf(buffer, format, arglist);
	va_end (arglist);
	
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	
	char data[1024] = {0};
	sprintf(data,"%02d:%02d:%02d %3d srs_error %s\n",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds,buffer);
	OutputDebugString(data);
#endif
#else
//    char data[1024] = {0};
//    char szData[1024] = {0};
//    va_list va_args;
//    va_start(va_args, format);
//	
//    size_t length = vsnprintf(NULL, 0,format, va_args);
//    int result = vsnprintf(szData, length + 1, format, va_args);
//	
//	time_t now = time(0);
//	tm *tnow = localtime(&now);
//	
//	sprintf(data,"%02d:%02d:%02d srs_error %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
//	
//    printf(data);
//	
//    va_end(va_args);
	
#endif
}
