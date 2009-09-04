/*
 * @(#)linker.h	1.18 06/10/30
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

#ifndef _INCLUDED_PORTING_LINKER_H
#define _INCLUDED_PORTING_LINKER_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Dynamic linking support.
 *
 * These routines look very similar to the dynamic linking facilities
 * on Solaris and Win32. However, on VxWorks there is currently no
 * notion of a local symbol table (submitted as an RFE; TSR# 143425.)
 * Therefore symbol lookup is slow on VxWorks as we have to iterate
 * through the entire system symbol table every time. It is required
 * by the Java libraries that sit above these routines that a notion
 * of local symbol namespaces be maintained.
 *
 * On VxWorks these routines require three libraries to be built into
 * your project if they are not already: loadLib, moduleLib, and
 * symLib.
 */

/*
 * create a string for the dynamic lib open call by adding the
 * appropriate pre and extensions to a filename and the path
 */
void
CVMdynlinkbuildLibName(char *holder, int holderlen, const char *pname,
                       char *fname);

/*
 * Dynamically link in a shared object. This takes a platform-
 * dependent, absolute pathname to the DSO. Calling code is
 * responsible for constructing this name. (We have yet to determine
 * how this will be done both for user libraries, which have the
 * properties java.library.path and java.path.separator available, and
 * system or "JVM helper" libraries like the JVMTI/JDWP backend, which
 * must be loaded before the VM is initialized.) Returns an abstract
 * "handle" to the shared object which must be used for later symbol
 * queries, or NULL if an error occurred. (sysLoadLibrary in the JDK)
 *
 * If absolutePathName is NULL, a handle to the "main program" is
 * returned.
 */
void *CVMdynlinkOpen(const void *absolutePathName);

/*
 * Find a function or data pointer in an already-opened shared object.
 * Takes the dynamic shared object handle (see CVMdlopen, above) and a
 * platform-dependent symbol as argument (typically, but not always, a
 * char *). CVMdynlinkSym() is also responsible for adding any necessary
 * "decorations" to the symbol name before doing the lookup.
 */

void *CVMdynlinkSym(void *dsoHandle, const void *name);

/*
 * Close a dynamic shared object. This should probably have the
 * semantics that its symbols are unloaded, but VxWorks, at least,
 * doesn't currently appear to have a module unloading facility. Takes
 * the dynamic shared object handle from CVMdlopen(), above.
 * (sysUnloadLibrary in the JDK)
 */

void CVMdynlinkClose(void *dsoHandle);

/* check is library exists */

CVMBool CVMdynlinkExists(const char *name);

#include CVM_HDR_LINKER_H

#endif  /* #defined _INCLUDED_PORTING_LINKER_H */
