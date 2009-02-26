/*
 * @(#)string.h	1.17 06/10/10
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
 * String-related routines.
 * For use by Java_java_lang_String_intern, classloading,
 * and garbage collection.
 */

#ifndef CVM_STRING_H
#define CVM_STRING_H

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"

/*
 * Initialize on VM start-up.
 */
void 
CVMinternInit();

/*
 * The JNI entry point: java.lang.String.intern.
 * by way of JVM_InternString, which is re-mapped to 
 * this function:
 */
JNIEXPORT jobject JNICALL
CVMstringIntern(JNIEnv *env, jstring);

/*
 * CVMinternUTF called by the class loader to turn a null-terminated,
 * UTF8-encoded C string into a Java String, put it in the intern table,
 * and return a pointer to the intern table cell.
 * Will lock the intern table during search, unlock at end.
 */
CVMStringICell *
CVMinternUTF( CVMExecEnv *, const CVMUtf8 * );

/*
 * Also for use by the class loader, when unloading a class.
 * If an interned String ref is found in the constant pool, call
 * this on it.
 */
void
CVMinternDecRef( CVMExecEnv * ee, CVMStringICell * target );


/*
 * CVMscanInternedStrings is used to scan the intern table,
 * calling the func on every live (non-null, non-deleted) entry.
 *
 * Used by GC.
 * Assumes intern table already locked, so it is safe to iterate
 * without further locking.
 */
void
CVMscanInternedStrings( CVMRefCallbackFunc func, void * data );

/*
 * CVMdeleteUnreferencedStrings called during garbage collection:
 * iterate over all the intern table entries. For any that have a 0
 * reference count, discover if they have Java references by calling
 * queryStringReferenced. If not, then the String will be deleted from
 * the intern table.
 *
 * Assumes intern table already locked, so it is safe to change
 * entries.
 */
void
CVMinternProcessNonStrong(CVMRefLivenessQueryFunc isLive,
			  void* isLiveData,
			  CVMRefCallbackFunc transitiveScanner,
			  void* transitiveScannerData);

/*
 * Completes GC processing of interned strings
 */
void
CVMinternFinalizeProcessing(CVMRefLivenessQueryFunc isLive,
			    void* isLiveData,
			    CVMRefCallbackFunc transitiveScanner,
			    void* transitiveScannerData);
/* Define as macro because currently, there is nothing to do for this API. */
#define CVMinternFinalizeProcessing(isLive, isLiveData, transitiveScanner, \
                                    transitiveScannerData)    {}

void
CVMinternRollbackHandling(CVMExecEnv* ee,
			  CVMGCOptions* gcOpts,
			  CVMRefCallbackFunc rootRollbackFunction,
			  void* rootRollbackData);

/*
 * give back dynamically allocated stuff when shutting down
 * the VM.
 */
void
CVMinternDestroy();

#endif /* CVM_STRING_H */
