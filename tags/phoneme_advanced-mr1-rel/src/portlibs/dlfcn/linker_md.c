/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#ifdef CVM_DYNAMIC_LINKING

#include "javavm/include/porting/linker.h"
#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/string.h"
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>

void *
CVMdynlinkOpen(const void *absolutePathName)
{
    void *handle;

    handle = dlopen((const char *) absolutePathName, RTLD_LAZY);
#ifdef CVM_DEBUG
    if (!handle) {
        fputs(dlerror(), stderr);
        fputs("\n", stderr);         
    }
#endif
    return handle;
}

void *
CVMdynlinkSym(void *dsoHandle, const void *name)
{
    assert(dsoHandle != NULL);
    /*
     * Some platforms (namely bsd) require that you prepend the
     * symbol with an underscore.
     */
#ifdef CVM_DYNLINKSYM_PREPEND_UNDERSCORE
    {
	void* result;
	char *buf = malloc(strlen(name) + 1 + 1);
	if (buf == NULL) {
	    return NULL;
	}
	strcpy(buf, "_");
	strcat(buf, name);
	result = dlsym(dsoHandle, (const char *) buf);
	free(buf);
	/* 
	 * On darwin, the dlcompat library requires that you prepend the
	 * underscore, but the Panther version (10.3?) of dlsym does not, so
	 * we need to try both.
	 */
	if (result == NULL) {
	    result = dlsym(dsoHandle, (const char *) name);
	}
	return result;
    }
#else
    return dlsym(dsoHandle, (const char *) name);
#endif
}

void
CVMdynlinkClose(void *dsoHandle)
{
    dlclose(dsoHandle);
}

#endif /* #defined CVM_DYNAMIC_LINKING */
