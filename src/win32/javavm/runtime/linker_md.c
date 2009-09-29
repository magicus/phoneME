/*
 * @(#)linker_md.c  1.16 06/10/30
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
#ifdef WINCE
#include <winbase.h>
#ifdef CVM_DEBUG
#include <windows.h>
#endif
#else
#include <stdlib.h>
#include <windows.h>
#endif

#ifdef CVM_DYNAMIC_LINKING

#include "javavm/include/porting/linker.h"
#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/jni_md.h"

#ifdef WINCE
#include "javavm/include/wceUtil.h"
#endif

#ifdef _WIN32_WINNT
#include "javavm/include/winntUtil.h"
#endif

/*
 * Build a machine dependent library name out of a path and file name.
 */
void
CVMdynlinkbuildLibName(char *holder, int holderlen, const char *pname,
                       char *fname)
{
    const int pnamelen = pname ? strlen(pname) : 0;
    const char c = (pnamelen > 0) ? pname[pnamelen-1] : 0;
    char *suffix;

#if defined(CVM_DEBUG) && (!defined(JAVASE) || JAVASE < 16)
    suffix = "_g";
#else
    suffix = "";
#endif

    /* Quietly truncates on buffer overflow. Should be an error. */
    if (pnamelen + strlen(fname) + 10 > (unsigned int)holderlen) {
        *holder = '\0';
        return;
    }

    if (pnamelen == 0) {
        sprintf(holder, "%s%s%s%s", JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    } else if (c == ':' || c == '\\') {
        sprintf(holder, "%s%s%s%s%s", pname, JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    } else {
        sprintf(holder, "%s\\%s%s%s%s", pname, JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    }
}

void *
CVMdynlinkOpen(const void *absolutePathName)
{
    HANDLE hh;

    if (absolutePathName == NULL) {
#ifdef CVM_DLL
        hh = LoadLibrary(TEXT("cvmi.dll"));
#else
        hh = GetModuleHandle(NULL);
#endif
    } else {
#ifdef UNICODE
        char *pathName = (char*)absolutePathName;
        WCHAR *wc = createWCHAR(pathName);
        hh = LoadLibrary(wc);
        free(wc);
#else
        char *wc = (char*)absolutePathName;
        hh = LoadLibraryA(wc);
#endif
    }

#ifdef CVM_DEBUG
    /* Print an error message if we failed to open the dll. */
    if (hh == NULL) {
        fprintf(stderr, "CVMdynlinkOpen(%s) failed. err=0x%x\n",
                absolutePathName == NULL ? "NULL" : absolutePathName,
                GetLastError());
    }
#endif

#ifdef CVM_DEBUG
    /* Put up an error dialog if we failed to open the dll. */
    if (hh == NULL) {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      0, /* default language */
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL);

        MessageBox(NULL,
                   (LPCTSTR) lpMsgBuf,
                   TEXT("Error"),
                   MB_OK | MB_ICONINFORMATION);

        LocalFree(lpMsgBuf);
    }
#endif

    return hh;
}

void *
CVMdynlinkSym(void *dsoHandle, const void *name)
{
    int i = 0;
    void *v = NULL;

#ifdef WINCE
    v = GetProcAddressA((HANDLE)dsoHandle, name);
#else
    v = GetProcAddress((HANDLE)dsoHandle, name);
#endif

    /* Enable this if you want to know why a symbol lookup is failing. */
#if 0
    if (v == NULL) {
	char buf[256];
	int err = CVMioGetLastErrorString(buf, sizeof(buf));
	fprintf(stderr, "CVMdynlinkSym(%s) failed. err=0x%x (%s)\n",
			 name, err, err == 0 ? "???" : buf);
    }
#endif
    return v;
}

void
CVMdynlinkClose(void *dsoHandle)
{
    FreeLibrary((HANDLE)dsoHandle);
}

CVMBool
CVMdynlinkExists(const char *name)
{
    void *handle;

    handle = CVMdynlinkOpen((const char *) name);
    if (handle != NULL) {
        CVMdynlinkClose(handle);
    }
    return (handle != NULL);
}

#endif /* #defined CVM_DYNAMIC_LINKING */
