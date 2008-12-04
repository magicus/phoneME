/*
 * @(#)wceIOWrite.c	1.9 06/10/10
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

/* 
 * An implementation of writeStandardIO and readStandardIO
 * defined in io.h for the winCE platform, headless device.
 * This implementation directs standard IO to 
 * IN.txt, OUT.txt and ERR.txt respectively.
 * Personal Profile could overwrite this implementation
 * by providing a GUI-based console.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "jni.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/io_md.h"
#include "javavm/include/wceUtil.h"
#include "javavm/include/globals.h"
#include "javavm/include/winntUtil.h"

static int initialized = 0;
static HANDLE standardin  = INVALID_HANDLE_VALUE;
static HANDLE standardout = INVALID_HANDLE_VALUE;
static HANDLE standarderr = INVALID_HANDLE_VALUE;
static char *memoryBuffer=NULL;

static int
initializeFileHandlers() {
    char *consolePrefix=NULL;
    TCHAR *inStr=NULL, *errStr=NULL, *outStr=NULL, *prefixStr=NULL;

    if (CVMglobals.target.stdioPrefix == NULL) {
        return 0;
    }

    if (CVMglobals.target.stdinPath != NULL) {
        wchar_t *w = createWCHAR(CVMglobals.target.stdinPath);
        if (w != NULL) {
            FILE *fp = _wfreopen(w, L"rb", stdin);
            free(w);
            CVMglobals.target.stdinPath = NULL;
        }
    }
    if (CVMglobals.target.stdoutPath != NULL) {
        wchar_t *w = createWCHAR(CVMglobals.target.stdoutPath);
        if (w != NULL) {
            FILE *fp = _wfreopen(w, L"wb", stdout);
            free(w);
            CVMglobals.target.stdoutPath = NULL;
        }
    }
    if (CVMglobals.target.stderrPath != NULL) {
        wchar_t *w = createWCHAR(CVMglobals.target.stderrPath);
        if (w != NULL) {
            FILE *fp = _wfreopen(w, L"wb", stderr);
            free(w);
            CVMglobals.target.stderrPath = NULL;
        }
    }

    consolePrefix = CVMglobals.target.stdioPrefix;
    if (strlen(consolePrefix) > 0) {
        prefixStr = createTCHAR(consolePrefix);
    } else {
        return 1;
    }
    inStr = (TCHAR*)malloc(sizeof(TCHAR)*(strlen("\\IN.txt") + _tcslen(prefixStr)+1));
    _stprintf(inStr, _T("%s\\IN.txt"), prefixStr);
    standardin = CreateFile(inStr, GENERIC_READ, 
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    outStr = (TCHAR*)malloc(sizeof(TCHAR)*(strlen("\\OUT.txt") + _tcslen(prefixStr)+1));
    _stprintf(outStr, _T("%s\\OUT.txt"), prefixStr);
    standardout = CreateFile(outStr, GENERIC_WRITE, 
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    errStr = (TCHAR*)malloc(sizeof(TCHAR)*(strlen("\\ERR.txt") + _tcslen(prefixStr)+1));
    _stprintf(errStr, _T("%s\\ERR.txt"), prefixStr);
    standarderr = CreateFile(errStr, GENERIC_WRITE, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (inStr != NULL) free(inStr);
    if (outStr != NULL) free(outStr);
    if (errStr != NULL) free(errStr);
    if (prefixStr != NULL) freeTCHAR(prefixStr);
    return 1;
}

void 
writeStandardIO(CVMInt32 fd, const void *buf, CVMUint32 nBytes) {

   DWORD bytes;
   int b = 0;

   if (!initialized) {
        initialized = initializeFileHandlers();
   }

    if (!initialized) {
        if (memoryBuffer == NULL) {
            memoryBuffer = (char*)malloc(sizeof(char)*(nBytes + 1));
            *memoryBuffer = '\0';
        } else
            memoryBuffer = (char*)realloc((char*)memoryBuffer, 
                                          sizeof(char)*(strlen(memoryBuffer) + nBytes + 1));
        strncat(memoryBuffer, buf, nBytes);
    } else if (memoryBuffer != NULL) {
        if (fd == 1) {
            if (standardout != INVALID_HANDLE_VALUE) {
                WriteFile(standardout, memoryBuffer, strlen(memoryBuffer), &bytes, NULL);
            }
        } else if (fd == 2) {
            if (standarderr != INVALID_HANDLE_VALUE) {
                WriteFile(standarderr, memoryBuffer, strlen(memoryBuffer), &bytes, NULL);
            }
        }
        free(memoryBuffer);
        memoryBuffer = NULL;
    }

   if (fd == 1) { /* stdout */
      if (standardout != INVALID_HANDLE_VALUE) {
         b = WriteFile(standardout, buf, nBytes, &bytes, NULL);
      }
   } else if (fd == 2) {
      if (standarderr != INVALID_HANDLE_VALUE) {
         b = WriteFile(standarderr, buf, nBytes, &bytes, NULL);
      }
   } else {
      NKDbgPrintfW(TEXT("Wrong file handler at writeStandardIO: %d"), fd);
   } 
}

int 
readStandardIO(CVMInt32 fd, void *buf, CVMUint32 nBytes) {
    DWORD bytes;

    if (!initialized) {
        initialized = initializeFileHandlers();
    }

    if (standardin != INVALID_HANDLE_VALUE) { 
        BOOL b = ReadFile(standardin, buf, nBytes, &bytes, NULL);
        if (b) {
            return bytes;
        }
    }
    return 0;
}

void
initializeStandardIO() {
   if (!initialized) {
      initialized = initializeFileHandlers();
    }
}

