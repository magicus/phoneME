/*
 * @(#)linker_md.cpp	1.6 06/10/10
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
#include "javavm/include/porting/linker.h"
#include "javavm/include/porting/ansi/stdlib.h"
}
#include <e32def.h>
#include <e32std.h>
#include <utf.h>

typedef void (*TFunc)();
typedef TFunc lookup_t(const char *);

extern "C" {
IMPORT_C TFunc __cvm_lookup(const char *);
}

typedef struct {
    RLibrary *h;
    lookup_t *lookup;
} CVMLib;

void *
CVMdynlinkOpen(const void *absolutePathName)
{
    CVMLib *l = (CVMLib *)CVMmalloc(sizeof *l);
    if (l == NULL) {
	return NULL;
    }
    if (absolutePathName == NULL) {
	l->lookup = __cvm_lookup;
	l->h = NULL;
    } else {
	TUidType uid(KNullUid, KNullUid, KNullUid);
	void *mem = CVMmalloc(sizeof *l->h);
	if (mem == NULL) {
	    return NULL;
	}
#ifdef _UNICODE
#if 0
	TBuf16<KMaxFileName> path;
	CnvUtfConverter::ConvertToUnicodeFromUtf8(path, absolutePathName);
#else
	wchar_t pw[KMaxFileName];
	mbstowcs(pw, (const char *)absolutePathName, sizeof pw);
	TPtrC16 path((TUint16*)pw);
#endif
#else
	TPtrC8 path((const TUint8 *)absolutePathName);
#endif
	l->h = new ((TAny *)mem) RLibrary();
	if (l->h->Load(path, uid) != KErrNone) {
	    CVMfree(mem);
	    CVMfree(l);
	    return NULL;
	}
	l->lookup = (lookup_t *)l->h->Lookup(1);
    }
    return l;
}

void *
CVMdynlinkSym(void *dsoHandle, const void *name)
{
    CVMLib *l = (CVMLib *)dsoHandle;
    return (void *)(*l->lookup)((const char *)name);
}

void
CVMdynlinkClose(void *dsoHandle)
{
    CVMLib *l = (CVMLib *)dsoHandle;
    if (l->h != NULL) {
	l->h->Close();
//	l->h->operator delete((TAny *)l->h);
	l->h->~RLibrary();
	CVMfree(l->h);
    }
    CVMfree(l);
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
