/*
 * @(#)agentlib.h	1.2 06/10/27
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


#ifndef _INCLUDED_AGENTLIB_H
#define _INCLUDED_AGENTLIB_H

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

typedef jint (JNICALL *Agent_OnLoad_t)(JavaVM *, char *, void *);
typedef void (JNICALL *Agent_OnUnload_t)(JavaVM *);

typedef struct CVMAgentlibArg {
  char *str;
  CVMBool is_absolute;
} CVMAgentlibArg_t;

/*
 * Each item stored in a table contains Agentlib loaded share native
 * library object ref and the function pointer of calling Agent_OnUnload
 */
typedef struct CVMAgentItem {
    void *libHandle;
    Agent_OnUnload_t onUnloadFunc;
} CVMAgentItem;

/*
 * A table of items containing a set of Agentlib loaded share native
 * library object ref and the function pointer of Agent_onUnload
 */
typedef struct CVMAgentTable {
    CVMInt32 elemIdx;
    CVMInt32 elemCnt;
    CVMAgentItem *table;
} CVMAgentTable;

extern CVMBool
CVMAgentInitTable(CVMAgentTable *table, CVMInt32 numAgentArguments);
extern void
CVMAgentAppendToTable(CVMAgentTable *table, void *libHandle,
		      Agent_OnUnload_t fptr);
extern void
CVMAgentProcessTableUnload(CVMAgentTable *agentTable,
			   JNIEnv *env, JavaVM *vm);
extern CVMBool
CVMAgentHandleArgument(CVMAgentTable *agentTable, JNIEnv* env,
		       CVMAgentlibArg_t* arg);

#endif /* _INCLUDED_AGENTLIB_H */
