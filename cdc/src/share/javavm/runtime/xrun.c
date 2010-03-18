/*
 * @(#)xrun.c	1.13 06/10/10
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
#include "javavm/include/xrun.h"

/*
 * Initialize or append to existing table
 */
CVMBool CVMXrunInitTable(CVMXrunTable *table, 
			 CVMInt32 numXrunArguments)
{
    if (table->elemCnt > 0) {
	CVMXrunItem* newTable;
	CVMUint32 newNumArgs = table->elemCnt + numXrunArguments;
	
	CVMassert(table->table != NULL);
	newTable = (CVMXrunItem *)calloc(newNumArgs, sizeof(CVMXrunItem));
	if (newTable == NULL) return CVM_FALSE;
	memcpy(newTable, table->table, table->elemCnt * sizeof(CVMXrunItem));
	free(table->table);
	table->table = newTable;
	table->elemIdx = table->elemCnt;
	table->elemCnt = newNumArgs;
    } else {
	CVMXrunItem* newTable;

	CVMassert(table->elemCnt == 0);
	CVMassert(table->elemIdx == 0);
	CVMassert(table->table == NULL);
	newTable = (CVMXrunItem *)calloc(numXrunArguments, 
					 sizeof(CVMXrunItem));
	if (newTable == NULL) return CVM_FALSE;
	table->table = newTable;
	table->elemIdx = 0;
	table->elemCnt = numXrunArguments;
    }
	
    return CVM_TRUE;
}

void CVMXrunAppendToTable(CVMXrunTable *table,
			  jobject libRef, JVM_OnUnload_t fptr) 
{
    CVMassert(table->elemIdx < table->elemCnt);
    table->table[table->elemIdx].shareLibRef = libRef;
    table->table[table->elemIdx].onUnloadFunc = fptr;
    table->elemIdx++;
}

/*
 * Call JVM_OnUnload function on each shared library.
 * Delete all nativeLibrary object refs. 
 * Free Xrun_table in CVMglobals.
 */
void CVMXrunProcessTable(CVMXrunTable *Xrun_table, JNIEnv *env, JavaVM *vm)
{
    CVMXrunItem *itemPtr = Xrun_table->table;
    CVMInt32 i = 0;

    if (Xrun_table->elemCnt == 0) {
	return;
    }
    while (i < Xrun_table->elemCnt) {
	JVM_OnUnload_t JVM_OnUnload = itemPtr[i].onUnloadFunc;
	jobject libRef = itemPtr[i++].shareLibRef;
	/* call JVM_OnUnload function if any */
	if (JVM_OnUnload != NULL) {
	    (*JVM_OnUnload)(vm);
	}

	/* delete native library object ref */
	CVMassert(libRef != NULL);
	(*env)->DeleteGlobalRef(env, libRef); 
    }
    free(Xrun_table->table);
}

/* %comment: k036 */

/* This takes in the entire -Xrun string and takes care of 
 * loading the library, 
 *
 * finding the JVM_OnUnload symbol and storing the function pointer of 
 * it, if any, and the loaded native library object handle in Xrun table.
 *
 * finding the JVM_Onload symbol, and calling it with the option string, 
 * if any, at the tail end of the argument. 
 *
 * Note that this just reuses the ClassLoader's native library loading 
 * code, so it automatically looks in java.library.path and 
 * sun.boot.library.path for the named library.
 */

CVMBool
CVMXrunHandleArgument(CVMXrunTable* Xrun_table, JNIEnv* env, char* arg)
{
    jclass classLoaderClass;
    jclass nativeLibraryClass;
    jmethodID loadLibraryInternalID;
    jmethodID findID;
    char* libraryNamePtr;
    char* separatorPtr;
    char* optionsPtr;
    jstring jLibraryName = NULL;
    jobject nativeLibrary = NULL;
    JVM_OnLoad_t onLoadFunc;
    jstring jOnLoadName = NULL;
    JVM_OnUnload_t onUnloadFunc;
    jstring jOnUnloadName = NULL;
    JavaVM* vm;
    CVMBool result = CVM_FALSE;

    classLoaderClass =
        CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader));
    nativeLibraryClass =
        CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader_NativeLibrary));

    /* Get method IDs */
    loadLibraryInternalID =
	(*env)->GetStaticMethodID(env, classLoaderClass,
				  "loadLibraryInternal",
                                  "(Ljava/lang/Class;Ljava/lang/String;ZZ)Ljava/lang/Object;");
    if (loadLibraryInternalID == NULL) {
	CVMconsolePrintf("can't find method ClassLoader.loadLibraryInternal()\n");
	goto done;
    }
    findID =
	(*env)->GetMethodID(env, nativeLibraryClass,
			    "find",
			    "(Ljava/lang/String;)J");
    if (findID == NULL) {
	CVMconsolePrintf("can't find method ClassLoader$NativeLibrary.find()\n");
	goto done;
    }

    libraryNamePtr = arg + 5;
    if (*libraryNamePtr == '\0') {
	CVMconsolePrintf("Missing library name\n");
	goto done;
    }
    
    separatorPtr = strchr(libraryNamePtr, ':');
    
    if (separatorPtr == NULL) {
	/* No options. */
	optionsPtr = NULL;
	jLibraryName = (*env)->NewStringUTF(env, libraryNamePtr);
    } else {
	char *tmpLibraryNamePtr;
	int len;

	/* Have to trim options. */
	optionsPtr = separatorPtr + 1;
	/* Leave room for null terminator */
	len = separatorPtr - libraryNamePtr + 1;
	tmpLibraryNamePtr =
	    malloc(len * sizeof(char));
	memcpy(tmpLibraryNamePtr, libraryNamePtr, len - 1);
	tmpLibraryNamePtr[len - 1] = 0;
	jLibraryName = (*env)->NewStringUTF(env, tmpLibraryNamePtr);
	free(tmpLibraryNamePtr);
    }

    if (jLibraryName == NULL) {
	CVMconsolePrintf("Allocation of jLibraryName failed\n");
	goto done;
    }

    /* Call library loading function */
    nativeLibrary = (*env)->CallStaticObjectMethod(env, classLoaderClass,
						   loadLibraryInternalID,
                                                   NULL, jLibraryName,
                                                   JNI_FALSE, JNI_TRUE);
    if ((*env)->ExceptionCheck(env)) {
	/* Exception already thrown, I believe */
	CVMconsolePrintf("Could not find helper library for argument %s\n",
			 arg);
	goto done;
    }

    /* Look up JVM_OnLoad symbol */
    jOnLoadName = (*env)->NewStringUTF(env, "JVM_OnLoad");
    if (jOnLoadName == NULL) {
	CVMconsolePrintf("Allocation of JVM_OnLoad String failed\n");
	goto done;
    }
    {
	jlong onloadVal =
	    (*env)->CallLongMethod(env, nativeLibrary, findID, jOnLoadName);
	if ((*env)->ExceptionCheck(env)) {
	    CVMconsolePrintf("Could not find JVM_OnLoad of helper library "
			     "for argument %s\n", arg);
	    goto done;
	}
	onLoadFunc = (JVM_OnLoad_t)CVMlong2VoidPtr(onloadVal);
    }

    /* Look up JVM_OnUnload symbol
     * Store away NativeLibrary object ref and function pointer for calling
     * JVM_onUnload.
     */
    jOnUnloadName = (*env)->NewStringUTF(env, "JVM_OnUnload");
    if (jOnUnloadName == NULL) {
	CVMconsolePrintf("Allocation of JVM_OnUnload String failed\n");
        goto done;
    }
    {
        jlong onunloadVal =
            (*env)->CallLongMethod(env, nativeLibrary, findID, jOnUnloadName);
        jobject globalRef = NULL;

        if ((*env)->ExceptionCheck(env)) {
	    CVMconsolePrintf("Could not find JVM_OnUnload of helper library "
			     "for argument %s\n", arg);
            goto done;
        }
        globalRef = (*env)->NewGlobalRef(env, nativeLibrary);
        if (globalRef == NULL) {
	    CVMconsolePrintf("Allocation of global root failed\n");
	    goto done;
        }

        onUnloadFunc = (JVM_OnUnload_t)CVMlong2VoidPtr(onunloadVal);
        CVMXrunAppendToTable(Xrun_table, globalRef, onUnloadFunc);
    }

    /* Call JVM_OnLoad after the lookup and store away
     * of JVM_OnUnload so we won't call JVM_OnUnload 
     * at VM shutdown if JVM_OnLoad was never called.
     * This also avoid a failure after createing the globalRef,
     * after adding entry to the Xrun table, and after
     * calling the JVM_OnLoad function.
     */
    if (onLoadFunc != NULL) {
        (*env)->GetJavaVM(env, &vm);
        /* %comment: rt018 */
        (*onLoadFunc)(vm, optionsPtr, NULL);
    }
    result = CVM_TRUE;

    done:
    (*env)->DeleteLocalRef(env, jLibraryName);
    (*env)->DeleteLocalRef(env, nativeLibrary);
    (*env)->DeleteLocalRef(env, jOnLoadName);
    (*env)->DeleteLocalRef(env, jOnUnloadName);
    return result; 
}

