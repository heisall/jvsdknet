//----------------------------------------------------------------------
// JRCDef.h
// 通用定义
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#ifdef WIN32
#pragma warning(disable:4786)
#endif

//编译配置--------------------------------------------------------------
#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif

#if !defined(WINDOWS) && !defined(WIN32) && !defined(LINUX)
#  define LINUX
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#  define DEBUG
#endif

#if !defined(DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif

#if defined(_MSC_VER) && !defined(MSVC)
#  define MSVC _MSC_VER
#endif

#if defined(MSVC) && !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS
#endif


//全局定义--------------------------------------------------------------
#define JMS_COREVER "1.0"

#ifdef __cplusplus
#  define JMS_DLL_EXTERN extern "C"
#else
#  define JMS_DLL_EXTERN
#endif

#ifdef LINUX
#  define JMS_DLL_EXPORT JMS_DLL_EXTERN
#  define JMS_DLL_CALLTYPE
#else
#  ifdef MSVC
#    define JMS_DLL_EXPORT JMS_DLL_EXTERN __declspec(dllexport)
#    define JMS_DLL_CALLTYPE __stdcall
#  else
#    define JMS_DLL_EXPORT JMS_DLL_EXTERN
#    define JMS_DLL_CALLTYPE
#  endif
#endif


//头文件--------------------------------------------------------------
#ifdef LINUX
#  include <pthread.h>
#  include <semaphore.h>
#  include <unistd.h>
#  include <errno.h>
#  include <signal.h>
#  include <dlfcn.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <sys/time.h>
#  include <sys/stat.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#else
#  include <WinSock2.h>
#  include <Windows.h>
#  include <process.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <map>
#include <fstream>
#include <cassert>
#include <exception>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
