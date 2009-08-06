/*
 * @(#)DynLinkTest.c	1.8 06/10/10
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
#include "javavm/test/DynLinkTest.h"
#include "javavm/include/porting/linker.h"

/* Native code for DynLinkTest. Note that this makes CVM-internal
   calls to the porting layer to be able to test dynamic linking
   without having dynamic classloading fully in place. */

JNIEXPORT void JNICALL Java_DynLinkTest_callDynLinkedFunc
  (JNIEnv *env, jclass clazz, jstring jLibName, jstring jFuncName)
{
    const char *libName;
    const char *funcName;
    void *handle;
    void (*func)(void);

    libName = (*env)->GetStringUTFChars(env, jLibName, NULL);
    funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
    handle = CVMdynlinkOpen(libName);
    if (handle == NULL) {
	(*env)->ThrowNew(env,
			 (*env)->FindClass(env, "java/lang/RuntimeException"),
			 "Error opening library");
	return;
    }
    func = (void (*)(void)) CVMdynlinkSym(handle, funcName);
    if (func == NULL) {
	(*env)->ThrowNew(env,
			 (*env)->FindClass(env, "java/lang/RuntimeException"),
			 "Error finding function");
	return;
    }
    (*func)();
}
