/*
 * @(#)linker_md.c	1.33 06/10/10
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

#ifdef CVM_DYNAMIC_LINKING

/*
 * On VxWorks these routines require three libraries to be built into
 * your project if they are not already: loadLib, moduleLib, and
 * symLib.
 */

#include "javavm/include/porting/linker.h"
#include <ioLib.h>
#include <loadLib.h>
#include <unldLib.h>
#include <moduleLib.h>
#include <symLib.h>
#include <sysSymTbl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* VxWorks doesn't have local (per-module) symbol tables. Therefore we
   have to search the entire system symbol table every
   time we want to do a symbol lookup, and ensure that the "group" of
   the returned symbol is equal to the module's group ID. This has
   been filed as an RFE with TSR# 143425. */

/*
 * The following library handle is used for the VxWorks CVMdynlink* calls.
 *
 * We differentiate between 'global' symbol lookup (built-in symbols), vs.
 * symbols looked up from shared libraries.
 */

typedef struct VxWorksLibraryHandle {
    MODULE_ID id;
    int isGlobal;
} VxWorksLibraryHandle;

void *
CVMdynlinkOpen(const void *absolutePathName)
{
    VxWorksLibraryHandle* handle;

    handle = malloc(sizeof(VxWorksLibraryHandle));
    if (handle == NULL) {
	return NULL; /* Out of memory */
    }
    if (absolutePathName == NULL) {
	handle->id = NULL;
	handle->isGlobal = 1;
    } else {
	MODULE_ID mid;
	int fd;
	/*
	 * Not a global symbol. Load module.
	 */
	handle->isGlobal = 0;
	fd = open((char *) absolutePathName, O_RDONLY, 0);
	if (fd == ERROR) {
	    free(handle);
	    return NULL;
	}
	mid = loadModule(fd, LOAD_GLOBAL_SYMBOLS);
	if (mid == NULL) {
	    free(handle);
	    handle = NULL;
	    perror("CVMdynlinkOpen/loadModule");
	} else {
	    handle->id = mid;
	}
	close(fd);
    }
    return (void*)handle;
}

struct FindWellKnownSymbolCallbackInfo {
    char *name;
    UINT16 group;
};

static BOOL
findWellKnownSymbolCB(char *name,
		      int val,
		      SYM_TYPE type,
		      int arg,
		      UINT16 group)
{
    struct FindWellKnownSymbolCallbackInfo *info =
	(struct FindWellKnownSymbolCallbackInfo *) arg;

    if (!strcmp(name, info->name)) {
	info->group = group;
	return FALSE;
    }
    return TRUE;
}

/* Returns CVM_TRUE upon success, CVM_FALSE upon error */
static CVMBool
getWellKnownSymbolGroupId(UINT16* group)
{
    /* This finds the group ID associated with a well-known symbol
       ("CVMgcUnsafeExecuteJavaMethod") and returns it. Note that this
       is static and therefore persistent across Java VMs. This should
       not be a problem because they will all be using the same
       program code and symbol table. */
    static CVMBool gotId = CVM_FALSE;
    static UINT16 id;

    if (gotId == CVM_FALSE) {
	struct FindWellKnownSymbolCallbackInfo info;
	SYMBOL* sym;
#ifdef CVM_DYNLINKSYM_PREPEND_UNDERSCORE
	info.name = "_CVMgcUnsafeExecuteJavaMethod";
#else
	info.name = "CVMgcUnsafeExecuteJavaMethod";
#endif
	
	sym = symEach(sysSymTbl, (FUNCPTR) &findWellKnownSymbolCB,
		      (int) &info);
	/* If sym is NULL, failed to find symbol -- should not happen. */
	if (sym == NULL) {
	    return CVM_FALSE;
	}

	id = info.group;
	gotId = CVM_TRUE;
    }

    *group = id;
    return gotId;
}

struct CallbackInfo {
    char *name;
    UINT16 group;
    void *val;
};

static BOOL
findSymbolCB(char *name,
	     int val,
	     SYM_TYPE type,
	     int arg,
	     UINT16 group)
{
    struct CallbackInfo *info = (struct CallbackInfo *) arg;

    if ((!strcmp(name, info->name)) &&
	(info->group == group)) {
	/* Found the symbol we're looking for. Returning FALSE from
	   this callback will cause symEach() to return the current
	   symbol to its caller. */
	info->val = (void *) val;
	return FALSE;
    }
    return TRUE;
}

/*
 * If dsoHandle is NULL, or if the handle indicates that we should do a
 * global, module-independent lookup, we do it. Otherwise, we have to match
 * on module and symbol name.
 */
void *
CVMdynlinkSym(void *dsoHandle, const void *nameArg)
{
    VxWorksLibraryHandle* handle = (VxWorksLibraryHandle*)dsoHandle;
    const char* name = (const char*)nameArg;
    struct CallbackInfo info;
    SYMBOL* sym;
    void *value;

#ifdef CVM_DYNLINKSYM_PREPEND_UNDERSCORE
    /*
     * %comment: mam051
     * It might be nice if we could modify nameArg in place.
     */
    char *buf = malloc(strlen(nameArg) + 1 + 1);
    if (buf == NULL) {
	return NULL;
    }
    strcpy(buf, "_");
    strcat(buf, nameArg);
    name = buf;
#endif

    assert(handle != NULL);
    
    if (handle->isGlobal) {
	/* Look up a well-known CVM symbol and match its group
	   ID. This protects us whether cvm.o is dynamically loaded,
	   or statically linked into the kernel. */
	if (getWellKnownSymbolGroupId(&info.group) == CVM_FALSE) {
	    value = NULL;
	    goto done;
	}
    } else {
	MODULE_ID mid;
	MODULE_INFO mInfo;

	mid = handle->id;
	if (moduleInfoGet(mid, &mInfo) == ERROR) {
	    value = NULL;
	    goto done;
	}
	info.group = (UINT16) mInfo.group;
    }
    info.name = (char*)name;
    sym = symEach(sysSymTbl, (FUNCPTR) &findSymbolCB, (int) &info);
    if (sym == NULL) {
	value = NULL;
    } else {
	value = info.val;
    }

done:
#ifdef CVM_DYNLINKSYM_PREPEND_UNDERSCORE
    free(buf);
#endif
    return value;
}

void
CVMdynlinkClose(void *dsoHandle)
{
    VxWorksLibraryHandle* handle = (VxWorksLibraryHandle *)dsoHandle;
    if (handle->id != NULL) {
	unldByModuleId(handle->id, 0);
    }
    free(handle);
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
