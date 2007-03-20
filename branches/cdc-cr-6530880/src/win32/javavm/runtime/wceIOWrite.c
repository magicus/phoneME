/*
 * @(#)wceIOWrite.c	1.9 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

#include "javavm/include/porting/io.h"
#include "javavm/include/io_md.h"
#include "javavm/include/wceUtil.h"

int initialized = 0;
static HANDLE standardin, standardout, standarderr;

static void
initializeFileHandlers() {
   standardin = CreateFile(_T("\\IN.txt"), GENERIC_READ, 
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
   standardout = CreateFile(_T("\\OUT.txt"), GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
   standarderr = CreateFile(_T("\\ERR.txt"), GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

int 
writeStandardIO(CVMInt32 fd, const void *buf, CVMUint32 nBytes) {

   DWORD bytes;
   int b = 0;

   if (!initialized) {
      initialized = 1;
      initializeFileHandlers();
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
   if (b) {
      return bytes;
   } else {
      return -1;
   }
}

int 
readStandardIO(CVMInt32 fd, void *buf, CVMUint32 nBytes) {

   DWORD bytes;
   int b = 0;

   if (!initialized) {
      initialized = 1;
      initializeFileHandlers();
   }

   if (standardin != INVALID_HANDLE_VALUE) { 
      b = ReadFile(standardin, buf, nBytes, &bytes, NULL);
   }

   if (b) {
      return bytes;
   } else {
      return 0;
   }
}

void
initializeStandardIO() {
   if (!initialized) {
      initialized = 1;
      initializeFileHandlers();
   }
}

