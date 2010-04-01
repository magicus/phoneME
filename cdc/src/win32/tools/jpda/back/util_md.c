/*
 * @(#)util_md.c	1.1 07/08/5
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

#include <windows.h>
#include <util_md.h>
#ifndef WINCE
#include <time.h>
#else
#include <winbase.h>
#include "javavm/include/porting/time.h"
#include <string.h>
#include <stddef.h>
#endif
#include "javavm/include/jni_md.h"
#include "javavm/export/jni.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/ansi/fcntl.h"
#include <sys/stat.h>

#ifdef WINCE
#define FILETAB_SIZE 32
static FILE* filetab[FILETAB_SIZE];
static BOOL filetab_initted = FALSE;

static void init_filetab()
{
    int i;
    for (i = 0; i < FILETAB_SIZE; i++) {
        filetab[i] = NULL;
    }
}

static FILE* GETFILE(int fd)
{
    if (fd < 0 || fd >= FILETAB_SIZE)
        return NULL;
    return filetab[fd];
}

static int file_to_fd(FILE *fp)
{
    int i;
    if (!filetab_initted) {
        init_filetab();
        filetab_initted = TRUE;
    }
    for (i = 0; i < FILETAB_SIZE; i++) {
        if (filetab[i] == NULL) {
            filetab[i] = fp;
            return i;
        }
    }
    return -1;
}

#define GETPID()        GetCurrentProcessId()


void _sleep(int secs)
{
    Sleep(secs);
}

void
abort() {
    ExitProcess(-1);
}

#define FT2INT64(ft) \
        ((CVMInt64)(ft).dwHighDateTime << 32 | (CVMInt64)(ft).dwLowDateTime)

long
time(void)
{
    CVMInt64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;
    static CVMInt64 originTick = 0;
    CVMInt64 ttt;

    if (originTick == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);

        GetSystemTime(&st0);
        SystemTimeToFileTime(&st0, &ft0);
        originTick = (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
        originTick -= GetTickCount(); 
    }

    ttt = GetTickCount() + originTick;
    return ttt/1000;
}

#else
#define GETFILE(x) x
#define GETPID()        getpid()
#endif

void CVMformatTime(char *format, size_t format_size, time_t t) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(format, format_size, "%d.%d.%d %d:%d%:%d.%%.3d", st.wDay,
	     st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
}

int
md_init(JavaVM *jvm)
{
    return JNI_OK;
}

int 
md_creat(const char *filename)
{
#ifdef WINCE
    FILE *fp = fopen(filename, "wb");
    if (fp != NULL) {
	return file_to_fd(fp);
    } else {
	return -1;
    }
#else
    return open(filename, O_CREAT | O_WRONLY | O_TRUNC, 
			     _S_IREAD | _S_IWRITE);
#endif
}

void
md_close(int filedes)
{
#ifdef WINCE
    fclose(GETFILE(filedes));
    filetab[filedes] = NULL;
#else
    (void)close(filedes);
#endif
}

int
md_write(int fd, char *buf, int len)
{
    return write(fd, buf, len);
}
