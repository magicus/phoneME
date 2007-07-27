///********************************************************************
// VvUtils.h - Miscellaneous Vivo utility functions
//
// Author:     Tom Kent, 6/20/96
// Inspector:
//
// Copyright (c) 1996-1997 Vivo Software, Inc.  All Rights Reserved.
///*************************************************************************
// MODIFICATIONS:
//  03/26/97    jsalman Added GetLimitDataRate() function prototype
//  11/12/96    awanka  Uses OutputDebugString if defined(FOR_OUTPUTDEBUGWINDOW)
//  11/12/96    tkent   Rearranged functions into VvUtils.h (platform-
//                      independent stuff), VvUtilsM.h (Mac) and VvUtilsW.h
//                      (Windows). Include VvUtils.h regardless of the
//                      platform you are compiling for.
//  06/20/96    tkent   Implemented MacUtils.cpp to contain utilities for
//                      the Mac port
//
///*************************************************************************

#ifndef VVUTILS_H
#define VVUTILS_H

#include "machine.h"
#include <stdio.h>

#ifdef FOR_MAC
// #define MEM_LEAK_TEST 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define kNumberOfTempBlocks 100
#define kSizeOfEachBlock 20000

#ifdef FOR_MAC
int _strnicmp(const char *a, const char *b, int len);
#endif

void GetDebugSettings(void);
S32 GetUserPlaybackSampleRate(void);
char LoggingEnabled(void);
S32 GetMinimumRequiredMemory(BOOL isStandalone);
S32 GetMinRingBlocks(void);
S32 GetMaxRingBlocks(void);
S32 GetRingBlockSize(void);
BOOL VMIsEnabled(void);
S32 GetLimitDataRate(void);


// Comment this out to suppress debugging logfile generation
#if defined(FOR_MAC)
#define FOR_LOGFILES 1

// For compatibility with older include files
// #define USE_OLDER_NAMES 1
#endif

/* this brings dprintf() to life for PC: Win95 and MSVC but not yet for Watcom */
#if defined(FOR_WINDOWS)
#if defined(_DEBUG)
/* WIN16 or __WATCOMC__ */
/* #define FOR_OUTPUTDEBUGWINDOW */
#endif
#endif

extern void dprintf(char *format, ...);
extern void dflush(void);

#ifdef __cplusplus
}
#endif

// Include platform-specific utilities as well

#ifdef FOR_MAC
#include "VvUtilsM.h"
#endif
#ifdef FOR_WINDOWS
#include "VvUtilsW.h"
#endif
#ifdef FOR_UNIX
#if 0
#include "vvutilsu.h"
#endif

#define STRCMP      strcmp
#define STRICMP     strcasecmp
#define STRNICMP    strncasecmp
#define STRCOLL     strcoll
#define STRSTR      strstr
#define STRCHR      strchr
#define STRPBRK     strpbrk
#define STRPBRK     strpbrk
#define STRSPN      strspn
#define STRCSPN     strcspn

#endif

#endif // VVUTILS_H
