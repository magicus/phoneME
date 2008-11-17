/*
 * @(#)xrun.h	1.11 06/10/10
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

#ifndef _INCLUDED_XRUN_H
#define _INCLUDED_XRUN_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/sync.h"
#include "javavm/include/cstates.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/packages.h"

typedef jint (JNICALL *JVM_OnLoad_t)(JavaVM *, char *, void *);
typedef void (JNICALL *JVM_OnUnload_t)(JavaVM *);

/*
 * Each item stored in a table contains Xrun loaded share native
 * library object ref and the function pointer of calling JVM_OnUnload
 */
typedef struct CVMXrunItem {
    jobject shareLibRef;
    JVM_OnUnload_t onUnloadFunc;
} CVMXrunItem;

/*
 * A table of items containing a set of Xrun loaded share native
 * library object ref and the function pointer of JVM_onUnload
 */
typedef struct CVMXrunTable {
    CVMInt32 elemIdx;
    CVMInt32 elemCnt;
    CVMXrunItem *table;
} CVMXrunTable;

extern CVMBool
CVMXrunInitTable(CVMXrunTable *onUnloadTable, CVMInt32 numXrunArguments);
extern void
CVMXrunAppendToTable(CVMXrunTable *onUnloadTable, jobject libRef,
		     JVM_OnUnload_t fptr);
extern void
CVMXrunProcessTable(CVMXrunTable *onUnloadTable, JNIEnv *env, JavaVM *vm);
extern CVMBool
CVMXrunHandleArgument(CVMXrunTable *onUnloadTable, JNIEnv* env, char* arg);

#endif /* _INCLUDED_XRUN_H */
