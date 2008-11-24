/*
 * @(#)agentlib.c	1.3 06/10/30
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


#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/sync.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/utils.h"
#include "javavm/include/agentlib.h"
#include "javavm/include/porting/linker.h"
#include "javavm/include/path_md.h"


/*
 * Initialize or append to existing table
 */
CVMBool CVMAgentInitTable(CVMAgentTable *table, 
			  CVMInt32 numAgentArguments)
{
    if (table->elemCnt > 0) {
	CVMAgentItem* newTable;
	CVMUint32 newNumArgs = table->elemCnt + numAgentArguments;
	
	CVMassert(table->table != NULL);
	newTable = (CVMAgentItem *)calloc(newNumArgs, sizeof(CVMAgentItem));
	if (newTable == NULL) {
	    return CVM_FALSE;
	}
	memcpy(newTable, table->table, table->elemCnt * sizeof(CVMAgentItem));
	free(table->table);
	table->table = newTable;
	table->elemIdx = table->elemCnt;
	table->elemCnt = newNumArgs;
    } else {
	CVMAgentItem* newTable;

	CVMassert(table->elemCnt == 0);
	CVMassert(table->elemIdx == 0);
	CVMassert(table->table == NULL);
	newTable = (CVMAgentItem *)calloc(numAgentArguments, 
					  sizeof(CVMAgentItem));
	if (newTable == NULL) {
	    return CVM_FALSE;
	}
	table->table = newTable;
	table->elemIdx = 0;
	table->elemCnt = numAgentArguments;
    }
	
    return CVM_TRUE;
}

void CVMAgentAppendToTable(CVMAgentTable *table,
			   void *libHandle, Agent_OnUnload_t fptr) 
{
    CVMassert(table->elemIdx < table->elemCnt);
    table->table[table->elemIdx].libHandle = libHandle;
    table->table[table->elemIdx].onUnloadFunc = fptr;
    table->elemIdx++;
}

/*
 * Call Agent_OnUnload function on each shared library.
 * Delete all nativeLibrary object refs. 
 * Free agentTable in CVMglobals.
 */
void CVMAgentProcessTableUnload(CVMAgentTable *agentTable,
				JNIEnv *env, JavaVM *vm)
{
    CVMAgentItem *itemPtr = agentTable->table;
    CVMInt32 i = 0;

    if (agentTable->elemCnt == 0) {
	return;
    }
    while (i < agentTable->elemCnt) {
	Agent_OnUnload_t Agent_OnUnload = itemPtr[i].onUnloadFunc;
	void*libHandle = itemPtr[i++].libHandle;
	/* call Agent_OnUnload function if any */
	if (Agent_OnUnload != NULL) {
	    (*Agent_OnUnload)(vm);
	}

	/* delete native library object ref */
	CVMassert(libHandle != NULL);
	CVMdynlinkClose(libHandle);
    }
    free(agentTable->table);
}

/* This takes in the entire -agentlib string and takes care of 
 * loading the library, 
 *
 * finding the Agent_OnUnload symbol and storing the function pointer of 
 * it, if any, and the loaded native library object handle in Agent table.
 *
 * finding the Agent_Onload symbol, and calling it with the option string, 
 * if any, at the tail end of the argument. 
 *
 * Note that this just reuses the ClassLoader's native library loading 
 * code, so it automatically looks in java.library.path and 
 * sun.boot.library.path for the named library.
 */


CVMBool
CVMAgentHandleArgument(CVMAgentTable* agentTable, JNIEnv* env,
                       CVMAgentlibArg_t* arg)
{
    char buffer[CVM_PATH_MAXLEN];
    char* libraryNamePtr;
    char* separatorPtr;
    char* optionsPtr;
    char nullStr[1] = {0};
    void * nativeLibrary = NULL;
    Agent_OnLoad_t onLoadFunc;
    Agent_OnUnload_t onUnloadFunc;
    JavaVM* vm;
    CVMBool result = CVM_FALSE;
    const CVMProperties *sprops;

    sprops = CVMgetProperties();
    /* must be in the form -agentlib: or -agentpath: */
    separatorPtr = strchr(arg->str, ':');
    if (separatorPtr == NULL) {
	CVMconsolePrintf("Malformed library name\n");
	goto done;
    }
    libraryNamePtr = separatorPtr + 1;
    if (*libraryNamePtr == '\0') {
	CVMconsolePrintf("Missing library name\n");
	goto done;
    }
    optionsPtr = strchr(libraryNamePtr, '=');
    if (optionsPtr != NULL) {
	*optionsPtr++ = '\0';
    } else {
	/* no options, point at NULL char */
	optionsPtr = nullStr;
    }
    if (arg->is_absolute) {
	nativeLibrary = CVMdynlinkOpen((const void*)libraryNamePtr);
    } else {
	CVMdynlinkbuildLibName(buffer, sizeof(buffer), sprops->dll_dir,
			       libraryNamePtr);
	if (*buffer == '\0') {
	    return CVM_FALSE;
	}
	nativeLibrary = CVMdynlinkOpen(buffer);
	if (nativeLibrary == NULL) {
	    /* Try local directory */
	    char ns[1] = {0};
	    CVMdynlinkbuildLibName(buffer, sizeof(buffer), ns, libraryNamePtr);
	    nativeLibrary = CVMdynlinkOpen(buffer);
	}
    }
    if (nativeLibrary == NULL) {
	return CVM_FALSE;
    }

    /* Look up Agent_OnLoad symbol */
    onLoadFunc = (Agent_OnLoad_t)CVMdynlinkSym(nativeLibrary,
					       "Agent_OnLoad");
    if (onLoadFunc == NULL) {
	CVMconsolePrintf("Could not find Agent_OnLoad of helper library "
			 "for argument %s\n", arg->str);
	goto done;
    }
    /* Look up Agent_OnUnload symbol
     * Store away NativeLibrary object ref and function pointer for calling
     * Agent_onUnload.
     */
    onUnloadFunc = (Agent_OnUnload_t)CVMdynlinkSym(nativeLibrary,
						   "Agent_OnUnload");

    CVMAgentAppendToTable(agentTable, nativeLibrary, onUnloadFunc);

    /* Call Agent_OnLoad after the lookup and store away
     * of Agent_OnUnload so we won't call Agent_OnUnload 
     * at VM shutdown if Agent_OnLoad was never called.
     * This also avoid a failure after createing the globalRef,
     * after adding entry to the Agent table, and after
     * calling the Agent_OnLoad function.
     */
    if (onLoadFunc != NULL) {
	(*env)->GetJavaVM(env, &vm);
	/* %comment: rt018 */
	(*onLoadFunc)(vm, optionsPtr, NULL);
    }
    result = CVM_TRUE;

 done:
    return result; 
}

