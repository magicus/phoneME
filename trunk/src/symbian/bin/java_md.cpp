/*
 * @(#)java_md.c	1.8 03/11/14
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
 *
 */

extern "C" {
#include "javavm/export/jni.h"
#include "portlibs/ansi_c/java.h"
#include "javavm/include/porting/io.h"
}

#include <stdio.h>
#include <unistd.h>

#include <e32def.h>
#include <e32std.h>
#include <estlib.h>

#if !defined(__WINS__)
#define ArgSentinel ((char*)0xffffffff)

static char ** append_arg(char **argv, int count, 
                          char *newarg, int newarg_len=-1) {
    char * p;
    if (newarg_len == -1) {
        newarg_len = strlen(newarg);
    }

  if (argv == NULL || argv[count] == ArgSentinel) {
    // Need to expand argv
    int newsize = count * 2;
    if (newsize < 16) {
        newsize = 16;
    }
    argv = (char**)User::ReAlloc(argv, (newsize+1) * sizeof(char*));
    int i;
    for (i=count; i<newsize; i++) {
      argv[i] = 0x0;
    }
    argv[newsize] = (char*)ArgSentinel;
  }

  p = (char*)User::Alloc(newarg_len + 1);
  memset(p, 0, newarg_len + 1);
  memcpy(p, newarg, newarg_len);
  p[newarg_len] = 0;
  argv[count] = p;

  return argv;
}

static char ** translateCommandLine(char* buf, int *argc_ret) {
    int argc = 0;
    char ** argv;

    char *start = buf, *p = buf;
    bool done = false;
    bool inquote = false;

    argv = append_arg(NULL, argc, "cvm");
    argc ++;

    while (!done) {
        switch (*p) {
        case 0:
          if (p-start > 0) {
              argv = append_arg(argv, argc, start, p-start); 
              argc++;
          }
          done = true;
          break;

        case ' ': 
        case '\t': 
        case '\r': 
        case '\n':
          if (!inquote) {
              if (p-start > 0) {
                  argv = append_arg(argv, argc, 
                                    start, p-start); 
                  argc++;
              }
              /* FIXME: what happens if the next is also ' '? */
              p++;
              start = p;
          } else {
              p++;
          }
          break;

        case '"':
          if (!inquote) {
              inquote=true;
              p++;
              start = p;
          } else {
              argv = append_arg(argv, argc, 
                                start, p-start);
              argc++;
              p++;
              start = p;
              inquote=false;
          }
          break;

        default:
            p++;
      }
  }

  *argc_ret = argc;
  return argv;
}

static char ** getCommandLine(int *argc_ret) {
    RProcess proc;
    TBuf<1024> cmdLine;

    const int max_buf = 1024;
    static char buf[max_buf + 1];
    int n;
    char **argv = NULL;

    proc.Open(KCurrentProcessHandle);
#ifdef EKA2
    User::CommandLine(cmdLine);
#else
    proc.CommandLine(cmdLine);
#endif
    if (cmdLine.Length() == 0) {
        /* no argument */
        char *default_argv[1] = {"cvm"};
        *argc_ret = 1;
        return default_argv;
    }
    for (n = 0; n < cmdLine.Length() &&
                n < max_buf; n++) {
        buf[n] = (char)cmdLine[n];
    }
    buf[n] = 0;
    return translateCommandLine(buf, argc_ret);
}
#endif

#ifdef CVM_DLL
void *
getAnsiJavaMain0() {
    TUidType uid(KNullUid, KNullUid, KNullUid);
    RLibrary cvmdll;
#ifdef _UNICODE
    wchar_t pw[KMaxFileName];
    mbstowcs(pw, (const char *)"CVM.DLL", sizeof pw);
    TPtrC16 path((TUint16*)pw);
#else
    TPtrC8 path((const TUint8 *)"CVM.DLL");
#endif
    if (cvmdll.Load(path, uid) != KErrNone) {
	return NULL;
    }

    return cvmdll.Lookup(2);
}
#endif

typedef int ansiJavaMain0_func(int argc, const char **argv);

#if defined(__WINS__)
int
main(int argc, const char **argv)
#else
int E32Main()
#endif
{
#if !defined(__WINS__)
    int argc;
    char ** argv;
  

    fprintf(stderr, "SpawnPosixServerThread...\n");
    TInt res = SpawnPosixServerThread();
    fprintf(stderr, "%d\n", res);
    argv = getCommandLine(&argc);
#endif

#ifdef CVM_DLL
   ansiJavaMain0_func *ansiJavaMain0Func =
       (ansiJavaMain0_func *)getAnsiJavaMain0();
   if (ansiJavaMain0Func != NULL) {
       return (*ansiJavaMain0Func)(argc, argv);
   }

    return -1;
#else
    return ansiJavaMain(argc, argv, JNI_CreateJavaVM);
#endif
}
