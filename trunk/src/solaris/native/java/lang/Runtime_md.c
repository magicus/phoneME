/*
 * @(#)Runtime_md.c	1.11 06/10/10
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

#include "jni.h"
#include "jni_util.h"

#include "java_lang_Runtime.h"

JNIEXPORT jobject JNICALL  
Java_java_lang_Runtime_execInternal(JNIEnv *env, jobject this,
				    jobjectArray cmdarray, jobjectArray envp,
                                    jstring path)
{
    if (cmdarray == NULL) {
        JNU_ThrowNullPointerException(env, 0);
	return 0;
    }

    if ((*env)->GetArrayLength(env, cmdarray) == 0) {
	JNU_ThrowArrayIndexOutOfBoundsException(env, 0);
	return 0;
    }

    return JNU_NewObjectByName(env,
			       "java/lang/UNIXProcess",
			       "([Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V", 
			       cmdarray, envp, path);
} 
