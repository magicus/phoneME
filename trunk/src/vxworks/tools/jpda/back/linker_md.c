/*
 * @(#)linker_md.c	1.13 06/10/10
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
 * Adapted from JDK 1.2 linker_md.c v1.37. NOTE: (kbr) Requires native
 * threads. VxWorks-specific.
 */

#include <ioLib.h>
#include <loadLib.h>
#include <moduleLib.h>
#include <symLib.h>
#include <sysSymTbl.h>

#include "path_md.h"

/*
 * create a string for the JNI native function name by adding the
 * appropriate decorations.
 */
int
dbgsysBuildFunName(char *name, int nameLen, int args_size, int encodingIndex)
{
  /* On VxWorks, there is only one encoding method. */
    if (encodingIndex == 0)
        return 1;
    return 0;
}

/*
 * create a string for the dynamic lib open call by adding the
 * appropriate pre and extensions to a filename and the path
 */
void
dbgsysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    const int pnamelen = pname ? strlen(pname) : 0;
    char *suffix;

#ifdef DEBUG   
    suffix = "_g";
#else
    suffix = "";
#endif 

    /* Quietly truncate on buffer overflow.  Should be an error. */
    if (pnamelen + strlen(fname) + 10 > holderlen) {
        *holder = '\0';
        return;
    }

    if (pnamelen == 0) {
        sprintf(holder, "lib%s%s.o", fname, suffix);
    } else {
        sprintf(holder, "%s/lib%s%s.o", pname, fname, suffix);
    }
}

void *
dbgsysLoadLibrary(const char *name, char *err_buf, int err_buflen)
{
    MODULE_ID mid;
    int fd;
    static const char *openErrorString = "Error opening library";
    static const char *loadErrorString = "Error loading library";

    fd = open((char *) name, O_RDONLY, 0);
    if (fd == ERROR) {
	strncpy(err_buf, openErrorString, err_buflen - 2);
	err_buf[err_buflen-1] = 0;
	return NULL;
    }
    mid = loadModule(fd, LOAD_GLOBAL_SYMBOLS);
    close(fd);
    if (mid == NULL) {
	strncpy(err_buf, loadErrorString, err_buflen - 2);
	err_buf[err_buflen-1] = 0;
    }
    return mid;
}

void dbgsysUnloadLibrary(void *handle)
{
    unldByModuleId(handle);
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

void * dbgsysFindLibraryEntry(void *handle, const char *name)
{
    MODULE_ID mid;
    MODULE_INFO mInfo;
    struct CallbackInfo info;
    SYMBOL *sym;

    mid = (MODULE_ID) handle;
    if (moduleInfoGet(mid, &mInfo) == ERROR)
	return NULL;
    info.name = (char *) name;
    info.group = (UINT16) mInfo.group;
    sym = symEach(sysSymTbl, (FUNCPTR) &findSymbolCB, (int) &info);
    if (sym == NULL)
	return NULL;
    return info.val;
}
