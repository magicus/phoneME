/*
 * @(#)jam.hpp	1.24 06/10/10 10:08:35
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#ifndef _JAM_HPP_
#define _JAM_HPP_

#ifdef _WIN32_WCE
#define _WIN32_ 1
#endif 

#ifdef _WIN32_
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
typedef __int64  int64;
#define SEP ";"
#endif

#ifdef __SYMBIAN32__
# include <e32std.h>
# include <ctype.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stddef.h>// for offsetof
# include <stdio.h> // for va_list
# include <time.h>
# include <fcntl.h>
# include <unistd.h>

typedef TInt64 int64;
#define SEP ";"

#define TRUE 1
#define FALSE 0

#endif

#ifdef _UNIX_
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#ifdef __linux__
#include <stdint.h>
#endif
#ifdef __sun__
#include <sys/types32.h>
#endif
typedef  int64_t int64;
#define TRUE 1
#define FALSE 0
#define SEP ":"
#endif

#define IS_SPACE(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')
#define MAX_BUF          1024
#define MAX_URL          256

// how often memory statistics is gathered, in ms
#define MEM_TRACK_SLICE  100
#define TRACK_MEMORY     0x1
#define TRACK_TIME       0x2

typedef struct {
	unsigned int total;
	unsigned int used;
} MemInfo;

typedef struct {  
  int                 type;
  const char*         typeStr;
  const char*         heapswitch;
  const char*         classpath;
  const char*         stickyConsole;
  const char**        midpAutotest;
  int                 flags;
} VMOptInfo;

#define VM_TYPE_KVM     0 
#define VM_TYPE_MONTY   1
#define VM_TYPE_JEODE   2
#define VM_TYPE_J9      3
#define VM_TYPE_JDK1_3  4
#define VM_TYPE_CDC     5
#define VM_TYPE_MIDP    6
#define VM_TYPE_MIDP2   7
// insert new VM options here, BEFORE UNKNOWN 
#define VM_TYPE_UNKNOWN 8

#define VM_OPT_HEAP_SEP  0x1
#define VM_OPT_CP_SEP    0x2

extern void     JamHttpInitialize(void);
extern char*    JamHttpGet(const char* url, 
			   int* contentLengthP, 
			   int complain);
extern char *   JamHttpPost(const char* url, 
			    int len, char* buf, 
			    int *reslen,
			    int complain);
extern int      JamListen(int port, int blocking);
extern int      JamAccept(int servfd, int blocking);
extern int      JamSelect(int fd, int tmo);
extern int      JamRead(int fd, char* buf, int maxlen);

extern int      JamRunURL(char* vm, const char* url, int timeout);
extern int      JamRunMIDPURL(char* vm, const char* url, int timeout);
extern int      JamRunStandaloneURL(char* vm, const char* url, int g_timeout);
extern void     JamUpdateMemInfo(int reset);
extern char*    JamGetProp(char* buffer, char* name, int* length);
extern void*    JamThreadFunction(void* args);

extern char *   GetCharsOfLong64(int64 val);
extern int64    ParseInt64(char* str, int len);
extern void     ParseCmdArgs(int argc, char **argv);
/*
 * These are implemented by the platform-dependent code
 */
extern int      PlafSaveJar(char * jarContent, int jarLength);
extern int      PlafRunJar (char* vm, char * mainClass, long javaHeap);
extern void     PlafErrorMsg(const char *format, ...);
extern void     PlafStatusMsg(const char *format, ...);
extern void     PlafDownloadProgress(int total, int downloaded);
extern char *   PlafCurrentTimeMillisStr();
extern int64    PlafCurrentTimeMillis();
extern MemInfo  PlafGetMemInfo();
extern int      PlafExec(const char*   prog,
			 const char**  argv, 
			 int           timeout,
			 int           flags);
extern void    PlafUpdateMemInfo(int reset);
extern int     PlafSleep(long ms);
extern char*   PlafGetUsage();

/* The number of the current test being prepared and executed */
extern int      testNumber;

/* timestamp of VM startup */
extern int64    testStartMillis;

/* memory consumption in bytes */
extern unsigned int    minMemSize;
extern unsigned int    avgMemSize;
extern unsigned int    maxMemSize;

/* non-zero if running in MIDP mode */
extern int          g_midpMode;

/* non-zero if running in standalone mode */
extern int          g_standaloneMode;

/* some global vars */
extern char *    g_vm;
extern char *    g_url;
extern char *    g_bootclasspath;
extern char *    g_tempfile;
extern char *    g_vmargs[100];
extern int       g_vm_type;
extern VMOptInfo g_opt_info[VM_TYPE_UNKNOWN+1];
// repeat | retry | action
// -------+-------+----------------
// false  | any   | execute only one JAR file
// -------+-------+----------------
// true   | false | execute JAR files repeatedly until error is encountered
// -------+-------+----------------
// true   | true  | execute JAR files repeatedly until retry is unchecked,
//        |       | regardless of error conditions.
extern int      g_repeat;
extern int      g_retry;
extern int      g_timeout;
extern int      g_sticky; 

/* status of VM */
extern int volatile      vmStatus;

/* one of those */
#define         JAM_VM_STOPPED 0
#define         JAM_VM_STARTED 1

//  #define JAM_TRACE_MEM 1

#ifdef  JAM_TRACE_MEM
extern void *my_malloc(int size);
extern void my_free(void* ptr);
extern void *my_realloc(void *ptr, size_t size); 
#define jam_malloc(x) my_malloc(x) 
#define jam_free(x) my_free(x)
#define jam_realloc(x,y) my_realloc(x, y)
#else
#define jam_malloc  malloc
#define jam_free(x) free(x)
#define jam_realloc realloc
#endif

#define strnzcpy(_dst, _src, _len) \
    strncpy(_dst, _src, _len); \
    _dst[_len] = '\0'

#endif // _JAM_HPP_

